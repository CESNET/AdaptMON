/**
 * @file
 * @author Samuel Luptak <luptak@cesnet.cz>
 * @brief Per-queue statistics
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 */

#include "Stats.hpp"
#include "StatsQueue.hpp"

#include <cstdint>

#include <rte_cycles.h>

static uint64_t castPerSecond(uint64_t stat, uint64_t deltaCycles)
{
    static const uint64_t hz = rte_get_tsc_hz();
    return static_cast<uint64_t>(
        static_cast<double>(stat) * static_cast<double>(hz) / static_cast<double>(deltaCycles));
}

StatsQueue::StatsQueue(uint16_t queueId)
{
    m_statsPQ.queueId = queueId;
}

void StatsQueue::updateStats()
{
    const StatsTimestamped newStats = m_timestampedAtomic.load();
    const uint64_t deltaCycles = newStats.cycles - m_oldCycles;

    if (deltaCycles == 0) {
        return;
    }

    if (m_oldCycles == 0) {
        m_oldCycles = newStats.cycles;
        m_statsPQ.statsTotal = newStats.stats;
        return;
    }

    auto& st = m_statsPQ.statsTotal;
    m_statsPQ.statsPerSecond
        = {.pkts = castPerSecond(newStats.stats.pkts - st.pkts, deltaCycles),
           .l1Bytes = castPerSecond(newStats.stats.l1Bytes - st.l1Bytes, deltaCycles),
           .l2Bytes = castPerSecond(newStats.stats.l2Bytes - st.l2Bytes, deltaCycles),
           .l3Bytes = castPerSecond(newStats.stats.l3Bytes - st.l3Bytes, deltaCycles)};
    m_statsPQ.statsTotal = newStats.stats;

    m_oldCycles = newStats.cycles;
}

StatsPQ StatsQueue::getCurrentStats() const
{
    return m_statsPQ;
}

StatsTimestampedAtomic& StatsQueue::getAtomicForWorker()
{
    return m_timestampedAtomic;
}
