/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Simple packet parser benchmark.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 */

#include <amon/PacketParser.hpp>
#include <amon/util/ArgParser.hpp>
#include <amon/util/Pcap.hpp>

#include <chrono>
#include <print>
#include <string>

namespace {
std::vector<std::vector<std::byte>> readPktsIntoMemory(const std::string& pcapFile)
{
    auto pcapReader = amon::util::PcapReader::fromFile(pcapFile);
    std::vector<std::vector<std::byte>> pkts;

    while (auto pkt = pcapReader.readPacket()) {
        pkts.emplace_back(pkt->data.begin(), pkt->data.end());
    }

    return pkts;
}

class BurstProvider {
public:
    BurstProvider(std::vector<std::vector<std::byte>>& pkts)
        : m_pkts(pkts)
        , m_end(m_pkts.end())
        , m_it(m_pkts.begin())
    {
    }

    void getNextBurst(std::span<std::span<std::byte>> burst)
    {
        if (static_cast<size_t>(std::distance(m_it, m_end)) > burst.size()) [[likely]] {
            for (auto& item : burst) {
                item = *(m_it++);
            }
        } else {
            for (auto& item : burst) {
                if (m_it == m_end) [[unlikely]] {
                    m_it = m_pkts.begin();
                }
                item = *(m_it++);
            }
        }
    }

private:
    std::vector<std::vector<std::byte>>& m_pkts;
    std::vector<std::vector<std::byte>>::iterator m_end;
    std::vector<std::vector<std::byte>>::iterator m_it;
};
} // namespace

int main(int argc, char** argv) // NOLINT
{
    std::locale::global(std::locale(""));

    static const auto argParser = amon::util::ArgParser(
        {amon::util::PosArg("INPUT_FILE").required(true).help("The input file")},
        {});

    auto args = argParser.parse(argc, argv);

    std::println(
        "loading packets from '{}' ...",
        args["INPUT_FILE"].value()); // NOLINT(bugprone-unchecked-optional-access)
    auto pkts = readPktsIntoMemory(
        args["INPUT_FILE"].value()); // NOLINT(bugprone-unchecked-optional-access)

    std::println("loaded {} packets", pkts.size());

    ////////////////////////////////////////////////////////////////////////////////

    uint64_t nProcessed = 0;
    uint64_t totalBytes = 0;
    uint64_t ipv4Pkts = 0;
    uint64_t ipv6Pkts = 0;
    uint64_t tcpPkts = 0;
    uint64_t udpPkts = 0;
    uint64_t errPkts = 0;
    auto startTime = std::chrono::steady_clock::now();

    std::array<std::span<std::byte>, 64> burst;
    auto burstProvider = BurstProvider {pkts};

    for (uint64_t i = 0; i < 2'000'000; i++) {
        burstProvider.getNextBurst(burst);

        amon::PacketParser parser;
        for (const auto& pktData : burst) {
            amon::Packet packet;
            packet.data = pktData;
            auto res = parser.parse(packet, amon::layers::LayerType::ETHERNET);
            if (res == amon::layers::LayerError::NONE) {
                if (auto l3 = packet.layout[0].l3) {
                    auto type = std::get<amon::PacketLayer>(packet.layers[l3.value()]).type;
                    if (type == amon::layers::LayerType::IPV4) {
                        ++ipv4Pkts;
                    } else if (type == amon::layers::LayerType::IPV6) {
                        ++ipv6Pkts;
                    }
                }
                if (auto l4 = packet.layout[0].l4) {
                    auto type = std::get<amon::PacketLayer>(packet.layers[l4.value()]).type;
                    if (type == amon::layers::LayerType::TCP) {
                        ++tcpPkts;
                    } else if (type == amon::layers::LayerType::UDP) {
                        ++udpPkts;
                    }
                }
            } else {
                ++errPkts;
            }

            totalBytes += pktData.size();
            ++nProcessed;
        }

        if (i % (1 << 14) == 0) [[unlikely]] {
            auto endTime = std::chrono::steady_clock::now();
            double secsElapsed = std::chrono::duration<double>(endTime - startTime).count();
            double pktsPerSec = static_cast<double>(nProcessed) / secsElapsed;

            std::println(
                "processed {:L} packets in {:.2Lf} sec ({:.2Lf} pkts/sec)",
                nProcessed,
                secsElapsed,
                pktsPerSec);
        }
    }

    auto endTime = std::chrono::steady_clock::now();
    double secsElapsed = std::chrono::duration<double>(endTime - startTime).count();
    double pktsPerSec = static_cast<double>(nProcessed) / secsElapsed;

    std::println("=======================================");
    std::println("total bytes: {}", totalBytes);
    std::println("ipv4 pkts: {}", ipv4Pkts);
    std::println("ipv6 pkts: {}", ipv6Pkts);
    std::println("tcp pkts: {}", tcpPkts);
    std::println("udp pkts: {}", udpPkts);
    std::println("err pkts: {}", errPkts);
    std::println(
        "processed {:L} packets in {:.2Lf} sec ({:.2Lf} pkts/sec)",
        nProcessed,
        secsElapsed,
        pktsPerSec);
}
