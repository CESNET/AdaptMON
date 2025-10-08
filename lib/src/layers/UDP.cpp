/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief UDP layer definitions.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/Test.hpp>
#include <amon/layers/UDP.hpp>

#include <vector>

namespace amon::layers {

std::expected<UDPView, LayerError> UDPView::create(std::span<const std::byte> data) noexcept
{
    if (data.size() < sizeof(UDPHeader)) {
        return std::unexpected(LayerError::HEADER_SHORT);
    }
    auto view = UDPView(data);

    if (view.header().length < sizeof(UDPHeader)) {
        return std::unexpected(LayerError::BAD_LEN_FIELD);
    }
    // Check disabled due to use-cases with trimmed packets.
    // if (data.size() < view.header().length) {
    //     return std::unexpected(LayerError::BAD_PAYLOAD_LEN);
    // }
    return view;
}

static_assert(std::is_standard_layout<UDPHeader>(), "Unexpected data type layout for UDPHeader");
static_assert(
    std::is_trivially_default_constructible<UDPHeader>(),
    "Unexpected construction for UDPHeader");
static_assert(sizeof(UDPHeader) == 8, "UDPHeader size must be 8 bytes");

TEST_CASE("UDPView create valid UDP header")
{
    // Minimal valid UDP header (8 bytes) + 4 bytes payload
    /* clang-format off */
    const unsigned char packet[] = {
        0x12, 0x34, // Source port
        0x56, 0x78, // Destination port
        0x00, 0x0C, // Length (12 bytes: 8 header + 4 payload)
        0xAB, 0xCD, // Checksum
        0xDE, 0xAD, 0xBE, 0xEF // Payload
    };
    /* clang-format on */
    const std::span frame {reinterpret_cast<const std::byte*>(packet), sizeof(packet)};

    auto result = UDPView::create(frame);
    REQUIRE(result.has_value());
    auto view = result.value();

    CHECK(view.srcPort() == 4660);
    CHECK(view.dstPort() == 22136);
    CHECK(view.payload().size() == 4);
    CHECK(view.payload()[0] == std::byte {0xDE});
    CHECK(view.payload()[3] == std::byte {0xEF});
}

TEST_CASE("UDPView header too short")
{
    std::vector<std::byte> bytes(6, std::byte {0});
    auto result = UDPView::create(std::span<const std::byte>(bytes));
    CHECK(result.error() == LayerError::HEADER_SHORT);
}

TEST_CASE("UDPView bad length field")
{
    // Length field less than header size
    /* clang-format off */
    const unsigned char packet[] = {
        0x00, 0x01, // Source port
        0x00, 0x02, // Destination port
        0x00, 0x06, // Length (6 bytes, invalid)
        0x00, 0x00  // Checksum
    };
    /* clang-format on */
    const std::span frame {reinterpret_cast<const std::byte*>(packet), sizeof(packet)};

    auto result = UDPView::create(frame);
    CHECK(result.error() == LayerError::BAD_LEN_FIELD);
}

} // namespace amon::layers
