/**
 * @file
 * @author Samuel Luptak <luptak@cesnet.cz>
 * @brief Receiver statistics data structures
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 */

#pragma once

#include <amon/types/SeqLock.hpp>

#include <cstdint>

/**
 * @brief Structure representing arbitrary statistics.
 */
struct Stats {
    uint64_t pkts = 0; ///< Number of packets (Total, PerSecond, ...).
    uint64_t l1Bytes = 0; ///< Number of Layer 1 bytes (Total, PerSecond, ...).
    uint64_t l2Bytes = 0; ///< Number of layer 2 bytes (Total, PerSecond, ...).
    uint64_t l3Bytes = 0; ///< Number of layer 3 bytes (Total, PerSecond, ...).

    /**
     * @brief Accumulates the counters of @p other into this Stats.
     * @param other Counters to accumulate.
     * @return Reference to this Stats.
     */
    constexpr Stats& operator+=(const Stats& other)
    {
        pkts += other.pkts;
        l1Bytes += other.l1Bytes;
        l2Bytes += other.l2Bytes;
        l3Bytes += other.l3Bytes;
        return *this;
    }
};

/**
 * @brief Stats tagged with the TSC cycle count at sampling time.
 *
 * This structure is used as an interface between the worker threads
 * and the statistics thread.
 */
struct StatsTimestamped {
    Stats stats; ///< Total data received updated by the worker.
    uint64_t cycles = 0; ///< TSC value at the time of the update.
};
/**
 * @brief Lock-free single-writer/multi-reader holder of StatsTimestamped,
 * written by the RX worker and read by the stats thread.
 */
using StatsTimestampedAtomic = amon::types::SeqLock<StatsTimestamped>;

/**
 * @brief Per-queue pre-computed statistics.
 *
 * This structure is used as an interface between the StatsManager/StatsQueue and
 * the OutputPrinter abstract class.
 */
struct StatsPQ {
    uint16_t queueId; ///< Identifier of the queue these stats belong to.

    Stats statsPerSecond; ///< 'Per second' statistics.
    Stats statsTotal; ///< 'Total received' statistics.
};
