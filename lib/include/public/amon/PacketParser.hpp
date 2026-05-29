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

#include <expected>
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
 * @brief Configuration structure for PacketParser.
 */
struct PacketParserConfig {
    /**
     * @brief Choice of MPLS parsing mode.
     */
    enum class MPLSMode {
        /// Try parse payload as IP (4 or 6), on fail data is treated as pseudo-wire with or without
        /// control word
        AUTO,
        /// Payload could be only IP (4 or 6). If not, return parsing error
        IP_ONLY,
        /// Parse payload as pseudo-wire with or without control word
        PW_AUTO,
        /// Parse data as pseudo-wire without control word
        PW_NOCW,
    };

    /**
     * @brief Mode of parsing data which continues after MPLS labels.
     */
    MPLSMode mplsMode = MPLSMode::AUTO;
};

/**
 * @brief Struct holding state while parsing packet. It is constructed for each packet and passed to
 * layers parsing methods.
 */
struct PacketParserContext {
    /// Index of last layer added to PacketLayout while packet parsing (2=L2, 3=L3, 4=L4). Max
    /// unsigned value is used as resetted value.
    unsigned layoutLastLayerIndex = std::numeric_limits<unsigned>::max();
};

/**
 * @brief Packet parser class.
 *
 * Provides method to parse raw packet data and populate Packet structure.
 *
 * @note This class is thread-safe. The parse() method can be called concurrently from multiple
 * threads on the same PacketParser instance.
 */
class PacketParser {
public:
    /**
     * @brief Constructor with configuration.
     * @param config Packet parser configuration. If not defined, default values are used.
     */
    explicit PacketParser(const PacketParserConfig& config = PacketParserConfig())
        : m_config(config) {};

    /**
     * @brief Parse raw packet data and populate Packet structure.
     * @param packet Packet object to populate with parsed data.
     * Data member in packet object must be filled.
     * @param firstLayer Type of the first layer in the packet (e.g., LayerType::ETHERNET).
     * @param startOffset Offset in the packet data where parsing should start.
     * @return LayerError indicating success or type of error encountered.
     */
    layers::LayerError
    parse(Packet& packet, layers::LayerType firstLayer, uint16_t startOffset = 0) const;

    /**
     * @brief Parse raw packet data and populate Packet structure.
     * @param packet Packet object to populate with parsed data.
     * Data member in packet object must be filled.
     * @param firstLayer Type of the first layer in the packet as LayerTypeVariant.
     * @param startOffset Offset in the packet data where parsing should start.
     * @return LayerError indicating success or type of error encountered.
     */
    layers::LayerError
    parse(Packet& packet, LayerTypeVariant firstLayer, uint16_t startOffset = 0) const;

protected:
    /**
     * @brief Construct the context object for the current packet and call processFirstLayer method.
     *
     * Method is intended to be overridden in a child to provide an extended parsing context.
     * Extended context must inherit from PacketParserContext to properly work with default parser
     * implementations but can be up-casted in custom-overridden virtual methods.
     *
     * @param packet Packet object propagated from parse method.
     * @param firstLayer Type of the first layer in the packet propagated from parse method.
     * @param startOffset Offset in the packet data propagated from parse method.
     * @return LayerError indicating success or type of error encountered.
     */
    virtual layers::LayerError
    createContextAndParse(Packet& packet, LayerTypeVariant firstLayer, uint16_t startOffset) const;

    /**
     * @brief Parse raw packet data and populate Packet structure.
     * @param ctx Parser context holding state for the current packet.
     * @param packet Packet object propagated from parse method.
     * @param firstLayer Type of the first layer in the packet propagated from parse method.
     * @param startOffset Offset in the packet data propagated from parse method.
     * @return LayerError indicating success or type of error encountered.
     */
    layers::LayerError processFirstLayer(
        PacketParserContext& ctx,
        Packet& packet,
        LayerTypeVariant firstLayer,
        uint16_t startOffset = 0) const;

    /**
     * @brief Add a layer info (PacketLayer) into layers vector in packet. If layer view
     * construction fails, ErrorLayer is added.
     * @tparam LayerViewT Type of the layer view to add into the layers vector.
     * @param ctx Parser context holding state for the current packet.
     * @param packet Packet being parsed.
     * @param currentOffset Current offset in the packet data.
     * @return Expected containing the added layer view on success, or LayerError on failure.
     *
     * @note Packet layout is updated if parsing and adding layer is successful.
     */
    template <typename LayerViewT>
    std::expected<LayerViewT, layers::LayerError>
    addLayer(PacketParserContext& ctx, Packet& packet, const uint16_t currentOffset) const
    {
        if (packet.layers.full()) {
            return std::unexpected(layers::LayerError::LAYER_VECTOR_FULL);
        }
        // You are probably scratching your head at this, but yes, it has a reason to be this way,
        // and yes, I know it looks completely stupid and unnecessary... This being wrapped in a
        // lambda makes RVO (return-value-optimization) work in this function, which makes a HUGE
        // difference in performance.
        return [&]() {
            auto res = LayerViewT::create(packet.data.subspan(currentOffset));
            if (res) {
                PacketLayer layer {.type = LayerViewT::LAYER_TYPE, .offset = currentOffset};
                packet.layers.pushBack(layer);
                addToLayout<LayerViewT::LAYER_TYPE>(ctx, packet);
            } else {
                ErrorLayer error {
                    .type = LayerViewT::LAYER_TYPE,
                    .error = res.error(),
                    .offset = currentOffset};
                packet.layers.pushBack(error);
            }
            return res;
        }();
    }

    /**
     * @brief Add last inserted layer to packet layout based on its type.
     * @tparam LayerType_ Type of the layer being added.
     * @param ctx Parser context holding state for the current packet.
     * @param packet Packet being parsed.
     */
    template <layers::LayerType LayerType_>
    void addToLayout(PacketParserContext& ctx, Packet& packet) const
    {
        assert(!packet.layers.empty());

        if constexpr (layerTypeIsKey(LayerType_)) {
            if constexpr (layerTypeBase(LayerType_) == layers::LAYERTYPE_L2) {
                addToLayoutHelper<&PacketLayout::l2, 2>(ctx, packet);
            } else if constexpr (layerTypeBase(LayerType_) == layers::LAYERTYPE_L3) {
                addToLayoutHelper<&PacketLayout::l3, 3>(ctx, packet);
            } else if constexpr (layerTypeBase(LayerType_) == layers::LAYERTYPE_L4) {
                addToLayoutHelper<&PacketLayout::l4, 4>(ctx, packet);
            } else if constexpr (LayerType_ == layers::LayerType::PAYLOAD_APP) {
                addToLayoutHelper<&PacketLayout::l7, 7>(ctx, packet);
            }
        }
    }

    /**
     * @brief Process layer after link layer.
     * @param ctx Parser context holding state for the current packet.
     * @param packet Packet being parsed.
     * @param offset Offset in the packet data.
     * @param linktype Link type indicating the next layer type.
     * @return LayerError indicating success or type of error encountered.
     */
    virtual layers::LayerError processLinktype(
        PacketParserContext& ctx,
        Packet& packet,
        uint16_t offset,
        PacketParserLinkType linktype) const;
    /**
     * @brief Process layer after ethernet.
     * @param ctx Parser context holding state for the current packet.
     * @param packet Packet being parsed.
     * @param offset Offset in the packet data.
     * @param ethertype Ethertype indicating the next layer type.
     * @return LayerError indicating success or type of error encountered.
     */
    virtual layers::LayerError processEthertype(
        PacketParserContext& ctx,
        Packet& packet,
        uint16_t offset,
        PacketParserEtherType ethertype) const;
    /**
     * @brief Process layer after IP layer.
     * @param ctx Parser context holding state for the current packet.
     * @param packet Packet being parsed.
     * @param offset Offset in the packet data.
     * @param prototype Protocol number indicating the next layer type.
     * @return LayerError indicating success or type of error encountered.
     */
    virtual layers::LayerError processPrototype(
        PacketParserContext& ctx,
        Packet& packet,
        uint16_t offset,
        PacketParserProtoType prototype) const;
    /**
     * @brief Process rest of the packet – payload.
     * @param ctx Parser context holding state for the current packet.
     * @param packet Packet being parsed.
     * @param offset Offset in the packet data.
     * @param payloadType Type of the payload layer.
     * @return LayerError indicating success or type of error encountered.
     */
    virtual layers::LayerError processPayload(
        PacketParserContext& ctx,
        Packet& packet,
        uint16_t offset,
        layers::LayerType payloadType) const;

    /**
     * @brief Process rest of the packet – payload as LayerType::PAYLOAD_APP.
     * @param ctx Parser context holding state for the current packet.
     * @param packet Packet being parsed.
     * @param offset Offset in the packet data.
     * @return LayerError indicating success or type of error encountered.
     */
    layers::LayerError
    processPayload(PacketParserContext& ctx, Packet& packet, uint16_t offset) const
    {
        return processPayload(ctx, packet, offset, layers::LayerType::PAYLOAD_APP);
    };

private:
    // Following functions are handlers for the given single protocol. Each of them should check
    // whether the packet's layers vector is not full before appending or use addLayer function
    // which checks it internally.
    layers::LayerError
    processEthernet(PacketParserContext& ctx, Packet& packet, uint16_t offset) const;
    layers::LayerError processVLAN(PacketParserContext& ctx, Packet& packet, uint16_t offset) const;
    layers::LayerError processMPLS(
        PacketParserContext& ctx,
        Packet& packet,
        uint16_t offset,
        PacketParserEtherType ethertype) const;
    layers::LayerError
    processMPLSBottomOfStack(PacketParserContext& ctx, Packet& packet, uint16_t offset) const;
    layers::LayerError
    processIPFromPayload(PacketParserContext& ctx, Packet& packet, uint16_t offset) const;
    layers::LayerError processIPv4(PacketParserContext& ctx, Packet& packet, uint16_t offset) const;
    layers::LayerError processIPv6(PacketParserContext& ctx, Packet& packet, uint16_t offset) const;
    layers::LayerError
    processIPv6Fragment(PacketParserContext& ctx, Packet& packet, uint16_t offset) const;
    template <typename LayerViewT>
    layers::LayerError
    processIPv6Extension(PacketParserContext& ctx, Packet& packet, uint16_t offset) const;
    template <typename LayerViewT, bool payloadFollows>
    layers::LayerError
    processSimpleProtoType(PacketParserContext& ctx, Packet& packet, uint16_t offset) const;
    layers::LayerError processGRE(PacketParserContext& ctx, Packet& packet, uint16_t offset) const;
    layers::LayerError processESP(PacketParserContext& ctx, Packet& packet, uint16_t offset) const;
    layers::LayerError processAH(PacketParserContext& ctx, Packet& packet, uint16_t offset) const;
    static layers::LayerError processUnknown(Packet& packet, uint16_t offset);

    /**
     * @brief Add last inserted layer to packet layout.
     * @tparam PacketLayoutMember Pointer to a member in PacketLayout structure which should be
     * updated.
     * @tparam LayerIndex OSI layer number (2=L2, 3=L3, 4=L4) for tracking the current layer level.
     * @param ctx Parser context holding state for the current packet.
     * @param packet Packet being parsed.
     */
    template <auto PacketLayoutMember, unsigned LayerIndex>
        requires std::is_member_object_pointer_v<decltype(PacketLayoutMember)>
    void addToLayoutHelper(PacketParserContext& ctx, Packet& packet) const
    {
        if (ctx.layoutLastLayerIndex >= LayerIndex) {
            // new encapsulation level detected
            if (packet.layout.full()) {
                // parsed packet has more encapsulations than supported by packet layout
                // ensure that any layer will not be added to the layout
                ctx.layoutLastLayerIndex = std::numeric_limits<unsigned>::max();
                return;
            }
            packet.layout.emplaceBack();
        }

        auto& layout = packet.layout.back();
        if (!(layout.*PacketLayoutMember)) {
            layout.*PacketLayoutMember = packet.layers.size() - 1;
            ctx.layoutLastLayerIndex = LayerIndex;
        }
    }

    PacketParserConfig m_config;
};

} // namespace amon
