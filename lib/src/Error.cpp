/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Error handling classes and utilities for the amon library.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Make sure that XSI-version is used
#undef _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L // NOLINT

#include <amon/Error.hpp>

#include <array>
#include <cstring>
#include <string>

namespace amon {

std::string errnoToString(int errNum)
{
    std::array<char, 256> buf;
    const int ret = strerror_r(errNum, buf.data(), buf.size());
    if (ret != 0) {
        return "<unknown>";
    }
    return std::string {buf.data()};
}

} // namespace amon
