/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief Representation of MAC (Media Access Control) address.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/Test.hpp>
#include <amon/types/MACAddress.hpp>

namespace amon::types {

// STATIC CHECKS
static_assert(std::is_standard_layout<MACAddress>(), "Unexpected data type layout for MACAddress");
static_assert(
    std::is_trivially_default_constructible<MACAddress>(),
    "Unexpected construction for MACAddress");
static_assert(sizeof(MACAddress) == 6, "MACAddress size must be 6 bytes");
static_assert(
    sizeof(std::byte) == sizeof(unsigned char),
    "Type unsigned char must be equal to std::byte for conversion in MACAddress constructor.");

// TESTS
#ifdef AMON_ENABLE_TESTS

TEST(AmonTypesMACAddress, MacaddressCopyConstruction)
{
    const std::array<uint8_t, 6> mac = {1, 2, 3, 4, 5, 6};
    const auto addr1 = MACAddress(std::span(mac));
    const MACAddress addr2(addr1);
    EXPECT_TRUE(addr1 == addr2);
}

TEST(AmonTypesMACAddress, MacaddressConstructFromSpan)
{
    const std::array<uint8_t, 6> mac = {10, 20, 30, 40, 50, 60};
    const MACAddress addr = std::span<const uint8_t>(mac);
    for (size_t i = 0; i < 6; ++i) {
        EXPECT_TRUE(addr[i] == static_cast<std::byte>(mac[i]));
    }
}

TEST(AmonTypesMACAddress, MacaddressConstructionFromSpanWithWrongSizeThrows)
{
    const std::array<uint8_t, 5> mac = {1, 2, 3, 4, 5};
    EXPECT_THROW(MACAddress(std::span(mac)), std::logic_error);
}

TEST(AmonTypesMACAddress, MacaddressStringConstructorValid)
{
    const MACAddress addr("01:23:45:67:89:ab");
    EXPECT_TRUE(addr[0] == std::byte {0x01});
    EXPECT_TRUE(addr[1] == std::byte {0x23});
    EXPECT_TRUE(addr[2] == std::byte {0x45});
    EXPECT_TRUE(addr[3] == std::byte {0x67});
    EXPECT_TRUE(addr[4] == std::byte {0x89});
    EXPECT_TRUE(addr[5] == std::byte {0xab});
}

TEST(AmonTypesMACAddress, MacaddressStringConstructorInvalidLengthThrows)
{
    EXPECT_THROW(MACAddress("01:23:45:67:89"), std::invalid_argument);
}

TEST(AmonTypesMACAddress, MacaddressStringConstructorInvalidFormatThrows)
{
    EXPECT_THROW(MACAddress("01:23:45:67:89:ZZ"), std::invalid_argument);
    EXPECT_THROW(MACAddress("notamacaddress"), std::invalid_argument);
}

TEST(AmonTypesMACAddress, MacaddressComparisonOperators)
{
    const MACAddress addr1("01:23:45:67:89:ab");
    const MACAddress addr2("01:23:45:67:89:ac");
    const MACAddress addr3("01:23:45:67:89:ab");
    EXPECT_TRUE(addr1 == addr3);
    EXPECT_TRUE(addr1 != addr2);
    EXPECT_TRUE(addr1 < addr2);
    EXPECT_TRUE(addr2 > addr1);
}

TEST(AmonTypesMACAddress, MacaddressIndexingOutOfBoundsThrows)
{
    const MACAddress addr("01:23:45:67:89:ab");
    EXPECT_THROW(addr.at(6), std::out_of_range);
    EXPECT_THROW(addr.at(100), std::out_of_range);
}

TEST(AmonTypesMACAddress, MacaddressMutableIndexing)
{
    MACAddress addr("01:23:45:67:89:ab");
    addr[0] = static_cast<std::byte>(0xff);
    EXPECT_TRUE(addr[0] == static_cast<std::byte>(0xff));
}

TEST(AmonTypesMACAddress, MacaddressConstructWithReinterpretCast)
{
    std::array<uint8_t, 6> mac = {0xde, 0xad, 0xbe, 0xef, 0x00, 0x01};
    const auto* addr = reinterpret_cast<const MACAddress*>(mac.data());
    for (size_t i = 0; i < 6; ++i) {
        EXPECT_TRUE((*addr)[i] == static_cast<std::byte>(mac[i]));
    }
}

#endif

} // namespace amon::types
