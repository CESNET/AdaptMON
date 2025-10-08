/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
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
    DOCTEST_CHECK(safeAdd(1, 2) == 3);
    DOCTEST_CHECK(safeAdd(0, 0) == 0);
    DOCTEST_CHECK(safeAdd(123456789, 987654321) == 1111111110);
}

TEST_CASE("safeAdd edge cases")
{
    DOCTEST_CHECK(
        safeAdd(0, std::numeric_limits<uint64_t>::max()) == std::numeric_limits<uint64_t>::max());
    DOCTEST_CHECK(
        safeAdd(std::numeric_limits<uint64_t>::max(), 0) == std::numeric_limits<uint64_t>::max());
}

TEST_CASE("safeAdd overflow")
{
    DOCTEST_CHECK_THROWS_AS(safeAdd(std::numeric_limits<uint64_t>::max(), 1), std::overflow_error);
    DOCTEST_CHECK_THROWS_AS(safeAdd(1, std::numeric_limits<uint64_t>::max()), std::overflow_error);
}

TEST_CASE("safeMul normal multiplication")
{
    DOCTEST_CHECK(safeMul(2, 3) == 6);
    DOCTEST_CHECK(safeMul(0, 123456) == 0);
    DOCTEST_CHECK(safeMul(1, 987654321) == 987654321);
}

TEST_CASE("safeMul edge cases")
{
    DOCTEST_CHECK(safeMul(0, std::numeric_limits<uint64_t>::max()) == 0);
    DOCTEST_CHECK(safeMul(std::numeric_limits<uint64_t>::max(), 0) == 0);
    DOCTEST_CHECK(
        safeMul(std::numeric_limits<uint64_t>::max(), 1) == std::numeric_limits<uint64_t>::max());
}

TEST_CASE("safeMul overflow")
{
    DOCTEST_CHECK_THROWS_AS(safeMul(std::numeric_limits<uint64_t>::max(), 2), std::overflow_error);
    DOCTEST_CHECK_THROWS_AS(
        safeMul((std::numeric_limits<uint64_t>::max() / 2) + 1, 2),
        std::overflow_error);
    DOCTEST_CHECK_THROWS_AS(
        safeMul(std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max()),
        std::overflow_error);
}

} // namespace amon::util::numeric
