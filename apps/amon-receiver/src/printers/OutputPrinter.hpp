/**
 * @file
 * @author Samuel Luptak <luptak@cesnet.cz>
 * @brief Output printer interface
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 */

#pragma once

#include "../stats/Stats.hpp"

#include <span>

/** @brief Abstract interface for rendering packet statistics. */
class OutputPrinter {
public:
    /**
     * @brief Constructs the printer with the L3 display flag.
     * @param l3Enabled Whether the L3 statistics column should be rendered.
     */
    explicit OutputPrinter(bool l3Enabled)
        : m_l3Enabled(l3Enabled)
    {
    }

    /** @brief Default destructor. */
    virtual ~OutputPrinter() = default;

    /** @brief Prints the current statistics for all queues.
     * @param statsAll Per-queue statistics to print.
     */
    virtual void printStats(std::span<const StatsPQ> statsAll) const = 0;

    /** @brief Prints the final summary across all queues.
     * @param statsTotal Statistics per queue to summarize.
     */
    virtual void printSummary(std::span<const Stats> statsTotal) const = 0;

protected:
    /**
     * @brief Reports whether the L3 statistics should be rendered.
     * @return True if L3 statistics should be rendered.
     */
    bool l3Enabled() const { return m_l3Enabled; }

private:
    bool m_l3Enabled; ///< L3 statistics flag.
};
