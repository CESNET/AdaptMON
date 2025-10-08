/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief GRE layer definitions.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/Test.hpp>
#include <amon/Logger.hpp>
#include <amon/layers/GRE.hpp>
#include <amon/layers/Layer.hpp>

#include <vector>

namespace amon::layers {

std::expected<GREView, LayerError> GREView::create(std::span<const std::byte> data) noexcept
{
    if (data.size() < sizeof(GREHeader)) {
        return std::unexpected(LayerError::HEADER_SHORT);
    }
    auto view = GREView(data);
    if (data.size() < view.headerLength()) {
        return std::unexpected(LayerError::HEADER_SHORT);
    }
    if (view.version() == 1) {
        // Checksum, routing, strict, recursion and other (unused) bits must be 0 for PPTP GRE
        if ((view.header().flagsVersion & 0xCF78) != 0) {
            return std::unexpected(LayerError::BAD_MAGIC);
        }
        // Key bit must be 1 for PPTP GRE (key is always present)
        if ((view.header().flagsVersion & 0x2000) == 0) {
            return std::unexpected(LayerError::BAD_MAGIC);
        }
    } else if (view.version() == 0) {
        // Routing bit is currently not supported, other (unused) flags must be zero
        if ((view.header().flagsVersion & 0x40F8) != 0) {
            return std::unexpected(LayerError::UNSUPPORTED);
        }
    } else {
        return std::unexpected(LayerError::UNSUPPORTED);
    }
    return view;
}

static_assert(std::is_standard_layout<GREHeader>(), "Unexpected data type layout for GREHeader");
static_assert(
    std::is_trivially_default_constructible<GREHeader>(),
    "Unexpected construction for GREHeader");
static_assert(sizeof(GREHeader) == 4, "GREHeader size must be 4 bytes");

TEST_CASE("GREView create valid GRE header")
{
    // Minimal valid GRE header (4 bytes) + 4 bytes payload
    /* clang-format off */
    const unsigned char packet[] = {
        0x00, 0x00, // Flags/Version
        0x08, 0x00, // Protocol Type (example: 0x0800 for IPv4)
        0xDE, 0xAD, 0xBE, 0xEF // Payload
    };
    /* clang-format on */
    const std::span frame {reinterpret_cast<const std::byte*>(packet), sizeof(packet)};

    auto result = GREView::create(frame);
    REQUIRE(result.has_value());
    auto view = result.value();

    CHECK(view.headerLength() == 4);
    CHECK(view.protocolType() == 0x0800);
    CHECK(view.payload().size() == 4);
    CHECK(view.payload()[0] == std::byte {0xDE});
    CHECK(view.payload()[3] == std::byte {0xEF});
}

TEST_CASE("GREView create valid GRE header with checksum")
{
    /* clang-format off */
    const unsigned char packet[] = {
        0x80, 0x00, // Flags/Version
        0x08, 0x00, // Protocol Type (example: 0x0800 for IPv4)
        0x12, 0x34, // Checksum
        0x00, 0x00, // Reserved bytes
        0xDE, 0xAD, 0xBE, 0xEF // Payload
    };
    /* clang-format on */
    const std::span frame {reinterpret_cast<const std::byte*>(packet), sizeof(packet)};

    auto result = GREView::create(frame);
    REQUIRE(result.has_value());
    auto view = result.value();

    CHECK(view.headerLength() == 8);
    CHECK(view.protocolType() == 0x0800);
    CHECK(view.payload().size() == 4);
    CHECK(view.payload()[0] == std::byte {0xDE});
    CHECK(view.payload()[3] == std::byte {0xEF});
}

TEST_CASE("GREView create valid GRE header with key")
{
    /* clang-format off */
    const unsigned char packet[] = {
        0x20, 0x0, // Flags/Version
        0x1, 0x1, // Protocol Type (unknown)
        0xcf, 0xfe, 0x71, 0xa, // Key
        0xDE, 0xAD, 0xBE, 0xEF // Payload
    };
    /* clang-format on */
    const std::span frame {reinterpret_cast<const std::byte*>(packet), sizeof(packet)};

    auto result = GREView::create(frame);
    REQUIRE(result.has_value());
    auto view = result.value();

    CHECK(view.headerLength() == 8);
    CHECK(view.protocolType() == 0x0101);
    CHECK(view.payload().size() == 4);
    CHECK(view.payload()[0] == std::byte {0xDE});
    CHECK(view.payload()[3] == std::byte {0xEF});
}

TEST_CASE(
    "GREView create valid PPTP GRE (ver. 1) header with sequence number and acknowledgement number")
{
    /* clang-format off */
    const unsigned char packet[] = {
        0x30, 0x81, // Flags/Version
        0x08, 0x00, // Protocol Type (example: 0x0800 for IPv4)
        0x12, 0x34, // Key Payload Length
        0x56, 0x78, // Key Call ID
        0xDE, 0xAD, 0xBE, 0xEF, // Sequence number
        0x00, 0x00, 0x00, 0x01  // Acknowledgement number
    };
    /* clang-format on */
    const std::span frame {reinterpret_cast<const std::byte*>(packet), sizeof(packet)};

    auto result = GREView::create(frame);
    REQUIRE(result.has_value());
    auto view = result.value();

    CHECK(view.headerLength() == 16);
    CHECK(view.protocolType() == 0x0800);
    CHECK(view.payload().size() == 0);
}

TEST_CASE("GREView header too short")
{
    std::vector<std::byte> bytes(2, std::byte {0});
    auto result = GREView::create(std::span<const std::byte>(bytes));
    CHECK(result.error() == LayerError::HEADER_SHORT);
}

TEST_CASE("GREView bad magic/version")
{
    // Set version bits to non-zero (invalid)
    /* clang-format off */
    const unsigned char packet[] = {
        0x00, 0x01, // Version bits set
        0x08, 0x00,
        0x00, 0x00,
        0x00, 0x00
    };
    /* clang-format on */
    const std::span frame {reinterpret_cast<const std::byte*>(packet), sizeof(packet)};

    auto result = GREView::create(frame);
    CHECK(result.error() == LayerError::BAD_MAGIC);
}

TEST_CASE("GREView unsupported routing flag")
{
    // Set routing bit to 1 (unsupported)
    /* clang-format off */
    const unsigned char packet[] = {
        0x40, 0x00, // Routing bit set
        0x08, 0x00,
        0x00, 0x00,
        0x00, 0x00
    };
    /* clang-format on */
    const std::span frame {reinterpret_cast<const std::byte*>(packet), sizeof(packet)};

    auto result = GREView::create(frame);
    CHECK(result.error() == LayerError::UNSUPPORTED);
}

TEST_CASE("GREView missing options")
{
    // Key bit set but no key field present
    /* clang-format off */
    const unsigned char packet[] = {
        0x20, 0x00, // Key bit set
        0x08, 0x00, // Protocol Type
    };
    /* clang-format on */
    const std::span frame {reinterpret_cast<const std::byte*>(packet), sizeof(packet)};

    auto result = GREView::create(frame);
    CHECK(result.error() == LayerError::HEADER_SHORT);
}

} // namespace amon::layers
