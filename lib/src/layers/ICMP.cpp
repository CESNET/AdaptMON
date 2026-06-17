/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief ICMP layer definitions.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/Test.hpp>
#include <amon/layers/ICMP.hpp>

#include <vector>

namespace amon::layers {

std::expected<ICMPView, LayerError> ICMPView::create(std::span<const std::byte> data) noexcept
{
    if (data.size() < sizeof(ICMPHeader)) {
        return std::unexpected(LayerError::HEADER_SHORT);
    }
    return ICMPView(data);
}

static_assert(std::is_standard_layout<ICMPHeader>(), "Unexpected data type layout for ICMPHeader");
static_assert(
    std::is_trivially_default_constructible<ICMPHeader>(),
    "Unexpected construction for ICMPHeader");
static_assert(sizeof(ICMPHeader) == 4, "ICMPHeader size must be 4 bytes");

#ifdef AMON_ENABLE_TESTS

TEST(AmonLayersICMP, ICMPviewCreateValidICMPHeader)
{
    // Minimal valid ICMP header (8 bytes) + 4 bytes payload
    /* clang-format off */
    const unsigned char packet[] = {
        0x08, 0x00,       // Type (Echo), Code
        0x12, 0x34,       // Checksum
        0x00, 0x01, 0x00, 0x02, // Rest of header
        0xDE, 0xAD, 0xBE, 0xEF  // Payload
    };
    /* clang-format on */
    const std::span frame {reinterpret_cast<const std::byte*>(packet), sizeof(packet)};

    auto result = ICMPView::create(frame);
    ASSERT_TRUE(result.has_value());
    auto view = result.value();

    EXPECT_TRUE(view.type() == 8);
    EXPECT_TRUE(view.code() == 0);
    // Rest (sometimes noted in ICMP header) is part of payload
    EXPECT_TRUE(view.payload().size() == 8);
    EXPECT_TRUE(view.payload()[0] == std::byte {0x00});
    EXPECT_TRUE(view.payload()[3] == std::byte {0x02});
    // Continuous ICMP data
    EXPECT_TRUE(view.payload()[4] == std::byte {0xDE});
    EXPECT_TRUE(view.payload()[7] == std::byte {0xEF});
}

TEST(AmonLayersICMP, ICMPviewHeaderTooShort)
{
    std::vector<std::byte> bytes(2, std::byte {0});
    auto result = ICMPView::create(std::span<const std::byte>(bytes));
    EXPECT_TRUE(result.error() == LayerError::HEADER_SHORT);
}

#endif

} // namespace amon::layers
