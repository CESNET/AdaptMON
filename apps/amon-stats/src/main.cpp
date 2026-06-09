/**
 * @file
 * @author Petr Macha <petr.macha@cesnet.cz>
 * @brief PCAP statistics tool
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 */

#include <amon/Error.hpp>
#include <amon/Logger.hpp>
#include <amon/PacketParser.hpp>
#include <amon/amon.hpp>
#include <amon/util/Pcap.hpp>

#include <string>
#include <vector>

static constexpr amon::layers::LayerType dltToLayerType(int dataLinkType)
{
    switch (dataLinkType) {
    case DLT_EN10MB: {
        return amon::layers::LayerType::ETHERNET;
    }
    case DLT_IPV4: {
        return amon::layers::LayerType::IPV4;
    }
    case DLT_IPV6: {
        return amon::layers::LayerType::IPV6;
    }
    case DLT_MPLS: {
        return amon::layers::LayerType::MPLS;
    }
    case DLT_RAW: {
        return amon::layers::LayerType::RAW;
    }
    default: {
        throw amon::Error("Unsupported data link type {}", dataLinkType);
    }
    }
}

int main(int argc, char** argv) // NOLINT(bugprone-exception-escape)
{
    const amon::Logger logger = amon::LoggerProvider::get("main");
    if (argc != 2) {
        logger.critical("Invalid number of parameters (expects 2)");
        return 1;
    }
    const std::string filePath = argv[1];

    amon::util::PcapReader reader = amon::util::PcapReader::fromFile(filePath);

    const amon::PacketParser parser;
    std::vector<std::byte> buffer;

    auto linkLayer = dltToLayerType(reader.linkLayerType());

    uint count = 0;
    while (true) {
        std::optional<amon::util::PcapPacket> pcapPacket = reader.readPacket();
        if (!pcapPacket) {
            if (reader.eof()) {
                break;
            }
            continue;
        }

        logger.debug("Count: {0}", ++count);

        amon::Packet packet;

        buffer.assign(pcapPacket->data.begin(), pcapPacket->data.end());
        packet.data = buffer;

        parser.parse(packet, linkLayer);

        for (auto& currentLayer : packet.layers) {
            auto layer = amon::match(
                currentLayer,
                [&](const amon::PacketLayer& packetLayer) { return packetLayer.type; },
                [&](const amon::ErrorLayer& error) { return error.type; });
            logger.debug("Layer: {0}", layer);
        }
    }
}
