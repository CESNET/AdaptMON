
/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief TCP option layer definitions.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/Test.hpp>
#include <amon/layers/TCPOptions.hpp>

#include <cassert>
#include <optional>
#include <vector>

namespace amon::layers {

static constexpr std::size_t KIND_AND_LENGTH_SIZE = 2;

static bool isEOLorNOP(uint8_t kind) noexcept
{
    return kind == std::to_underlying(TCPOptionKind::EOL)
        || kind == std::to_underlying(TCPOptionKind::NOP);
}

std::expected<TCPOptionView, LayerError>
TCPOptionView::create(std::span<const std::byte> data) noexcept
{
    if (data.size() < sizeof(TCPOptionHeader)) {
        return std::unexpected(LayerError::HEADER_SHORT);
    }

    auto view = TCPOptionView(data);

    if (isEOLorNOP(view.kind())) {
        return view;
    }

    if (data.size() < KIND_AND_LENGTH_SIZE) {
        return std::unexpected(LayerError::HEADER_SHORT);
    }

    const uint8_t length = view.optionLength();

    if (data.size() < length || length < KIND_AND_LENGTH_SIZE) {
        return std::unexpected(LayerError::BAD_LEN_FIELD);
    }

    return view;
}

uint8_t TCPOptionView::optionLength() const noexcept
{
    /**
     * Special TCP options END_OF_OPTION_LIST (0) and NO_OPERATION (1) do not have a length
     * field, so their size is just 1 byte.
     */
    if (isEOLorNOP(kind())) {
        return sizeof(TCPOptionHeader);
    }

    /**
     * For all other options (e.g., MSS, SACK, Timestamp), the second byte in the data stream
     * (data[1]) is the length field, which indicates the total size of the TCP option in bytes,
     * including kind + length.
     */
    return static_cast<uint8_t>(data[1]);
}

std::span<const std::byte> TCPOptionView::getValueSpan() const noexcept
{
    if (isEOLorNOP(kind())) {
        return {};
    }

    const std::size_t valueLength = static_cast<std::size_t>(data[1]) - KIND_AND_LENGTH_SIZE;
    const std::byte* valuePointer = data.data() + KIND_AND_LENGTH_SIZE;
    return {valuePointer, valueLength};
}

TCPOptionIterator TCPOptionRange::begin() const noexcept
{
    return {m_data, 0};
}

TCPOptionIterator TCPOptionRange::end() const noexcept
{
    return {m_data, m_data.size()};
}

static_assert(
    std::is_standard_layout<TCPOptionHeader>(),
    "Unexpected data type layout for TCPOptionHeader");
static_assert(
    std::is_trivially_default_constructible<TCPOptionHeader>(),
    "Unexpected construction for TCPOptionHeader");
static_assert(sizeof(TCPOptionHeader) == 1, "TCPOptionHeader size must be 1 byte");

#ifdef AMON_ENABLE_TESTS

TEST(AmonLayersTCPOptions, TCPoptionrangeIterationOverMultipleValidOptions)
{
    // clang-format off
    const std::vector<std::byte> rawData = {
        // MSS
        std::byte {0x02}, std::byte {0x04}, std::byte {0x05}, std::byte {0xb4},
        // SACK
        std::byte {0x04}, std::byte {0x02},
        // TIMESTAMP
        std::byte {0x08}, std::byte {0x0a}, std::byte {0x05}, std::byte {0x33},
        std::byte {0x9f}, std::byte {0x0e}, std::byte {0x00}, std::byte {0x00},
        std::byte {0x00}, std::byte {0x00},
        // NOP
        std::byte {0x01},
        // SCALE
        std::byte {0x03}, std::byte {0x03}, std::byte {0x00},
        // EOL
        std::byte {0x00},
    };
    // clang-format on

    const TCPOptionRange optionRange(rawData);
    std::size_t optionIndex = 0;
    for (const auto& option : optionRange) {
        ASSERT_TRUE(option.has_value());

        if (optionIndex == 0) {
            EXPECT_TRUE(option->kind() == std::to_underlying(TCPOptionKind::MSS));
        } else if (optionIndex == 1) {
            EXPECT_TRUE(option->kind() == std::to_underlying(TCPOptionKind::SACK_PERMITTED));
        } else if (optionIndex == 2) {
            EXPECT_TRUE(option->kind() == std::to_underlying(TCPOptionKind::TIMESTAMP));
        } else if (optionIndex == 3) {
            EXPECT_TRUE(option->kind() == std::to_underlying(TCPOptionKind::NOP));
        } else if (optionIndex == 4) {
            EXPECT_TRUE(option->kind() == std::to_underlying(TCPOptionKind::SCALE));
        } else if (optionIndex == 5) {
            EXPECT_TRUE(option->kind() == std::to_underlying(TCPOptionKind::EOL));
        }
        ++optionIndex;
    }

    EXPECT_TRUE(optionIndex == 6);
}

TEST(AmonLayersTCPOptions, TCPoptionrangeIterationWithEolTermination)
{
    // clang-format off
    const std::vector<std::byte> rawData = {
        // MSS
        std::byte {0x02}, std::byte {0x04}, std::byte {0x05}, std::byte {0xb4},
        // SACK
        std::byte {0x04}, std::byte {0x02},
        // TIMESTAMP
        std::byte {0x08}, std::byte {0x0a}, std::byte {0x05}, std::byte {0x33},
        std::byte {0x9f}, std::byte {0x0e}, std::byte {0x00}, std::byte {0x00},
        std::byte {0x00}, std::byte {0x00},
        // EOL
        std::byte {0x00},
        // NOP
        std::byte {0x01},
        // SCALE
        std::byte {0x03}, std::byte {0x03}, std::byte {0x00},
        // EOL
        std::byte {0x00},
    };
    // clang-format on

    const TCPOptionRange optionRange(rawData);
    std::size_t optionIndex = 0;
    for (const auto& option : optionRange) {
        ASSERT_TRUE(option.has_value());

        if (optionIndex == 0) {
            EXPECT_TRUE(option->kind() == std::to_underlying(TCPOptionKind::MSS));
        } else if (optionIndex == 1) {
            EXPECT_TRUE(option->kind() == std::to_underlying(TCPOptionKind::SACK_PERMITTED));
        } else if (optionIndex == 2) {
            EXPECT_TRUE(option->kind() == std::to_underlying(TCPOptionKind::TIMESTAMP));
        } else if (optionIndex == 3) {
            EXPECT_TRUE(option->kind() == std::to_underlying(TCPOptionKind::EOL));
        }
        ++optionIndex;
    }

    EXPECT_TRUE(optionIndex == 4);
}

TEST(AmonLayersTCPOptions, TCPoptionrangeIterationWithInvalidSackOptionSize)
{
    // clang-format off
    const std::vector<std::byte> rawData = {
        // MSS
        std::byte {0x02}, std::byte {0x04}, std::byte {0x05}, std::byte {0xb4},
        // SACK (invalid length - should be 5 bytes but 2 provided)
        std::byte {0x04}, std::byte {0x05},
    };
    // clang-format on

    const TCPOptionRange optionRange(rawData);
    std::size_t optionIndex = 0;
    for (const auto& option : optionRange) {
        if (optionIndex == 0) {
            ASSERT_TRUE(option.has_value());
            EXPECT_TRUE(option->kind() == std::to_underlying(TCPOptionKind::MSS));
        } else if (optionIndex == 1) {
            ASSERT_TRUE(!option.has_value());
            EXPECT_TRUE(option.error() == LayerError::BAD_LEN_FIELD);
        }
        ++optionIndex;
    }

    EXPECT_TRUE(optionIndex == 2);
}

TEST(AmonLayersTCPOptions, TCPoptionrangeIterationWithInvalidTimestampOptionSize)
{
    // clang-format off
    const std::vector<std::byte> rawData = {
        // MSS
        std::byte {0x02}, std::byte {0x04}, std::byte {0x05}, std::byte {0xb4},
        // TIMESTAMP (invalid length - should be 10 bytes but only 9 provided)
        std::byte {0x08}, std::byte {0x0a}, std::byte {0x05}, std::byte {0x33},
        std::byte {0x9f}, std::byte {0x0e}, std::byte {0x00}, std::byte {0x00},
        std::byte {0x00},
    };
    // clang-format on

    const TCPOptionRange optionRange(rawData);
    std::size_t optionIndex = 0;
    for (const auto& option : optionRange) {
        if (optionIndex == 0) {
            ASSERT_TRUE(option.has_value());
            EXPECT_TRUE(option->kind() == std::to_underlying(TCPOptionKind::MSS));
        } else if (optionIndex == 1) {
            ASSERT_TRUE(!option.has_value());
            EXPECT_TRUE(option.error() == LayerError::BAD_LEN_FIELD);
        }
        ++optionIndex;
    }

    EXPECT_TRUE(optionIndex == 2);
}

TEST(AmonLayersTCPOptions, TCPoptionviewCreate)
{
    const std::vector<std::byte> rawDataShort = {std::byte {0xb4}};
    const std::vector<std::byte> rawDataBadLen = {std::byte {0xb4}, std::byte {0xb4}};

    EXPECT_TRUE(TCPOptionView::create({}) == std::unexpected(LayerError::HEADER_SHORT));
    EXPECT_TRUE(TCPOptionView::create(rawDataShort) == std::unexpected(LayerError::HEADER_SHORT));
    EXPECT_TRUE(TCPOptionView::create(rawDataBadLen) == std::unexpected(LayerError::BAD_LEN_FIELD));
}

TEST(AmonLayersTCPOptions, TCPoptionviewDataAccessorsValidation)
{
    const std::vector<std::byte> rawData
        = {std::byte {0x02}, std::byte {0x04}, std::byte {0x05}, std::byte {0xb4}};

    const auto optionView = TCPOptionView::create(rawData);
    ASSERT_TRUE(optionView.has_value());
    if (optionView.has_value()) {
        EXPECT_TRUE(optionView->kind() == 0x02);
        EXPECT_TRUE(optionView->optionLength() == 0x04);
        const auto valueSpan = optionView->getValueSpan();
        EXPECT_TRUE(valueSpan.size() == 2);
        EXPECT_TRUE(valueSpan[0] == std::byte {0x05});
        EXPECT_TRUE(valueSpan[1] == std::byte {0xb4});
    }
}

#endif

} // namespace amon::layers
