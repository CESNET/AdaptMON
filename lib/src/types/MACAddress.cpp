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
TEST_CASE("MACAddress copy construction")
{
    const std::array<uint8_t, 6> mac = {1, 2, 3, 4, 5, 6};
    const auto addr1 = MACAddress(std::span(mac));
    const MACAddress addr2(addr1);
    CHECK(addr1 == addr2);
}

TEST_CASE("MACAddress construct from span")
{
    const std::array<uint8_t, 6> mac = {10, 20, 30, 40, 50, 60};
    const MACAddress addr = std::span<const uint8_t>(mac);
    for (size_t i = 0; i < 6; ++i) {
        CHECK(addr[i] == static_cast<std::byte>(mac[i]));
    }
}

TEST_CASE("MACAddress construction from span with wrong size throws")
{
    const std::array<uint8_t, 5> mac = {1, 2, 3, 4, 5};
    CHECK_THROWS_AS(MACAddress(std::span(mac)), std::logic_error);
}

TEST_CASE("MACAddress string constructor valid")
{
    const MACAddress addr("01:23:45:67:89:ab");
    CHECK(addr[0] == std::byte {0x01});
    CHECK(addr[1] == std::byte {0x23});
    CHECK(addr[2] == std::byte {0x45});
    CHECK(addr[3] == std::byte {0x67});
    CHECK(addr[4] == std::byte {0x89});
    CHECK(addr[5] == std::byte {0xab});
}

TEST_CASE("MACAddress string constructor invalid length throws")
{
    CHECK_THROWS_AS(MACAddress("01:23:45:67:89"), std::invalid_argument);
}

TEST_CASE("MACAddress string constructor invalid format throws")
{
    CHECK_THROWS_AS(MACAddress("01:23:45:67:89:ZZ"), std::invalid_argument);
    CHECK_THROWS_AS(MACAddress("notamacaddress"), std::invalid_argument);
}

TEST_CASE("MACAddress comparison operators")
{
    const MACAddress addr1("01:23:45:67:89:ab");
    const MACAddress addr2("01:23:45:67:89:ac");
    const MACAddress addr3("01:23:45:67:89:ab");
    CHECK(addr1 == addr3);
    CHECK(addr1 != addr2);
    CHECK(addr1 < addr2);
    CHECK(addr2 > addr1);
}

TEST_CASE("MACAddress indexing out of bounds throws")
{
    const MACAddress addr("01:23:45:67:89:ab");
    CHECK_THROWS_AS(addr.at(6), std::out_of_range);
    CHECK_THROWS_AS(addr.at(100), std::out_of_range);
}

TEST_CASE("MACAddress mutable indexing")
{
    MACAddress addr("01:23:45:67:89:ab");
    addr[0] = static_cast<std::byte>(0xff);
    CHECK(addr[0] == static_cast<std::byte>(0xff));
}

TEST_CASE("MACAddress construct with reinterpret_cast")
{
    std::array<uint8_t, 6> mac = {0xde, 0xad, 0xbe, 0xef, 0x00, 0x01};
    const auto* addr = reinterpret_cast<const MACAddress*>(mac.data());
    for (size_t i = 0; i < 6; ++i) {
        CHECK((*addr)[i] == static_cast<std::byte>(mac[i]));
    }
}

} // namespace amon::types
