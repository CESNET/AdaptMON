/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief TCP layer definitions.
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
 * @brief Structure representing a TCP header.
 */
struct [[gnu::packed]] TCPHeader {
    UInt16BE sourcePort; ///< Source Port
    UInt16BE destinationPort; ///< Destination Port
    UInt32BE sequenceNumber; ///< Sequence Number
    UInt32BE acknowledgeNumber; ///< Acknowledge Number
    UInt8 dataOffsetReserved; ///< Data Offset and Reserved bits
    UInt8 flags; ///< Flags
    UInt16BE window; ///< Window Size
    UInt16BE checksum; ///< Checksum
    UInt16BE urgentPointer; ///< Urgent Pointer
};

/**
 * @brief Layer view for TCP segments.
 *
 * Provides access to TCP header fields and payload.
 */
class TCPView : public LayerView<TCPHeader> {
public:
    /// Layer type identifier for TCP.
    static const LayerType LAYER_TYPE = LayerType::TCP;

    /**
     * @brief Create and validate a TCPView from a byte span.
     * @param data Span of bytes containing the TCP segment.
     * @return std::expected containing TCPView or LayerError.
     */
    static std::expected<TCPView, LayerError> create(std::span<const std::byte> data) noexcept;

    /**
     * @brief Get the length of the TCP header.
     * @return Length of the TCP header in bytes.
     */
    uint16_t headerLength() const noexcept
    {
        // Calculate the header length from the data offset field
        const uint8_t dataOffset
            = header().dataOffsetReserved >> 4; // Data offset is the top 4 bits in BE
        return dataOffset * 4; // Data offset is in 32-bit words
    }

    /**
     * @brief Get the payload of the TCP segment.
     * @return Span of bytes representing the payload.
     */
    std::span<const std::byte> payload() const
    {
        // Return the payload starting after header
        return data.subspan(headerLength());
    }

    /**
     * @brief Get the source port from the TCP header.
     * @return Source port number.
     */
    uint16_t srcPort() const noexcept { return header().sourcePort; }

    /**
     * @brief Get the destination port from the TCP header.
     * @return Destination port number.
     */
    uint16_t dstPort() const noexcept { return header().destinationPort; }

    /**
     * @brief Get the sequence number from the TCP header.
     * @return Sequence number.
     */
    uint32_t sequenceNumber() const noexcept { return header().sequenceNumber; }

    /**
     * @brief Get the acknowledge number from the TCP header.
     * @return Acknowledge number.
     */
    uint32_t acknowledgeNumber() const noexcept { return header().acknowledgeNumber; }

    /**
     * @brief Get the window size from the TCP header.
     * @return Window size.
     */
    uint16_t window() const noexcept { return header().window; }

    /**
     * @brief Get the TCP flags.
     * @return Flags byte.
     */
    uint8_t flags() const noexcept { return header().flags; }

    /**
     * @brief Get the TCP options.
     * @return Span of bytes representing the TCP options.
     */
    std::span<const std::byte> options() const
    {
        return data.subspan(sizeof(TCPHeader), headerLength() - sizeof(TCPHeader));
    }
};

} // namespace amon::layers
