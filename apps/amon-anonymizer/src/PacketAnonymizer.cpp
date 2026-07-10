/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Packet anonymizer class
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 */

#include "PacketAnonymizer.hpp"

#include <amon/Error.hpp>
#include <amon/amon.hpp>
#include <amon/layers/AuthenticationHeader.hpp>
#include <amon/layers/ESP.hpp>
#include <amon/layers/Ethernet.hpp>
#include <amon/layers/ICMP.hpp>
#include <amon/layers/ICMPv6.hpp>
#include <amon/layers/IPv4.hpp>
#include <amon/layers/IPv6.hpp>
#include <amon/layers/TCP.hpp>
#include <amon/layers/UDP.hpp>

static constexpr uint8_t IPV6_ROUTING_TYPE_SEGMENT_ROUTING = 4;

PacketAnonymizer::PacketAnonymizer(
    const std::string& anonymizationKey,
    amon::layers::LayerType firstLayerType,
    Options options)
    : m_valueAnon(anonymizationKey)
    , m_firstLayerType(firstLayerType)
    , m_options(options)
{
}

template <typename T>
static std::pair<T&, std::span<std::byte>> take(std::span<std::byte> data)
{
    AMON_ASSERT(
        sizeof(T) <= data.size(),
        "data too short (sizeof(T)={}, data.size()={})",
        sizeof(T),
        data.size());
    return {*reinterpret_cast<T*>(data.data()), data.subspan(sizeof(T))};
}

void PacketAnonymizer::anonymizeIPv6SegmentRouting(std::span<std::byte> data)
{
    struct SegmentInfo {
        amon::types::UInt8 lastEntry;
        amon::types::UInt8 flags;
        amon::types::UInt16BE tag;
    };
    auto [info, rest] = take<SegmentInfo>(data);
    if (rest.size() < info.lastEntry * sizeof(amon::types::IPv6)) {
        // LastEntry value is invalid... Anonymize everything
        for (size_t i = 0; i < info.lastEntry; i++) {
            auto [ipv6, nextRest] = take<amon::types::IPv6>(rest);
            m_valueAnon.anonymize(ipv6);
            rest = nextRest;
        }
    }
    m_valueAnon.anonymize(rest);
}

void PacketAnonymizer::anonymizeLayer(
    std::span<std::byte> data,
    amon::layers::LayerType layerType,
    bool& outTruncateFlag)
{
    outTruncateFlag = false;

    switch (layerType) {
    case amon::layers::LayerType::ETHERNET: {
        auto [ethHdr, rest] = take<amon::layers::EthernetHeader>(data);
        m_valueAnon.anonymize(ethHdr.source);
        m_valueAnon.anonymize(ethHdr.destination);
        m_valueAnon.anonymize(rest);
    } break;

    case amon::layers::LayerType::VLAN: {
        m_stats.nVLAN++;
    } break;

    case amon::layers::LayerType::MPLS: {
        m_stats.nMPLS++;
    } break;

    case amon::layers::LayerType::GRE: {
        m_stats.nGRE++;
    } break;

    case amon::layers::LayerType::TCP: {
        auto [tcpHdr, rest] = take<amon::layers::TCPHeader>(data);
        tcpHdr.checksum = 0;
        m_valueAnon.anonymize(rest);
        m_stats.nTCP++;
    } break;

    case amon::layers::LayerType::UDP: {
        auto [udpHdr, rest] = take<amon::layers::UDPHeader>(data);
        udpHdr.checksum = 0;
        m_valueAnon.anonymize(rest);
        m_stats.nUDP++;
    } break;

    case amon::layers::LayerType::IPV4: {
        auto [ipv4Hdr, rest] = take<amon::layers::IPv4Header>(data);
        m_valueAnon.anonymize(ipv4Hdr.sourceAddress);
        m_valueAnon.anonymize(ipv4Hdr.destinationAddress);
        ipv4Hdr.headerChecksum = 0;
        m_valueAnon.anonymize(rest);
        m_stats.nIPv4++;
    } break;

    case amon::layers::LayerType::IPV6: {
        auto [ipv6Hdr, rest] = take<amon::layers::IPv6Header>(data);
        m_valueAnon.anonymize(ipv6Hdr.sourceAddress);
        m_valueAnon.anonymize(ipv6Hdr.destinationAddress);
        m_valueAnon.anonymize(rest);
        m_stats.nIPv6++;
    } break;

    case amon::layers::LayerType::IPV6_OPT_FRAGMENT: {
        auto [_, rest] = take<amon::layers::IPv6FragmentHeader>(data);
        m_valueAnon.anonymize(rest);
    } break;

    case amon::layers::LayerType::IPV6_OPT_ROUTING: {
        auto [ipv6RoutingHdr, rest] = take<amon::layers::IPv6RoutingHeader>(data);
        if (ipv6RoutingHdr.routingType == IPV6_ROUTING_TYPE_SEGMENT_ROUTING) {
            anonymizeIPv6SegmentRouting(rest);
        } else {
            m_valueAnon.anonymize(rest);
        }
    } break;

    case amon::layers::LayerType::IPV6_OPT_HOPBYHOP:
    case amon::layers::LayerType::IPV6_OPT_DESTINATION_OPTIONS:
    case amon::layers::LayerType::IPV6_OPT_GENERIC: {
        auto [_, rest] = take<amon::layers::IPv6ExtensionHeader>(data);
        m_valueAnon.anonymize(rest);
    } break;

    case amon::layers::LayerType::ICMP: {
        auto [icmpHdr, rest] = take<amon::layers::ICMPHeader>(data);
        icmpHdr.checksum = 0;
        m_valueAnon.anonymize(rest);
        m_stats.nICMP++;
    } break;

    case amon::layers::LayerType::ICMPV6: {
        auto [icmpv6Hdr, rest] = take<amon::layers::ICMPv6Header>(data);
        icmpv6Hdr.checksum = 0;
        m_valueAnon.anonymize(rest);
        m_stats.nICMPv6++;
    } break;

    case amon::layers::LayerType::AUTHENTICATION_HEADER: {
        auto [ahHdr, rest] = take<amon::layers::AuthenticationHeader>(data);
        m_valueAnon.anonymize(rest);
    } break;

    case amon::layers::LayerType::ESP: {
        auto [espHdr, rest] = take<amon::layers::ESPHeader>(data);
        m_valueAnon.anonymize(rest);
        m_stats.nESP++;
    } break;

    default: {
        if (m_options.truncate) {
            outTruncateFlag = true;
        } else {
            m_valueAnon.anonymize(data);
        }
    } break;
    }
}

static uint16_t getLayerOffset(const amon::LayerVariant& layerV)
{
    return amon::match(layerV, [](const auto& layer) { return layer.offset; });
}

static std::span<std::byte> getLayerData(amon::Packet& pkt, size_t idx)
{
    AMON_ASSERT(
        idx < pkt.layers.size(),
        "layer index out of range (idx={}, pkt.layers.size()={})",
        idx,
        pkt.layers.size());

    const size_t start = getLayerOffset(pkt.layers[idx]);
    const size_t end
        = idx + 1 < pkt.layers.size() ? getLayerOffset(pkt.layers[idx + 1]) : pkt.data.size();
    return pkt.data.subspan(start, end - start);
}

bool PacketAnonymizer::anonymizePacket(std::vector<std::byte>& packetData)
{
    amon::Packet packet;
    packet.data = {packetData.begin(), packetData.end()};

    m_stats.nPackets++;
    m_stats.nBytes += packet.data.size();
    m_stats.updatePacketSizeHistogram(packet.data.size());

    const amon::layers::LayerError err = m_parser.parse(packet, m_firstLayerType);
    if (err != amon::layers::LayerError::NONE) {
        m_stats.nErrPackets++;
        if (m_options.skipErroneous) {
            return false;
        }
    }

    for (size_t i = 0; i < packet.layers.size(); i++) {
        const auto layerData = getLayerData(packet, i);
        bool truncateFlag = false;

        amon::match(
            packet.layers[i],
            [&](const amon::PacketLayer& layer) {
                anonymizeLayer(layerData, layer.type, truncateFlag);
            },
            [&](const amon::ErrorLayer&) {
                if (m_options.truncate) {
                    truncateFlag = true;
                } else {
                    m_valueAnon.anonymize(layerData);
                }
                m_stats.nErrPackets++;
            });

        if (truncateFlag) {
            const std::byte* pktBegin = &*packetData.begin();
            const std::byte* layerBegin = &*layerData.begin();
            // NOLINTNEXTLINE(readability-simplify-boolean-expr)
            AMON_REQUIRE(pktBegin <= layerBegin && layerBegin <= &*packetData.end());
            const auto truncatedSize = static_cast<size_t>(std::distance(pktBegin, layerBegin));
            if (truncatedSize == 0) {
                return false;
            }
            packetData.resize(truncatedSize);
            break;
        }
    }

    return true;
}
