/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief DPDK NUMA socket ID representation.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon/dpdk/SocketID.hpp>

#include <rte_memory.h>

namespace amon::dpdk {

const SocketID SocketID::ANY = SocketID {SOCKET_ID_ANY};

} // namespace amon::dpdk
