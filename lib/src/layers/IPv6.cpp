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

std::expected<IPv6FragmentView, LayerError>
IPv6FragmentView::create(std::span<const std::byte> data) noexcept
{
    if (data.size() < sizeof(IPv6FragmentHeader)) {
        return std::unexpected(LayerError::HEADER_SHORT);
    }
    return IPv6FragmentView(data);
}

std::expected<IPv6RoutingView, LayerError>
IPv6RoutingView::create(std::span<const std::byte> data) noexcept
{
    if (data.size() < sizeof(IPv6RoutingHeader)) {
        return std::unexpected(LayerError::HEADER_SHORT);
    }
    return IPv6RoutingView(data);
}

static_assert(std::is_standard_layout<IPv6Header>(), "Unexpected data type layout for IPv6Header");
static_assert(
    std::is_standard_layout<IPv6FragmentHeader>(),
    "Unexpected data type layout for IPv6FragmentHeader");
static_assert(
    std::is_standard_layout<IPv6RoutingHeader>(),
    "Unexpected data type layout for IPv6RoutingHeader");
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
    std::is_trivially_default_constructible<IPv6RoutingHeader>(),
    "Unexpected construction for IPv6RoutingHeader");
static_assert(
    std::is_trivially_default_constructible<IPv6ExtensionHeader>(),
    "Unexpected construction for IPv6ExtensionHeader");
static_assert(sizeof(IPv6Header) == 40, "IPv6Header size must be 40 bytes");
static_assert(sizeof(IPv6FragmentHeader) == 8, "IPv6FragmentHeader size must be 8 bytes");
static_assert(sizeof(IPv6RoutingHeader) == 4, "IPv6RoutingHeader size must be 4 bytes");
static_assert(sizeof(IPv6ExtensionHeader) == 2, "IPv6ExtensionHeader size must be 2 bytes");

#ifdef AMON_ENABLE_TESTS

TEST(AmonLayersIPv6, IPv6viewCreateValidIPv6Header)
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
    ASSERT_TRUE(result.has_value());
    auto view = result.value();

    EXPECT_TRUE(view.headerLength() == 40);
    EXPECT_TRUE(view.nextHeader() == 0x3A);
    EXPECT_TRUE(view.hopLimit() == 0x40);
    EXPECT_TRUE(view.payload().size() == 0); // Payload is empty in this minimal example
    EXPECT_TRUE(view.payloadLength() == 20);
    // Check source and destination addresses are accessible
    // FIXME: check IPs when better IP address type API will be implemented
    (void) view.srcIP();
    (void) view.dstIP();
}

TEST(AmonLayersIPv6, IPv6viewHeaderTooShort)
{
    std::vector<std::byte> bytes(10, std::byte {0});
    auto result = IPv6View::create(std::span<const std::byte>(bytes));
    EXPECT_TRUE(result.error() == LayerError::HEADER_SHORT);
}

TEST(AmonLayersIPv6, IPv6viewBadVersion)
{
    std::vector<std::byte> bytes(40, std::byte {0});
    bytes[0] = std::byte {0x40}; // Version=4, should be 6
    auto result = IPv6View::create(std::span<const std::byte>(bytes));
    EXPECT_TRUE(result.error() == LayerError::BAD_MAGIC);
}

TEST(AmonLayersIPv6, IPv6extensionviewHeaderTooShort)
{
    std::vector<std::byte> bytes(1, std::byte {0});
    auto result = IPv6ExtensionView::create(std::span<const std::byte>(bytes));
    EXPECT_TRUE(result.error() == LayerError::HEADER_SHORT);
}

TEST(AmonLayersIPv6, IPv6extensionviewCreateValidExtensionHeader)
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
    ASSERT_TRUE(result.has_value());
    auto view = result.value();

    EXPECT_TRUE(view.headerLength() == 24);
    EXPECT_TRUE(view.nextHeader() == 0x3A);
    EXPECT_TRUE(view.options().size() == 22);
    EXPECT_TRUE(view.payload().size() == 0);
}

TEST(AmonLayersIPv6, IPv6fragmentviewHeaderTooShort)
{
    std::vector<std::byte> bytes(4, std::byte {0});
    auto result = IPv6FragmentView::create(std::span<const std::byte>(bytes));
    EXPECT_TRUE(result.error() == LayerError::HEADER_SHORT);
}

TEST(AmonLayersIPv6, IPv6fragmentviewCreateValidFragmentHeader)
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
    ASSERT_TRUE(result.has_value());
    auto view = result.value();

    EXPECT_TRUE(view.headerLength() == 8);
    EXPECT_TRUE(view.nextHeader() == 0x06);
    EXPECT_TRUE(view.fragmentOffset() == 181); // Only upper 13 bits, in 8-byte units
    EXPECT_TRUE(view.moreFragmentsFollow());
    EXPECT_TRUE(view.payload().size() == 0);
}

TEST(AmonLayersIPv6, IPv6routingviewHeaderTooShort)
{
    std::vector<std::byte> bytes(2, std::byte {0});
    auto result = IPv6RoutingView::create(std::span<const std::byte>(bytes));
    EXPECT_TRUE(result.error() == LayerError::HEADER_SHORT);
}

TEST(AmonLayersIPv6, IPv6routingviewCreateValidRoutingHeader)
{
    // Next Header = 0x3A (ICMPv6), Length = 2 (header is 24 bytes: 2*8+8)
    // Routing Type = 1, Segments Left = 3, Routing Data = 20 bytes (arbitrary)
    /* clang-format off */
    const unsigned char packet[] = {
        0x3A, // Next Header
        0x02, // Length (2*8+8 = 24 bytes total)
        0x01, // Routing Type
        0x03, // Segments Left
        // Routing Data (20 bytes)
        0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0x33, 0x44,
        0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x12, 0x34, 0x56, 0x78
    };
    /* clang-format on */
    const std::span frame {reinterpret_cast<const std::byte*>(packet), sizeof(packet)};

    auto result = IPv6RoutingView::create(frame);
    ASSERT_TRUE(result.has_value());
    auto view = result.value();

    EXPECT_TRUE(view.headerLength() == 24);
    EXPECT_TRUE(view.nextHeader() == 0x3A);
    EXPECT_TRUE(view.routingType() == 0x01);
    EXPECT_TRUE(view.segmentsLeft() == 0x03);
    EXPECT_TRUE(view.routingData().size() == 20);
    EXPECT_TRUE(view.payload().size() == 0);

    // Optionally check routingData contents
    const auto routingData = view.routingData();
    EXPECT_TRUE(reinterpret_cast<const unsigned char*>(routingData.data())[0] == 0xAA);
    EXPECT_TRUE(reinterpret_cast<const unsigned char*>(routingData.data())[19] == 0x78);
}

#endif

} // namespace amon::layers
