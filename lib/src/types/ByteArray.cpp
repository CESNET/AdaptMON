/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief Fixed-size array of bytes with utility methods.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/Test.hpp>
#include <amon/types/ByteArray.hpp>

namespace amon::types {

TEST_CASE("ByteArray element access and assignment")
{
    ByteArray<4> arr;
    arr[0] = std::byte {0x11};
    arr[1] = std::byte {0x22};
    arr[2] = std::byte {0x33};
    arr[3] = std::byte {0x44};
    CHECK(arr[0] == std::byte {0x11});
    CHECK(arr[1] == std::byte {0x22});
    CHECK(arr[2] == std::byte {0x33});
    CHECK(arr[3] == std::byte {0x44});
}

TEST_CASE("ByteArray at() throws on out of range")
{
    ByteArray<2> arr;
    arr[0] = std::byte {0xAA};
    arr[1] = std::byte {0xBB};
    CHECK_NOTHROW(arr.at(0));
    CHECK_NOTHROW(arr.at(1));
    CHECK_THROWS_AS(arr.at(2), std::out_of_range);
}

TEST_CASE("ByteArray front and back")
{
    ByteArray<3> arr;
    arr[0] = std::byte {0x01};
    arr[1] = std::byte {0x02};
    arr[2] = std::byte {0x03};
    CHECK(arr.front() == std::byte {0x01});
    CHECK(arr.back() == std::byte {0x03});
}

TEST_CASE("ByteArray fill and clear")
{
    ByteArray<5> arr;
    arr.fill(std::byte {0xAB});
    for (auto i : arr) {
        CHECK(i == std::byte {0xAB});
    }
    arr.clear();
    for (auto i : arr) {
        CHECK(i == std::byte {0x00});
    }
}

TEST_CASE("ByteArray comparison operators")
{
    ByteArray<2> a;
    ByteArray<2> b;
    ByteArray<2> c;
    a[0] = b[0] = std::byte {0x01};
    a[1] = b[1] = std::byte {0x02};
    c[0] = std::byte {0x01};
    c[1] = std::byte {0x03};
    CHECK(a == b);
    CHECK(a != c);
    CHECK((a < c));
    CHECK((c > a));
}

} // namespace amon::types
