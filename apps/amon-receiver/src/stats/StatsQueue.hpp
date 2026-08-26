/**
 * @file
 * @author Samuel Luptak <luptak@cesnet.cz>
 * @brief Per-queue statistics.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 */

#pragma once

#include "Stats.hpp"

#include <cstdint>

/**
 * @brief Tracks statistics for a single RX queue.
 */
class StatsQueue {
public:
    /**
     * @brief Constructor.
     * @param queueId Identifier of the RX queue this instance tracks.
     */
    StatsQueue(uint16_t queueId);

    /**
     * @brief Samples the worker's total data received and recomputes m_statsPQ.
     */
    void updateStats();

    /**
     * @brief Returns the most recently computed statistics.
     * @return Per-queue stats prepared for printing.
     */
    [[nodiscard]] StatsPQ getCurrentStats() const;

    /**
     * @brief Returns the atomic structure m_timestampedAtomic.
     * @return Reference to the seqlock-protected structure.
     */
    [[nodiscard]] StatsTimestampedAtomic& getAtomicForWorker();

private:
    StatsPQ m_statsPQ; ///< Last computed statistics.
    uint64_t m_oldCycles {0}; ///< TSC value of the previous sample.

    StatsTimestampedAtomic m_timestampedAtomic; ///< Worker-written seqlock.
};
