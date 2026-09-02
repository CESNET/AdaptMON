/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief UDP layer definitions.
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
 * @brief Structure representing a UDP header.
 */
struct [[gnu::packed]] UDPHeader {
    types::UInt16BE sourcePort; ///< Source Port
    types::UInt16BE destinationPort; ///< Destination Port
    types::UInt16BE length; ///< Length
    types::UInt16BE checksum; ///< Checksum
};

/**
 * @brief Layer view for UDP packets.
 *
 * Provides access to UDP header fields and payload.
 */
class UDPView : public LayerView<UDPHeader> {
public:
    /// Layer type identifier for UDP.
    static const LayerType LAYER_TYPE = LayerType::UDP;

    /**
     * @brief Create and validate a UDPView from a byte span.
     * @param data Span of bytes containing the UDP packet.
     * @return std::expected containing UDPView or LayerError.
     */
    static std::expected<UDPView, LayerError> create(std::span<const std::byte> data) noexcept;

    /**
     * @brief Get the length of the UDP header.
     * @return Length of the UDP header in bytes.
     */
    constexpr uint16_t
    headerLength() const noexcept // NOLINT(readability-convert-member-functions-to-static)
    {
        return sizeof(UDPHeader);
    }

    /**
     * @brief Get the payload of the UDP packet.
     * @return Span of bytes representing the payload.
     */
    std::span<const std::byte> payload() const { return data.subspan(headerLength()); }

    /**
     * @brief Get the source port from the UDP header.
     * @return Source port number.
     */
    uint16_t srcPort() const noexcept { return header().sourcePort; }

    /**
     * @brief Get the destination port from the UDP header.
     * @return Destination port number.
     */
    uint16_t dstPort() const noexcept { return header().destinationPort; }
};

} // namespace amon::layers
