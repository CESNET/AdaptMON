/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief DPDK pktmempool representation.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon/Logger.hpp>
#include <amon/amon.hpp>
#include <amon/dpdk/Error.hpp>
#include <amon/dpdk/PktMempool.hpp>
#include <amon/util/NameGenerator.hpp>

#include <format>
#include <string>

namespace amon::dpdk {

static Logger g_logger = LoggerProvider::get("PktMempool");

PktMempoolHandle::PktMempoolHandle(const PktMempoolConfig& config)
{
    static util::NameGenerator nameGen {"amonPktMempool{}"};
    const std::string name {nameGen.next()};

    g_logger.info("Creating mempool '{}'", name);

    auto dataRoomSize = static_cast<unsigned>(config.dataRoomSize + RTE_PKTMBUF_HEADROOM + 50);
    if (dataRoomSize > std::numeric_limits<uint16_t>::max()) {
        dataRoomSize = std::numeric_limits<uint16_t>::max();
    }

    rte_mempool* ptr = rte_pktmbuf_pool_create(
        name.c_str(),
        config.nPackets,
        config.cacheSize,
        config.privateDataSize,
        static_cast<uint16_t>(dataRoomSize),
        config.socketId);
    if (ptr == nullptr) {
        throw RteErrnoError(
            "rte_pktmbufpool_create",
            name,
            config.nPackets,
            config.cacheSize,
            config.privateDataSize,
            dataRoomSize,
            static_cast<int>(config.socketId));
    }

    g_logger.debug(
        "Allocated mempool {} (addr={}, nPackets={}, cacheSize={}, privateDataSize={}, "
        "dataRoomSize={}, socketId={})",
        name,
        (void*) (ptr),
        config.nPackets,
        config.cacheSize,
        config.privateDataSize,
        dataRoomSize,
        config.socketId);

    m_ptr.reset(ptr, [name = name](rte_mempool* ptr) {
        g_logger.debug("Freeing mempool '{}' (addr={})", name, (void*) (ptr));
        rte_mempool_free(ptr);
    });
}

bool PktMempoolHandle::tryAllocate(PktBurst<rte_mbuf>& burst) const noexcept
{
    int ret;
    burst.fill([&](std::span<rte_mbuf*> mbufs) -> size_t {
        ret = rte_pktmbuf_alloc_bulk(get(), mbufs.data(), static_cast<unsigned int>(mbufs.size()));
        return ret == 0 ? mbufs.size() : 0;
    });
    return ret == 0;
}

bool PktMempoolHandle::tryAllocate(PktBurst<DpdkPacket>& burst) const noexcept
{
    int ret;
    burst.fill([&](std::span<DpdkPacket*> packets) -> size_t {
        auto** mbufs = reinterpret_cast<rte_mbuf**>(packets.data());
        ret = rte_pktmbuf_alloc_bulk(get(), mbufs, static_cast<unsigned int>(packets.size()));
        if (ret != 0) {
            return 0;
        }
        for (auto& pkt : packets) {
            std::construct_at(&pkt->packet());
        }
        return packets.size();
    });
    return ret == 0;
}

bool PktMempoolHandle::tryAllocate(std::span<rte_mbuf*> mbufs) const noexcept
{
    return rte_pktmbuf_alloc_bulk(get(), mbufs.data(), static_cast<unsigned int>(mbufs.size()))
        == 0;
}

void PktMempoolHandle::allocate(PktBurst<rte_mbuf>& burst) const
{
    if (!tryAllocate(burst)) {
        throw DpdkError {"not enough free entries in mempool"};
    }
}

void PktMempoolHandle::allocate(PktBurst<DpdkPacket>& burst) const
{
    if (!tryAllocate(burst)) {
        throw DpdkError {"not enough free entries in mempool"};
    }
}

void PktMempoolHandle::allocate(std::span<rte_mbuf*> mbufs) const
{
    if (!tryAllocate(mbufs)) {
        throw DpdkError {"not enough free entries in mempool"};
    }
}

std::shared_ptr<PktMempoolProvider> PktMempoolBasicProvider::create(PktMempoolConfig config)
{
    return std::make_shared<PktMempoolBasicProvider>(config);
}

PktMempoolBasicProvider::PktMempoolBasicProvider(PktMempoolConfig config)
    : m_config(config)
{
}

PktMempoolHandle PktMempoolBasicProvider::get(unsigned id)
{
    auto [it, _] = m_mempools.try_emplace(id, m_config);
    return it->second;
}

} // namespace amon::dpdk
