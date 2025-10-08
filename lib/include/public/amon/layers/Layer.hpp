/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief Layer type and error definitions.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <cstdint>
#include <format>
#include <stdexcept>
#include <string_view>

namespace amon::layers {

static constexpr uint16_t LAYERTYPE_L2 = 0x2000; ///< Layer 2 type base
static constexpr uint16_t LAYERTYPE_L3 = 0x3000; ///< Layer 3 type base
static constexpr uint16_t LAYERTYPE_L4 = 0x4000; ///< Layer 4 type base

/**
 * @brief Enumeration of supported layer types.
 */
enum class LayerType : uint16_t {
    ERROR = 0x00,
    UNKNOWN = 0x01,
    PAYLOAD = 0x02,
    ETHERNET = LAYERTYPE_L2 | 0x01,
    MPLS = LAYERTYPE_L2 | 0x02,
    VLAN = LAYERTYPE_L2 | 0x03,

    IPV4 = LAYERTYPE_L3 | 0x01,
    IPV6 = LAYERTYPE_L3 | 0x02,
    IPV6_EXTENSION = LAYERTYPE_L3 | 0x03,
    IPV6_FRAGMENT = LAYERTYPE_L3 | 0x04,
    IPV6_HOPBYHOP = LAYERTYPE_L3 | 0x05,
    IPV6_ROUTING = LAYERTYPE_L3 | 0x06,
    IPV6_AH = LAYERTYPE_L3 | 0x07,
    IPV6_ESP = LAYERTYPE_L3 | 0x08,
    IPV6_DESTINATION_OPTIONS = LAYERTYPE_L3 | 0x09,
    ICMP = LAYERTYPE_L3 | 0x0A,
    ICMPV6 = LAYERTYPE_L3 | 0x0B,

    UDP = LAYERTYPE_L4 | 0x01,
    TCP = LAYERTYPE_L4 | 0x02,
    GRE = LAYERTYPE_L4 | 0x03,
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
    case LayerType::PAYLOAD:
        return "PAYLOAD";
    case LayerType::ETHERNET:
        return "ETHERNET";
    case LayerType::IPV4:
        return "IPV4";
    case LayerType::IPV6:
        return "IPV6";
    case LayerType::IPV6_EXTENSION:
        return "IPV6_EXTENSION";
    case LayerType::IPV6_FRAGMENT:
        return "IPV6_FRAGMENT";
    case LayerType::IPV6_HOPBYHOP:
        return "IPV6_HOPBYHOP";
    case LayerType::IPV6_ROUTING:
        return "IPV6_ROUTING";
    case LayerType::IPV6_AH:
        return "IPV6_AH";
    case LayerType::IPV6_ESP:
        return "IPV6_ESP";
    case LayerType::IPV6_DESTINATION_OPTIONS:
        return "IPV6_DESTINATION_OPTIONS";
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
 * @brief Enumeration of possible layer errors.
 */
enum class LayerError : uint16_t {
    NONE, // Layer is valid

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

} // namespace amon::layers

// (suppress inconsistency doxygen error)
/// @cond
/**
 * @brief std::formatter specialization for amon::layers::LayerType.
 */
template <>
struct std::formatter<amon::layers::LayerType> : std::formatter<std::string_view> {
    /**
     * @brief Format LayerType for output.
     * @tparam FormatContext Type of the format context.
     * @param type LayerType to format.
     * @param ctx Format context.
     * @return Formatter object.
     */
    template <typename FormatContext>
    auto format(amon::layers::LayerType type, FormatContext& ctx) const
    {
        return std::formatter<std::string_view>::format(layerTypeToString(type), ctx);
    }
};

/**
 * @brief std::formatter specialization for amon::layers::LayerError.
 */
template <>
struct std::formatter<amon::layers::LayerError> : std::formatter<std::string_view> {
    /**
     * @brief Format LayerError for output.
     * @tparam FormatContext Type of the format context.
     * @param err LayerError to format.
     * @param ctx Format context.
     * @return Formatter object.
     */
    template <typename FormatContext>
    auto format(amon::layers::LayerError err, FormatContext& ctx) const
    {
        return std::formatter<std::string_view>::format(layerErrorToString(err), ctx);
    }
};
/// @endcond
