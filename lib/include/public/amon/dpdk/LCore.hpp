/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief RAII handle for a DPDK lcore with remote-launch support.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/Error.hpp>
#include <amon/Logger.hpp>
#include <amon/dpdk/SocketID.hpp>
#include <amon/types/Singleton.hpp>
#include <amon/util/Formatter.hpp>
#include <amon/util/UniqueResource.hpp>

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>

namespace amon::dpdk {

/// @brief Lcore identification.
struct LCore {
    uint16_t coreId; ///< Lcore ID.
    uint16_t socketId; ///< NUMA socket ID.
};

/**
 * @brief Formatter for LCore.
 * @param fmt The formatter.
 * @param value The value to format.
 */
void format(const LCore& value, util::Formatter& fmt);

/// @brief Forward declaration.
class LCoreHandle;

/**
 * @brief Base class for pools that lease DPDK worker lcores as LCoreHandle objects.
 *
 * Derived pools implement the storage and selection policy in acquireCore() and releaseCore().
 * LCorePool wraps those hooks in an RAII lease: a successfully acquired core is returned to the
 * same pool when the corresponding LCoreHandle is destroyed or overwritten.
 *
 * Instances must be owned by std::shared_ptr before acquire() is called because the returned handle
 * keeps the pool alive until its lease is released.
 */
class LCorePool : public std::enable_shared_from_this<LCorePool> {
public:
    /**
     * @brief Acquire a worker lcore as an RAII handle.
     *
     * The returned LCoreHandle owns the selected core until the handle releases its lease. If the
     * socket-specific acquisition cannot be satisfied, the exact fallback behavior is defined by
     * the concrete pool implementation.
     *
     * @param socketId Preferred NUMA socket, or SocketID::ANY if no socket is preferred.
     * @return Handle owning the acquired core lease.
     * @throws amon::Error if no suitable core is available.
     */
    LCoreHandle acquire(SocketID socketId = SocketID::ANY);

    /**
     * @brief Get the number of cores available on a specified NUMA socket.
     *
     * Supports SocketID::ANY which returns all available cores across all sockets.
     *
     * @param socketId NUMA socket.
     * @return Number of available cores.
     */
    virtual size_t size(SocketID socketId) = 0;

    /// @brief Virtual destructor.
    virtual ~LCorePool() = default;

protected:
    friend class LCoreHandle;

    /**
     * @brief Constructor should not be exposed in derived classes. Provide a static ::create method
     * that returns a std::shared_ptr.
     */
    LCorePool() = default;

    /**
     * @brief Implementation-specific core acquisition logic.
     *
     * Must be overridden in derived classes.
     * @param socketId Preferred NUMA socket.
     * @return The acquired core.
     * @throws amon::Error if no suitable core is available.
     */
    virtual LCore acquireCore(SocketID socketId) = 0;

    /**
     * @brief Implementation-specific core release logic.
     *
     * Must be overridden in derived classes. Called when a core lease is returned.
     * @param core The core to release back to the pool.
     */
    virtual void releaseCore(LCore core) = 0;
};

/**
 * @brief Manages a DPDK worker lcore lifetime and task execution.
 *
 * Supports launching a function on the remote core, signalling stop, and joining (waiting for
 * completion).
 *
 * All non-const methods of this class must be called from the main lcore only!
 */
class LCoreHandle {
public:
    /** @brief Type for the target function executed on the remote lcore.
     *
     * The function receives a reference to an atomic bool stopFlag; when set to true,
     * the function should stop its execution gracefully.
     */
    using TargetFn = std::function<void(const std::atomic_bool& stopFlag)>;

    /**
     * @brief Use the default implementation of move constructor.
     * @param other LCoreHandle to be moved.
     */
    LCoreHandle(LCoreHandle&& other) = default;

    /**
     * @brief Use the default implementation of move assignment operator.
     * @param other LCoreHandle to be moved.
     * @return Reference to moved LCoreHandle.
     */
    LCoreHandle& operator=(LCoreHandle&& other) = default;

    /**
     * @brief Stops and joins any running task before releasing the core back to the pool.
     *
     * @warning To be called from main lcore only!
     */
    ~LCoreHandle() noexcept;

    /**
     * @brief Launches a function on the owned lcore via rte_eal_remote_launch.
     *
     * The `start()` method can be called again after the previously started function has
     * completed and `join()` has been called.
     *
     * @param target Function to execute.
     * @throws DpdkError or RteErrnoError if a task is already running or launch fails.
     * @note std::terminate is called if target function throws.
     * @warning To be called from main lcore only!
     */
    void start(TargetFn target);

    /**
     * @brief Signals the running task to stop by setting the stop flag.
     *
     * If no task is running, this method does nothing.
     */
    void requestStop();

    /**
     * @brief Blocks until the launched function finishes. Resets internal launch state.
     *
     * You need to call this before calling `start()` again.
     *
     * @throws DpdkError when core never started.
     * @warning To be called from main lcore only!
     */
    void join();

    /**
     * @brief Returns true if a launched function is currently executing.
     *
     * The launched function is considered finished once it returns.
     *
     * @return True if the launched function running.
     */
    bool isRunning() const;

    /**
     * @brief Format this value to string.
     * @param fmt The formatter.
     */
    void format(amon::util::Formatter& fmt) const;

private:
    friend class LCorePool;

    /// @brief Holds a leased core and the pool it belongs to.
    struct Lease {
        LCore core; ///< The leased core.
        std::shared_ptr<LCorePool> owner; ///< The pool that owns the core.

        /// @brief Releases a leased core back to its originating pool.
        struct Release {
            /// @brief Releases the leased core back to the pool.
            /// @param lease Lease to release.
            void operator()(const Lease& lease) const noexcept;
        };
    };

    /// @brief Holds the target function, its context, and running state.
    struct LaunchContext {
        /// User-supplied target function.
        TargetFn function;
        /// The core/socket ID.
        LCore core;
        /// The logger.
        Logger logger;
        /// True while the function is executing on the lcore.
        std::atomic_bool running = true;
        /// Set to true to request the target function to stop.
        std::atomic_bool stopFlag = false;
    };

    LCoreHandle(LCore core, std::shared_ptr<LCorePool> pool);

    /// RAII-owned core, returned to pool on release.
    util::UniqueResource<Lease, Lease::Release> m_lease;
    /// Launch state (nullptr before start / after join).
    std::unique_ptr<LaunchContext> m_launchCtx;
    /// Per-instance logger.
    Logger m_logger;
};

} // namespace amon::dpdk
