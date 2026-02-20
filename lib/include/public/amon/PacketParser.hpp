/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief AdaptMON packet parser.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/Packet.hpp>
#include <amon/layers/Layer.hpp>

#include <variant>

namespace amon {

/**
 * @brief Enumeration of link layer types for packet parsing.
 */
enum PacketParserLinkType {
    ETHERNET,
    RAW,
};

/**
 * @brief Enumeration of payload types for packet parsing.
 */
enum PacketParserPayloadType {
    PAYLOAD_APP,
    PAYLOAD_IP_FRAGMENT,
};

/**
 * @brief Enumeration of ethertype values for packet parsing.
 */
enum PacketParserEtherType : uint16_t {
    IPV4 = 0x0800,
    IPV6 = 0x86DD,
    VLAN_8021Q = 0x8100,
    VLAN_8021AD = 0x88A8,
    MPLS = 0x8847,
    MPLS_MULTICAST = 0x8848,
};

/**
 * @brief Enumeration of protocol types for packet parsing.
 */
enum PacketParserProtoType : uint8_t {
    IPV6_OPT_HOPBYHOP = 0,
    IPV6_OPT_ROUTING = 43,
    IPV6_OPT_FRAGMENT = 44,
    ESP = 50,
    AUTHENTICATION_HEADER = 51,
    IPV6_NO_NEXT = 59,
    IPV6_OPT_DESTINATION_OPTIONS = 60,
    IPV6_MOBILITY = 135,
    IPV6_HOST_IDENTITY = 139,
    IPV6_SHIM6 = 140,
    ICMP = 1,
    TCP = 6,
    UDP = 17,
    GRE = 47,
    ICMPV6 = 58,
    IPV4_ENCAPSULATION = 4,
    IPV6_ENCAPSULATION = 41,
    RESERVED1 = 253,
    RESERVED2 = 254,
    IPV6_OPT_GENERIC = 255, // generic extension for mapping LayerType => ProtoType
                            // IANA reserved value is used
};

/// Variant type for representing different layer type categories.
using LayerTypeVariant = std::variant<
    PacketParserLinkType,
    PacketParserEtherType,
    PacketParserProtoType,
    PacketParserPayloadType,
    std::monostate>;

/**
 * @brief Convert LayerType to LayerTypeVariant variant.
 * @param type LayerType value.
 * @return Corresponding variant.
 */
static constexpr LayerTypeVariant layerTypeToVariant(layers::LayerType type) noexcept
{
    using namespace layers;

    switch (type) {
    case LayerType::RAW:
        return PacketParserLinkType::RAW;
    case LayerType::ETHERNET:
        return PacketParserLinkType::ETHERNET;

    case LayerType::IPV4:
        return PacketParserEtherType::IPV4;
    case LayerType::IPV6:
        return PacketParserEtherType::IPV6;
    case LayerType::MPLS:
        return PacketParserEtherType::MPLS;
    case LayerType::VLAN:
        return PacketParserEtherType::VLAN_8021Q;

    case LayerType::IPV6_OPT_GENERIC:
        // generic extension, not real prototype number
        return PacketParserProtoType::IPV6_OPT_GENERIC;
    case LayerType::IPV6_OPT_FRAGMENT:
        return PacketParserProtoType::IPV6_OPT_FRAGMENT;
    case LayerType::IPV6_OPT_HOPBYHOP:
        return PacketParserProtoType::IPV6_OPT_HOPBYHOP;
    case LayerType::IPV6_OPT_ROUTING:
        return PacketParserProtoType::IPV6_OPT_ROUTING;
    case LayerType::AUTHENTICATION_HEADER:
        return PacketParserProtoType::AUTHENTICATION_HEADER;
    case LayerType::ESP:
        return PacketParserProtoType::ESP;
    case LayerType::IPV6_OPT_DESTINATION_OPTIONS:
        return PacketParserProtoType::IPV6_OPT_DESTINATION_OPTIONS;

    case LayerType::ICMP:
        return PacketParserProtoType::ICMP;
    case LayerType::ICMPV6:
        return PacketParserProtoType::ICMPV6;
    case LayerType::UDP:
        return PacketParserProtoType::UDP;
    case LayerType::TCP:
        return PacketParserProtoType::TCP;
    case LayerType::GRE:
        return PacketParserProtoType::GRE;

    case LayerType::PAYLOAD_APP:
        return PacketParserPayloadType::PAYLOAD_APP;
    case LayerType::PAYLOAD_IP_FRAGMENT:
        return PacketParserPayloadType::PAYLOAD_IP_FRAGMENT;

    case LayerType::ERROR:
    case LayerType::UNKNOWN:
        return std::monostate {};
    }

    return std::monostate {};
}

/**
 * @brief Packet parser class.
 *
 * Provides static method to parse raw packet data and populate Packet structure.
 */
class PacketParser {
public:
    /**
     * @brief Parse raw packet data and populate Packet structure.
     * @param packet Packet object to populate with parsed data.
     * Data member in packet object must be filled.
     * @param firstLayer Type of the first layer in the packet (e.g., LayerType::ETHERNET).
     * @param startOffset Offset in the packet data where parsing should start.
     * @return LayerError indicating success or type of error encountered.
     */
    static layers::LayerError
    parse(Packet& packet, layers::LayerType firstLayer, uint16_t startOffset = 0);

    /**
     * @brief Parse raw packet data and populate Packet structure.
     * @param packet Packet object to populate with parsed data.
     * Data member in packet object must be filled.
     * @param firstLayer Type of the first layer in the packet as LayerTypeVariant.
     * @param startOffset Offset in the packet data where parsing should start.
     * @return LayerError indicating success or type of error encountered.
     */
    static layers::LayerError
    parse(Packet& packet, LayerTypeVariant firstLayer, uint16_t startOffset = 0);

private:
    /**
     * @brief Process layer after link layer.
     * @param packet Packet being parsed.
     * @param offset Offset in the packet data.
     * @param linktype Link type indicating the next layer type.
     * @return LayerError indicating success or type of error encountered.
     */
    static layers::LayerError
    processLinktype(Packet& packet, uint16_t offset, PacketParserLinkType linktype);
    /**
     * @brief Process layer after ethernet.
     * @param packet Packet being parsed.
     * @param offset Offset in the packet data.
     * @param ethertype Ethertype indicating the next layer type.
     * @return LayerError indicating success or type of error encountered.
     */
    static layers::LayerError
    processEthertype(Packet& packet, uint16_t offset, PacketParserEtherType ethertype);
    /**
     * @brief Process layer after IP layer.
     * @param packet Packet being parsed.
     * @param offset Offset in the packet data.
     * @param prototype Protocol number indicating the next layer type.
     * @return LayerError indicating success or type of error encountered.
     */
    static layers::LayerError
    processPrototype(Packet& packet, uint16_t offset, PacketParserProtoType prototype);
    /**
     * @brief Process rest of the packet – payload.
     * @param packet Packet being parsed.
     * @param offset Offset in the packet data.
     * @param payloadType Type of the payload layer (default is LayerType::PAYLOAD_APP).
     * @return LayerError indicating success or type of error encountered.
     */
    static layers::LayerError processPayload(
        Packet& packet,
        uint16_t offset,
        layers::LayerType payloadType = layers::LayerType::PAYLOAD_APP);

    // Following functions are handlers for the given single protocol. Each of them should check
    // whether the packet's layers vector is not full before appending or use addLayer function
    // which checks it internally.
    static layers::LayerError processEthernet(Packet& packet, uint16_t offset);
    static layers::LayerError processVLAN(Packet& packet, uint16_t offset);
    static layers::LayerError
    processMPLS(Packet& packet, uint16_t offset, PacketParserEtherType ethertype);
    static layers::LayerError processIPFromPayload(Packet& packet, uint16_t offset);
    static layers::LayerError processIPv4(Packet& packet, uint16_t offset);
    static layers::LayerError processIPv6(Packet& packet, uint16_t offset);
    static layers::LayerError processIPv6Fragment(Packet& packet, uint16_t offset);
    template <typename LayerViewT>
    static layers::LayerError processIPv6Extension(Packet& packet, uint16_t offset);
    template <typename LayerViewT, bool payloadFollows>
    static layers::LayerError processSimpleProtoType(Packet& packet, uint16_t offset);
    static layers::LayerError processGRE(Packet& packet, uint16_t offset);
    static layers::LayerError processESP(Packet& packet, uint16_t offset);
    static layers::LayerError processAH(Packet& packet, uint16_t offset);
    static layers::LayerError processUnknown(Packet& packet, uint16_t offset);
};

} // namespace amon
