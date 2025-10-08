/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief Ethernet layer definitions.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/Test.hpp>
#include <amon/layers/Ethernet.hpp>

namespace amon::layers {

std::expected<EthernetView, LayerError>
EthernetView::create(std::span<const std::byte> data) noexcept
{
    if (data.size() < sizeof(EthernetHeader)) {
        return std::unexpected(LayerError::HEADER_SHORT);
    }
    return EthernetView(data);
}

static_assert(
    std::is_standard_layout<EthernetHeader>(),
    "Unexpected data type layout for EthernetHeader");
static_assert(
    std::is_trivially_default_constructible<EthernetHeader>(),
    "Unexpected construction for EthernetHeader");
static_assert(sizeof(EthernetHeader) == 14, "EthernetHeader size must be 14 bytes");

// TESTS
TEST_CASE("EthernetView creation with valid frame")
{
    std::array<std::byte, 18> frame {};
    auto* hdr = reinterpret_cast<EthernetHeader*>(frame.data());
    hdr->destination = MACAddress("10:20:30:40:50:60");
    hdr->source = MACAddress("01:02:03:04:05:06");
    hdr->protocol = 0x0800; // IPv4

    auto viewOrErr = EthernetView::create(std::span(frame));
    REQUIRE(viewOrErr.has_value());
    const EthernetView view = *viewOrErr;

    CHECK(view.protocol() == 0x0800);
    CHECK(view.dst() == MACAddress("10:20:30:40:50:60"));
    CHECK(view.src() == MACAddress("01:02:03:04:05:06"));
    CHECK(view.payload().size() == 4);
}

TEST_CASE("EthernetView creation fails on short frame")
{
    std::array<std::byte, 10> shortFrame {};
    auto viewOrErr = EthernetView::create(std::span(shortFrame));
    CHECK(!viewOrErr.has_value());
    CHECK(viewOrErr.error() == LayerError::HEADER_SHORT);
}

TEST_CASE("EthernetView payload extraction")
{
    /* clang-format off */
    const unsigned char packet[] = {
        0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, // dst mac
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, // src mac
        0x12, 0x34, // protocol
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, // payload
    };
    /* clang-format on */

    const std::span frame {reinterpret_cast<const std::byte*>(packet), sizeof(packet)};

    auto viewOrErr = EthernetView::create(frame);
    REQUIRE(viewOrErr.has_value());
    const EthernetView view = *viewOrErr;

    CHECK(view.payload().size() == 6);
    // Check payload is correct (all zeros in this test)
    for (auto b : view.payload()) {
        CHECK(b == std::byte {0});
    }
}

TEST_CASE("EthernetView header fields")
{
    /* clang-format off */
    const unsigned char packet[] = {
        0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, // dst mac
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, // src mac
        0x12, 0x34 // protocol
    };
    /* clang-format on */

    const std::span frame {reinterpret_cast<const std::byte*>(packet), sizeof(packet)};

    auto viewOrErr = EthernetView::create(std::span(frame));
    REQUIRE(viewOrErr.has_value());
    const EthernetView view = *viewOrErr;

    CHECK(view.dst() == MACAddress("aa:bb:cc:dd:ee:ff"));
    CHECK(view.src() == MACAddress("11:22:33:44:55:66"));
    CHECK(view.protocol() == 0x1234);
}

}; // namespace amon::layers
