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

using namespace types;

static_assert(
    std::is_standard_layout<EthernetHeader>(),
    "Unexpected data type layout for EthernetHeader");
static_assert(
    std::is_trivially_default_constructible<EthernetHeader>(),
    "Unexpected construction for EthernetHeader");
static_assert(sizeof(EthernetHeader) == 14, "EthernetHeader size must be 14 bytes");

// TESTS
#ifdef AMON_ENABLE_TESTS

TEST(AmonLayersEthernet, EthernetviewCreationWithValidFrame)
{
    std::array<std::byte, 18> frame {};
    auto* hdr = reinterpret_cast<EthernetHeader*>(frame.data());
    hdr->destination = MACAddress("10:20:30:40:50:60");
    hdr->source = MACAddress("01:02:03:04:05:06");
    hdr->protocol = 0x0800; // IPv4

    auto viewOrErr = EthernetView::create(std::span(frame));
    ASSERT_TRUE(viewOrErr.has_value());
    const EthernetView view = *viewOrErr;

    EXPECT_TRUE(view.protocol() == 0x0800);
    EXPECT_TRUE(view.dst() == MACAddress("10:20:30:40:50:60"));
    EXPECT_TRUE(view.src() == MACAddress("01:02:03:04:05:06"));
    EXPECT_TRUE(view.payload().size() == 4);
}

TEST(AmonLayersEthernet, EthernetviewCreationFailsOnShortFrame)
{
    std::array<std::byte, 10> shortFrame {};
    auto viewOrErr = EthernetView::create(std::span(shortFrame));
    EXPECT_TRUE(!viewOrErr.has_value());
    EXPECT_TRUE(viewOrErr.error() == LayerError::HEADER_SHORT);
}

TEST(AmonLayersEthernet, EthernetviewPayloadExtraction)
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
    ASSERT_TRUE(viewOrErr.has_value());
    const EthernetView view = *viewOrErr;

    EXPECT_TRUE(view.payload().size() == 6);
    // Check payload is correct (all zeros in this test)
    for (auto b : view.payload()) {
        EXPECT_TRUE(b == std::byte {0});
    }
}

TEST(AmonLayersEthernet, EthernetviewHeaderFields)
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
    ASSERT_TRUE(viewOrErr.has_value());
    const EthernetView view = *viewOrErr;

    EXPECT_TRUE(view.dst() == MACAddress("aa:bb:cc:dd:ee:ff"));
    EXPECT_TRUE(view.src() == MACAddress("11:22:33:44:55:66"));
    EXPECT_TRUE(view.protocol() == 0x1234);
}

#endif

} // namespace amon::layers
