/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Global LCore Pool.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon/dpdk/GlobalLCorePool.hpp>

#include <cstdint>

#include <rte_lcore.h>

namespace amon::dpdk {

std::shared_ptr<LCorePool> GlobalLCorePool::create()
{
    return std::shared_ptr<LCorePool>(
        new GlobalLCorePool); // make_shared does not work with private constructors
}

GlobalLCorePool::GlobalLCorePool()
    : m_logger(LoggerProvider::get("GlobalLCorePool"))
{
    unsigned coreId;

    m_logger.debug("Initializing");

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
    RTE_LCORE_FOREACH_WORKER(coreId)
#pragma GCC diagnostic pop
    {
        if (rte_eal_get_lcore_state(coreId) != rte_lcore_state_t::WAIT) {
            continue;
        }

        const LCore core {
            .coreId = static_cast<uint16_t>(coreId),
            .socketId = static_cast<uint16_t>(rte_lcore_to_socket_id(coreId)),
        };
        m_cores.push_back(core);
        m_logger.debug("Discovered {}", core);
    }
}

size_t GlobalLCorePool::size(SocketID socketId)
{
    const std::lock_guard<std::mutex> _ {m_lock};

    if (socketId == SocketID::ANY) {
        return m_cores.size();
    }

    return static_cast<size_t>(
        std::ranges::count(m_cores, static_cast<unsigned int>(socketId), &LCore::socketId));
}

LCore GlobalLCorePool::acquireCore(SocketID socketId)
{
    const std::lock_guard<std::mutex> _ {m_lock};

    if (m_cores.empty()) {
        throw amon::Error("no more available cores");
    }

    auto coreIt = m_cores.begin();

    if (socketId != SocketID::ANY) {
        coreIt = std::ranges::find_if(m_cores, [&](const LCore& core) {
            return core.socketId == unsigned(socketId);
        });

        if (coreIt == m_cores.end()) {
            m_logger.warn(
                "Core with socket ID {} not available, fallback to first available core",
                socketId);
            coreIt = m_cores.begin();
        }
    }

    const LCore core = *coreIt;
    m_cores.erase(coreIt);

    m_logger.debug("Handing out {}", core);

    return core;
}

void GlobalLCorePool::releaseCore(LCore core)
{
    const std::lock_guard<std::mutex> _ {m_lock};

    m_cores.push_back(core);

    m_logger.debug("Got back {}", core);
}

} // namespace amon::dpdk
