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

#include <cstring>
#include <stdexcept>
#include <type_traits>

#include <arpa/inet.h>
#include <endian.h>

namespace amon::types {

IPAddress::IPAddress() noexcept
{
    clear();
}

IPAddress::IPAddress(const std::string& ip)
{
    uint32_t ipv4Be;

    if (inet_pton(AF_INET, ip.c_str(), &ipv4Be) == 1) {
        u64()[0] = 0;
        u32()[2] = htobe32(0x0000FFFF);
        u32()[3] = ipv4Be;
        return;
    }

    if (inet_pton(AF_INET6, ip.c_str(), u64()) == 1) {
        return;
    }

    throw std::invalid_argument("Unable to convert '" + ip + "' to IP address");
}

IPAddress::IPAddress(const IPv4& ip) noexcept
{
    u64()[0] = 0;
    u32()[2] = htobe32(0x0000FFFF);
    u32()[3] = *reinterpret_cast<const uint32_t*>(ip.data());
}

IPAddress::IPAddress(const IPv6& ip) noexcept
{
    std::memcpy(data(), ip.data(), 16);
}

bool IPAddress::isIPv4() const noexcept
{
    return u64()[0] == 0 && u32()[2] == htobe32(0x0000FFFF);
}

bool IPAddress::isIPv6() const noexcept
{
    return !isIPv4();
}

std::string IPAddress::toString() const
{
    if (isIPv4()) {
        std::array<char, INET_ADDRSTRLEN> buffer;
        const char* result = inet_ntop(AF_INET, &u32()[3], buffer.data(), buffer.size());
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

static_assert(std::is_standard_layout<IPv4>(), "Unexpected data type layout for IPv4");
static_assert(std::is_standard_layout<IPv6>(), "Unexpected data type layout for IPv6");
static_assert(std::is_trivially_default_constructible<IPv4>(), "Unexpected construction for IPv4");
static_assert(std::is_trivially_default_constructible<IPv6>(), "Unexpected construction for IPv6");
static_assert(sizeof(IPv4) == 4, "IPv4 size must be 4 bytes");
static_assert(sizeof(IPv6) == 16, "IPv6 size must be 16 bytes");
static_assert(sizeof(IPAddress) == 16U, "IPAddr size must be 16 bytes");

TEST_CASE("IPAddress IPv4 construction and string conversion")
{
    const IPAddress ip("192.168.1.1");
    CHECK(ip.isIPv4());
    CHECK(!ip.isIPv6());
    CHECK(ip.toString() == "192.168.1.1");
}

TEST_CASE("IPAddress IPv6 construction and string conversion")
{
    const IPAddress ip("2001:db8::1");
    CHECK(ip.isIPv6());
    CHECK(!ip.isIPv4());
    CHECK(ip.toString() == "2001:db8::1");
}

TEST_CASE("IPAddress invalid string throws")
{
    CHECK_THROWS_AS(IPAddress("invalid_ip"), std::invalid_argument);
}

TEST_CASE("IPAddress IPv4 comparison")
{
    const IPAddress ip1("192.168.1.1");
    const IPAddress ip2("192.168.1.2");
    const IPAddress ip3("192.168.1.1");
    CHECK(ip1 < ip2);
    CHECK(ip2 > ip1);
    CHECK(ip1 == ip3);
    CHECK(ip1 <= ip3);
    CHECK(ip1 >= ip3);
    CHECK(ip1 != ip2);
}

TEST_CASE("IPAddress IPv6 comparison")
{
    const IPAddress ip1("2001:db8::1");
    const IPAddress ip2("2001:db8::2");
    CHECK(ip1 < ip2);
    CHECK(ip2 > ip1);
    CHECK(ip1 != ip2);
}

TEST_CASE("IPAddress IPv4 is equal to IPv4-mapped IPv6 address")
{
    const IPAddress ipv4("192.168.1.1");
    const IPAddress ipv6("::ffff:192.168.1.1");
    CHECK(ipv4 == ipv6);
}

TEST_CASE("IPAddress construct from IPv4")
{
    IPv4 ipv4;
    // 192.168.1.1 in network byte order
    uint8_t bytes[4] = {192, 168, 1, 1};
    std::memcpy(ipv4.data(), bytes, 4);

    const IPAddress ip(ipv4);
    CHECK(ip.isIPv4());
    CHECK(!ip.isIPv6());
    CHECK(ip.toString() == "192.168.1.1");
}

TEST_CASE("IPAddress construct from IPv6")
{
    IPv6 ipv6;
    // 2001:db8::1 in network byte order
    uint8_t bytes[16] = {0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
    std::memcpy(ipv6.data(), bytes, 16);

    const IPAddress ip(ipv6);
    CHECK(ip.isIPv6());
    CHECK(!ip.isIPv4());
    CHECK(ip.toString() == "2001:db8::1");
}

} // namespace amon::types
