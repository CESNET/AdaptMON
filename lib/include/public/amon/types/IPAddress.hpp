/**
 * @file
 * @author Lukáš Huták <hutak@cesnet.cz>
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief IP address and network representation and manipulation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/types/ByteArray.hpp>

namespace amon::types {

/**
 * @class IPv4
 * @brief Represents an IPv4 address.
 *
 * Inherits from ByteArray<4>.
 */
class [[gnu::packed]] IPv4 : public ByteArray<4> {};

/**
 * @class IPv6
 * @brief Represents an IPv6 address.
 *
 * Inherits from ByteArray<16>.
 */
class [[gnu::packed]] IPv6 : public ByteArray<16> {};

} // namespace amon::types
