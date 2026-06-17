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

#ifdef AMON_ENABLE_TESTS

TEST(AmonUtilNumeric, SafeaddNormalAddition)
{
    EXPECT_TRUE(safeAdd(1, 2) == 3);
    EXPECT_TRUE(safeAdd(0, 0) == 0);
    EXPECT_TRUE(safeAdd(123456789, 987654321) == 1111111110);
}

TEST(AmonUtilNumeric, SafeaddEdgeCases)
{
    EXPECT_TRUE(
        safeAdd(0, std::numeric_limits<uint64_t>::max()) == std::numeric_limits<uint64_t>::max());
    EXPECT_TRUE(
        safeAdd(std::numeric_limits<uint64_t>::max(), 0) == std::numeric_limits<uint64_t>::max());
}

TEST(AmonUtilNumeric, SafeaddOverflow)
{
    EXPECT_THROW(safeAdd(std::numeric_limits<uint64_t>::max(), 1), std::overflow_error);
    EXPECT_THROW(safeAdd(1, std::numeric_limits<uint64_t>::max()), std::overflow_error);
}

TEST(AmonUtilNumeric, SafemulNormalMultiplication)
{
    EXPECT_TRUE(safeMul(2, 3) == 6);
    EXPECT_TRUE(safeMul(0, 123456) == 0);
    EXPECT_TRUE(safeMul(1, 987654321) == 987654321);
}

TEST(AmonUtilNumeric, SafemulEdgeCases)
{
    EXPECT_TRUE(safeMul(0, std::numeric_limits<uint64_t>::max()) == 0);
    EXPECT_TRUE(safeMul(std::numeric_limits<uint64_t>::max(), 0) == 0);
    EXPECT_TRUE(
        safeMul(std::numeric_limits<uint64_t>::max(), 1) == std::numeric_limits<uint64_t>::max());
}

TEST(AmonUtilNumeric, SafemulOverflow)
{
    EXPECT_THROW(safeMul(std::numeric_limits<uint64_t>::max(), 2), std::overflow_error);
    EXPECT_THROW(safeMul((std::numeric_limits<uint64_t>::max() / 2) + 1, 2), std::overflow_error);
    EXPECT_THROW(
        safeMul(std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max()),
        std::overflow_error);
}

TEST(AmonUtilNumeric, SafecastUnsignedToUnsignedNoOverflow)
{
    const uint8_t val = 42;
    EXPECT_TRUE(safeCast<uint16_t>(val) == 42);
}

TEST(AmonUtilNumeric, SafecastSignedToSignedNoOverflow)
{
    const int8_t val = -42;
    EXPECT_TRUE(safeCast<int16_t>(val) == -42);
}

TEST(AmonUtilNumeric, SafecastUnsignedToSignedWithinRange)
{
    const uint8_t val = 127;
    EXPECT_TRUE(safeCast<int8_t>(val) == 127);
}

TEST(AmonUtilNumeric, SafecastUnsignedToSignedOverflow)
{
    const uint16_t val = 300;
    EXPECT_THROW(safeCast<int8_t>(val), std::overflow_error);
}

TEST(AmonUtilNumeric, SafecastSignedToUnsignedNegativeValue)
{
    const int8_t val = -1;
    EXPECT_THROW(safeCast<uint8_t>(val), std::overflow_error);
}

TEST(AmonUtilNumeric, SafecastSignedToUnsignedPositiveValue)
{
    const int8_t val = 42;
    EXPECT_TRUE(safeCast<uint8_t>(val) == 42);
}

TEST(AmonUtilNumeric, SafecastValueExceedsOutputtMax)
{
    const int32_t val = std::numeric_limits<int16_t>::max() + 1;
    EXPECT_THROW(safeCast<int16_t>(val), std::overflow_error);
}

TEST(AmonUtilNumeric, SafecastValueBelowOutputtMin)
{
    const int32_t val = std::numeric_limits<int16_t>::min() - 1;
    EXPECT_THROW(safeCast<int16_t>(val), std::overflow_error);
}

#endif

} // namespace amon::util::numeric
