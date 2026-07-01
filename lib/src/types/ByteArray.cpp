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

#ifdef AMON_ENABLE_TESTS

TEST(AmonTypesByteArray, BytearrayElementAccessAndAssignment)
{
    ByteArray<4> arr;
    arr[0] = std::byte {0x11};
    arr[1] = std::byte {0x22};
    arr[2] = std::byte {0x33};
    arr[3] = std::byte {0x44};
    EXPECT_TRUE(arr[0] == std::byte {0x11});
    EXPECT_TRUE(arr[1] == std::byte {0x22});
    EXPECT_TRUE(arr[2] == std::byte {0x33});
    EXPECT_TRUE(arr[3] == std::byte {0x44});
}

TEST(AmonTypesByteArray, BytearrayAtThrowsOnOutOfRange)
{
    ByteArray<2> arr;
    arr[0] = std::byte {0xAA};
    arr[1] = std::byte {0xBB};
    EXPECT_NO_THROW(arr.at(0));
    EXPECT_NO_THROW(arr.at(1));
    EXPECT_THROW(arr.at(2), std::out_of_range);
}

TEST(AmonTypesByteArray, BytearrayFrontAndBack)
{
    ByteArray<3> arr;
    arr[0] = std::byte {0x01};
    arr[1] = std::byte {0x02};
    arr[2] = std::byte {0x03};
    EXPECT_TRUE(arr.front() == std::byte {0x01});
    EXPECT_TRUE(arr.back() == std::byte {0x03});
}

TEST(AmonTypesByteArray, BytearrayFillAndClear)
{
    ByteArray<5> arr;
    arr.fill(std::byte {0xAB});
    for (auto i : arr) {
        EXPECT_TRUE(i == std::byte {0xAB});
    }
    arr.clear();
    for (auto i : arr) {
        EXPECT_TRUE(i == std::byte {0x00});
    }
}

TEST(AmonTypesByteArray, BytearrayComparisonOperators)
{
    ByteArray<2> a;
    ByteArray<2> b;
    ByteArray<2> c;
    a[0] = b[0] = std::byte {0x01};
    a[1] = b[1] = std::byte {0x02};
    c[0] = std::byte {0x01};
    c[1] = std::byte {0x03};
    EXPECT_TRUE(a == b);
    EXPECT_TRUE(a != c);
    EXPECT_TRUE((a < c));
    EXPECT_TRUE((c > a));
}

TEST(AmonTypesByteArray, BytearrayCopyConstructor)
{
    ByteArray<4> orig;
    orig[0] = std::byte {0xAA};
    orig[1] = std::byte {0xBB};
    orig[2] = std::byte {0xCC};
    orig[3] = std::byte {0xDD};

    ByteArray<4> copy = orig;
    EXPECT_TRUE(orig == copy);

    // Ensure copy is independent
    orig[0] = std::byte {0x00};
    EXPECT_FALSE(orig == copy);
}

TEST(AmonTypesByteArray, BytearrayCopyAssignment)
{
    ByteArray<4> orig;
    orig[0] = std::byte {0x11};
    orig[1] = std::byte {0x22};
    orig[2] = std::byte {0x33};
    orig[3] = std::byte {0x44};

    ByteArray<4> assigned;
    assigned = orig;
    EXPECT_TRUE(orig == assigned);

    // Independence after assignment
    orig[2] = std::byte {0xFF};
    EXPECT_FALSE(orig == assigned);

    // Self-assignment should be harmless and preserve value
    ByteArray<4> expected = assigned;
    assigned = assigned;
    EXPECT_TRUE(assigned == expected);
}

#endif

} // namespace amon::types
