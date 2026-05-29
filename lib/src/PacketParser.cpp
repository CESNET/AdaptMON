/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief AdaptMON packet parser.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/Test.hpp>
#include <amon/PacketParser.hpp>
#include <amon/layers/AuthenticationHeader.hpp>
#include <amon/layers/ESP.hpp>
#include <amon/layers/Ethernet.hpp>
#include <amon/layers/GRE.hpp>
#include <amon/layers/ICMP.hpp>
#include <amon/layers/ICMPv6.hpp>
#include <amon/layers/IPv4.hpp>
#include <amon/layers/IPv6.hpp>
#include <amon/layers/MPLS.hpp>
#include <amon/layers/TCP.hpp>
#include <amon/layers/UDP.hpp>
#include <amon/layers/VLAN.hpp>

#include <cassert>

namespace amon {

using namespace layers;

LayerError
PacketParser::parse(Packet& packet, LayerType firstLayer, const uint16_t startOffset) const
{
    return createContextAndParse(packet, firstLayer, startOffset);
}

LayerError PacketParser::createContextAndParse(
    Packet& packet,
    LayerType firstLayer,
    uint16_t startOffset) const
{
    PacketParserContext ctx;
    return processFirstLayer(ctx, packet, firstLayer, startOffset);
}

LayerError PacketParser::processFirstLayer(
    PacketParserContext& ctx,
    Packet& packet,
    LayerType firstLayer,
    uint16_t startOffset) const
{
    switch (firstLayer) {
    case LayerType::RAW:
        return processLinktype(ctx, packet, startOffset, PacketParserLinkType::RAW);
    case LayerType::ETHERNET:
        return processLinktype(ctx, packet, startOffset, PacketParserLinkType::ETHERNET);

    case LayerType::IPV4:
        return processEthertype(ctx, packet, startOffset, PacketParserEtherType::IPV4);
    case LayerType::IPV6:
        return processEthertype(ctx, packet, startOffset, PacketParserEtherType::IPV6);
    case LayerType::MPLS:
        return processEthertype(ctx, packet, startOffset, PacketParserEtherType::MPLS);
    case LayerType::VLAN:
        return processEthertype(ctx, packet, startOffset, PacketParserEtherType::VLAN_8021Q);

    case LayerType::IPV6_OPT_GENERIC:
        // generic extension, not real prototype number
        return processPrototype(ctx, packet, startOffset, PacketParserProtoType::IPV6_OPT_GENERIC);
    case LayerType::IPV6_OPT_FRAGMENT:
        return processPrototype(ctx, packet, startOffset, PacketParserProtoType::IPV6_OPT_FRAGMENT);
    case LayerType::IPV6_OPT_HOPBYHOP:
        return processPrototype(ctx, packet, startOffset, PacketParserProtoType::IPV6_OPT_HOPBYHOP);
    case LayerType::IPV6_OPT_ROUTING:
        return processPrototype(ctx, packet, startOffset, PacketParserProtoType::IPV6_OPT_ROUTING);
    case LayerType::AUTHENTICATION_HEADER:
        return processPrototype(
            ctx,
            packet,
            startOffset,
            PacketParserProtoType::AUTHENTICATION_HEADER);
    case LayerType::ESP:
        return processPrototype(ctx, packet, startOffset, PacketParserProtoType::ESP);
    case LayerType::IPV6_OPT_DESTINATION_OPTIONS:
        return processPrototype(
            ctx,
            packet,
            startOffset,
            PacketParserProtoType::IPV6_OPT_DESTINATION_OPTIONS);

    case LayerType::ICMP:
        return processPrototype(ctx, packet, startOffset, PacketParserProtoType::ICMP);
    case LayerType::ICMPV6:
        return processPrototype(ctx, packet, startOffset, PacketParserProtoType::ICMPV6);
    case LayerType::UDP:
        return processPrototype(ctx, packet, startOffset, PacketParserProtoType::UDP);
    case LayerType::TCP:
        return processPrototype(ctx, packet, startOffset, PacketParserProtoType::TCP);
    case LayerType::GRE:
        return processPrototype(ctx, packet, startOffset, PacketParserProtoType::GRE);

    case LayerType::PAYLOAD_APP:
        return processPayload(ctx, packet, startOffset);
    case LayerType::PAYLOAD_IP_FRAGMENT:
        return processPayload(ctx, packet, startOffset, LayerType::PAYLOAD_IP_FRAGMENT);
    default:
        return processUnknown(packet, startOffset);
    }
}

LayerError PacketParser::processLinktype(
    PacketParserContext& ctx,
    Packet& packet,
    const uint16_t offset,
    PacketParserLinkType linktype) const
{
    switch (linktype) {
    case ETHERNET:
        return processEthernet(ctx, packet, offset);
    case RAW:
        return processIPFromPayload(ctx, packet, offset);
    }

    // PacketParserLinkType is simple enum, handle undefined values
    return processUnknown(packet, offset);
}

LayerError PacketParser::processEthertype(
    PacketParserContext& ctx,
    Packet& packet,
    const uint16_t offset,
    PacketParserEtherType ethertype) const
{
    switch (ethertype) {
    case VLAN_8021Q:
    case VLAN_8021AD:
        return processVLAN(ctx, packet, offset);
    case MPLS:
    case MPLS_MULTICAST:
        return processMPLS(ctx, packet, offset, ethertype);
    case IPV4:
        return processIPv4(ctx, packet, offset);
    case IPV6:
        return processIPv6(ctx, packet, offset);
    }

    // PacketParserEtherType is simple enum, handle undefined values
    return processUnknown(packet, offset);
}

LayerError PacketParser::processPrototype(
    PacketParserContext& ctx,
    Packet& packet,
    const uint16_t offset,
    PacketParserProtoType prototype) const
{
    switch (prototype) {
    case IPV6_OPT_FRAGMENT:
        return processIPv6Fragment(ctx, packet, offset);
    case IPV6_OPT_HOPBYHOP:
        return processIPv6Extension<IPv6HopByHopView>(ctx, packet, offset);
    case IPV6_OPT_ROUTING:
        return processIPv6Extension<IPv6RoutingView>(ctx, packet, offset);
    case ESP:
        return processESP(ctx, packet, offset);
    case AUTHENTICATION_HEADER:
        return processAH(ctx, packet, offset);
    case IPV6_OPT_DESTINATION_OPTIONS:
        return processIPv6Extension<IPv6DestinationOptionsView>(ctx, packet, offset);
    case IPV6_OPT_GENERIC:
    case IPV6_MOBILITY:
    case IPV6_HOST_IDENTITY:
    case IPV6_SHIM6:
        return processIPv6Extension<IPv6ExtensionView>(ctx, packet, offset);
    case IPV6_NO_NEXT: {
        const auto data = packet.data.subspan(offset);
        if (!data.empty()) {
            processUnknown(packet, offset);
        }
        return LayerError::NONE;
    }
    case TCP:
        return processSimpleProtoType<TCPView, true>(ctx, packet, offset);
    case UDP:
        return processSimpleProtoType<UDPView, true>(ctx, packet, offset);
    case ICMP:
        return processSimpleProtoType<ICMPView, false>(ctx, packet, offset);
    case ICMPV6:
        return processSimpleProtoType<ICMPv6View, false>(ctx, packet, offset);
    case GRE:
        return processGRE(ctx, packet, offset);
    case IPV4_ENCAPSULATION:
        return processIPv4(ctx, packet, offset);
    case IPV6_ENCAPSULATION:
        return processIPv6(ctx, packet, offset);
    case RESERVED1:
    case RESERVED2:
        return processUnknown(packet, offset);
    }

    // PacketParserProtoType is simple enum, handle undefined values
    return processUnknown(packet, offset);
}

LayerError PacketParser::processPayload(
    PacketParserContext& ctx,
    Packet& packet,
    const uint16_t offset,
    LayerType payloadType) const
{
    assert(payloadType == LayerType::PAYLOAD_APP || payloadType == LayerType::PAYLOAD_IP_FRAGMENT);

    if (!packet.data.subspan(offset).empty()) {
        if (packet.layers.full()) {
            return LayerError::LAYER_VECTOR_FULL;
        }
        PacketLayer layer {.type = payloadType, .offset = offset};
        packet.layers.pushBack(layer);
        if (payloadType == LayerType::PAYLOAD_APP) {
            addToLayout<LayerType::PAYLOAD_APP>(ctx, packet);
        }
    }
    return LayerError::NONE;
}

LayerError
PacketParser::processEthernet(PacketParserContext& ctx, Packet& packet, const uint16_t offset) const
{
    auto ethOrError = addLayer<EthernetView>(ctx, packet, offset);
    if (!ethOrError) {
        return ethOrError.error();
    }
    return processEthertype(
        ctx,
        packet,
        offset + ethOrError->headerLength(),
        static_cast<PacketParserEtherType>(ethOrError->protocol()));
}

LayerError
PacketParser::processVLAN(PacketParserContext& ctx, Packet& packet, const uint16_t offset) const
{
    auto vlanOrError = addLayer<VLANView>(ctx, packet, offset);
    if (!vlanOrError) {
        return vlanOrError.error();
    }
    return processEthertype(
        ctx,
        packet,
        offset + vlanOrError->headerLength(),
        static_cast<PacketParserEtherType>(vlanOrError->protocol()));
}

LayerError PacketParser::processMPLS(
    PacketParserContext& ctx,
    Packet& packet,
    const uint16_t offset,
    PacketParserEtherType ethertype) const
{
    assert(ethertype == MPLS || ethertype == MPLS_MULTICAST);

    auto mplsOrError = addLayer<MPLSView>(ctx, packet, offset);
    if (!mplsOrError) {
        return mplsOrError.error();
    }
    if (mplsOrError->bottomOfStack()) {
        return processMPLSBottomOfStack(ctx, packet, offset + mplsOrError->headerLength());
    }
    // propagating MPLS or MPLS_MULTICAST
    return processEthertype(ctx, packet, offset + mplsOrError->headerLength(), ethertype);
}

LayerError PacketParser::processMPLSBottomOfStack(
    PacketParserContext& ctx,
    Packet& packet,
    uint16_t offset) const
{
    auto nextLayerIndex = packet.layers.size();
    std::optional<LayerType> nextLayerType = std::nullopt;

    switch (m_config.mplsMode) {
    case PacketParserConfig::MPLSMode::AUTO:
    case PacketParserConfig::MPLSMode::IP_ONLY: {
        // normally it is expected IPv4 or IPv6 continuing
        const LayerError ipError = processIPFromPayload(ctx, packet, offset);
        if (m_config.mplsMode == PacketParserConfig::MPLSMode::IP_ONLY) {
            // do not try another protocols other than IP
            return ipError;
        }

        if (nextLayerIndex < packet.layers.size()) {
            nextLayerType = std::visit(
                [](auto&& arg) -> auto { return arg.type; },
                packet.layers[nextLayerIndex]);
        }
        // Check if packet was successfully parsed as IP (LayerError::NONE) or as IP packet trimmed
        // somewhere after IP header (LayerError::HEADER_SHORT).
        if ((ipError == LayerError::NONE || ipError == LayerError::HEADER_SHORT) && nextLayerType
            && nextLayerType != LayerType::UNKNOWN) {
            // successful parsing as IP (4 or 6)
            return ipError;
        }
        // else rollback (pop invalid layers after actual index) and fallthrough
        while (nextLayerIndex != packet.layers.size()) {
            packet.layers.popBack();
        }
        [[fallthrough]];
    }
    case PacketParserConfig::MPLSMode::PW_AUTO: {
        // pseudo-wire continues, auto check control word

        // From RFC 4448 – Encapsulation Methods for Transport of Ethernet over MPLS Networks:
        //     The control word is defined as follows:
        //     0                   1                   2                   3
        //     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        //    |0 0 0 0|   Reserved            |       Sequence Number         |
        //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        //    In the above diagram, the first 4 bits MUST be set to 0 to indicate
        //    PW data.  The rest of the first 16 bits are reserved for future use.
        //    They MUST be set to 0 when transmitting, and MUST be ignored upon
        //    receipt.

        const auto data = packet.data.subspan(offset);
        if (data.size() >= 4) {
            // Reserved bits are checked to avoid confusion with MAC address starting with 0,
            // addresses starting with 00:00 still cause false positives.
            if (static_cast<uint8_t>(data[0]) == 0 && static_cast<uint8_t>(data[1]) == 0) {
                // skip control word
                offset += 4;
            }
        }
        [[fallthrough]];
    }
    case PacketParserConfig::MPLSMode::PW_NOCW:
        // pseudo-wire without control word
        return processEthernet(ctx, packet, offset);
    }
    std::unreachable();
}

LayerError PacketParser::processIPFromPayload(
    PacketParserContext& ctx,
    Packet& packet,
    const uint16_t offset) const
{
    const auto data = packet.data.subspan(offset);
    if (data.size() >= sizeof(IPv4Header) && (static_cast<uint8_t>(data[0]) >> 4) == 4) {
        return processIPv4(ctx, packet, offset);
    }
    if (data.size() >= sizeof(IPv6Header) && (static_cast<uint8_t>(data[0]) >> 4) == 6) {
        return processIPv6(ctx, packet, offset);
    }
    return processUnknown(packet, offset);
}

LayerError
PacketParser::processIPv4(PacketParserContext& ctx, Packet& packet, const uint16_t offset) const
{
    auto ipOrError = addLayer<IPv4View>(ctx, packet, offset);
    if (!ipOrError) {
        return ipOrError.error();
    }
    if (ipOrError->isFragmented() && ipOrError->fragmentOffset() > 0) {
        return processPayload(
            ctx,
            packet,
            offset + ipOrError->headerLength(),
            LayerType::PAYLOAD_IP_FRAGMENT);
    }
    return processPrototype(
        ctx,
        packet,
        offset + ipOrError->headerLength(),
        static_cast<PacketParserProtoType>(ipOrError->protocol()));
}

LayerError
PacketParser::processIPv6(PacketParserContext& ctx, Packet& packet, const uint16_t offset) const
{
    auto ipOrError = addLayer<IPv6View>(ctx, packet, offset);
    if (!ipOrError) {
        return ipOrError.error();
    }
    return processPrototype(
        ctx,
        packet,
        offset + ipOrError->headerLength(),
        static_cast<PacketParserProtoType>(ipOrError->nextHeader()));
}

template <typename LayerViewT>
LayerError
PacketParser::processIPv6Extension(PacketParserContext& ctx, Packet& packet, const uint16_t offset)
    const
{
    auto extOrError = addLayer<LayerViewT>(ctx, packet, offset);
    if (!extOrError) {
        return extOrError.error();
    }
    return processPrototype(
        ctx,
        packet,
        offset + extOrError->headerLength(),
        static_cast<PacketParserProtoType>(extOrError->nextHeader()));
}

LayerError PacketParser::processIPv6Fragment(
    PacketParserContext& ctx,
    Packet& packet,
    const uint16_t offset) const
{
    auto fragOrError = addLayer<IPv6FragmentView>(ctx, packet, offset);
    if (!fragOrError) {
        return fragOrError.error();
    }
    if (fragOrError->fragmentOffset() > 0) {
        return processPayload(
            ctx,
            packet,
            offset + fragOrError->headerLength(),
            LayerType::PAYLOAD_IP_FRAGMENT);
    }
    return processPrototype(
        ctx,
        packet,
        offset + fragOrError->headerLength(),
        static_cast<PacketParserProtoType>(fragOrError->nextHeader()));
}

template <typename LayerViewT, bool payloadFollows>
LayerError PacketParser::processSimpleProtoType(
    PacketParserContext& ctx,
    Packet& packet,
    const uint16_t offset) const
{
    auto viewOrError = addLayer<LayerViewT>(ctx, packet, offset);
    if (!viewOrError) {
        return viewOrError.error();
    }
    if constexpr (payloadFollows) {
        return processPayload(ctx, packet, offset + viewOrError->headerLength());
    } else {
        return LayerError::NONE;
    }
}

LayerError
PacketParser::processGRE(PacketParserContext& ctx, Packet& packet, const uint16_t offset) const
{
    auto greOrError = addLayer<GREView>(ctx, packet, offset);
    if (!greOrError) {
        return greOrError.error();
    }
    return processEthertype(
        ctx,
        packet,
        offset + greOrError->headerLength(),
        static_cast<PacketParserEtherType>(greOrError->protocolType()));
}

LayerError PacketParser::processESP(PacketParserContext& ctx, Packet& packet, uint16_t offset) const
{
    auto espOrError = addLayer<ESPView>(ctx, packet, offset);
    if (!espOrError) {
        return espOrError.error();
    }
    return processUnknown(packet, offset + espOrError->headerLength());
}

LayerError PacketParser::processAH(PacketParserContext& ctx, Packet& packet, uint16_t offset) const
{
    auto ahOrError = addLayer<AuthenticationHeaderView>(ctx, packet, offset);
    if (!ahOrError) {
        return ahOrError.error();
    }
    return processPrototype(
        ctx,
        packet,
        offset + ahOrError->headerLength(),
        static_cast<PacketParserProtoType>(ahOrError->nextHeader()));
}

LayerError PacketParser::processUnknown(Packet& packet, uint16_t offset)
{
    if (packet.layers.full()) {
        return LayerError::LAYER_VECTOR_FULL;
    }

    PacketLayer layer {.type = LayerType::UNKNOWN, .offset = offset};
    packet.layers.pushBack(layer);
    return LayerError::NONE;
}

static_assert(
    std::is_same_v<uint16_t, decltype(std::to_underlying(PacketParserEtherType::IPV4))>,
    "PacketParserEtherType must be uint16_t.");
static_assert(
    std::is_same_v<uint8_t, decltype(std::to_underlying(PacketParserProtoType::ICMP))>,
    "PacketParserProtoType must be uint8_t.");

#ifdef AMON_ENABLE_TESTS

TEST(AmonPacketParser, PacketparserSimplePacketParsing)
{
    unsigned char frame[]
        = {0x00, 0x1c, 0xc5, 0xfe, 0x03, 0xc8, 0x00, 0x19, 0x99, 0x0b, 0x3b, 0x1e, 0x08, 0x00,
           0x45, 0x00, 0x00, 0x43, 0x6b, 0x49, 0x00, 0x00, 0x40, 0x11, 0xb3, 0x5a, 0xc0, 0xa8,
           0x15, 0x59, 0xc0, 0xa8, 0xc5, 0x5c, 0xa0, 0x14, 0x00, 0x35, 0x00, 0x2f, 0xe3, 0x4a,
           0x38, 0x42, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x6e,
           0x65, 0x77, 0x70, 0x6f, 0x72, 0x74, 0x63, 0x6f, 0x6e, 0x63, 0x65, 0x70, 0x74, 0x73,
           0x6c, 0x61, 0x03, 0x63, 0x6f, 0x6d, 0x00, 0x00, 0x01, 0x00, 0x01};

    Packet packet;
    packet.data = {reinterpret_cast<std::byte*>(frame), sizeof(frame)};
    const PacketParser parser;
    parser.parse(packet, LayerType::ETHERNET);

    EXPECT_EQ(std::get<PacketLayer>(packet.layers[0]).type, LayerType::ETHERNET);
    EXPECT_EQ(std::get<PacketLayer>(packet.layers[0]).offset, 0);

    EXPECT_EQ(std::get<PacketLayer>(packet.layers[1]).type, LayerType::IPV4);
    EXPECT_EQ(std::get<PacketLayer>(packet.layers[1]).offset, 14);

    EXPECT_EQ(std::get<PacketLayer>(packet.layers[2]).type, LayerType::UDP);
    EXPECT_EQ(std::get<PacketLayer>(packet.layers[2]).offset, 34);

    EXPECT_EQ(std::get<PacketLayer>(packet.layers.back()).type, LayerType::PAYLOAD_APP);
    EXPECT_EQ(std::get<PacketLayer>(packet.layers.back()).offset, 42);

    EXPECT_EQ(packet.layout[0].l2, 0);
    EXPECT_EQ(packet.layout[0].l3, 1);
    EXPECT_EQ(packet.layout[0].l4, 2);
    EXPECT_EQ(packet.layout[0].l7, 3);
}

TEST(AmonPacketParser, PacketparserIpInIpParsingIPv4InIPv4)
{
    unsigned char frame[]
        = {0xca, 0x1,  0xb,  0x14, 0x0,  0x8,  0xca, 0x0,  0xb,  0x14, 0x0,  0x8,  0x8,  0x0,  0x45,
           0x0,  0x0,  0x78, 0x0,  0xf,  0x0,  0x0,  0xff, 0x4,  0xa3, 0x6e, 0xa,  0x1,  0x2,  0x1,
           0xa,  0x1,  0x2,  0x2,  0x45, 0x0,  0x0,  0x64, 0x0,  0xf,  0x0,  0x0,  0xff, 0x1,  0x3a,
           0x36, 0xc0, 0xa8, 0x0,  0x1,  0xc0, 0xa8, 0x0,  0x2,  0x8,  0x0,  0x2d, 0x16, 0x0,  0x3,
           0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0xd,  0x51, 0x24, 0xab, 0xcd, 0xab, 0xcd, 0xab,
           0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd,
           0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab,
           0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd,
           0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd};

    Packet packet;
    packet.data = {reinterpret_cast<std::byte*>(frame), sizeof(frame)};
    PacketParser parser;
    parser.parse(packet, LayerType::ETHERNET);

    EXPECT_EQ(std::get<PacketLayer>(packet.layers[0]).type, LayerType::ETHERNET);
    EXPECT_EQ(std::get<PacketLayer>(packet.layers[0]).offset, 0);

    EXPECT_EQ(std::get<PacketLayer>(packet.layers[1]).type, LayerType::IPV4);
    EXPECT_EQ(std::get<PacketLayer>(packet.layers[1]).offset, 14);

    EXPECT_EQ(std::get<PacketLayer>(packet.layers[2]).type, LayerType::IPV4);
    EXPECT_EQ(std::get<PacketLayer>(packet.layers[2]).offset, 34);

    EXPECT_EQ(std::get<PacketLayer>(packet.layers[3]).type, LayerType::ICMP);
    EXPECT_EQ(std::get<PacketLayer>(packet.layers[3]).offset, 54);

    // outer
    EXPECT_EQ(packet.layout[0].l2, 0);
    EXPECT_EQ(packet.layout[0].l3, 1);
    EXPECT_FALSE(packet.layout[0].l4);
    EXPECT_FALSE(packet.layout[0].l7);
    // inner
    EXPECT_EQ(packet.layout[1].l3, 2);
    EXPECT_EQ(packet.layout[1].l4, 3);
    EXPECT_FALSE(packet.layout[1].l7);
}

TEST(AmonPacketParser, PacketparserIpInIpParsingIPv6InIPv4)
{
    unsigned char frame[]
        = {0xb4, 0x14, 0x89, 0x8,  0x2d, 0x30, 0x20, 0x4e, 0x7f, 0x35, 0x9b, 0xb2, 0x81, 0x0,
           0x0,  0x4,  0x08, 0x00, 0x45, 0x0,  0x0,  0x5c, 0x0,  0x0,  0x40, 0x0,  0x40, 0x29,
           0xa1, 0xf0, 0xd5, 0x8d, 0x9a, 0xaa, 0xd5, 0x4f, 0x53, 0x1,  0x60, 0x0,  0x0,  0x0,
           0x0,  0x20, 0x6,  0x3f, 0x20, 0x1,  0x6,  0x7c, 0x21, 0x58, 0xa0, 0x19, 0x0,  0x0,
           0x0,  0x0,  0x0,  0x0,  0xa,  0xce, 0x20, 0x1,  0x0,  0x0,  0x5e, 0xf5, 0x79, 0xfd,
           0x38, 0xc,  0x1d, 0x57, 0xa6, 0x1,  0x24, 0xfa, 0xcf, 0x70, 0x35, 0xdc, 0x44, 0xdd,
           0xa0, 0x3d, 0x0,  0x0,  0x0,  0x0,  0x80, 0x2,  0x20, 0x0,  0x58, 0xcf, 0x0,  0x0,
           0x2,  0x4,  0x5,  0x84, 0x1,  0x3,  0x3,  0x2,  0x1,  0x1,  0x4,  0x2};

    Packet packet;
    packet.data = {reinterpret_cast<std::byte*>(frame), sizeof(frame)};
    PacketParser parser;
    parser.parse(packet, LayerType::ETHERNET);

    EXPECT_EQ(std::get<PacketLayer>(packet.layers[0]).type, LayerType::ETHERNET);
    EXPECT_EQ(std::get<PacketLayer>(packet.layers[0]).offset, 0);

    EXPECT_EQ(std::get<PacketLayer>(packet.layers[1]).type, LayerType::VLAN);
    EXPECT_EQ(std::get<PacketLayer>(packet.layers[1]).offset, 14);

    EXPECT_EQ(std::get<PacketLayer>(packet.layers[2]).type, LayerType::IPV4);
    EXPECT_EQ(std::get<PacketLayer>(packet.layers[2]).offset, 18);

    EXPECT_EQ(std::get<PacketLayer>(packet.layers[3]).type, LayerType::IPV6);
    EXPECT_EQ(std::get<PacketLayer>(packet.layers[3]).offset, 38);

    EXPECT_EQ(std::get<PacketLayer>(packet.layers[4]).type, LayerType::TCP);
    EXPECT_EQ(std::get<PacketLayer>(packet.layers[4]).offset, 78);

    // outer
    EXPECT_EQ(packet.layout[0].l2, 0);
    EXPECT_EQ(packet.layout[0].l3, 2);
    EXPECT_FALSE(packet.layout[0].l4);
    EXPECT_FALSE(packet.layout[0].l7);
    // inner
    EXPECT_EQ(packet.layout[1].l3, 3);
    EXPECT_EQ(packet.layout[1].l4, 4);
    EXPECT_FALSE(packet.layout[1].l7);
}

TEST(AmonPacketParser, PacketparserSynTCPParsing)
{
    unsigned char frame[]
        = {0x0,  0xe,  0xc,  0xc0, 0xff, 0xee, 0x0,  0x19, 0x30, 0xc0, 0xff, 0xee, 0x8,  0x0,
           0x45, 0x0,  0x0,  0x28, 0x1,  0x0,  0x0,  0x0,  0x73, 0x6,  0xc0, 0x5c, 0xa4, 0x7c,
           0x21, 0x4e, 0xc0, 0xa8, 0x0,  0x1,  0x89, 0x5d, 0x0,  0x50, 0x0,  0x0,  0x63, 0x3f,
           0x0,  0x0,  0x0,  0x0,  0x50, 0x2,  0x40, 0x0,  0xfc, 0x81, 0x0,  0x0};

    Packet packet;
    packet.data = {reinterpret_cast<std::byte*>(frame), sizeof(frame)};
    const PacketParser parser;
    parser.parse(packet, LayerType::ETHERNET);

    EXPECT_EQ(std::get<PacketLayer>(packet.layers[0]).type, LayerType::ETHERNET);
    EXPECT_EQ(std::get<PacketLayer>(packet.layers[0]).offset, 0);

    EXPECT_EQ(std::get<PacketLayer>(packet.layers[1]).type, LayerType::IPV4);
    EXPECT_EQ(std::get<PacketLayer>(packet.layers[1]).offset, 14);

    EXPECT_EQ(std::get<PacketLayer>(packet.layers.back()).type, LayerType::TCP);
    EXPECT_EQ(std::get<PacketLayer>(packet.layers.back()).offset, 34);

    EXPECT_EQ(packet.layers.size(), 3);

    EXPECT_EQ(packet.layout[0].l2, 0);
    EXPECT_EQ(packet.layout[0].l3, 1);
    EXPECT_EQ(packet.layout[0].l4, 2);
    EXPECT_EQ(packet.layout.size(), 1);
}

TEST(AmonPacketParser, PacketparserStandardMPLSWithIpContinuing)
{
    unsigned char frame[]
        = {0x18, 0xcf, 0x24, 0x14, 0xae, 0xca, 0xac, 0xb3, 0xb5, 0xe1, 0xdf, 0xde, 0x81, 0x0,
           0x0,  0x49, 0x88, 0x47, 0xb,  0xbf, 0x40, 0xff, 0x0,  0x59, 0x41, 0xff, 0x6c, 0x6c,
           0xd3, 0x18, 0x37, 0x7c, 0x44, 0xa9, 0x2c, 0x41, 0x9,  0x51, 0x81, 0x0,  0x0,  0x4b,
           0x8,  0x0,  0x45, 0x0,  0x0,  0x34, 0xd6, 0xac, 0x40, 0x0,  0x3b, 0x6,  0x2d, 0xc3,
           0xd4, 0x60, 0xb9, 0x4d, 0x1f, 0x1e, 0x8e, 0x88, 0x8f, 0xe2, 0x0,  0x50, 0x8e, 0xb7,
           0xac, 0xe,  0xe,  0xc1, 0xc6, 0x42, 0x80, 0x10, 0x38, 0x82, 0x66, 0x1,  0x0,  0x0,
           0x1,  0x1,  0x8,  0xa,  0xdf, 0x36, 0xfc, 0x53, 0xd4, 0x56, 0x4d, 0x7};

    Packet packet;
    packet.data = {reinterpret_cast<std::byte*>(frame), sizeof(frame)};
    const PacketParser parser;
    parser.parse(packet, LayerType::ETHERNET);

    EXPECT_EQ(std::get<PacketLayer>(packet.layers[0]).type, LayerType::ETHERNET);
    EXPECT_EQ(std::get<PacketLayer>(packet.layers[0]).offset, 0);

    EXPECT_EQ(std::get<PacketLayer>(packet.layers[1]).type, LayerType::VLAN);
    EXPECT_EQ(std::get<PacketLayer>(packet.layers[1]).offset, 14);

    EXPECT_EQ(std::get<PacketLayer>(packet.layers[2]).type, LayerType::MPLS);
    EXPECT_EQ(std::get<PacketLayer>(packet.layers[2]).offset, 18);

    EXPECT_EQ(std::get<PacketLayer>(packet.layers[3]).type, LayerType::MPLS);
    EXPECT_EQ(std::get<PacketLayer>(packet.layers[3]).offset, 22);

    EXPECT_EQ(std::get<PacketLayer>(packet.layers[4]).type, LayerType::IPV6);
    EXPECT_EQ(std::get<PacketLayer>(packet.layers[4]).offset, 26);

    EXPECT_EQ(std::get<PacketLayer>(packet.layers.back()).type, LayerType::UNKNOWN);
    EXPECT_EQ(std::get<PacketLayer>(packet.layers.back()).offset, 66);

    EXPECT_EQ(packet.layout[0].l2, 0);
    EXPECT_EQ(packet.layout[0].l3, 4);
    EXPECT_TRUE(!packet.layout[0].l7);
}

TEST(AmonPacketParser, PacketparserPseudoWireMPLSWithoutControlWordPacketParsing)
{
    unsigned char frame[]
        = {0xac, 0xb3, 0xb5, 0xe1, 0xdf, 0xde, 0x18, 0xcf, 0x24, 0x14, 0xae, 0xca, 0x81, 0x0,
           0x0,  0x49, 0x88, 0x47, 0x0,  0x40, 0x1,  0xfe, 0x44, 0xa9, 0x2c, 0x41, 0x9,  0x51,
           0x6c, 0x6c, 0xd3, 0x18, 0x37, 0x7c, 0x81, 0x0,  0x0,  0x4b, 0x8,  0x0,  0x45, 0x0,
           0x0,  0x34, 0x69, 0x99, 0x40, 0x0,  0x3e, 0x6,  0x95, 0xf8, 0xc7, 0xe8, 0x12, 0xf8,
           0xd4, 0x4,  0x8e, 0x4d, 0x1,  0xbb, 0xc9, 0xca, 0xef, 0x32, 0xb5, 0x89, 0x35, 0x61,
           0x12, 0xa4, 0x80, 0x10, 0x1,  0x20, 0xee, 0xe7, 0x0,  0x0,  0x1,  0x1,  0x8,  0xa,
           0x48, 0x8d, 0x54, 0x3b, 0xe4, 0xe4, 0xf,  0x8e};

    Packet packet;
    packet.data = {reinterpret_cast<std::byte*>(frame), sizeof(frame)};
    const PacketParser parser;
    parser.parse(packet, LayerType::ETHERNET);

    EXPECT_EQ(std::get<PacketLayer>(packet.layers[0]).type, LayerType::ETHERNET);
    EXPECT_EQ(std::get<PacketLayer>(packet.layers[0]).offset, 0);

    EXPECT_EQ(std::get<PacketLayer>(packet.layers[1]).type, LayerType::VLAN);
    EXPECT_EQ(std::get<PacketLayer>(packet.layers[1]).offset, 14);

    EXPECT_EQ(std::get<PacketLayer>(packet.layers[2]).type, LayerType::MPLS);
    EXPECT_EQ(std::get<PacketLayer>(packet.layers[2]).offset, 18);

    EXPECT_EQ(std::get<PacketLayer>(packet.layers[3]).type, LayerType::ETHERNET);
    EXPECT_EQ(std::get<PacketLayer>(packet.layers[3]).offset, 22);

    EXPECT_EQ(std::get<PacketLayer>(packet.layers[4]).type, LayerType::VLAN);
    EXPECT_EQ(std::get<PacketLayer>(packet.layers[4]).offset, 36);

    EXPECT_EQ(std::get<PacketLayer>(packet.layers[5]).type, LayerType::IPV4);
    EXPECT_EQ(std::get<PacketLayer>(packet.layers[5]).offset, 40);

    EXPECT_EQ(std::get<PacketLayer>(packet.layers.back()).type, LayerType::TCP);
    EXPECT_EQ(std::get<PacketLayer>(packet.layers.back()).offset, 60);

    EXPECT_EQ(packet.layers.size(), 7);

    EXPECT_EQ(packet.layout[0].l2, 0);
    EXPECT_EQ(packet.layout[1].l2, 3);
    EXPECT_EQ(packet.layout[1].l3, 5);
    EXPECT_EQ(packet.layout[1].l4, 6);
    EXPECT_TRUE(!packet.layout[1].l7);
}

#endif

} // namespace amon
