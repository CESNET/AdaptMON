/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief DPDK pktmempool representation.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/dpdk/PktBurst.hpp>
#include <amon/dpdk/SocketID.hpp>

#include <memory>
#include <unordered_map>

#include <rte_mbuf.h>

namespace amon::dpdk {

/**
 * @brief Configuration for a DPDK packet mempool.
 */
struct PktMempoolConfig {
    /// Number of packets in the mempool.
    unsigned int nPackets;
    /// Per-lcore cache size.
    unsigned int cacheSize = 0;
    /// Private data size for each mbuf.
    uint16_t privateDataSize = 0;
    /// Data room size for each mbuf, automatically adjusted for RTE_PKTMBUF_HEADROOM etc..
    uint16_t dataRoomSize;
    /// NUMA socket ID.
    SocketID socketId;
};

/**
 * @brief Shared handle for a DPDK packet mempool.
 */
class PktMempoolHandle {
public:
    /**
     * @brief Create a packet mempool using rte_pktmbuf_pool_create().
     * @param config Packet mempool configuration.
     * @throws RteErrnoError On DPDK error.
     */
    PktMempoolHandle(const PktMempoolConfig& config);

    /**
     * @brief Get the underlying DPDK mempool.
     * @return Raw DPDK mempool pointer.
     */
    rte_mempool* get() const { return m_ptr.get(); }

    /**
     * @brief Allocate packets from the mempool into a burst.
     *
     * Only remaining empty slots of the burst are filled. The mbufs already present in the burst
     * are unaffected. In other words, the method will try to populate at most `burst.capacity() -
     * burst.size()` mbuf pointers.
     *
     * In case allocation fails, the provided burst remains unmodified.
     *
     * @param burst Burst to fill with allocated packets.
     * @return true on success, false when the mempool does not have enough free entries.
     */
    bool tryAllocate(PktBurst& burst) const noexcept;

    /**
     * @brief Allocate packets from the mempool into a span.
     *
     * The caller takes ownership of the mbufs and is responsible for freeing them (e.g. using the
     * `rte_pkmtbuf_free` function).
     *
     * @param mbufs Span to fill with allocated packets.
     * @return true on success, false when the mempool does not have enough free entries.
     */
    bool tryAllocate(std::span<rte_mbuf*> mbufs) const noexcept;

    /**
     * @brief Allocate packets from the mempool into a burst.
     *
     * Only remaining empty slots of the burst are filled. The mbufs already present in the burst
     * are unaffected. In other words, the method will populate at most `burst.capacity() -
     * burst.size()` mbuf pointers.
     *
     * In case allocation fails, the provided burst remains unmodified.
     *
     * @param burst Burst to fill with allocated packets.
     * @throws amon::DpdkError When the mempool does not have enough free entries.
     */
    void allocate(PktBurst& burst) const;

    /**
     * @brief Allocate packets from the mempool into a span.
     *
     * The caller takes ownership of the mbufs and is responsible for freeing them (e.g. using the
     * `rte_pkmtbuf_free` function).
     *
     * @param mbufs Span to fill with allocated packets.
     * @throws amon::DpdkError When the mempool does not have enough free entries.
     */
    void allocate(std::span<rte_mbuf*> mbufs) const;

private:
    std::shared_ptr<rte_mempool> m_ptr;
};

/**
 * @brief Interface for providing packet mempools based on an ID.
 */
class PktMempoolProvider {
public:
    /**
     * @brief Get a packet mempool handle for the given ID.
     * @param id Identifier of the mempool (usually you'll want this to be worker ID/queue ID).
     * @return A shared handle to the packet mempool.
     */
    virtual PktMempoolHandle get(unsigned id) = 0;

    /**
     * @brief Virtual destructor.
     */
    virtual ~PktMempoolProvider() = default;
};

/**
 * @brief Simple provider that creates packet mempools on demand using a fixed configuration.
 *
 * All mempools are created with the same @ref PktMempoolConfig passed at construction.
 */
class PktMempoolBasicProvider : public PktMempoolProvider {
public:
    /**
     * @brief Create a shared instance of the basic provider.
     * @param config Fixed configuration for all created mempools.
     * @return Shared pointer to the provider.
     */
    static std::shared_ptr<PktMempoolProvider> create(PktMempoolConfig config);

    /**
     * @brief Construct a basic provider with a fixed configuration.
     * @param config Configuration for all mempools retrieved via get().
     */
    explicit PktMempoolBasicProvider(PktMempoolConfig config);

    /**
     * @brief Get or create a packet mempool for the given ID.
     *
     * If a mempool with the given ID does not exist yet, it is created using the fixed
     * configuration. Otherwise the existing handle is returned.
     *
     * @param id Identifier to distinguish different mempools.
     * @return Handle to the packet mempool.
     */
    PktMempoolHandle get(unsigned id) override;

private:
    PktMempoolConfig m_config;
    std::unordered_map<unsigned, PktMempoolHandle> m_mempools;
};

} // namespace amon::dpdk
