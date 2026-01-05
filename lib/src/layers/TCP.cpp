/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief TCP layer definitions.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/Test.hpp>
#include <amon/layers/TCP.hpp>

#include <vector>

namespace amon::layers {

std::expected<TCPView, LayerError> TCPView::create(std::span<const std::byte> data) noexcept
{
    if (data.size() < sizeof(TCPHeader)) {
        return std::unexpected(LayerError::HEADER_SHORT);
    }
    auto view = TCPView(data);

    if (view.headerLength() < sizeof(TCPHeader)) {
        return std::unexpected(LayerError::BAD_LEN_FIELD);
    }
    if (data.size() < view.headerLength()) {
        return std::unexpected(LayerError::HEADER_SHORT);
    }
    return view;
}

static_assert(std::is_standard_layout<TCPHeader>(), "Unexpected data type layout for TCPHeader");
static_assert(
    std::is_trivially_default_constructible<TCPHeader>(),
    "Unexpected construction for TCPHeader");
static_assert(sizeof(TCPHeader) == 20, "TCPHeader size must be 20 bytes");

TEST_CASE("TCPView create valid TCP header with payload")
{
    // Minimal valid TCP header (20 bytes) + 4 bytes payload
    const unsigned char packet[] = {
        0x12, 0x34, // Source port
        0x56, 0x78, // Destination port
        0x01, 0x02, 0x03, 0x04, // Sequence number
        0x05, 0x06, 0x07, 0x08, // Acknowledge number
        0x50, // Data offset (5) << 4, Reserved
        0x18, // Flags
        0x00, 0x10, // Window
        0xAB, 0xCD, // Checksum
        0x00, 0x00, // Urgent pointer
        0xDE, 0xAD, 0xBE, 0xEF // Payload
    };
    const std::span frame {reinterpret_cast<const std::byte*>(packet), sizeof(packet)};

    auto result = TCPView::create(frame);
    REQUIRE(result.has_value());
    auto view = result.value();

    CHECK(view.headerLength() == 20);
    CHECK(view.srcPort() == 4660);
    CHECK(view.dstPort() == 22136);
    CHECK(view.sequenceNumber() == 0x01020304);
    CHECK(view.acknowledgeNumber() == 0x05060708);
    CHECK(view.payload().size() == 4);
    CHECK(view.payload()[0] == std::byte {0xDE});
    CHECK(view.payload()[3] == std::byte {0xEF});
}

TEST_CASE("TCPView header too short")
{
    std::vector<std::byte> bytes(10, std::byte {0});
    auto result = TCPView::create(std::span<const std::byte>(bytes));
    CHECK(result.error() == LayerError::HEADER_SHORT);
}

TEST_CASE("TCPView options missing")
{
    // Data offset field indicates header longer than actual data
    const unsigned char packet[] = {
        0x00, 0x01, // Source port
        0x00, 0x02, // Destination port
        0x00, 0x00, 0x00, 0x00, // Sequence number
        0x00, 0x00, 0x00, 0x00, // Acknowledge number
        0xF0, // Data offset (15) << 4, Reserved (header = 60 bytes)
        0x00, // Flags
        0x00, 0x00, // Window
        0x00, 0x00, // Checksum
        0x00, 0x00 // Urgent pointer
        // Only 20 bytes, but header claims 60
    };
    const std::span frame {reinterpret_cast<const std::byte*>(packet), sizeof(packet)};
    auto result = TCPView::create(frame);
    CHECK(result.error() == LayerError::HEADER_SHORT);
}

TEST_CASE("TCPView bad length field")
{
    // Data offset field indicates header shorted than allowed
    const unsigned char packet[] = {
        0x00, 0x01, // Source port
        0x00, 0x02, // Destination port
        0x00, 0x00, 0x00, 0x00, // Sequence number
        0x00, 0x00, 0x00, 0x00, // Acknowledge number
        0x40, // Data offset (4), Reserved (header = 16 bytes)
        0x00, // Flags
        0x00, 0x00, // Window
        0x00, 0x00, // Checksum
        0x00, 0x00 // Urgent pointer
    };
    const std::span frame {reinterpret_cast<const std::byte*>(packet), sizeof(packet)};
    auto result = TCPView::create(frame);
    CHECK(result.error() == LayerError::BAD_LEN_FIELD);
}

} // namespace amon::layers
