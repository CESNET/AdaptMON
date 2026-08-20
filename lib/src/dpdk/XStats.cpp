/**
 * @file
 * @author Jaroslav Renda <jaroslav.renda@cesnet.cz>
 * @brief DPDK extended statistics.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon/amon.hpp>
#include <amon/dpdk/Error.hpp>
#include <amon/dpdk/XStats.hpp>
#include <amon/util/numeric.hpp>

#include <rte_ethdev.h>

namespace amon::dpdk {

XStats::XStats(PortHandle portHandle)
    : m_portHandle(std::move(portHandle))
{
    // Get number of statistics
    const int nStats = rte_eth_xstats_get_names(m_portHandle.id(), nullptr, 0);
    AMON_REQUIRE(nStats >= 0);

    // the get_names function requires only unsigned and the size of vector is in size_t
    const auto vectorSize = static_cast<unsigned>(nStats);

    // Fetch and store the names of the statistics
    std::vector<rte_eth_xstat_name> names;
    names.resize(vectorSize);
    rte_eth_xstats_get_names(m_portHandle.id(), names.data(), vectorSize);

    // Create a string vector of statistics names
    m_names.reserve(names.size());
    for (const auto& stat : names) {
        m_names.emplace_back(stat.name);
    }
}

} // namespace amon::dpdk
