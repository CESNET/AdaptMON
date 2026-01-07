/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief IPv6 layer definitions.
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

using namespace types;

/**
 * @brief Structure representing an IPv6 header.
 */
struct [[gnu::packed]] IPv6Header {
    UInt32BE versionTrafficClassFlowLabel; ///< Version, Traffic Class, Flow Label
    UInt16BE payloadLength; ///< Payload Length
    UInt8 nextHeader; ///< Next Header
    UInt8 hopLimit; ///< Hop Limit
    IPv6 sourceAddress; ///< Source Address
    IPv6 destinationAddress; ///< Destination Address
};

/**
 * @brief Layer view for IPv6 packets.
 *
 * Provides access to IPv6 header fields and payload.
 */
class IPv6View : public LayerView<IPv6Header> {
public:
    /// Layer type identifier for IPv6.
    static const LayerType LAYER_TYPE = LayerType::IPV6;

    /**
     * @brief Create and validate an IPv6View from a byte span.
     * @param data Span of bytes containing the IPv6 packet.
     * @return std::expected containing IPv6View or LayerError.
     */
    static std::expected<IPv6View, LayerError> create(std::span<const std::byte> data);

    /**
     * @brief Get the length of the IPv6 header.
     * @return Length of the IPv6 header in bytes.
     */
    constexpr uint16_t
    headerLength() const noexcept // NOLINT(readability-convert-member-functions-to-static)
    {
        return sizeof(IPv6Header);
    }

    /**
     * @brief Get the payload of the IPv6 packet.
     * @return Span of bytes representing the payload.
     */
    std::span<const std::byte> payload() const
    {
        // Return the payload starting after the IPv6 header
        return data.subspan(headerLength());
    }

    /**
     * @brief Get the next header field from the IPv6 header.
     * @return Next header identifier.
     */
    uint8_t nextHeader() const { return header().nextHeader; }

    /**
     * @brief Get the source IPv6 address.
     * @return Source IPv6 address.
     */
    const IPv6& srcIP() const { return header().sourceAddress; }

    /**
     * @brief Get the destination IPv6 address.
     * @return Destination IPv6 address.
     */
    const IPv6& dstIP() const { return header().destinationAddress; }

    /**
     * @brief Get the payload length from the IPv6 header.
     * @return Payload length in bytes.
     */
    uint16_t payloadLength() const { return header().payloadLength; }

    /**
     * @brief Get the hop limit from the IPv6 header.
     * @return Hop limit value.
     */
    uint8_t hopLimit() const { return header().hopLimit; }

private:
    /**
     * @brief Construct an IPv6View from a byte span.
     * @param data Span of bytes containing the IPv6 packet.
     */
    explicit IPv6View(std::span<const std::byte> data)
        : LayerView<IPv6Header>(data)
    {
    }
};

/**
 * @brief Structure representing an IPv6 extension header.
 *
 * Used for IPv6 extension headers such as Hop-by-Hop, Routing, etc.
 */
struct [[gnu::packed]] IPv6ExtensionHeader {
    UInt8 nextHeader; ///< Next Header
    UInt8 length; ///< Length in 8-byte units, not including the first 8 bytes
    // Followed by options...
};

/**
 * @brief Layer view base class for IPv6 extension headers.
 *
 * This class should be used for deriving IPv6 extensions.
 * Provides access to extension header fields, options and payload.
 */
template <typename Derived, LayerType TypeValue>
class IPv6ExtensionViewBase : public LayerView<IPv6ExtensionHeader> {
public:
    /// Layer type identifier for IPv6 extension headers.
    static const LayerType LAYER_TYPE = TypeValue;

    /**
     * @brief Create and validate an IPv6ExtensionView from a byte span.
     * @param data Span of bytes containing the IPv6 extension header.
     * @return std::expected containing IPv6ExtensionView or LayerError.
     */
    static std::expected<Derived, LayerError> create(std::span<const std::byte> data) noexcept
    {
        if (data.size() < sizeof(IPv6ExtensionHeader)) {
            return std::unexpected(LayerError::HEADER_SHORT);
        }
        auto view = Derived(data);
        if (data.size() < view.headerLength()) {
            return std::unexpected(LayerError::BAD_LEN_FIELD);
        }
        return view;
    };

    /**
     * @brief Get the length of the IPv6 extension header.
     *
     * Length field in extension header is in 8-octets units, not including the first 8 octets.
     *
     * @return Length of the extension header in bytes.
     */
    uint16_t headerLength() const noexcept
    {
        return static_cast<uint16_t>(header().length * 8) + 8;
    }

    /**
     * @brief Get the payload after IPv6 extension (options are included in the header).
     * @return Span of bytes representing the payload.
     */
    std::span<const std::byte> payload() const
    {
        // Return the payload starting after the IPv6 Extension header
        return data.subspan(headerLength());
    }

    /**
     * @brief Get the next header field from the extension header.
     * @return Next header identifier.
     */
    uint8_t nextHeader() const noexcept { return header().nextHeader; }

    /**
     * @brief Get the options in the IPv6 extension header.
     * @return Span of bytes representing the options.
     */
    std::span<const std::byte> options() const
    {
        return data.subspan(
            sizeof(IPv6ExtensionHeader),
            headerLength() - sizeof(IPv6ExtensionHeader));
    }

private:
    explicit IPv6ExtensionViewBase(std::span<const std::byte> data) noexcept
        : LayerView<IPv6ExtensionHeader>(data)
    {
    }
    friend Derived;
};

/**
 * @brief Layer view for IPv6 generic extension.
 *
 * This class should only be used for IPv6 extensions that are not defined below.
 */
class IPv6ExtensionView
    : public IPv6ExtensionViewBase<IPv6ExtensionView, LayerType::IPV6_EXTENSION> {
    using IPv6ExtensionViewBase::IPv6ExtensionViewBase; // inherit constructor
};

/**
 * @brief Layer view for IPv6 Hop-by-Hop extension headers.
 */
class IPv6HopByHopView : public IPv6ExtensionViewBase<IPv6HopByHopView, LayerType::IPV6_HOPBYHOP> {
    using IPv6ExtensionViewBase::IPv6ExtensionViewBase; // inherit constructor
};

/**
 * @brief Layer view for IPv6 Routing extension headers.
 */
class IPv6RoutingView : public IPv6ExtensionViewBase<IPv6RoutingView, LayerType::IPV6_ROUTING> {
    using IPv6ExtensionViewBase::IPv6ExtensionViewBase; // inherit constructor
};

/**
 * @brief Layer view for IPv6 Authentication Header extension headers.
 */
class IPv6AHView : public IPv6ExtensionViewBase<IPv6AHView, LayerType::IPV6_AH> {
    using IPv6ExtensionViewBase::IPv6ExtensionViewBase; // inherit constructor
};

/**
 * @brief Layer view for IPv6 ESP extension headers.
 */
class IPv6ESPView : public IPv6ExtensionViewBase<IPv6ESPView, LayerType::IPV6_ESP> {
    using IPv6ExtensionViewBase::IPv6ExtensionViewBase; // inherit constructor
};

/**
 * @brief Layer view for IPv6 Destination Options extension headers.
 */
class IPv6DestinationOptionsView
    : public IPv6ExtensionViewBase<
          IPv6DestinationOptionsView,
          LayerType::IPV6_DESTINATION_OPTIONS> {
    using IPv6ExtensionViewBase::IPv6ExtensionViewBase; // inherit constructor
};

/**
 * @brief Structure representing an IPv6 fragment header.
 */
struct [[gnu::packed]] IPv6FragmentHeader {
    UInt8 nextHeader; ///< Next Header
    UInt8 reserved; ///< Reserved
    UInt16BE fragmentOffsetAndFlags; ///< Fragment Offset and Flags
    UInt32BE identification; ///< Identification
};

/**
 * @brief Layer view for IPv6 fragment headers.
 *
 * Provides access to fragment header fields and payload.
 */
class IPv6FragmentView : public LayerView<IPv6FragmentHeader> {
public:
    /// Layer type identifier for IPv6 fragment headers.
    static const LayerType LAYER_TYPE = LayerType::IPV6_FRAGMENT;

    /**
     * @brief Create and validate an IPv6FragmentView from a byte span.
     * @param data Span of bytes containing the IPv6 fragment header.
     * @return std::expected containing IPv6FragmentView or LayerError.
     */
    static std::expected<IPv6FragmentView, LayerError>
    create(std::span<const std::byte> data) noexcept;

    /**
     * @brief Get the length of the IPv6 fragment header.
     * @return Length of the fragment header in bytes.
     */
    constexpr uint16_t
    headerLength() const noexcept // NOLINT(readability-convert-member-functions-to-static)
    {
        return sizeof(IPv6FragmentHeader);
    }

    /**
     * @brief Get the payload of the IPv6 fragment header.
     * @return Span of bytes representing the payload.
     */
    std::span<const std::byte> payload() const
    {
        // Return the payload starting after the IPv6 Fragment header
        return data.subspan(headerLength());
    }

    /**
     * @brief Get the next header field from the fragment header.
     * @return Next header identifier.
     */
    uint8_t nextHeader() const noexcept { return header().nextHeader; }

    /**
     * @brief Get the fragment offset.
     * @return Fragment offset value in 8 bytes units.
     */
    uint32_t fragmentOffset() const noexcept { return header().fragmentOffsetAndFlags >> 3; }

    /**
     * @brief Check if more fragments follow this fragment.
     * @return True if more fragments follow, false otherwise.
     */
    bool moreFragmentsFollow() const noexcept
    {
        return (header().fragmentOffsetAndFlags & 0x0001) == 1;
    }
};

} // namespace amon::layers
