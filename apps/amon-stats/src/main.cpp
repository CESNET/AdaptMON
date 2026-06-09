/**
 * @file
 * @author Petr Macha <petr.macha@cesnet.cz>
 * @brief PCAP statistics tool
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 */

#include "LayerCounter.hpp"

#include <amon/Error.hpp>
#include <amon/Logger.hpp>
#include <amon/PacketParser.hpp>
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
    LayerCounter counter;
    std::vector<std::byte> buffer;

    auto linkLayer = dltToLayerType(reader.linkLayerType());

    while (true) {
        // packet reading
        std::optional<amon::util::PcapPacket> pcapPacket = reader.readPacket();
        if (!pcapPacket) {
            if (reader.eof()) {
                break;
            }
            continue;
        }

        amon::Packet packet;

        buffer.assign(pcapPacket->data.begin(), pcapPacket->data.end());
        packet.data = buffer;

        parser.parse(packet, linkLayer);

        counter.countAll(packet);
    }

    counter.sort();
    counter.printNodes();
}
