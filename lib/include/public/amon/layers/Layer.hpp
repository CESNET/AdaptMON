/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief Layer type and error definitions.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/util/Formatter.hpp>

#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

namespace amon::layers {

static constexpr uint16_t LAYERTYPE_LINK = 0x1000; ///< Link layers (e.g. first layer in pcap)
static constexpr uint16_t LAYERTYPE_L2 = 0x2000; ///< Layer 2 type base
static constexpr uint16_t LAYERTYPE_L3 = 0x3000; ///< Layer 3 type base
static constexpr uint16_t LAYERTYPE_L4 = 0x4000; ///< Layer 4 type base

/// Bit flag that marks "key" layer, typically the first and most important layer of a given layer
/// group (L2, L3, L4) in a packet. The flag is used by the PacketParser which notes a key layer
/// into the PacketLayout. Non-key layers are skipped in the PacketLayout.
static constexpr uint16_t LAYERTYPE_KEY = 0x8000;

/**
 * @brief Enumeration of supported layer types.
 */
enum class LayerType : uint16_t {
    ERROR = 0x00,
    UNKNOWN = 0x01,
    PAYLOAD_APP = LAYERTYPE_KEY | 0x02,
    PAYLOAD_IP_FRAGMENT = 0x03,

    RAW = LAYERTYPE_LINK | 0x01, // DLT_RAW (packet starts with IPv4 or IPv6)

    ETHERNET = LAYERTYPE_KEY | LAYERTYPE_L2 | 0x01,
    MPLS = LAYERTYPE_L2 | 0x02,
    VLAN = LAYERTYPE_L2 | 0x03,

    IPV4 = LAYERTYPE_KEY | LAYERTYPE_L3 | 0x01,
    IPV6 = LAYERTYPE_KEY | LAYERTYPE_L3 | 0x02,
    // generic extensions that have the same structure of header, use for extensions that
    // have not own type in this enum
    IPV6_OPT_GENERIC = LAYERTYPE_L3 | 0x03,
    IPV6_OPT_FRAGMENT = LAYERTYPE_L3 | 0x04,
    IPV6_OPT_HOPBYHOP = LAYERTYPE_L3 | 0x05,
    IPV6_OPT_ROUTING = LAYERTYPE_L3 | 0x06,
    AUTHENTICATION_HEADER = LAYERTYPE_L3 | 0x07,
    IPV6_OPT_DESTINATION_OPTIONS = LAYERTYPE_L3 | 0x08,

    UDP = LAYERTYPE_KEY | LAYERTYPE_L4 | 0x01,
    TCP = LAYERTYPE_KEY | LAYERTYPE_L4 | 0x02,
    GRE = LAYERTYPE_KEY | LAYERTYPE_L4 | 0x03,
    ICMP = LAYERTYPE_KEY | LAYERTYPE_L4 | 0x04,
    ICMPV6 = LAYERTYPE_KEY | LAYERTYPE_L4 | 0x05,
    ESP = LAYERTYPE_KEY | LAYERTYPE_L4 | 0x06,
};

/**
 * @brief Convert LayerType to a human-readable string.
 * @param type LayerType value.
 * @return String representation of the layer type.
 */
constexpr std::string_view layerTypeToString(LayerType type)
{
    switch (type) {
    case LayerType::ERROR:
        return "ERROR";
    case LayerType::UNKNOWN:
        return "UNKNOWN";
    case LayerType::PAYLOAD_APP:
        return "PAYLOAD_APP";
    case LayerType::PAYLOAD_IP_FRAGMENT:
        return "PAYLOAD_IP_FRAGMENT";
    case LayerType::RAW:
        return "RAW";
    case LayerType::ETHERNET:
        return "ETHERNET";
    case LayerType::IPV4:
        return "IPV4";
    case LayerType::IPV6:
        return "IPV6";
    case LayerType::IPV6_OPT_GENERIC:
        return "IPV6_OPT_GENERIC";
    case LayerType::IPV6_OPT_FRAGMENT:
        return "IPV6_OPT_FRAGMENT";
    case LayerType::IPV6_OPT_HOPBYHOP:
        return "IPV6_OPT_HOPBYHOP";
    case LayerType::IPV6_OPT_ROUTING:
        return "IPV6_OPT_ROUTING";
    case LayerType::AUTHENTICATION_HEADER:
        return "AUTHENTICATION_HEADER";
    case LayerType::ESP:
        return "ESP";
    case LayerType::IPV6_OPT_DESTINATION_OPTIONS:
        return "IPV6_OPT_DESTINATION_OPTIONS";
    case LayerType::ICMP:
        return "ICMP";
    case LayerType::ICMPV6:
        return "ICMPV6";
    case LayerType::UDP:
        return "UDP";
    case LayerType::TCP:
        return "TCP";
    case LayerType::MPLS:
        return "MPLS";
    case LayerType::VLAN:
        return "VLAN";
    case LayerType::GRE:
        return "GRE";
    }

    throw std::logic_error("Invalid LayerType to convert.");
}

/**
 * @brief Get the base type of a LayerType – first byte (e.g. LAYERTYPE_L2, LAYERTYPE_L3, ...).
 * @param type LayerType value.
 * @return Base type of the layer.
 */
constexpr uint16_t layerTypeBase(LayerType type) noexcept
{
    static_assert(
        std::is_same_v<uint16_t, decltype(std::to_underlying(type))>,
        "LayerType must be uint16_t.");
    constexpr uint16_t mask = LAYERTYPE_LINK | LAYERTYPE_L2 | LAYERTYPE_L3 | LAYERTYPE_L4;
    return static_cast<uint16_t>(type) & mask;
}

/**
 * @brief Check if LayerType has the "key" bit set which indicates it should be noted in
 * PacketLayout.
 * @param type LayerType value.
 * @return True if the layer type is a key layer, false otherwise.
 */
constexpr bool layerTypeIsKey(LayerType type) noexcept
{
    static_assert(
        std::is_same_v<uint16_t, decltype(std::to_underlying(type))>,
        "LayerType must be uint16_t.");
    return (static_cast<uint16_t>(type) & LAYERTYPE_KEY) != 0;
}

/**
 * @brief Enumeration of possible layer errors.
 */
enum class LayerError : uint16_t {
    NONE, // Layer is valid
    LAYER_VECTOR_FULL,

    // Global errors
    HEADER_SHORT,
    BAD_PAYLOAD_LEN,
    BAD_MAGIC,
    BAD_LEN_FIELD,
    UNSUPPORTED,

    // Ethernet
    BAD_ENCAPSULATION,
    BAD_ETHERTYPE,

};

/**
 * @brief Convert LayerError to a human-readable string.
 * @param err LayerError value.
 * @return String representation of the error.
 */
constexpr std::string_view layerErrorToString(LayerError err)
{
    switch (err) {
    case LayerError::NONE:
        return "NONE";
    case LayerError::LAYER_VECTOR_FULL:
        return "LAYER_VECTOR_FULL";
    case LayerError::HEADER_SHORT:
        return "HEADER_SHORT";
    case LayerError::BAD_PAYLOAD_LEN:
        return "BAD_PAYLOAD_LEN";
    case LayerError::BAD_MAGIC:
        return "BAD_MAGIC";
    case LayerError::BAD_LEN_FIELD:
        return "BAD_LEN_FIELD";
    case LayerError::UNSUPPORTED:
        return "UNSUPPORTED";
    case LayerError::BAD_ENCAPSULATION:
        return "BAD_ENCAPSULATION";
    case LayerError::BAD_ETHERTYPE:
        return "BAD_ETHERTYPE";
    }

    throw std::logic_error("Invalid LayerError to convert.");
}

/**
 * @brief Format LayerType for output.
 * @param type LayerType to format.
 * @param fmt Formatter to write to.
 */
inline void format(LayerType type, amon::util::Formatter& fmt)
{
    fmt.format("{}", layerTypeToString(type));
}

/**
 * @brief Format LayerError for output.
 * @param err LayerError to format.
 * @param fmt Formatter to write to.
 */
inline void format(LayerError err, amon::util::Formatter& fmt)
{
    fmt.format("{}", layerErrorToString(err));
}

} // namespace amon::layers
