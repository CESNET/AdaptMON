/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief GRE layer definitions.
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
 * @brief Structure representing an GRE header.
 */
struct [[gnu::packed]] GREHeader {
    types::UInt16BE flagsVersion; ///< Version dependent flags and GRE version
    types::UInt16BE protocolType; ///< Protocol Type
};

/**
 * @brief Layer view for GRE packets. Provides access to GRE header fields and payload.
 *
 * Standard (RFC 2784) and extension (RFC 2890) header (GRE ver. 0) is supported. Routing
 * information from original GRE header (RFC 1701) is NOT supported.
 * PPTP GRE (GRE ver. 1, RFC 2637) is supported.
 */
class GREView : public LayerView<GREHeader> {
public:
    /// Layer type identifier for GRE.
    static const LayerType LAYER_TYPE = LayerType::GRE;

    /**
     * @brief Create and validate an GREView from a byte span.
     * @param data Span of bytes containing the GRE packet.
     * @return std::expected containing GREView or LayerError.
     */
    static std::expected<GREView, LayerError> create(std::span<const std::byte> data) noexcept;

    /**
     * @brief Get the length of the GRE header.
     * @return Length of the GRE header in bytes.
     */
    uint16_t headerLength() const noexcept
    {
        uint16_t size = sizeof(GREHeader);
        // Checksum bit
        if ((header().flagsVersion & 0x8000) != 0) {
            size += 4;
        }
        // Key bit
        if ((header().flagsVersion & 0x2000) != 0) {
            size += 4;
        }
        // Sequence number bit
        if ((header().flagsVersion & 0x1000) != 0) {
            size += 4;
        }
        // Acknowledgment number bit, only for PPTP GRE (GRE ver. 1)
        if (version() == 1 && (header().flagsVersion & 0x0080) != 0) {
            size += 4;
        }

        return size;
    }

    /**
     * @brief Get the payload of the GRE packet.
     * @return Span of bytes representing the payload.
     */
    std::span<const std::byte> payload() const { return data.subspan(headerLength()); }

    /**
     * @brief Get the protocol type.
     * @return Protocol type value.
     */
    uint16_t protocolType() const noexcept { return header().protocolType; }

    /**
     * @brief Get the GRE version.
     * @return GRE version (0 or 1).
     */
    uint8_t version() const noexcept { return header().flagsVersion & 0x0007; }
};

} // namespace amon::layers
