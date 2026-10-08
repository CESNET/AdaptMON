/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Example pipeline application.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 */

#include "DummyStage.hpp"

#include <amon/Logger.hpp>
#include <amon/SignalHandler.hpp>
#include <amon/dpdk/EAL.hpp>
#include <amon/dpdk/GlobalLCorePool.hpp>
#include <amon/dpdk/PktMempool.hpp>
#include <amon/dpdk/Port.hpp>
#include <amon/dpdk/pipeline/Pipeline.hpp>
#include <amon/dpdk/pipeline/PortReader.hpp>
#include <amon/dpdk/pipeline/PortWriter.hpp>
#include <amon/hello.hpp>
#include <amon/util/ArgParser.hpp>

#include <chrono>
#include <print>
#include <string>
#include <vector>

/* The example application sets-up the following architecture:
 *
 *
 *                    Pipeline "primary":
 *                    +-------------------------------------------+
 *                    |                                           |
 *     <net_pcap0> -- | [PortReader] --> [Dummy] --> [PortWriter] | -- <net_ring0>
 *                    |                                           |        .
 *                    +-------------------------------------------+        .
 *                                                                         .
 *         .................................................................
 *         .
 *         .          Pipeline "secondary":
 *         .          +-------------------------------------------+
 *         V          |                                           |
 *     <net_ring0> -- | [PortReader] --> [Dummy] --> [PortWriter] | -- <net_null>
 *                    |                                           |
 *                    +-------------------------------------------+
 *
 */

// NOLINTBEGIN(bugprone-exception-escape,bugprone-unchecked-optional-access)
int main(int argc, char** argv)
{
    /// Setup basic facilities
    amon::LoggerProvider::setGlobalLogLevel(amon::LoggerLevel::INFO);
    amon::SignalHandler::install();

    /// Parse args
    const auto argParser = amon::util::ArgParser {
        {
            amon::util::KwArg('i', "input-pcap")
                .metaVar("FILE")
                .defaultValue("input.pcap")
                .help("input pcap file"),
            amon::util::KwArg('t', "timeout")
                .metaVar("SECONDS")
                .defaultValue("5")
                .help("number of seconds to wait before terminating")
                .constraint(amon::util::ArgConstraint::number<unsigned>()),
        },
    }.setAutoHelp(true);
    const auto args = argParser.parse(argc, argv);

    /// Setup EAL
    std::vector<std::string> ealArgs {
        "hello-pipeline",
        "--no-huge",
        "--no-pci",
        "--vdev=net_pcap0,rx_pcap=" + *args["--input-pcap"],
        "--vdev=net_ring0",
        "--vdev=net_null",
    };
    const amon::dpdk::EAL eal {ealArgs};

    /// Setup ports
    auto portA = amon::dpdk::PortHandle::fromDevName("net_pcap0");
    auto portB = amon::dpdk::PortHandle::fromDevName("net_ring0");
    auto portC = amon::dpdk::PortHandle::fromDevName("net_null");

    portA.configure(amon::dpdk::PortConfig{
        .rx = amon::dpdk::PortConfig::Rx{
            .nQueues = 1,
            .nDescriptors = 128,
            .mempoolProvider = amon::dpdk::PktMempoolBasicProvider::create(amon::dpdk::PktMempoolConfig{
                .nPackets = 512,
                .privateDataSize = sizeof(amon::Packet),
                .dataRoomSize = 2000,
                .socketId = amon::dpdk::SocketID::ANY,
            }),
        },
        .mtu = 2000,
    });
    portB.configure(amon::dpdk::PortConfig{
        .rx = amon::dpdk::PortConfig::Rx{
            .nQueues = 4,
            .nDescriptors = 128,
            .mempoolProvider = amon::dpdk::PktMempoolBasicProvider::create(amon::dpdk::PktMempoolConfig{
                .nPackets = 512,
                .privateDataSize = sizeof(amon::Packet),
                .dataRoomSize = 2000,
                .socketId = amon::dpdk::SocketID::ANY,
            }),
        },
        .tx = amon::dpdk::PortConfig::Tx{
            .nQueues = 1,
            .nDescriptors = 128,
        },
        .mtu = 2000,
    });
    portC.configure(amon::dpdk::PortConfig{
        .tx = amon::dpdk::PortConfig::Tx{
            .nQueues = 4,
            .nDescriptors = 128,
        },
        .mtu = 2000,
    });

    /// Setup pipelines
    auto lcorePool = amon::dpdk::GlobalLCorePool::create();

    amon::dpdk::pipeline::Pipeline pipelinePrimary {
        amon::dpdk::pipeline::Blueprint {}
            .add<amon::dpdk::pipeline::PortReader>(portA)
            .add<DummyStage>()
            .add<amon::dpdk::pipeline::PortWriter>(portB),
        amon::dpdk::pipeline::PipelineConfig {
            .socketId = amon::dpdk::SocketID::ANY,
            .lcorePool = lcorePool,
            .numStreams = 1,
            .burstSize = 64,
            .name = "primary",
        },
    };

    amon::dpdk::pipeline::Pipeline pipelineSecondary {
        amon::dpdk::pipeline::Blueprint {}
            .add<amon::dpdk::pipeline::PortReader>(portB)
            .add<DummyStage>()
            .add<amon::dpdk::pipeline::PortWriter>(portC),
        amon::dpdk::pipeline::PipelineConfig {
            .socketId = amon::dpdk::SocketID::ANY,
            .lcorePool = lcorePool,
            .numStreams = 4,
            .burstSize = 64,
            .name = "secondary",
        },
    };

    pipelineSecondary.start();
    pipelinePrimary.start();

    /// Main loop
    const auto startTime = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::seconds(args.get<unsigned>("--timeout"));
    while (true) {
        const auto sig = amon::SignalHandler::waitForSignals(100);
        if (sig && (*sig == SIGINT || *sig == SIGTERM)) {
            std::println("signal {} received, exiting", *sig);
            break;
        }
        const auto elapsedTime = std::chrono::steady_clock::now() - startTime;
        if (elapsedTime >= timeout) {
            break;
        }
    }

    /// Finish
    pipelinePrimary.requestStop();
    pipelineSecondary.requestStop();
    pipelinePrimary.join();
    pipelineSecondary.join();

    /// Print stats
    const auto* dummyStagePrimary = dynamic_cast<DummyStage*>(pipelinePrimary.stages()[1].get());
    std::println("primary: packets processed: {}", dummyStagePrimary->pktsProcessedTotal());

    const auto* dummyStageSecondary
        = dynamic_cast<DummyStage*>(pipelineSecondary.stages()[1].get());
    std::println("secondary: packets processed: {}", dummyStageSecondary->pktsProcessedTotal());

    return 0;
}
// NOLINTEND(bugprone-exception-escape,bugprone-unchecked-optional-access)
