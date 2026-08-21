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

#include <cstddef>

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

const std::vector<std::string>& XStats::names() const
{
    return m_names;
}

uint64_t XStats::get(const std::string& name) const
{
    int ret;
    uint64_t id = 0;
    uint64_t value = 0;

    // get Id from name
    ret = rte_eth_xstats_get_id_by_name(m_portHandle.id(), name.c_str(), &id);
    if (ret < 0) {
        throw RteErrnoError(-ret, "rte_eth_xstats_get_id_by_name", m_portHandle.id(), name);
    }

    // get Value from ID
    ret = rte_eth_xstats_get_by_id(m_portHandle.id(), &id, &value, 1);
    if (ret < 0) {
        throw DpdkError("Couldn't fetch value of name: {}", name);
    }
    AMON_REQUIRE(ret == 1);

    return value;
}

std::vector<XStatsItem> XStats::all()
{
    std::vector<rte_eth_xstat> stats;
    stats.resize(m_names.size());
    const int ret = rte_eth_xstats_get(
        m_portHandle.id(),
        stats.data(),
        static_cast<unsigned>(m_names.size()));
    AMON_REQUIRE(ret > 0);
    AMON_REQUIRE(static_cast<size_t>(ret) == m_names.size());

    std::vector<XStatsItem> out;
    out.reserve(m_names.size());
    for (size_t i = 0; i < m_names.size(); ++i) {
        out.push_back({.idx = stats[i].id, .name = m_names[i], .value = stats[i].value});
    }
    return out;
}

} // namespace amon::dpdk
