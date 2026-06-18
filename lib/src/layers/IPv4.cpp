/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief IPv4 layer definitions.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/Test.hpp>
#include <amon/layers/IPv4.hpp>

#include <vector>

namespace amon::layers {

std::expected<IPv4View, LayerError> IPv4View::create(std::span<const std::byte> data) noexcept
{
    if (data.size() < sizeof(IPv4Header)) {
        return std::unexpected(LayerError::HEADER_SHORT);
    }
    auto view = IPv4View(data);

    if ((view.header().versionIhl >> 4) != 4) {
        return std::unexpected(LayerError::BAD_MAGIC);
    }
    if (view.headerLength() < sizeof(IPv4Header)) {
        return std::unexpected(LayerError::BAD_LEN_FIELD);
    }
    if (data.size() < view.headerLength()) {
        return std::unexpected(LayerError::HEADER_SHORT);
    }
    // Check disabled due to use-cases with trimmed packets.
    // if (data.size() < view.header().totalLength) {
    //     return std::unexpected(LayerError::BAD_PAYLOAD_LEN);
    // }
    // Check disabled because of tolerance of packets with set reserved bit.
    // if ((view.header().flagsFragmentOffset >> 15) != 0) {
    //     return std::unexpected(LayerError::BAD_MAGIC);
    // }
    return view;
}

static_assert(std::is_standard_layout<IPv4Header>(), "Unexpected data type layout for IPv4Header");
static_assert(
    std::is_trivially_default_constructible<IPv4Header>(),
    "Unexpected construction for IPv4Header");
static_assert(sizeof(IPv4Header) == 20, "IPv4Header size must be 20 bytes");

#ifdef AMON_ENABLE_TESTS

TEST(AmonLayersIPv4, IPv4viewCreateValidIPv4Header)
{
    // Minimal valid IPv4 header (20 bytes)
    const unsigned char packet[] = {
        0x45, 0x00, // Version=4, IHL=5, TOS=0
        0x00, 0x14, // Total length = 20 bytes
        0x00, 0x00, // ID = 0
        0x40, 0x00, // Flags=2 (Don't Fragment), Fragment Offset=0
        0x40, // TTL = 64
        0x06, // Protocol = TCP
        0x00, 0x00, // Header checksum = 0
        0xC0, 0xA8, 0x01, 0x01, // Source: 192.168.1.1
        0x08, 0x08, 0x08, 0x08 // Destination: 8.8.8.8
    };

    const std::span frame {reinterpret_cast<const std::byte*>(packet), sizeof(packet)};

    auto result = IPv4View::create(frame);
    ASSERT_TRUE(result.has_value());
    auto view = result.value();

    EXPECT_TRUE(view.headerLength() == 20);
    EXPECT_TRUE(view.protocol() == 0x06);
    EXPECT_TRUE(view.srcIP()[0] == std::byte {192});
    EXPECT_TRUE(view.srcIP()[1] == std::byte {168});
    EXPECT_TRUE(view.srcIP()[2] == std::byte {1});
    EXPECT_TRUE(view.srcIP()[3] == std::byte {1});
    EXPECT_TRUE(view.dstIP()[0] == std::byte {8});
    EXPECT_TRUE(view.dstIP()[1] == std::byte {8});
    EXPECT_TRUE(view.dstIP()[2] == std::byte {8});
    EXPECT_TRUE(view.dstIP()[3] == std::byte {8});
    EXPECT_FALSE(view.isFragmented());
    EXPECT_TRUE(view.fragmentOffset() == 0);
    EXPECT_TRUE(view.payload().size() == 0);
    EXPECT_TRUE(view.ttl() == 0x40);
    EXPECT_TRUE(view.totalLength() == 20);
}

TEST(AmonLayersIPv4, IPv4viewHeaderTooShort)
{
    std::vector<std::byte> bytes(10, std::byte {0});
    auto result = IPv4View::create(std::span<const std::byte>(bytes));
    EXPECT_TRUE(result.error() == LayerError::HEADER_SHORT);
}

TEST(AmonLayersIPv4, IPv4viewBadVersion)
{
    std::vector<std::byte> bytes(20, std::byte {0});
    bytes[0] = std::byte {0x55}; // Version=5, IHL=5
    auto result = IPv4View::create(std::span<const std::byte>(bytes));
    EXPECT_TRUE(result.error() == LayerError::BAD_MAGIC);
}

TEST(AmonLayersIPv4, IPv4viewBadIhl)
{
    std::vector<std::byte> bytes(20, std::byte {0});
    bytes[0] = std::byte {0x41}; // Version=4, IHL=1 (too short)
    auto result = IPv4View::create(std::span<const std::byte>(bytes));
    EXPECT_TRUE(result.error() == LayerError::BAD_LEN_FIELD);
}

TEST(AmonLayersIPv4, IPv4viewFragmentedPacket)
{
    // First: More fragments follow, offset = 0
    std::vector<std::byte> bytes(20, std::byte {0});
    bytes[0] = std::byte {0x45}; // Version=4, IHL=5
    bytes[2] = std::byte {0x00};
    bytes[3] = std::byte {0x14};
    bytes[6] = std::byte {0x20}; // Flags=1 (More Fragments), Fragment Offset=0
    auto result = IPv4View::create(std::span<const std::byte>(bytes));
    ASSERT_TRUE(result.has_value());
    auto view = result.value();
    EXPECT_TRUE(view.isFragmented());
    EXPECT_TRUE(view.moreFragmentsFollow());
    EXPECT_TRUE(view.fragmentOffset() == 0);

    // Second: Last fragment, non-zero offset
    std::vector<std::byte> lastFrag(20, std::byte {0});
    lastFrag[0] = std::byte {0x45}; // Version=4, IHL=5
    lastFrag[2] = std::byte {0x00};
    lastFrag[3] = std::byte {0x14};
    lastFrag[6] = std::byte {0x01}; // Flags=0 (last fragment), Fragment Offset high bits
    lastFrag[7] = std::byte {0x20}; // Fragment Offset low bits (0x120 = 288 bytes)
    auto result2 = IPv4View::create(std::span<const std::byte>(lastFrag));
    ASSERT_TRUE(result2.has_value());
    auto view2 = result2.value();
    EXPECT_TRUE(view2.isFragmented());
    EXPECT_FALSE(view2.moreFragmentsFollow());
    EXPECT_TRUE(view2.fragmentOffset() == 288);
}

#endif

} // namespace amon::layers
