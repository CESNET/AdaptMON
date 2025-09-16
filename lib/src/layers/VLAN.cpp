/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief VLAN layer definitions.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/Test.hpp>
#include <amon/layers/VLAN.hpp>

#include <vector>

namespace amon::layers {

std::expected<VLANView, LayerError> VLANView::create(std::span<const std::byte> data) noexcept
{
    if (data.size() < sizeof(VLANHeader)) {
        return std::unexpected(LayerError::HEADER_SHORT);
    }
    return VLANView(data);
}

static_assert(std::is_standard_layout<VLANHeader>(), "Unexpected data type layout for VLANHeader");
static_assert(
    std::is_trivially_default_constructible<VLANHeader>(),
    "Unexpected construction for VLANHeader");
static_assert(sizeof(VLANHeader) == 4, "VLANHeader size must be 4 bytes");

TEST_CASE("VLANView create valid VLAN header")
{
    // Example VLAN header: TCI = 0x1234, Protocol = 0x0800 (IPv4)

    /* clang-format off */
    const unsigned char packet[] = {
        0x12, 0x34, // TCI
        0x08, 0x00, // Protocol (EtherType)
        0xDE, 0xAD  // Payload (example)
    };
    /* clang-format on */
    const std::span frame {reinterpret_cast<const std::byte*>(packet), sizeof(packet)};

    auto result = VLANView::create(frame);
    REQUIRE(result.has_value());
    auto view = result.value();

    CHECK(view.protocol() == 0x0800);
    CHECK(view.tag() == 564);
    CHECK(view.payload().size() == 2);
    CHECK(view.payload()[0] == std::byte {0xDE});
    CHECK(view.payload()[1] == std::byte {0xAD});
}

TEST_CASE("VLANView header too short")
{
    std::vector<std::byte> bytes(2, std::byte {0});
    auto result = VLANView::create(std::span<const std::byte>(bytes));
    CHECK(result.error() == LayerError::HEADER_SHORT);
}

} // namespace amon::layers
