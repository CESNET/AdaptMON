/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief IPv6 layer definitions.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/Test.hpp>
#include <amon/layers/IPv6.hpp>

#include <vector>

namespace amon::layers {

std::expected<IPv6View, LayerError> IPv6View::create(std::span<const std::byte> data)
{
    if (data.size() < sizeof(IPv6Header)) {
        return std::unexpected(LayerError::HEADER_SHORT);
    }
    auto view = IPv6View(data);

    if ((view.header().versionTrafficClassFlowLabel >> 28) != 6) {
        return std::unexpected(LayerError::BAD_MAGIC);
    }
    // Check disabled due to use-cases with trimmed packets.
    // if (data.size() < (view.header().payloadLength + sizeof(IPv6Header))) {
    //     return std::unexpected(LayerError::BAD_PAYLOAD_LEN);
    // }
    return view;
}

std::expected<IPv6ExtensionView, LayerError>
IPv6ExtensionView::create(std::span<const std::byte> data) noexcept
{
    if (data.size() < sizeof(IPv6ExtensionHeader)) {
        return std::unexpected(LayerError::HEADER_SHORT);
    }
    auto view = IPv6ExtensionView(data);
    if (data.size() < view.headerLength()) {
        return std::unexpected(LayerError::BAD_LEN_FIELD);
    }
    return view;
}

std::expected<IPv6FragmentView, LayerError>
IPv6FragmentView::create(std::span<const std::byte> data) noexcept
{
    if (data.size() < sizeof(IPv6FragmentHeader)) {
        return std::unexpected(LayerError::HEADER_SHORT);
    }
    return IPv6FragmentView(data);
}

static_assert(std::is_standard_layout<IPv6Header>(), "Unexpected data type layout for IPv6Header");
static_assert(
    std::is_standard_layout<IPv6FragmentHeader>(),
    "Unexpected data type layout for IPv6FragmentHeader");
static_assert(
    std::is_standard_layout<IPv6ExtensionHeader>(),
    "Unexpected data type layout for IPv6ExtensionHeader");
static_assert(
    std::is_trivially_default_constructible<IPv6Header>(),
    "Unexpected construction for IPv6Header");
static_assert(
    std::is_trivially_default_constructible<IPv6FragmentHeader>(),
    "Unexpected construction for IPv6FragmentHeader");
static_assert(
    std::is_trivially_default_constructible<IPv6ExtensionHeader>(),
    "Unexpected construction for IPv6ExtensionHeader");
static_assert(sizeof(IPv6Header) == 40, "IPv6Header size must be 40 bytes");
static_assert(sizeof(IPv6FragmentHeader) == 8, "IPv6FragmentHeader size must be 8 bytes");
static_assert(sizeof(IPv6ExtensionHeader) == 2, "IPv6ExtensionHeader size must be 2 bytes");

TEST_CASE("IPv6View create valid IPv6 header")
{
    /* clang-format off */
    const unsigned char packet[] = {
        0x60, 0x00, 0x00, 0x00,       // Version=6, Traffic Class=0, Flow Label=0
        0x00, 0x14,                   // Payload Length = 20 bytes
        0x3A,                         // Next Header (ICMPv6)
        0x40,                         // Hop Limit
        // Source: 2001:db8::1
        0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
        // Destination: 2001:db8::2
        0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02
    };
    /* clang-format on */

    const std::span frame {reinterpret_cast<const std::byte*>(packet), sizeof(packet)};

    auto result = IPv6View::create(frame);
    REQUIRE(result.has_value());
    auto view = result.value();

    CHECK(view.headerLength() == 40);
    CHECK(view.nextHeader() == 0x3A);
    CHECK(view.hopLimit() == 0x40);
    CHECK(view.payload().size() == 0); // Payload is empty in this minimal example
    CHECK(view.payloadLength() == 20);
    // Check source and destination addresses are accessible
    // FIXME: check IPs when better IP address type API will be implemented
    (void) view.srcIP();
    (void) view.dstIP();
}

TEST_CASE("IPv6View header too short")
{
    std::vector<std::byte> bytes(10, std::byte {0});
    auto result = IPv6View::create(std::span<const std::byte>(bytes));
    CHECK(result.error() == LayerError::HEADER_SHORT);
}

TEST_CASE("IPv6View bad version")
{
    std::vector<std::byte> bytes(40, std::byte {0});
    bytes[0] = std::byte {0x40}; // Version=4, should be 6
    auto result = IPv6View::create(std::span<const std::byte>(bytes));
    CHECK(result.error() == LayerError::BAD_MAGIC);
}

TEST_CASE("IPv6ExtensionView header too short")
{
    std::vector<std::byte> bytes(1, std::byte {0});
    auto result = IPv6ExtensionView::create(std::span<const std::byte>(bytes));
    CHECK(result.error() == LayerError::HEADER_SHORT);
}

TEST_CASE("IPv6ExtensionView create valid extension header")
{
    // Example: Next Header = 0x3A (ICMPv6), Length = 2 (header is 24 bytes: 2*8+8)
    // Options: 22 bytes (arbitrary values)
    const unsigned char packet[]
        = {0x3A, // Next Header
           0x02, // Length (2*8+8 = 24 bytes total)
           0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
           0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16};
    const std::span frame {reinterpret_cast<const std::byte*>(packet), sizeof(packet)};

    auto result = IPv6ExtensionView::create(frame);
    REQUIRE(result.has_value());
    auto view = result.value();

    CHECK(view.headerLength() == 24);
    CHECK(view.nextHeader() == 0x3A);
    CHECK(view.options().size() == 22);
    CHECK(view.payload().size() == 0);
}

TEST_CASE("IPv6FragmentView header too short")
{
    std::vector<std::byte> bytes(4, std::byte {0});
    auto result = IPv6FragmentView::create(std::span<const std::byte>(bytes));
    CHECK(result.error() == LayerError::HEADER_SHORT);
}

TEST_CASE("IPv6FragmentView create valid fragment header")
{
    // Example: Next Header = 0x06 (TCP), Reserved = 0,
    // Fragment Offset = 0x0018 (24 bytes), More Fragments flag set, Identification = 0x12345678
    /* clang-format off */
    const unsigned char packet[] = {
        0x06,       // Next Header (TCP)
        0x00,       // Reserved
        0x05, 0xa9, // Fragment Offset (0x05A8 >> 3 = 181), More Fragments flag set (lowest bit = 1)
        0x12, 0x34, 0x56, 0x78 // Identification
    };
    /* clang-format on */
    const std::span frame {reinterpret_cast<const std::byte*>(packet), sizeof(packet)};

    auto result = IPv6FragmentView::create(frame);
    REQUIRE(result.has_value());
    auto view = result.value();

    CHECK(view.headerLength() == 8);
    CHECK(view.nextHeader() == 0x06);
    CHECK(view.fragmentOffset() == 181); // Only upper 13 bits, in 8-byte units
    CHECK(view.moreFragmentsFollow());
    CHECK(view.payload().size() == 0);
}

} // namespace amon::layers
