/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief VLAN layer definitions.
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
 * @brief Structure representing a VLAN header.
 */
struct [[gnu::packed]] VLANHeader {
    types::UInt16BE tci; ///< Tag Control Information (Priority, CFI, VLAN ID)
    types::UInt16BE protocol; ///< Encapsulated Protocol (EtherType)
};

/**
 * @brief Layer view for VLAN packets.
 *
 * Provides access to VLAN header fields and payload.
 */
class VLANView : public LayerView<VLANHeader> {
public:
    /// Layer type identifier for VLAN.
    static const LayerType LAYER_TYPE = LayerType::VLAN;

    /**
     * @brief Create and validate a VLANView from a byte span.
     * @param data Span of bytes containing the VLAN header.
     * @return std::expected containing VLANView or LayerError.
     */
    static std::expected<VLANView, LayerError> create(std::span<const std::byte> data) noexcept
    {
        if (data.size() < sizeof(VLANHeader)) {
            return std::unexpected(LayerError::HEADER_SHORT);
        }
        return VLANView(data);
    }

    /**
     * @brief Get the length of the VLAN header.
     * @return Length of the VLAN header in bytes.
     */
    constexpr uint16_t
    headerLength() const noexcept // NOLINT(readability-convert-member-functions-to-static)
    {
        return sizeof(VLANHeader);
    }

    /**
     * @brief Get the payload of the VLAN packet.
     * @return Span of bytes representing the payload.
     */
    std::span<const std::byte> payload() const { return data.subspan(headerLength()); }

    /**
     * @brief Get the encapsulated protocol (EtherType) from the VLAN header.
     * @return Protocol value as uint16_t.
     */
    uint16_t protocol() const noexcept { return header().protocol; }

    /**
     * @brief Get the VLAN tag (VLAN ID) from the TCI field.
     * @return VLAN tag value (12 bits).
     */
    uint16_t tag() const noexcept { return header().tci & 0xFFF; }
};

} // namespace amon::layers
