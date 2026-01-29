/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief Encapsulating Security Payload layer definitions.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/layers/Layer.hpp>
#include <amon/layers/LayerView.hpp>
#include <amon/types/UIntBE.hpp>

#include <expected>
#include <span>

namespace amon::layers {

/**
 * @brief Structure representing an IPsec ESP Header.
 */
struct [[gnu::packed]] ESPHeader {
    types::UInt32BE spi; ///< Security Parameters Index
    types::UInt32BE sequenceNumber; ///< Sequence Number
};

/**
 * @brief Layer view for IPsec ESP Header.
 */
class ESPView : public LayerView<ESPHeader> {
public:
    /// Layer type identifier.
    static const LayerType LAYER_TYPE = LayerType::ESP;

    /**
     * @brief Create and validate an ESPView from a byte span.
     * @param data Span of bytes containing the ESP header.
     * @return std::expected containing ESPView or LayerError.
     */
    static std::expected<ESPView, LayerError> create(std::span<const std::byte> data) noexcept;

    /**
     * @brief Get the length of the ESP header.
     * @return Length of the ESP header in bytes.
     */
    constexpr uint16_t
    headerLength() const noexcept // NOLINT(readability-convert-member-functions-to-static)
    {
        return sizeof(ESPHeader);
    }

    /**
     * @brief Get the payload continuing after ESP header.
     * @return Span of bytes representing the payload.
     */
    std::span<const std::byte> payload() const { return data.subspan(headerLength()); }

    /**
     * @brief Get the Security Parameters Index (SPI).
     * @return SPI value.
     */
    uint32_t securityParametersIndex() const noexcept { return header().spi; }

    /**
     * @brief Get the sequence number.
     * @return Sequence number.
     */
    uint32_t sequenceNumber() const noexcept { return header().sequenceNumber; }
};

} // namespace amon::layers
