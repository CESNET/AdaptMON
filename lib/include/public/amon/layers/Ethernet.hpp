/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief Ethernet layer definitions.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/layers/Layer.hpp>
#include <amon/layers/LayerView.hpp>
#include <amon/types/MACAddress.hpp>
#include <amon/types/UIntBE.hpp>

#include <expected>
#include <span>

namespace amon::layers {

/**
 * @brief Structure representing an Ethernet header.
 */
struct [[gnu::packed]] EthernetHeader {
    types::MACAddress destination; ///< Destination MAC address
    types::MACAddress source; ///< Source MAC address
    types::UInt16BE protocol; ///< EtherType protocol field
};

/**
 * @brief Layer view for Ethernet frames.
 *
 * Provides access to Ethernet header fields and payload.
 */
class EthernetView : public LayerView<EthernetHeader> {
public:
    /// Layer type identifier for Ethernet.
    static const LayerType LAYER_TYPE = LayerType::ETHERNET;

    /**
     * @brief Create and validate an EthernetView from a byte span.
     * @param data Span of bytes containing the Ethernet frame.
     * @return std::expected containing EthernetView or LayerError.
     */
    static std::expected<EthernetView, LayerError> create(std::span<const std::byte> data) noexcept
    {
        if (data.size() < sizeof(EthernetHeader)) {
            return std::unexpected(LayerError::HEADER_SHORT);
        }
        return EthernetView(data);
    }

    /**
     * @brief Get the length of the Ethernet header.
     * @return Length of the Ethernet header in bytes.
     */
    constexpr uint16_t
    headerLength() const noexcept // NOLINT(readability-convert-member-functions-to-static)
    {
        return sizeof(EthernetHeader);
    }

    /**
     * @brief Get the payload of the Ethernet frame.
     * @return Span of bytes representing the payload.
     */
    std::span<const std::byte> payload() const { return data.subspan(headerLength()); }

    /**
     * @brief Get the EtherType protocol field.
     * @return EtherType protocol field value.
     */
    uint16_t protocol() const noexcept { return header().protocol; }

    /**
     * @brief Get the destination MAC address.
     * @return Destination MAC address.
     */
    const types::MACAddress& dst() const noexcept { return header().destination; }

    /**
     * @brief Get the source MAC address.
     * @return Source MAC address.
     */
    const types::MACAddress& src() const noexcept { return header().source; }
};

} // namespace amon::layers
