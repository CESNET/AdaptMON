/**
 * @file
 * @author Samuel Luptak <luptak@cesnet.cz>
 * @brief Statistics manager
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 */

#pragma once

#include "../printers/OutputPrinter.hpp"
#include "Stats.hpp"
#include "StatsQueue.hpp"

#include <cstdint>
#include <memory>
#include <vector>

/**
 * @brief Owns the per-queue trackers and drives periodic statistics printing.
 *
 * Creates one StatsQueue per RX queue, hands each worker its atomic interface,
 * and periodically samples all queues and forwards the results to the configured printer.
 */
class StatsManager {
public:
    /**
     * @brief Constructs the manager for the given number of queues.
     * @param queueCnt Number of RX queues to track.
     * @param printer Output printer used for periodic and final stats.
     */
    StatsManager(uint16_t queueCnt, std::unique_ptr<OutputPrinter> printer);

    /**
     * @brief Returns the atomic interface for the worker of the given queue.
     * @param queueId Identifier of the queue whose slot is requested.
     * @return Reference to the seqlock-protected structure.
     */
    [[nodiscard]] StatsTimestampedAtomic& getAtomic(uint16_t queueId);

    /**
     * @brief Samples every queue and prints the current statistics.
     */
    void updateAndPrintStats();

    /**
     * @brief Prints the final cumulative summary across all queues.
     */
    void printSummary();

private:
    std::vector<std::unique_ptr<StatsQueue>> m_queues; ///< Per-queue trackers.
    std::unique_ptr<OutputPrinter> m_printer; ///< Output printer.
};
