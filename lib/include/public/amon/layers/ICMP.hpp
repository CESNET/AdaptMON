/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief ICMP layer definitions.
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
 * @brief Structure representing an ICMP header.
 */
struct [[gnu::packed]] ICMPHeader {
    types::UInt8 type; ///< Type
    types::UInt8 code; ///< Code
    types::UInt16BE checksum; ///< Checksum
};

/**
 * @brief Layer view for ICMP packets.
 *
 * Provides access to ICMP header fields and payload.
 */
class ICMPView : public LayerView<ICMPHeader> {
public:
    /// Layer type identifier for ICMP.
    static const LayerType LAYER_TYPE = LayerType::ICMP;

    /**
     * @brief Create and validate an ICMPView from a byte span.
     * @param data Span of bytes containing the ICMP packet.
     * @return std::expected containing ICMPView or LayerError.
     */
    static std::expected<ICMPView, LayerError> create(std::span<const std::byte> data) noexcept;

    /**
     * @brief Get the length of the ICMP header.
     * @return Length of the ICMP header in bytes.
     */
    constexpr uint16_t
    headerLength() const noexcept // NOLINT(readability-convert-member-functions-to-static)
    {
        return sizeof(ICMPHeader);
    }

    /**
     * @brief Get the data of the ICMP packet (after ICMP header). The length of the data is not
     * validated to see if it matches the combination of type and code.
     * @return Span of bytes representing ICMP data.
     */
    std::span<const std::byte> payload() const { return data.subspan(headerLength()); }

    /**
     * @brief Get the ICMP type.
     * @return Type value.
     */
    uint8_t type() const noexcept { return header().type; }

    /**
     * @brief Get the ICMP code.
     * @return Code value.
     */
    uint8_t code() const noexcept { return header().code; }
};

} // namespace amon::layers
