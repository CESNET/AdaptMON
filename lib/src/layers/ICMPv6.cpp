/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief ICMPv6 layer definitions.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/Test.hpp>
#include <amon/layers/ICMPv6.hpp>

#include <vector>

namespace amon::layers {

std::expected<ICMPv6View, LayerError> ICMPv6View::create(std::span<const std::byte> data) noexcept
{
    if (data.size() < sizeof(ICMPv6Header)) {
        return std::unexpected(LayerError::HEADER_SHORT);
    }
    return ICMPv6View(data);
}

static_assert(
    std::is_standard_layout<ICMPv6Header>(),
    "Unexpected data type layout for ICMPv6Header");
static_assert(
    std::is_trivially_default_constructible<ICMPv6Header>(),
    "Unexpected construction for ICMPv6Header");
static_assert(sizeof(ICMPv6Header) == 4, "ICMPv6Header size must be 4 bytes");

TEST_CASE("ICMPv6View create valid ICMPv6 header")
{
    // Minimal valid ICMPv6 header (4 bytes) + 4 bytes payload
    /* clang-format off */
    const unsigned char packet[] = {
        0x08, 0x00,       // Type (Echo Request), Code
        0x12, 0x34,       // Checksum
        0xDE, 0xAD, 0xBE, 0xEF // Payload
    };
    /* clang-format on */
    const std::span frame {reinterpret_cast<const std::byte*>(packet), sizeof(packet)};

    auto result = ICMPv6View::create(frame);
    REQUIRE(result.has_value());
    auto view = result.value();

    CHECK(view.type() == 8);
    CHECK(view.code() == 0);
    CHECK(view.payload().size() == 4);
    CHECK(view.payload()[0] == std::byte {0xDE});
    CHECK(view.payload()[3] == std::byte {0xEF});
}

TEST_CASE("ICMPv6View header too short")
{
    std::vector<std::byte> bytes(2, std::byte {0});
    auto result = ICMPv6View::create(std::span<const std::byte>(bytes));
    CHECK(result.error() == LayerError::HEADER_SHORT);
}

} // namespace amon::layers
