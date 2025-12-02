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

LayerError PacketParser::parse(Packet& packet, LayerType firstLayer, const uint16_t startOffset)
{
    return parse(packet, layerTypeToVariant(firstLayer), startOffset);
}

LayerError
PacketParser::parse(Packet& packet, LayerTypeVariant firstLayer, const uint16_t startOffset)
{
    if (std::holds_alternative<PacketParserLinkType>(firstLayer)) {
        return processLinktype(packet, startOffset, std::get<PacketParserLinkType>(firstLayer));
    }
    if (std::holds_alternative<PacketParserEtherType>(firstLayer)) {
        return processEthertype(packet, startOffset, std::get<PacketParserEtherType>(firstLayer));
    }
    if (std::holds_alternative<PacketParserProtoType>(firstLayer)) {
        return processPrototype(packet, startOffset, std::get<PacketParserProtoType>(firstLayer));
    }
    if (std::holds_alternative<PacketParserPayloadType>(firstLayer)) {
        if (std::get<PacketParserPayloadType>(firstLayer)
            == PacketParserPayloadType::PAYLOAD_IP_FRAGMENT) {
            return processPayload(packet, startOffset, LayerType::PAYLOAD_IP_FRAGMENT);
        }
        return processPayload(packet, startOffset);
    }
    // unknown first layer
    return processUnknown(packet, startOffset);
}

LayerError
PacketParser::processLinktype(Packet& packet, const uint16_t offset, PacketParserLinkType linktype)
{
    switch (linktype) {
    case ETHERNET:
        return processEthernet(packet, offset);
    case RAW:
        return processIPFromPayload(packet, offset);
    }

    // PacketParserLinkType is simple enum, handle undefined values
    return processUnknown(packet, offset);
}

LayerError PacketParser::processEthertype(
    Packet& packet,
    const uint16_t offset,
    PacketParserEtherType ethertype)
{
    switch (ethertype) {
    case VLAN_8021Q:
    case VLAN_8021AD:
        return processVLAN(packet, offset);
    case MPLS:
    case MPLS_MULTICAST:
        return processMPLS(packet, offset, ethertype);
    case IPV4:
        return processIPv4(packet, offset);
    case IPV6:
        return processIPv6(packet, offset);
    }

    // PacketParserEtherType is simple enum, handle undefined values
    return processUnknown(packet, offset);
}

LayerError PacketParser::processPrototype(
    Packet& packet,
    const uint16_t offset,
    PacketParserProtoType prototype)
{
    switch (prototype) {
    case IPV6_OPT_FRAGMENT:
        return processIPv6Fragment(packet, offset);
    case IPV6_OPT_HOPBYHOP:
        return processIPv6Extension<IPv6HopByHopView>(packet, offset);
    case IPV6_OPT_ROUTING:
        return processIPv6Extension<IPv6RoutingView>(packet, offset);
    case ESP:
        return processESP(packet, offset);
    case AUTHENTICATION_HEADER:
        return processAH(packet, offset);
    case IPV6_OPT_DESTINATION_OPTIONS:
        return processIPv6Extension<IPv6DestinationOptionsView>(packet, offset);
    case IPV6_OPT_GENERIC:
    case IPV6_MOBILITY:
    case IPV6_HOST_IDENTITY:
    case IPV6_SHIM6:
        return processIPv6Extension<IPv6ExtensionView>(packet, offset);
    case IPV6_NO_NEXT: {
        const auto data = packet.data.subspan(offset);
        if (!data.empty()) {
            processUnknown(packet, offset);
        }
        return LayerError::NONE;
    }
    case TCP:
        return processSimpleProtoType<TCPView, true>(packet, offset);
    case UDP:
        return processSimpleProtoType<UDPView, true>(packet, offset);
    case ICMP:
        return processSimpleProtoType<ICMPView, false>(packet, offset);
    case ICMPV6:
        return processSimpleProtoType<ICMPv6View, false>(packet, offset);
    case GRE:
        return processGRE(packet, offset);
    case IPV4_ENCAPSULATION:
        return processIPv4(packet, offset);
    case IPV6_ENCAPSULATION:
        return processIPv6(packet, offset);
    case RESERVED1:
    case RESERVED2:
        return processUnknown(packet, offset);
    }

    // PacketParserProtoType is simple enum, handle undefined values
    return processUnknown(packet, offset);
}

LayerError
PacketParser::processPayload(Packet& packet, const uint16_t offset, LayerType payloadType)
{
    assert(payloadType == LayerType::PAYLOAD_APP || payloadType == LayerType::PAYLOAD_IP_FRAGMENT);

    if (!packet.data.subspan(offset).empty()) {
        if (packet.layers.full()) {
            return LayerError::LAYER_VECTOR_FULL;
        }
        PacketLayer layer {.type = payloadType, .offset = offset};
        packet.layers.pushBack(layer);
    }
    return LayerError::NONE;
}

LayerError PacketParser::processEthernet(Packet& packet, const uint16_t offset)
{
    auto ethOrError = addLayer<EthernetView>(packet, offset);
    if (!ethOrError) {
        return ethOrError.error();
    }
    return processEthertype(
        packet,
        offset + ethOrError->headerLength(),
        static_cast<PacketParserEtherType>(ethOrError->protocol()));
}

LayerError PacketParser::processVLAN(Packet& packet, const uint16_t offset)
{
    auto vlanOrError = addLayer<VLANView>(packet, offset);
    if (!vlanOrError) {
        return vlanOrError.error();
    }
    return processEthertype(
        packet,
        offset + vlanOrError->headerLength(),
        static_cast<PacketParserEtherType>(vlanOrError->protocol()));
}

LayerError
PacketParser::processMPLS(Packet& packet, const uint16_t offset, PacketParserEtherType ethertype)
{
    assert(ethertype == MPLS || ethertype == MPLS_MULTICAST);

    auto mplsOrError = addLayer<MPLSView>(packet, offset);
    if (!mplsOrError) {
        return mplsOrError.error();
    }
    if (mplsOrError->bottomOfStack()) {
        // it is expected IPv4 or IPv6 continuing
        return processIPFromPayload(packet, offset + mplsOrError->headerLength());
    }
    // propagating MPLS or MPLS_MULTICAST
    return processEthertype(packet, offset + mplsOrError->headerLength(), ethertype);
}

LayerError PacketParser::processIPFromPayload(Packet& packet, const uint16_t offset)
{
    const auto data = packet.data.subspan(offset);
    if (data.size() >= sizeof(IPv4Header) && (static_cast<uint8_t>(data[0]) >> 4) == 4) {
        return processIPv4(packet, offset);
    }
    if (data.size() >= sizeof(IPv6Header) && (static_cast<uint8_t>(data[0]) >> 4) == 6) {
        return processIPv6(packet, offset);
    }
    return processUnknown(packet, offset);
}

LayerError PacketParser::processIPv4(Packet& packet, const uint16_t offset)
{
    auto ipOrError = addLayer<IPv4View>(packet, offset);
    if (!ipOrError) {
        return ipOrError.error();
    }
    if (ipOrError->isFragmented() && ipOrError->fragmentOffset() > 0) {
        return processPayload(
            packet,
            offset + ipOrError->headerLength(),
            LayerType::PAYLOAD_IP_FRAGMENT);
    }
    return processPrototype(
        packet,
        offset + ipOrError->headerLength(),
        static_cast<PacketParserProtoType>(ipOrError->protocol()));
}

LayerError PacketParser::processIPv6(Packet& packet, const uint16_t offset)
{
    auto ipOrError = addLayer<IPv6View>(packet, offset);
    if (!ipOrError) {
        return ipOrError.error();
    }
    return processPrototype(
        packet,
        offset + ipOrError->headerLength(),
        static_cast<PacketParserProtoType>(ipOrError->nextHeader()));
}

template <typename LayerViewT>
LayerError PacketParser::processIPv6Extension(Packet& packet, const uint16_t offset)
{
    auto extOrError = addLayer<LayerViewT>(packet, offset);
    if (!extOrError) {
        return extOrError.error();
    }
    return processPrototype(
        packet,
        offset + extOrError->headerLength(),
        static_cast<PacketParserProtoType>(extOrError->nextHeader()));
}

LayerError PacketParser::processIPv6Fragment(Packet& packet, const uint16_t offset)
{
    auto fragOrError = addLayer<IPv6FragmentView>(packet, offset);
    if (!fragOrError) {
        return fragOrError.error();
    }
    if (fragOrError->fragmentOffset() > 0) {
        return processPayload(
            packet,
            offset + fragOrError->headerLength(),
            LayerType::PAYLOAD_IP_FRAGMENT);
    }
    return processPrototype(
        packet,
        offset + fragOrError->headerLength(),
        static_cast<PacketParserProtoType>(fragOrError->nextHeader()));
}

template <typename LayerViewT, bool payloadFollows>
LayerError PacketParser::processSimpleProtoType(Packet& packet, const uint16_t offset)
{
    auto viewOrError = addLayer<LayerViewT>(packet, offset);
    if (!viewOrError) {
        return viewOrError.error();
    }
    if constexpr (payloadFollows) {
        return processPayload(packet, offset + viewOrError->headerLength());
    } else {
        return LayerError::NONE;
    }
}

LayerError PacketParser::processGRE(Packet& packet, const uint16_t offset)
{
    auto greOrError = addLayer<GREView>(packet, offset);
    if (!greOrError) {
        return greOrError.error();
    }
    return processEthertype(
        packet,
        offset + greOrError->headerLength(),
        static_cast<PacketParserEtherType>(greOrError->protocolType()));
}

LayerError PacketParser::processESP(Packet& packet, uint16_t offset)
{
    auto espOrError = addLayer<ESPView>(packet, offset);
    if (!espOrError) {
        return espOrError.error();
    }
    return processUnknown(packet, offset + espOrError->headerLength());
}

LayerError PacketParser::processAH(Packet& packet, uint16_t offset)
{
    auto ahOrError = addLayer<AuthenticationHeaderView>(packet, offset);
    if (!ahOrError) {
        return ahOrError.error();
    }
    return processPrototype(
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

TEST_CASE("PacketParser simple packet parsing")
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
    PacketParser parser;
    parser.parse(packet, LayerType::ETHERNET);

    CHECK_EQ(std::get<PacketLayer>(packet.layers[0]).type, LayerType::ETHERNET);
    CHECK_EQ(std::get<PacketLayer>(packet.layers[0]).offset, 0);

    CHECK_EQ(std::get<PacketLayer>(packet.layers[1]).type, LayerType::IPV4);
    CHECK_EQ(std::get<PacketLayer>(packet.layers[1]).offset, 14);

    CHECK_EQ(std::get<PacketLayer>(packet.layers[2]).type, LayerType::UDP);
    CHECK_EQ(std::get<PacketLayer>(packet.layers[2]).offset, 34);

    CHECK_EQ(std::get<PacketLayer>(packet.layers.back()).type, LayerType::PAYLOAD_APP);
    CHECK_EQ(std::get<PacketLayer>(packet.layers.back()).offset, 42);
}

TEST_CASE("PacketParser IP in IP parsing")
{
    SUBCASE("IPv4 in IPv4")
    {
        unsigned char frame[]
            = {0xca, 0x1,  0xb,  0x14, 0x0,  0x8,  0xca, 0x0,  0xb,  0x14, 0x0,  0x8,  0x8,  0x0,
               0x45, 0x0,  0x0,  0x78, 0x0,  0xf,  0x0,  0x0,  0xff, 0x4,  0xa3, 0x6e, 0xa,  0x1,
               0x2,  0x1,  0xa,  0x1,  0x2,  0x2,  0x45, 0x0,  0x0,  0x64, 0x0,  0xf,  0x0,  0x0,
               0xff, 0x1,  0x3a, 0x36, 0xc0, 0xa8, 0x0,  0x1,  0xc0, 0xa8, 0x0,  0x2,  0x8,  0x0,
               0x2d, 0x16, 0x0,  0x3,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0xd,  0x51, 0x24,
               0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd,
               0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd,
               0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd,
               0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd,
               0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd, 0xab, 0xcd};

        Packet packet;
        packet.data = {reinterpret_cast<std::byte*>(frame), sizeof(frame)};
        PacketParser parser;
        parser.parse(packet, LayerType::ETHERNET);

        CHECK_EQ(std::get<PacketLayer>(packet.layers[0]).type, LayerType::ETHERNET);
        CHECK_EQ(std::get<PacketLayer>(packet.layers[0]).offset, 0);

        CHECK_EQ(std::get<PacketLayer>(packet.layers[1]).type, LayerType::IPV4);
        CHECK_EQ(std::get<PacketLayer>(packet.layers[1]).offset, 14);

        CHECK_EQ(std::get<PacketLayer>(packet.layers[2]).type, LayerType::IPV4);
        CHECK_EQ(std::get<PacketLayer>(packet.layers[2]).offset, 34);

        CHECK_EQ(std::get<PacketLayer>(packet.layers[3]).type, LayerType::ICMP);
        CHECK_EQ(std::get<PacketLayer>(packet.layers[3]).offset, 54);
    }

    SUBCASE("IPv6 in IPv4")
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

        CHECK_EQ(std::get<PacketLayer>(packet.layers[0]).type, LayerType::ETHERNET);
        CHECK_EQ(std::get<PacketLayer>(packet.layers[0]).offset, 0);

        CHECK_EQ(std::get<PacketLayer>(packet.layers[1]).type, LayerType::VLAN);
        CHECK_EQ(std::get<PacketLayer>(packet.layers[1]).offset, 14);

        CHECK_EQ(std::get<PacketLayer>(packet.layers[2]).type, LayerType::IPV4);
        CHECK_EQ(std::get<PacketLayer>(packet.layers[2]).offset, 18);

        CHECK_EQ(std::get<PacketLayer>(packet.layers[3]).type, LayerType::IPV6);
        CHECK_EQ(std::get<PacketLayer>(packet.layers[3]).offset, 38);

        CHECK_EQ(std::get<PacketLayer>(packet.layers[4]).type, LayerType::TCP);
        CHECK_EQ(std::get<PacketLayer>(packet.layers[4]).offset, 78);
    }
}

TEST_CASE("PacketParser SYN TCP parsing")
{
    unsigned char frame[]
        = {0x0,  0xe,  0xc,  0xc0, 0xff, 0xee, 0x0,  0x19, 0x30, 0xc0, 0xff, 0xee, 0x8,  0x0,
           0x45, 0x0,  0x0,  0x28, 0x1,  0x0,  0x0,  0x0,  0x73, 0x6,  0xc0, 0x5c, 0xa4, 0x7c,
           0x21, 0x4e, 0xc0, 0xa8, 0x0,  0x1,  0x89, 0x5d, 0x0,  0x50, 0x0,  0x0,  0x63, 0x3f,
           0x0,  0x0,  0x0,  0x0,  0x50, 0x2,  0x40, 0x0,  0xfc, 0x81, 0x0,  0x0};

    Packet packet;
    packet.data = {reinterpret_cast<std::byte*>(frame), sizeof(frame)};
    PacketParser parser;
    parser.parse(packet, LayerType::ETHERNET);

    CHECK_EQ(std::get<PacketLayer>(packet.layers[0]).type, LayerType::ETHERNET);
    CHECK_EQ(std::get<PacketLayer>(packet.layers[0]).offset, 0);

    CHECK_EQ(std::get<PacketLayer>(packet.layers[1]).type, LayerType::IPV4);
    CHECK_EQ(std::get<PacketLayer>(packet.layers[1]).offset, 14);

    CHECK_EQ(std::get<PacketLayer>(packet.layers.back()).type, LayerType::TCP);
    CHECK_EQ(std::get<PacketLayer>(packet.layers.back()).offset, 34);

    CHECK_EQ(packet.layers.size(), 3);
}

} // namespace amon
