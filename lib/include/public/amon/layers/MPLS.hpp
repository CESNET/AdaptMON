/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief MPLS layer definitions.
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
 * @brief Union representing an MPLS header.
 */
union MPLSHeader {
    types::UInt32BE label; ///< MPLS Label, Traffic Class, Bottom of Stack, TTL
    types::UInt8 bytes[4]; ///< Raw bytes for easier access to individual fields
};

/**
 * @brief Layer view for MPLS packets.
 *
 * Provides access to MPLS header fields and payload.
 */
class MPLSView : public LayerView<MPLSHeader> {
public:
    /// Layer type identifier for MPLS.
    static const LayerType LAYER_TYPE = LayerType::MPLS;

    /**
     * @brief Create and validate an MPLSView from a byte span.
     * @param data Span of bytes containing the MPLS header.
     * @return std::expected containing MPLSView or LayerError.
     */
    static std::expected<MPLSView, LayerError> create(std::span<const std::byte> data) noexcept;

    /**
     * @brief Get the length of the MPLS header.
     * @return Length of the MPLS header in bytes.
     */
    constexpr uint16_t
    headerLength() const noexcept // NOLINT(readability-convert-member-functions-to-static)
    {
        return sizeof(MPLSHeader);
    }

    /**
     * @brief Get the payload after MPLS header.
     * @return Span of bytes representing the payload.
     */
    std::span<const std::byte> payload() const
    {
        // Return the payload starting after the IPv6 Extension header
        return data.subspan(headerLength());
    }

    /**
     * @brief Get the MPLS label value.
     * @return MPLS label (20 bits).
     */
    uint32_t label() const noexcept { return header().label >> 12; }

    /**
     * @brief Check if this is the bottom of the MPLS stack.
     * @return True if the bottom-of-stack bit is set, false otherwise.
     */
    bool bottomOfStack() const noexcept { return (header().bytes[2] & 0x1) == 1; }
};

} // namespace amon::layers
