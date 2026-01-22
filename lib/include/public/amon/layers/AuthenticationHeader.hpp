/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief Authentication Header layer definitions.
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

using namespace types;

/**
 * @brief Structure representing an IPsec Authentication Header.
 */
struct [[gnu::packed]] AuthenticationHeader {
    UInt8 nextHeader; ///< Next Header
    UInt8 length; ///< Length in 4-octets units, not including the first 2 units
    UInt16BE reserved; ///< Reserved
    UInt32BE spi; ///< Security Parameters Index
    UInt32BE sequenceNumber; ///< Sequence Number
    // variable length Integrity Check Value
};

/**
 * @brief Layer view for IPsec Authentication Header.
 */
class AuthenticationHeaderView : public LayerView<AuthenticationHeader> {
public:
    /// Layer type identifier.
    static const LayerType LAYER_TYPE = LayerType::AUTHENTICATION_HEADER;

    /**
     * @brief Create and validate an AuthenticationHeaderView from a byte span.
     * @param data Span of bytes containing the Authentication header.
     * @return std::expected containing AuthenticationHeaderView or LayerError.
     */
    static std::expected<AuthenticationHeaderView, LayerError>
    create(std::span<const std::byte> data) noexcept;

    /**
     * @brief Get the length of the Authentication header.
     *
     * Length field in header is in 4-octets units, not including the first 2 units.
     *
     * @return Length of the Authentication header in bytes.
     */
    uint16_t headerLength() const noexcept
    {
        return static_cast<uint16_t>(header().length + 2) * 4;
    }

    /**
     * @brief Get the payload continuing after Authentication header.
     * @return Span of bytes representing the payload.
     */
    std::span<const std::byte> payload() const { return data.subspan(headerLength()); }

    /**
     * @brief Get the next header field from the Authentication header.
     * @return Next header identifier.
     */
    uint8_t nextHeader() const noexcept { return header().nextHeader; }

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

    /**
     * @brief Get the Integrity Check Value (ICV).
     * @return Span of bytes representing the ICV.
     */
    std::span<const std::byte> integrityCheckValue() const
    {
        return data.subspan(
            sizeof(AuthenticationHeader),
            headerLength() - sizeof(AuthenticationHeader));
    }
};

} // namespace amon::layers
