/**
 * @file
 * @author Lukáš Huták <hutak@cesnet.cz>
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief IP address and network representation and manipulation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon/types/IPAddress.hpp>

#include <type_traits>

namespace amon::types {

static_assert(std::is_standard_layout<IPv4>(), "Unexpected data type layout for IPv4");
static_assert(std::is_standard_layout<IPv6>(), "Unexpected data type layout for IPv6");
static_assert(std::is_trivially_default_constructible<IPv4>(), "Unexpected construction for IPv4");
static_assert(std::is_trivially_default_constructible<IPv6>(), "Unexpected construction for IPv6");
static_assert(sizeof(IPv4) == 4, "IPv4 size must be 4 bytes");
static_assert(sizeof(IPv6) == 16, "IPv6 size must be 16 bytes");

} // namespace amon::types
