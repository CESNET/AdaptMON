/**
 * @file
 * @author Lukáš Huták <hutak@cesnet.cz>
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief IP address and network representation and manipulation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/Test.hpp>
#include <amon/types/IPAddress.hpp>

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <type_traits>

#include <arpa/inet.h>
#include <endian.h>

namespace amon::types {

static constexpr std::array<std::byte, 12> ipv4MappedPrefix {
    std::byte {0x00},
    std::byte {0x00},
    std::byte {0x00},
    std::byte {0x00},
    std::byte {0x00},
    std::byte {0x00},
    std::byte {0x00},
    std::byte {0x00},
    std::byte {0x00},
    std::byte {0x00},
    std::byte {0xFF},
    std::byte {0xFF},
};

IPAddress::IPAddress() noexcept
{
    clear();
}

IPAddress::IPAddress(const std::string& ip)
{
    if (inet_pton(AF_INET, ip.c_str(), data() + ipv4MappedPrefix.size()) == 1) {
        std::memcpy(data(), ipv4MappedPrefix.data(), ipv4MappedPrefix.size());
        return;
    }

    if (inet_pton(AF_INET6, ip.c_str(), data()) == 1) {
        return;
    }

    throw std::invalid_argument("Unable to convert '" + ip + "' to IP address");
}

IPAddress::IPAddress(const IPv4& ip) noexcept
{
    std::memcpy(data(), ipv4MappedPrefix.data(), ipv4MappedPrefix.size());
    std::memcpy(data() + ipv4MappedPrefix.size(), ip.data(), ip.size());
}

IPAddress::IPAddress(const IPv6& ip) noexcept
{
    std::memcpy(data(), ip.data(), 16);
}

bool IPAddress::isIPv4() const noexcept
{
    return std::memcmp(data(), ipv4MappedPrefix.data(), ipv4MappedPrefix.size()) == 0;
}

bool IPAddress::isIPv6() const noexcept
{
    return !isIPv4();
}

std::string IPAddress::toString() const
{
    if (isIPv4()) {
        std::array<char, INET_ADDRSTRLEN> buffer;
        const char* result
            = inet_ntop(AF_INET, data() + ipv4MappedPrefix.size(), buffer.data(), buffer.size());
        if (result == nullptr) {
            throw std::runtime_error("Failed to convert IPv4 to string");
        }

        return {buffer.data()};
    }

    // IPv6
    std::array<char, INET6_ADDRSTRLEN> buffer;
    const char* result = inet_ntop(AF_INET6, data(), buffer.data(), buffer.size());
    if (result == nullptr) {
        throw std::runtime_error("Failed to convert IPv6 to string");
    }

    return {buffer.data()};
}

std::optional<IPv4> types::IPAddress::toIPv4() const noexcept
{
    if (!isIPv4()) {
        return std::nullopt;
    }
    IPv4 ip;
    std::memcpy(ip.data(), data() + ipv4MappedPrefix.size(), 4);
    return ip;
}

IPv6 types::IPAddress::toIPv6() const noexcept
{
    IPv6 ip;
    std::memcpy(ip.data(), data(), 16);
    return ip;
}

static_assert(std::is_standard_layout<IPv4>(), "Unexpected data type layout for IPv4");
static_assert(std::is_standard_layout<IPv6>(), "Unexpected data type layout for IPv6");
static_assert(std::is_trivially_default_constructible<IPv4>(), "Unexpected construction for IPv4");
static_assert(std::is_trivially_default_constructible<IPv6>(), "Unexpected construction for IPv6");
static_assert(sizeof(IPv4) == 4, "IPv4 size must be 4 bytes");
static_assert(sizeof(IPv6) == 16, "IPv6 size must be 16 bytes");
static_assert(sizeof(IPAddress) == 16U, "IPAddr size must be 16 bytes");

#ifdef AMON_ENABLE_TESTS

TEST(AmonTypesIPAddress, IpaddressIPv4ConstructionAndStringConversion)
{
    const IPAddress ip("192.168.1.1");
    EXPECT_TRUE(ip.isIPv4());
    EXPECT_TRUE(!ip.isIPv6());
    EXPECT_TRUE(ip.toString() == "192.168.1.1");
}

TEST(AmonTypesIPAddress, IpaddressIPv6ConstructionAndStringConversion)
{
    const IPAddress ip("2001:db8::1");
    EXPECT_TRUE(ip.isIPv6());
    EXPECT_TRUE(!ip.isIPv4());
    EXPECT_TRUE(ip.toString() == "2001:db8::1");
}

TEST(AmonTypesIPAddress, IpaddressInvalidStringThrows)
{
    EXPECT_THROW(IPAddress("invalid_ip"), std::invalid_argument);
}

TEST(AmonTypesIPAddress, IpaddressIPv4Comparison)
{
    const IPAddress ip1("192.168.1.1");
    const IPAddress ip2("192.168.1.2");
    const IPAddress ip3("192.168.1.1");
    EXPECT_TRUE(ip1 < ip2);
    EXPECT_TRUE(ip2 > ip1);
    EXPECT_TRUE(ip1 == ip3);
    EXPECT_TRUE(ip1 <= ip3);
    EXPECT_TRUE(ip1 >= ip3);
    EXPECT_TRUE(ip1 != ip2);
}

TEST(AmonTypesIPAddress, IpaddressIPv6Comparison)
{
    const IPAddress ip1("2001:db8::1");
    const IPAddress ip2("2001:db8::2");
    EXPECT_TRUE(ip1 < ip2);
    EXPECT_TRUE(ip2 > ip1);
    EXPECT_TRUE(ip1 != ip2);
}

TEST(AmonTypesIPAddress, IpaddressIPv4IsEqualToIPv4MappedIPv6Address)
{
    const IPAddress ipv4("192.168.1.1");
    const IPAddress ipv6("::ffff:192.168.1.1");
    EXPECT_TRUE(ipv4 == ipv6);
}

TEST(AmonTypesIPAddress, IpaddressConstructFromIPv4)
{
    IPv4 ipv4;
    // 192.168.1.1 in network byte order
    uint8_t bytes[4] = {192, 168, 1, 1};
    std::memcpy(ipv4.data(), bytes, 4);

    const IPAddress ip(ipv4);
    EXPECT_TRUE(ip.isIPv4());
    EXPECT_TRUE(!ip.isIPv6());
    EXPECT_TRUE(ip.toString() == "192.168.1.1");
}

TEST(AmonTypesIPAddress, IpaddressConstructFromIPv6)
{
    IPv6 ipv6;
    // 2001:db8::1 in network byte order
    uint8_t bytes[16] = {0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
    std::memcpy(ipv6.data(), bytes, 16);

    const IPAddress ip(ipv6);
    EXPECT_TRUE(ip.isIPv6());
    EXPECT_TRUE(!ip.isIPv4());
    EXPECT_TRUE(ip.toString() == "2001:db8::1");
}

TEST(AmonTypesIPAddress, IpaddressToipv4ReturnsIPv4ForIPv4Address)
{
    const IPAddress ip("192.168.1.1");
    auto opt = ip.toIPv4();
    ASSERT_TRUE(opt.has_value());
    if (opt.has_value()) { // make clang-tidy happy
        const IPv4 v = *opt;
        const uint8_t expect[4] = {192, 168, 1, 1};
        EXPECT_TRUE(std::memcmp(v.data(), expect, 4) == 0);
    }
}

TEST(AmonTypesIPAddress, IpaddressToipv4ReturnsStdNulloptForNativeIPv6)
{
    const IPAddress ip("2001:db8::1");
    EXPECT_TRUE(!ip.toIPv4().has_value());
}

TEST(AmonTypesIPAddress, IpaddressToipv4AcceptsIPv4MappedIPv6)
{
    const IPAddress ip("::ffff:192.168.1.1");
    auto opt = ip.toIPv4();
    ASSERT_TRUE(opt.has_value());
    if (opt.has_value()) { // make clang-tidy happy
        const IPv4 v = *opt;
        const uint8_t expect[4] = {192, 168, 1, 1};
        EXPECT_TRUE(std::memcmp(v.data(), expect, 4) == 0);
    }
}

TEST(AmonTypesIPAddress, IpaddressToipv6ReturnsFullIPv6ForBothIPv6AndIPv4MappedAddresses)
{
    // IPv6 input -> identical 16 bytes
    const IPAddress ip6("2001:db8::1");
    IPv6 out6 = ip6.toIPv6();
    const uint8_t expect6[16] = {0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
    EXPECT_TRUE(std::memcmp(out6.data(), expect6, 16) == 0);

    // IPv4 input -> IPv4-mapped IPv6
    const IPAddress ip4("192.168.1.1");
    IPv6 out6m = ip4.toIPv6();
    const uint8_t expect4[4] = {192, 168, 1, 1};
    EXPECT_TRUE(std::memcmp(out6m.data() + 12, expect4, 4) == 0);
    EXPECT_TRUE(out6m.data()[10] == std::byte {0xff});
    EXPECT_TRUE(out6m.data()[11] == std::byte {0xff});
}

#endif

} // namespace amon::types
