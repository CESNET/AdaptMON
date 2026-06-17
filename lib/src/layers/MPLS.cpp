/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief MPLS layer definitions.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/Test.hpp>
#include <amon/layers/MPLS.hpp>

#include <vector>

namespace amon::layers {

std::expected<MPLSView, LayerError> MPLSView::create(std::span<const std::byte> data) noexcept
{
    if (data.size() < sizeof(MPLSHeader)) {
        return std::unexpected(LayerError::HEADER_SHORT);
    }
    return MPLSView(data);
}

static_assert(std::is_standard_layout<MPLSHeader>(), "Unexpected data type layout for MPLSHeader");
static_assert(
    std::is_trivially_default_constructible<MPLSHeader>(),
    "Unexpected construction for MPLSHeader");
static_assert(sizeof(MPLSHeader) == 4, "MPLSHeader size must be 4 bytes");

#ifdef AMON_ENABLE_TESTS

TEST(AmonLayersMPLS, MPLSviewCreateValidMPLSHeader)
{
    // Example MPLS header: 0x00 0x01 0x23 0x45
    std::vector<std::byte> bytes
        = {std::byte {0x00}, std::byte {0x01}, std::byte {0x23}, std::byte {0x45}};

    auto result = MPLSView::create(std::span<const std::byte>(bytes));
    ASSERT_TRUE(result.has_value());
    auto view = result.value();

    // API: label() returns 20 bits, bottomOfStack() returns bool
    EXPECT_TRUE(view.label() == 0x00012);
    EXPECT_TRUE(view.bottomOfStack() == true);
}

TEST(AmonLayersMPLS, MPLSviewHeaderTooShort)
{
    std::vector<std::byte> bytes(2, std::byte {0});
    auto result = MPLSView::create(std::span<const std::byte>(bytes));
    EXPECT_TRUE(result.error() == LayerError::HEADER_SHORT);
}

#endif

} // namespace amon::layers
