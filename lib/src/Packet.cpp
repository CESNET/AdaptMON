/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief Packet structure and API for parsed network packets.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon/Packet.hpp>

namespace amon {

static_assert(
    sizeof(std::variant<PacketLayer, ErrorLayer>) == 8,
    "Layer (std::variant<PacketLayer, ErrorLayer>) structure size is not 8 bytes.");

} // namespace amon
