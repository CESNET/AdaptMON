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
#include <amon/util/ArgParser.hpp>
#include <amon/util/Pcap.hpp>
#include <amon/util/ansi.hpp>
#include <amon/util/string.hpp>

#include <condition_variable>
#include <print>
#include <string>
#include <thread>

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

static void printTimedWorkerLoop(
    const std::stop_token& stopToken,
    LayerCounter& counter,
    unsigned long printEvery)
{
    const amon::Logger logger = amon::LoggerProvider::get("printTimedWorkerLoop");
    if (printEvery == 0) {
        return;
    }

    std::mutex mut;
    std::condition_variable_any cv;
    std::unique_lock<std::mutex> lk(mut);
    while (!stopToken.stop_requested()) {
        try {
            counter.sort();

            const uint32_t lineCount = counter.printNodes();
            std::print("{}", amonAnsiMoveBackLines(lineCount));
        } catch (std::exception& error) {
            logger.error("Periodic printing failed: {}", error.what());
        }

        // wakes immediately on stop request; predicate guards spurious wakeups
        cv.wait_for(lk, stopToken, std::chrono::seconds(printEvery), [] { return false; });
    }
}

static void run(const amon::util::ParsedArgs& args)
{
    const amon::Logger logger = amon::LoggerProvider::get("run");
    const std::optional<std::string> outputPath = args["OUTPUT_FILE"];

    auto inputPath = args["--file"];
    auto inputInterface = args["--interface"];

    if (inputPath.has_value() && inputInterface.has_value()) {
        throw amon::Error(
            "Options --file and --interface are mutually exclusive. Specify only one.");
    }

    bool isInterface = false;

    auto reader = [&]() -> amon::util::PcapReader {
        if (inputPath.has_value()) {
            return amon::util::PcapReader::fromFile(inputPath.value());
        }
        if (inputInterface.has_value()) {
            isInterface = true;
            return amon::util::PcapReader::fromInterface(inputInterface.value());
        }
        throw amon::Error("No input specified. Use --file or --interface.");
    }();

    auto printEvery = amon::util::string::parse<uint32_t>(args["--print-every"].value_or("0"));

    const amon::PacketParser parser;
    const uint64_t readerEnd = reader.tellEnd();
    LayerCounter counter {readerEnd, isInterface};
    std::vector<std::byte> buffer;

    auto linkLayer = dltToLayerType(reader.linkLayerType());

    std::jthread printWorker(printTimedWorkerLoop, std::ref(counter), printEvery);

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

        counter.countAll(packet, reader.tell());
    }

    printWorker.request_stop();

    printWorker.join();

    counter.sort();

    if (outputPath.has_value()) {
        counter.writeToFile(outputPath.value());
    }

    counter.printNodes(false);
}

int main(int argc, char** argv) // NOLINT(bugprone-exception-escape)
{
    const amon::Logger logger = amon::LoggerProvider::get("main");
    static const auto argParser = amon::util::ArgParser(
        {
            amon::util::PosArg("OUTPUT_FILE")
                .required(false)
                .help("Path to the output file with YAML statistics"),
        },
        {
            amon::util::KwArg('f', "file").hasArg().help("Read packets from the given PCAP file"),
            amon::util::KwArg('i', "interface")
                .hasArg()
                .help("Capture packets live from the given network interface"),
            amon::util::KwArg('p', "print-every")
                .hasArg()
                .help("How often to print statistics, in seconds (default: don't print)"),
        });

    const amon::util::ParsedArgs args = argParser.parse(argc, argv);

    try {
        run(args);
    } catch (amon::Error& err) {
        logger.error("Error: {}", err.what());
        return 1;
    }

    return 0;
}
