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

namespace amon::types {

// STATIC CHECKS
static_assert(sizeof(Timestamp) == 8, "Timestamp size is not 8 bytes");

// TESTS
TEST_CASE("Timestamp default construction")
{
    const Timestamp t;
    CHECK(t.nanoseconds() == 0);
}

TEST_CASE("Timestamp nanosecond construction")
{
    const Timestamp t(123456789);
    CHECK(t.nanoseconds() == 123456789);
}

TEST_CASE("Timestamp from microseconds")
{
    auto t = Timestamp::from<TimeUnit::MICROSECONDS>(1234);
    CHECK(t.nanoseconds() == 1'234'000);
}

TEST_CASE("Timestamp from milliseconds")
{
    auto t = Timestamp::from<TimeUnit::MILLISECONDS>(1234);
    CHECK(t.nanoseconds() == 1'234'000'000);
}

TEST_CASE("Timestamp from seconds")
{
    auto t = Timestamp::from<TimeUnit::SECONDS>(3);
    CHECK(t.nanoseconds() == 3'000'000'000ULL);
}

TEST_CASE("Timestamp from timeval")
{
    timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 500;
    auto t = Timestamp::fromTimeval(tv);
    CHECK(t.nanoseconds() == 1'000'500'000ULL);
}

TEST_CASE("Timestamp comparison operators")
{
    const Timestamp t1(1000);
    const Timestamp t2(2000);
    const Timestamp t3(1000);
    CHECK(t1 < t2);
    CHECK(t2 > t1);
    CHECK(t1 == t3);
    CHECK(t1 <= t3);
    CHECK(t1 >= t3);
    CHECK(t1 != t2);
}

TEST_CASE("Timestamp toString nanoseconds")
{
    const Timestamp t(123456789);
    CHECK(t.toString<TimeUnit::NANOSECONDS>() == "123456789");
}

TEST_CASE("Timestamp toString microseconds")
{
    const Timestamp t(123456789);
    CHECK(t.toString<TimeUnit::MICROSECONDS>() == "123456.789");
}

TEST_CASE("Timestamp toString milliseconds")
{
    const Timestamp t(123456789);
    CHECK(t.toString<TimeUnit::MILLISECONDS>() == "123.456789");
}

TEST_CASE("Timestamp toString seconds")
{
    const Timestamp t(123456789);
    CHECK(t.toString<TimeUnit::SECONDS>() == "0.123456789");
}

TEST_CASE("Timestamp fromTimeval with negative throws")
{
    timeval tv1;
    tv1.tv_sec = -1;
    tv1.tv_usec = 500;
    CHECK_THROWS_AS(Timestamp::fromTimeval(tv1), std::invalid_argument);

    timeval tv2;
    tv2.tv_sec = 1;
    tv2.tv_usec = -500;
    CHECK_THROWS_AS(Timestamp::fromTimeval(tv2), std::invalid_argument);

    timeval tv3;
    tv3.tv_sec = -1;
    tv3.tv_usec = -500;
    CHECK_THROWS_AS(Timestamp::fromTimeval(tv3), std::invalid_argument);
}

} // namespace amon::types
