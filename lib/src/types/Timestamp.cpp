/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief A timestamp class
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/Test.hpp>
#include <amon/types/Timestamp.hpp>

#include <limits>

namespace amon::types {

// STATIC CHECKS
static_assert(sizeof(Timestamp) == 8, "Timestamp size is not 8 bytes");

// TESTS
#ifdef AMON_ENABLE_TESTS

TEST(AmonTypesTimestamp, TimestampDefaultConstruction)
{
    const Timestamp t;
    EXPECT_TRUE(t.nanoseconds() == 0);
}

TEST(AmonTypesTimestamp, TimestampNanosecondConstruction)
{
    const Timestamp t(123456789);
    EXPECT_TRUE(t.nanoseconds() == 123456789);
}

TEST(AmonTypesTimestamp, TimestampFromMicroseconds)
{
    auto t = Timestamp::from<TimeUnit::MICROSECONDS>(1234);
    EXPECT_TRUE(t.nanoseconds() == 1'234'000);
}

TEST(AmonTypesTimestamp, TimestampFromMilliseconds)
{
    auto t = Timestamp::from<TimeUnit::MILLISECONDS>(1234);
    EXPECT_TRUE(t.nanoseconds() == 1'234'000'000);
}

TEST(AmonTypesTimestamp, TimestampFromSeconds)
{
    auto t = Timestamp::from<TimeUnit::SECONDS>(3);
    EXPECT_TRUE(t.nanoseconds() == 3'000'000'000ULL);
}

TEST(AmonTypesTimestamp, TimestampFromTimeval)
{
    timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 500;
    auto t = Timestamp::fromTimeval(tv);
    EXPECT_TRUE(t.nanoseconds() == 1'000'500'000ULL);
}

TEST(AmonTypesTimestamp, TimestampComparisonOperators)
{
    const Timestamp t1(1000);
    const Timestamp t2(2000);
    const Timestamp t3(1000);
    EXPECT_TRUE(t1 < t2);
    EXPECT_TRUE(t2 > t1);
    EXPECT_TRUE(t1 == t3);
    EXPECT_TRUE(t1 <= t3);
    EXPECT_TRUE(t1 >= t3);
    EXPECT_TRUE(t1 != t2);
}

TEST(AmonTypesTimestamp, TimestampTostringNanoseconds)
{
    const Timestamp t(123456789);
    EXPECT_TRUE(t.toString<TimeUnit::NANOSECONDS>() == "123456789");
}

TEST(AmonTypesTimestamp, TimestampTostringMicroseconds)
{
    const Timestamp t(123456789);
    EXPECT_TRUE(t.toString<TimeUnit::MICROSECONDS>() == "123456.789");
}

TEST(AmonTypesTimestamp, TimestampTostringMilliseconds)
{
    const Timestamp t(123456789);
    EXPECT_TRUE(t.toString<TimeUnit::MILLISECONDS>() == "123.456789");
}

TEST(AmonTypesTimestamp, TimestampTostringSeconds)
{
    const Timestamp t(123456789);
    EXPECT_TRUE(t.toString<TimeUnit::SECONDS>() == "0.123456789");
}

TEST(AmonTypesTimestamp, TimestampFromtimevalWithNegativeThrows)
{
    timeval tv1;
    tv1.tv_sec = -1;
    tv1.tv_usec = 500;
    EXPECT_THROW(Timestamp::fromTimeval(tv1), std::invalid_argument);

    timeval tv2;
    tv2.tv_sec = 1;
    tv2.tv_usec = -500;
    EXPECT_THROW(Timestamp::fromTimeval(tv2), std::invalid_argument);

    timeval tv3;
    tv3.tv_sec = -1;
    tv3.tv_usec = -500;
    EXPECT_THROW(Timestamp::fromTimeval(tv3), std::invalid_argument);
}

TEST(AmonTypesTimestamp, TimestampConverttoNanoseconds)
{
    const Timestamp t(123456789);
    EXPECT_TRUE(t.convertTo(TimeUnit::NANOSECONDS) == 123456789);
    EXPECT_TRUE(t.convertTo<TimeUnit::NANOSECONDS>() == 123456789);
}

TEST(AmonTypesTimestamp, TimestampConverttoMicroseconds)
{
    const Timestamp t(123456789);
    EXPECT_TRUE(t.convertTo(TimeUnit::MICROSECONDS) == 123456);
    EXPECT_TRUE(t.convertTo<TimeUnit::MICROSECONDS>() == 123456);
}

TEST(AmonTypesTimestamp, TimestampConverttoMilliseconds)
{
    const Timestamp t(123456789);
    EXPECT_TRUE(t.convertTo(TimeUnit::MILLISECONDS) == 123);
    EXPECT_TRUE(t.convertTo<TimeUnit::MILLISECONDS>() == 123);
}

TEST(AmonTypesTimestamp, TimestampConverttoSeconds)
{
    const Timestamp t(123456789);
    EXPECT_TRUE(t.convertTo(TimeUnit::SECONDS) == 0);
    EXPECT_TRUE(t.convertTo<TimeUnit::SECONDS>() == 0);

    const Timestamp t2(3'000'000'000ULL);
    EXPECT_TRUE(t2.convertTo(TimeUnit::SECONDS) == 3);
    EXPECT_TRUE(t2.convertTo<TimeUnit::SECONDS>() == 3);
}

TEST(AmonTypesTimestamp, TimestampDifference)
{
    const Timestamp t1(5'000'000'000ULL);
    const Timestamp t2(3'000'000'000ULL);
    EXPECT_TRUE(t1.difference(t2) == 2'000'000'000);
    EXPECT_TRUE(t2.difference(t1) == 2'000'000'000);
}

TEST(AmonTypesTimestamp, TimestampDifferenceEdgeCases)
{
    const Timestamp t1(std::numeric_limits<uint64_t>::max());
    const Timestamp t2(std::numeric_limits<uint64_t>::max() - 7);
    const Timestamp t3(1);
    EXPECT_TRUE(t1.difference(t2) == 7);
    EXPECT_TRUE(t2.difference(t1) == 7);
    EXPECT_TRUE(t1.difference(t3) == std::numeric_limits<uint64_t>::max() - 1);
}

#endif

} // namespace amon::types
