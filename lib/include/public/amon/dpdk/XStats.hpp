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
private:
    friend class Port;

    PortHandle m_portHandle;
    std::vector<std::string> m_names;

    XStats(PortHandle portHandle);
};

} // namespace amon::dpdk
