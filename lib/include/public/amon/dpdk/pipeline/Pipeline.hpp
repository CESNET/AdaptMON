/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Packet processing pipeline orchestration.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/dpdk/DpdkPacket.hpp>
#include <amon/dpdk/GlobalLCorePool.hpp>
#include <amon/dpdk/LCore.hpp>
#include <amon/dpdk/PktBurst.hpp>
#include <amon/dpdk/pipeline/Blueprint.hpp>
#include <amon/dpdk/pipeline/Stage.hpp>

#include <memory>
#include <vector>

namespace amon::dpdk::pipeline {

/** @brief Runtime resources and sizing parameters used to start a pipeline. */
struct PipelineConfig {
    /// NUMA socket from which worker lcores are acquired.
    SocketID socketId = SocketID::ANY;
    /// Pool that supplies the worker lcores.
    std::shared_ptr<LCorePool> lcorePool;
    /// Number of parallel streams and worker lcores.
    uint16_t numStreams;
    /// Maximum number of packets processed in one burst.
    uint16_t burstSize;
    /// Name of the pipeline.
    std::string name = "main";
};

/**
 * @brief Instantiates and runs the stages described by a Blueprint.
 *
 * Example usage
 * -------------
 *
 * The following example creates a pipeline that receives packets from one port and transmits them
 * through another using four parallel streams:
 * ```
 * auto blueprint = Blueprint {};
 * blueprint.add<PortReader>(inputPort);
 * blueprint.add<PortWriter>(outputPort);
 *
 * auto pipeline = Pipeline {
 *     std::move(blueprint),
 *     {
 *         .socketId = inputPort.socketId(),
 *         .lcorePool = GlobalLCorePool::create(),
 *         .numStreams = 4,
 *         .burstSize = 64,
 *      }
 * };
 * pipeline.start();
 *
 * // Once processing should stop:
 * pipeline.requestStop();
 * pipeline.join();
 * ```
 *
 * Exception handling
 * ------------------
 *
 * If an exception is thrown by any of the workers, a stacktrace is logged and the exception pointer
 * is stored to be retrieved using the `activeException()` method. Additionally, signal SIGUSR1 is
 * sent to unblock the main thread in case it would be waiting on a signal handler.
 */
class Pipeline {
public:
    /**
     * @brief Construct a pipeline from a blueprint. Acquires lcores and instantiates stages.
     * @param blueprint Stage definitions to instantiate when the pipeline starts.
     * @param config Runtime resources and pipeline sizing parameters.
     */
    Pipeline(Blueprint blueprint, PipelineConfig config);

    /// @brief Deleted move ctor.
    Pipeline(Pipeline&&) = delete;
    /// @brief Deleted move assignment ctor.
    Pipeline& operator=(Pipeline&&) = delete;
    /// @brief Deleted copy ctor.
    Pipeline(const Pipeline&) = delete;
    /// @brief Deleted copy assignment ctor.
    Pipeline& operator=(const Pipeline&) = delete;
    /// @brief Default dtor.
    ~Pipeline() = default;

    /// @brief Start packet processing.
    void start();

    /// @brief Request all pipeline workers to stop.
    void requestStop();

    /// @brief Wait for all pipeline workers to finish.
    void join();

    /**
     * @brief Get the name of the pipeline.
     * @return The name.
     */
    const std::string& name() const { return m_config.name; }

    /**
     * @brief Get stages the pipeline holds.
     * @return The stages.
     */
    const std::vector<std::unique_ptr<Stage>>& stages() const { return m_stages; }

    /**
     * @brief Get the exception that was last thrown by a worker.
     * @return The exception (if any).
     * @warning Mutex operations are involved.
     */
    const std::exception_ptr& activeException();

private:
    std::vector<std::unique_ptr<Stage>> m_stages;
    std::vector<LCoreHandle> m_lcores;
    PipelineConfig m_config;
    Logger m_logger;

    std::mutex m_activeExceptionMutex;
    std::exception_ptr m_activeExceptionPtr {};

    void lcoreMain(uint16_t workerId, const std::atomic_bool& stopFlag) noexcept;
    void exceptionHandler(uint16_t workerId, const std::exception_ptr& ex) noexcept;
};

} // namespace amon::dpdk::pipeline
