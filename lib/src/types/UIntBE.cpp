/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief Types representing big-endian (network order) with implicit conversions.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/Test.hpp>
#include <amon/types/UIntBE.hpp>

#include <cstring>

namespace amon::types {

static_assert(std::is_standard_layout<UInt8>(), "Unexpected data type layout");
static_assert(std::is_standard_layout<UInt16BE>(), "Unexpected data type layout");
static_assert(std::is_standard_layout<UInt32BE>(), "Unexpected data type layout");
static_assert(std::is_standard_layout<UInt64BE>(), "Unexpected data type layout");
static_assert(std::is_trivially_default_constructible<UInt8>(), "Unexpected construction");
static_assert(std::is_trivially_default_constructible<UInt16BE>(), "Unexpected construction");
static_assert(std::is_trivially_default_constructible<UInt32BE>(), "Unexpected construction");
static_assert(std::is_trivially_default_constructible<UInt64BE>(), "Unexpected construction");
static_assert(sizeof(UInt8) == 1, "UInt8 size must be 1 byte");
static_assert(sizeof(UInt16BE) == 2, "UInt16BE size must be 2 bytes");
static_assert(sizeof(UInt32BE) == 4, "UInt32BE size must be 4 bytes");
static_assert(sizeof(UInt64BE) == 8, "UInt64BE size must be 8 bytes");

TEST_CASE("UIntBE Assignment and Conversion UInt16BE")
{
    UInt16BE be;
    const uint16_t hostValue = 0x1234;
    be = hostValue;
    CHECK(static_cast<uint16_t>(be) == hostValue);
}

TEST_CASE("UIntBE Assignment and Conversion UInt32BE")
{
    UInt32BE be;
    const uint32_t hostValue = 0x12345678;
    be = hostValue;
    CHECK(static_cast<uint32_t>(be) == hostValue);
}

TEST_CASE("UIntBE Assignment and Conversion UInt64BE")
{
    UInt64BE be;
    const uint64_t hostValue = 0x123456789ABCDEF0ULL;
    be = hostValue;
    CHECK(static_cast<uint64_t>(be) == hostValue);
}

TEST_CASE("UIntBE Data is saved in big-endian")
{
    UInt64BE be;
    be = 0x123456789ABCDEF0ULL;
    const uint8_t expectedBytes[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    auto* actualBytes = reinterpret_cast<uint8_t*>(&be);
    CHECK(std::memcmp(expectedBytes, actualBytes, 8) == 0);
}

TEST_CASE("UIntBE Assignment and Conversion UInt8")
{
    const UInt8 be = 0xAB;
    CHECK(be == 0xAB);
}

} // namespace amon::types
