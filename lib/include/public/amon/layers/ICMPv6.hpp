/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief ICMPv6 layer definitions.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/layers/Layer.hpp>
#include <amon/layers/LayerView.hpp>
#include <amon/types/BigEndian.hpp>

#include <expected>
#include <span>

namespace amon::layers {

/**
 * @brief Structure representing an ICMPv6 header.
 */
struct [[gnu::packed]] ICMPv6Header {
    types::UInt8 type; ///< Type
    types::UInt8 code; ///< Code
    types::UInt16BE checksum; ///< Checksum
};

/**
 * @brief Layer view for ICMPv6 packets.
 *
 * Provides access to ICMPv6 header fields and payload.
 */
class ICMPv6View : public LayerView<ICMPv6Header> {
public:
    /// Layer type identifier for ICMPv6.
    static const LayerType LAYER_TYPE = LayerType::ICMPV6;

    /**
     * @brief Create and validate an ICMPv6View from a byte span.
     * @param data Span of bytes containing the ICMPv6 packet.
     * @return std::expected containing ICMPv6View or LayerError.
     */
    static std::expected<ICMPv6View, LayerError> create(std::span<const std::byte> data) noexcept;

    /**
     * @brief Get the length of the ICMPv6 header.
     * @return Length of the ICMPv6 header in bytes.
     */
    constexpr uint16_t
    headerLength() const noexcept // NOLINT(readability-convert-member-functions-to-static)
    {
        return sizeof(ICMPv6Header);
    }

    /**
     * @brief Get the data of the ICMPv6 packet (after ICMPv6 header). The length of the data is not
     * validated to see if it matches the combination of type and code.
     * @return Span of bytes representing ICMPv6 data.
     */
    std::span<const std::byte> payload() const { return data.subspan(headerLength()); }

    /**
     * @brief Get the ICMPv6 type.
     * @return Type value.
     */
    uint8_t type() const noexcept { return header().type; }

    /**
     * @brief Get the ICMPv6 code.
     * @return Code value.
     */
    uint8_t code() const noexcept { return header().code; }
};

} // namespace amon::layers
