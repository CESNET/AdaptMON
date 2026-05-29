/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief IPv4 layer definitions.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/layers/Layer.hpp>
#include <amon/layers/LayerView.hpp>
#include <amon/types/IPAddress.hpp>
#include <amon/types/UIntBE.hpp>

#include <cstdint>
#include <expected>
#include <span>

namespace amon::layers {

/**
 * @brief Structure representing an IPv4 header.
 */
struct [[gnu::packed]] IPv4Header {
    types::UInt8 versionIhl; ///< Version and Internet Header Length
    types::UInt8 tos; ///< Type of Service
    types::UInt16BE totalLength; ///< Total Length
    types::UInt16BE id; ///< Identification
    types::UInt16BE flagsFragmentOffset; ///< Flags and Fragment Offset
    types::UInt8 ttl; ///< Time to Live
    types::UInt8 protocol; ///< Protocol
    types::UInt16BE headerChecksum; ///< Header Checksum
    types::IPv4 sourceAddress; ///< Source Address
    types::IPv4 destinationAddress; ///< Destination Address
};

/**
 * @brief Layer view for IPv4 packets.
 *
 * Provides access to IPv4 header fields and payload.
 */
class IPv4View : public LayerView<IPv4Header> {
public:
    /// Layer type identifier for IPv4.
    static const LayerType LAYER_TYPE = LayerType::IPV4;

    /**
     * @brief Create and validate an IPv4View from a byte span.
     * @param data Span of bytes containing the IPv4 packet.
     * @return std::expected containing IPv4View or LayerError.
     */
    static std::expected<IPv4View, LayerError> create(std::span<const std::byte> data) noexcept
    {
        std::expected<IPv4View, LayerError> res {std::unexpected(LayerError::NONE)};

        if (data.size() < sizeof(IPv4Header)) {
            std::expected<IPv4View, LayerError> err {std::unexpected(LayerError::HEADER_SHORT)};
            res.swap(err);
            return res;
        }

        res.emplace(data);
        if ((res->header().versionIhl >> 4) != 4) {
            std::expected<IPv4View, LayerError> err {std::unexpected(LayerError::BAD_MAGIC)};
            res.swap(err);
            return res;
        }
        if (res->headerLength() < sizeof(IPv4Header)) {
            std::expected<IPv4View, LayerError> err {std::unexpected(LayerError::BAD_LEN_FIELD)};
            res.swap(err);
            return res;
        }
        if (data.size() < res->headerLength()) {
            std::expected<IPv4View, LayerError> err {std::unexpected(LayerError::HEADER_SHORT)};
            res.swap(err);
            return res;
        }
        // Check disabled due to use-cases with trimmed packets.
        // if (data.size() < view.header().totalLength) {
        //     return std::unexpected(LayerError::BAD_PAYLOAD_LEN);
        // }
        // Check disabled because of tolerance of packets with set reserved bit.
        // if ((view.header().flagsFragmentOffset >> 15) != 0) {
        //     return std::unexpected(LayerError::BAD_MAGIC);
        // }
        return res;
    }

    /**
     * @brief Get the length of the IPv4 header.
     * @return Length of the IPv4 header in bytes.
     */
    uint16_t headerLength() const noexcept
    {
        // Calculate the header length from the ihl field
        const uint16_t ihl = header().versionIhl & 0x0F; // IHL is the top 4 bits in BE
        return ihl * 4; // IHL is in 32-bit words
    }

    /**
     * @brief Get the payload of the IPv4 packet.
     * @return Span of bytes representing the payload.
     */
    std::span<const std::byte> payload() const
    {
        // Return the payload starting after the IPv4 header
        return data.subspan(headerLength());
    }

    /**
     * @brief Get the protocol field from the IPv4 header.
     * @return Protocol identifier.
     */
    uint8_t protocol() const noexcept { return header().protocol; }

    /**
     * @brief Get the source IP address.
     * @return Source IP address.
     */
    const types::IPv4& srcIP() const noexcept { return header().sourceAddress; }

    /**
     * @brief Get the destination IP address.
     * @return Destination IP address.
     */
    const types::IPv4& dstIP() const noexcept { return header().destinationAddress; }

    /**
     * @brief Check if the packet is fragmented.
     * @return True if the packet is fragmented, false otherwise.
     */
    bool isFragmented() const noexcept
    {
        return (header().flagsFragmentOffset & 0x1FFF) > 0 // non-zero offset
            || (header().flagsFragmentOffset & 0x2000) != 0; // more-fragments flag set
    }

    /**
     * @brief Get the fragment offset.
     * @return Fragment offset value in 8 bytes units.
     */
    uint32_t fragmentOffset() const noexcept { return header().flagsFragmentOffset & 0x1FFF; }

    /**
     * @brief Check if more fragments follow this fragment.
     * @return True if more fragments follow, false otherwise.
     */
    bool moreFragmentsFollow() const noexcept
    {
        return (header().flagsFragmentOffset & 0x2000) != 0;
    }

    /**
     * @brief Get the Time to Live (TTL) field.
     * @return TTL value.
     */
    uint8_t ttl() const noexcept { return header().ttl; }

    /**
     * @brief Get the total length of the IPv4 packet.
     * @return Total length in bytes.
     */
    uint16_t totalLength() const noexcept { return header().totalLength; }

    /**
     * @brief Get the IP flags.
     * @return IP flags value in 3 least significant bits of byte.
     */
    uint8_t ipFlags() const noexcept
    {
        return static_cast<uint8_t>((header().flagsFragmentOffset & 0xE000) >> 13);
    }
};

} // namespace amon::layers
