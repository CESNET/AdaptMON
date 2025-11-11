/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Utils for numeric operations.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/Test.hpp>
#include <amon/util/numeric.hpp>

#include <limits>
#include <stdexcept>

namespace amon::util::numeric {

TEST_CASE("safeAdd normal addition")
{
    CHECK(safeAdd(1, 2) == 3);
    CHECK(safeAdd(0, 0) == 0);
    CHECK(safeAdd(123456789, 987654321) == 1111111110);
}

TEST_CASE("safeAdd edge cases")
{
    CHECK(safeAdd(0, std::numeric_limits<uint64_t>::max()) == std::numeric_limits<uint64_t>::max());
    CHECK(safeAdd(std::numeric_limits<uint64_t>::max(), 0) == std::numeric_limits<uint64_t>::max());
}

TEST_CASE("safeAdd overflow")
{
    CHECK_THROWS_AS(safeAdd(std::numeric_limits<uint64_t>::max(), 1), std::overflow_error);
    CHECK_THROWS_AS(safeAdd(1, std::numeric_limits<uint64_t>::max()), std::overflow_error);
}

TEST_CASE("safeMul normal multiplication")
{
    CHECK(safeMul(2, 3) == 6);
    CHECK(safeMul(0, 123456) == 0);
    CHECK(safeMul(1, 987654321) == 987654321);
}

TEST_CASE("safeMul edge cases")
{
    CHECK(safeMul(0, std::numeric_limits<uint64_t>::max()) == 0);
    CHECK(safeMul(std::numeric_limits<uint64_t>::max(), 0) == 0);
    CHECK(safeMul(std::numeric_limits<uint64_t>::max(), 1) == std::numeric_limits<uint64_t>::max());
}

TEST_CASE("safeMul overflow")
{
    CHECK_THROWS_AS(safeMul(std::numeric_limits<uint64_t>::max(), 2), std::overflow_error);
    CHECK_THROWS_AS(
        safeMul((std::numeric_limits<uint64_t>::max() / 2) + 1, 2),
        std::overflow_error);
    CHECK_THROWS_AS(
        safeMul(std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max()),
        std::overflow_error);
}

TEST_CASE("safeCast unsigned to unsigned, no overflow")
{
    const uint8_t val = 42;
    CHECK(safeCast<uint16_t>(val) == 42);
}

TEST_CASE("safeCast signed to signed, no overflow")
{
    const int8_t val = -42;
    CHECK(safeCast<int16_t>(val) == -42);
}

TEST_CASE("safeCast unsigned to signed, within range")
{
    const uint8_t val = 127;
    CHECK(safeCast<int8_t>(val) == 127);
}

TEST_CASE("safeCast unsigned to signed, overflow")
{
    const uint16_t val = 300;
    CHECK_THROWS_AS(safeCast<int8_t>(val), std::overflow_error);
}

TEST_CASE("safeCast signed to unsigned, negative value")
{
    const int8_t val = -1;
    CHECK_THROWS_AS(safeCast<uint8_t>(val), std::overflow_error);
}

TEST_CASE("safeCast signed to unsigned, positive value")
{
    const int8_t val = 42;
    CHECK(safeCast<uint8_t>(val) == 42);
}

TEST_CASE("safeCast value exceeds OutputT max")
{
    const int32_t val = std::numeric_limits<int16_t>::max() + 1;
    CHECK_THROWS_AS(safeCast<int16_t>(val), std::overflow_error);
}

TEST_CASE("safeCast value below OutputT min")
{
    const int32_t val = std::numeric_limits<int16_t>::min() - 1;
    CHECK_THROWS_AS(safeCast<int16_t>(val), std::overflow_error);
}

} // namespace amon::util::numeric
