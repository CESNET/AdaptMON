/**
 * @file
 * @author Jaroslav Renda <jaroslav.renda@cesnet.cz>
 * @brief DPDK extended statistics.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/dpdk/Port.hpp>

#include <string>
#include <vector>

#include <rte_ethdev.h>

namespace amon::dpdk {

/**
 * @brief Wrapper around DPDK's Extended Statistics.
 *
 * Cannot be constructed on it's own. Instead obtain the instance via PortHandle::stats().
 */
class XStats {
public:
    /**
     * @brief Get all the statistics names as strings.
     *
     * @return A vector of statistics names.
     */
    const std::vector<std::string>& names() const;

    /**
     * @brief Get stat value from the name of the statistic.
     *
     * @note Getting the value from a string is not ideal, but it is more convenient.
     *
     * @param name The name of the statistic.
     * @return Value of the statistic.
     * @throws RteErrnoError If rte_eth_xstats_get_id_by_name fails (e.g. unknown stat name).
     * @throws DpdkError If rte_eth_xstats_get_by_id fails
     */
    uint64_t get(const std::string& name) const;

private:
    friend class Port;

    PortHandle m_portHandle;
    std::vector<std::string> m_names;

    XStats(PortHandle portHandle);
};

} // namespace amon::dpdk
