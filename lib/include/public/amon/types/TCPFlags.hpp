/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief TCP Flags enumeration.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <cstdint>

namespace amon::types {

/**
 * @brief TCP Flags enumeration.
 */
enum TCPFlags : uint8_t {
    FIN = 0x01, ///< Finish Flag
    SYN = 0x02, ///< Synchronize Flag
    RST = 0x04, ///< Reset Flag
    PSH = 0x08, ///< Push Flag
    ACK = 0x10, ///< Acknowledge Flag
    URG = 0x20, ///< Urgent Flag
    ECE = 0x40, ///< ECN-Echo Flag
    CWR = 0x80, ///< Congestion Window Reduced Flag
};

} // namespace amon::types
