/**
 * @file
 * @author Samuel Luptak <luptak@cesnet.cz>
 * @brief Statistics manager
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 */

#include "../printers/OutputPrinter.hpp"
#include "Stats.hpp"
#include "StatsManager.hpp"
#include "StatsQueue.hpp"

#include <amon/Error.hpp>

#include <cstdint>
#include <memory>
#include <vector>

StatsManager::StatsManager(uint16_t queueCnt, std::unique_ptr<OutputPrinter> printer)
    : m_printer(std::move(printer))
{
    if (m_printer == nullptr) {
        throw amon::Error("StatsManager: printer must not be null");
    }

    m_queues.reserve(queueCnt);
    for (size_t queueId = 0; queueId < queueCnt; queueId++) {
        m_queues.emplace_back(std::make_unique<StatsQueue>(queueId));
    }
}

StatsTimestampedAtomic& StatsManager::getAtomic(uint16_t queueId)
{
    return m_queues.at(queueId)->getAtomicForWorker();
}

void StatsManager::updateAndPrintStats()
{
    std::vector<StatsPQ> currentStats;
    currentStats.reserve(m_queues.size());

    for (auto& queue : m_queues) {
        queue->updateStats();
        currentStats.push_back(queue->getCurrentStats());
    }

    m_printer->printStats(currentStats);
}

void StatsManager::printSummary()
{
    std::vector<Stats> statsFinal;
    statsFinal.reserve(m_queues.size());

    for (auto& queue : m_queues) {
        statsFinal.push_back(queue->getCurrentStats().statsTotal);
    }

    m_printer->printSummary(statsFinal);
}
