/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief PCAP anonymization tool
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 */

#include "PacketAnonymizer.hpp"

#include <amon/Error.hpp>
#include <amon/SignalHandler.hpp>
#include <amon/amon.hpp>
#include <amon/util/ArgParser.hpp>
#include <amon/util/Human.hpp>
#include <amon/util/Pcap.hpp>

#include <atomic>
#include <print>
#include <random>
#include <string>

#include <pcap/dlt.h>

static std::atomic_bool g_stop;

static void stopSignalHandler(int /*sigNum*/) noexcept
{
    g_stop.store(true);
}

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

static std::string randomString(size_t length)
{
    constexpr static std::string_view charset
        = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<size_t> distrib(0, charset.size() - 1);

    std::string res;
    res.reserve(length);
    for (size_t i = 0; i < length; i++) {
        res.push_back(charset[distrib(generator)]);
    }

    return res;
}

static void run(const amon::util::ParsedArgs& args)
{
    // Args
    const std::string key = args["--key"].value_or(randomString(32));
    const std::string inputFile = *args["INPUT_FILE"]; // NOLINT(bugprone-unchecked-optional-access)
    const std::string outputFile
        = *args["OUTPUT_FILE"]; // NOLINT(bugprone-unchecked-optional-access)
    const bool skipErroneous = args["--skip-erroneous"].has_value();

    std::println("Using key: {}", key);
    std::println();

    // State
    std::vector<std::byte> buffer {};
    amon::util::PcapReader reader {inputFile};
    amon::util::PcapWriter writer {
        outputFile,
        amon::util::PcapTimestampPrecision::MICRO,
        reader.linkLayerType()};
    const amon::layers::LayerType firstLayer = dltToLayerType(reader.linkLayerType());
    PacketAnonymizer anon {key, firstLayer, skipErroneous};

    // Progress
    const uint64_t readerEndPos = reader.tellEnd();
    uint64_t nPackets = 0;

    auto printProgress = [&]() {
        const uint64_t readerCurrPos = reader.tell();
        const double pctDone = double(readerCurrPos) / double(readerEndPos) * 100.0;
        std::print(
            " :: Anonymized {} packets ({} / {}, {:.2f} %)\n",
            nPackets,
            amon::util::HumanReadableByteAmount(readerCurrPos),
            amon::util::HumanReadableByteAmount(readerEndPos),
            pctDone);
    };

    // Main loop
    while (true) {
        if (g_stop.load(std::memory_order_relaxed)) {
            std::println(std::cerr, "Stop signal received... Stopping early.");
            break;
        }

        const std::optional<amon::util::PcapPacket> pcapPacket = reader.readPacket();
        if (!pcapPacket) {
            break;
        }
        if (nPackets % 1000 == 0) {
            printProgress();
        }
        nPackets++;

        buffer.assign(pcapPacket->data.begin(), pcapPacket->data.end());
        const bool doWritePacket = anon.anonymizePacket(buffer);
        if (doWritePacket) {
            writer.writePacket(buffer, pcapPacket->timestamp);
        }
    }
    printProgress();

    // Finish
    std::println();
    anon.stats().print();
}

int main(int argc, char** argv) // NOLINT(bugprone-exception-escape)
{
    amon::SignalHandler::installBasic();
    amon::SignalHandler::installSigAction({SIGINT, SIGTERM}, &stopSignalHandler);

    using PosArg = amon::util::PosArg;
    using KwArg = amon::util::KwArg;
    static const auto argParser = amon::util::ArgParser(
        {
            PosArg("INPUT_FILE").help("The input file"),
            PosArg("OUTPUT_FILE").help("The output file"),
        },
        {
            KwArg('k', "key")
                .metaVar("KEY")
                .help("The anonymization key, 32 characters long (default: randomly generated)"),
            KwArg("skip-erroneous").help("Skip erroneous packets (default: false)"),
        });

    const amon::util::ParsedArgs args = argParser.parse(argc, argv);

    try {
        run(args);
    } catch (const amon::Error& err) {
        std::println(std::cerr, "ERROR: {}", err.what());
        return 1;
    }

    return 0;
}
