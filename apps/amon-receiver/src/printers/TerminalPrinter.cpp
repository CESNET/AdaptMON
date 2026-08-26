/**
 * @file
 * @author Samuel Luptak <luptak@cesnet.cz>
 * @brief Terminal output printer
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 */

#include "../stats/Stats.hpp"
#include "TerminalPrinter.hpp"

#include <amon/util/ansi.hpp>

#include <print>

static void printQueueStats(const StatsPQ& queueStats, bool l3Enabled)
{
    const auto& perSecond = queueStats.statsPerSecond;
    uint64_t mbpsl1 = perSecond.l1Bytes * 8 / 1'000'000;
    uint64_t mbpsl2 = perSecond.l2Bytes * 8 / 1'000'000;
    uint64_t kpps = perSecond.pkts / 1'000;
    if (l3Enabled) {
        uint64_t mbpsl3 = perSecond.l3Bytes * 8 / 1'000'000;
        std::println(
            "{:>6}. | {:>10} | {:>10} | {:>10} | {:>8}",
            queueStats.queueId,
            mbpsl1,
            mbpsl2,
            mbpsl3,
            kpps);
    } else {
        std::println("{:>6}. | {:>10} | {:>10} | {:>8}", queueStats.queueId, mbpsl1, mbpsl2, kpps);
    }
}

void TerminalPrinter::printStats(std::span<const StatsPQ> statsAll) const
{
    const std::string longLine(57, '-');
    const std::string shortLine(44, '-');

    std::print(AMON_ANSI_CLEAR_SCREEN "\033[1;1H");

    std::print("{:<7} | {:>10} | {:>10} | ", "Queue", "Mbps (L1)", "Mbps (L2)");
    if (l3Enabled()) {
        std::print("{:>10} | ", "Mbps (L3)");
    }
    std::println("{:>8}", "Kpps");

    if (l3Enabled()) {
        std::println("{}", longLine);
    } else {
        std::println("{}", shortLine);
    }

    Stats cumulativePerSecond;
    for (const StatsPQ& queueStat : statsAll) {
        cumulativePerSecond += queueStat.statsPerSecond;
        printQueueStats(queueStat, l3Enabled());
    }

    if (l3Enabled()) {
        std::println("{}", longLine);
    } else {
        std::println("{}", shortLine);
    }

    uint64_t mbpsl1 = cumulativePerSecond.l1Bytes * 8 / 1'000'000;
    uint64_t mbpsl2 = cumulativePerSecond.l2Bytes * 8 / 1'000'000;
    uint64_t kpps = cumulativePerSecond.pkts / 1'000;
    std::print("{:<7} | {:>10} | {:>10} | ", "Summary", mbpsl1, mbpsl2);
    if (l3Enabled()) {
        uint64_t mbpsl3 = cumulativePerSecond.l3Bytes * 8 / 1'000'000;
        std::print("{:>10} | ", mbpsl3);
    }
    std::println("{:>8}", kpps);
}

void TerminalPrinter::printSummary(std::span<const Stats> statsTotal) const
{
    const std::string line(44, '-');

    std::print(AMON_ANSI_CLEAR_SCREEN "\033[1;1H");

    std::println("Final summary");
    std::println("{}", line);

    Stats cumulativeTotal;

    for (const Stats& queueStat : statsTotal) {
        cumulativeTotal += queueStat;
    }

    std::println("{:<15} {:>20}", "Total Packets:", cumulativeTotal.pkts);
    std::println("{:<15} {:>20}", "Total L1 Bytes:", cumulativeTotal.l1Bytes);
    std::println("{:<15} {:>20}", "Total L2 Bytes:", cumulativeTotal.l2Bytes);
    if (l3Enabled()) {
        std::println("{:<15} {:>20}", "Total L3 Bytes:", cumulativeTotal.l3Bytes);
    }

    std::println("{}", line);
}
