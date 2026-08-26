/**
 * @file
 * @author Samuel Luptak <luptak@cesnet.cz>
 * @brief Terminal output printer
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 */

#pragma once

#include "OutputPrinter.hpp"

/** @brief Prints packet statistics to the terminal. */
class TerminalPrinter final : public OutputPrinter {
public:
    /** @brief Constructs the terminal printer with the L3 display flag.
     * @param l3Enabled Whether the L3 statistics column is displayed.
     */
    explicit TerminalPrinter(bool l3Enabled)
        : OutputPrinter(l3Enabled)
    {
    }

    /** @brief Prints the current per-second rates for all queues.
     * @param statsAll Per-queue per-second statistics to print.
     */
    void printStats(std::span<const StatsPQ> statsAll) const override;

    /** @brief Prints the final cumulative summary across all queues.
     * @param statsTotal Cumulative statistics per queue to summarize.
     */
    void printSummary(std::span<const Stats> statsTotal) const override;
};
