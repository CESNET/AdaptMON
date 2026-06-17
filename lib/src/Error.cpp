/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Error handling classes and utilities for the amon library.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/Test.hpp>
#include <amon/Error.hpp>
#include <amon/Strerror.hpp>

#include <array>
#include <string>

namespace amon {

std::string errnoToString(int errNum)
{
    std::array<char, 256> buf;
    const int ret = amon::strerror(errNum, buf.data(), buf.size());
    if (ret != 0) {
        return "<unknown>";
    }
    return std::string {buf.data()};
}

TEST_CASE("ErrnoError")
{
    CHECK_THROWS_WITH(
        [] { throw ErrnoError(int(0), "fopen", "thing.xml"); }(),
        "fopen(\"thing.xml\") has failed: Success (0)");
}

} // namespace amon
