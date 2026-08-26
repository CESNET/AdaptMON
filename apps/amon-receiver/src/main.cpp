/**
 * @file
 * @author Samuel Luptak <luptak@cesnet.cz>
 * @brief Packet receiver tool
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 */

#include "Config.hpp"
#include "printers/OutputPrinter.hpp"
#include "printers/TerminalPrinter.hpp"
#include "stats/Stats.hpp"
#include "stats/StatsManager.hpp"

#include <amon/Error.hpp>
#include <amon/Logger.hpp>
#include <amon/Packet.hpp>
#include <amon/PacketParser.hpp>
#include <amon/SignalHandler.hpp>
#include <amon/dpdk/EAL.hpp>
#include <amon/dpdk/GlobalLCorePool.hpp>
#include <amon/dpdk/PktBurst.hpp>
#include <amon/dpdk/PktMempool.hpp>
#include <amon/dpdk/Port.hpp>
#include <amon/layers/Layer.hpp>
#include <amon/linuxos/Hugepages.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <limits>
#include <memory>
#include <span>

#include <generic/rte_cycles.h>

static uint64_t extractL3Bytes(const amon::PacketParser& parser, rte_mbuf* mbuf)
{
    amon::Packet packet;
    packet.data = {rte_pktmbuf_mtod(mbuf, std::byte*), rte_pktmbuf_pkt_len(mbuf)};

    parser.parse(packet, amon::layers::LayerType::ETHERNET);

    if (packet.layout.empty()) {
        return 0;
    }
    const auto& outerLayout = packet.layout[0];
    if (!outerLayout.l3) {
        return 0;
    }
    const auto layerVariant = packet.getLayer(*outerLayout.l3);
    if (!layerVariant) {
        return 0;
    }
    const auto* layerInfo = std::get_if<amon::PacketLayer>(&*layerVariant);
    if (layerInfo == nullptr) {
        return 0;
    }
    return packet.data.size() - static_cast<uint32_t>(layerInfo->offset);
}

static void workerThread(
    amon::dpdk::PortRxQueue queue,
    StatsTimestampedAtomic& atomicCounters,
    const Config& config,
    const std::atomic_bool& stopFlag)
{
    const amon::PacketParser parser;
    amon::dpdk::PktBurst<rte_mbuf> burst(config.burstSize);
    StatsTimestamped localStats;

    // Update localStats two times in one display interval (statsInterval)
    auto cycleInterval = static_cast<uint64_t>(
        (static_cast<double>(rte_get_tsc_hz()) * (config.statsInterval / 1000.0)) / 2);
    uint64_t oldCycles = rte_get_tsc_cycles();
    uint64_t newCycles;

    while (!stopFlag) {
        queue.receive(burst);

        uint64_t l2Bytes = 0;
        uint64_t l3Bytes = 0;
        for (auto& dpdkPacket : burst) {
            l2Bytes += rte_pktmbuf_pkt_len(dpdkPacket);

            if (config.enableParser) {
                l3Bytes += extractL3Bytes(parser, dpdkPacket);
            }
        }
        newCycles = rte_get_tsc_cycles();

        localStats.stats.l2Bytes += l2Bytes;
        localStats.stats.l3Bytes += l3Bytes;
        localStats.stats.pkts += burst.size();

        if (newCycles - oldCycles > cycleInterval) {
            localStats.stats.l1Bytes = localStats.stats.l2Bytes + (localStats.stats.pkts * 24);
            localStats.cycles = newCycles;
            atomicCounters.store(localStats);

            oldCycles = newCycles;
        }
        burst.clear();
    }

    // Final flush on exit
    localStats.stats.l1Bytes = localStats.stats.l2Bytes + (localStats.stats.pkts * 24);
    localStats.cycles = rte_get_tsc_cycles();
    atomicCounters.store(localStats);
}

static void statThread(
    StatsManager& statsManager,
    const uint32_t statsInterval,
    const std::atomic_bool& stopFlag)
{
    while (!stopFlag) {
        statsManager.updateAndPrintStats();

        // Convert to NS
        rte_delay_us_sleep(statsInterval * 1000);
    }

    statsManager.updateAndPrintStats();
    statsManager.printSummary();
}

static amon::dpdk::SocketID getDeviceSocket(std::string_view devName)
{
    return amon::dpdk::PortHandle::fromDevName(devName).socketId();
}

static std::shared_ptr<amon::dpdk::PktMempoolBasicProvider>
setupMempoolProvider(unsigned mempoolSize, const amon::dpdk::SocketID socketId)
{
    const amon::dpdk::PktMempoolConfig mempoolConfig {
        .nPackets = mempoolSize,
        .cacheSize = 256,
        .privateDataSize = 0,
        .dataRoomSize = 9400,
        .socketId = socketId};

    return std::make_shared<amon::dpdk::PktMempoolBasicProvider>(mempoolConfig);
}

static uint16_t determineQueueCnt(const Config& config, amon::dpdk::LCorePool& corePool)
{
    auto port = amon::dpdk::PortHandle::fromDevName(config.devName);

    const uint16_t maxQueueCnt = port.devInfo().rx.nQueuesMax;
    auto availableNUMACores
        = amon::util::numeric::safeCast<uint16_t>(corePool.size(port.socketId()));
    if (availableNUMACores <= 1) {
        throw amon::Error(
            "insufficient available cores on this NIC's NUMA node. Maybe wrong -l eal parameter "
            "set");
    }
    // One is subtracted because one core must remain available for the statistics thread
    availableNUMACores -= 1;
    uint16_t queueCnt;

    queueCnt = std::min(maxQueueCnt, availableNUMACores);
    if (config.rxQueues != 0) {
        if (config.rxQueues > queueCnt) {
            throw amon::Error(
                "requested {} RX queues exceeds the supported maximum of {}: NIC supports up to {} "
                "queues and {} cores are available on the NUMA node for RX transmition",
                config.rxQueues,
                queueCnt,
                maxQueueCnt,
                availableNUMACores);
        }
        queueCnt = config.rxQueues;
    }

    return queueCnt;
}

static amon::dpdk::PortHandle configureDevicePort(
    const Config& config,
    const uint16_t queueCnt,
    const std::shared_ptr<amon::dpdk::PktMempoolBasicProvider>& mempoolProvider)
{
    auto port = amon::dpdk::PortHandle::fromDevName(config.devName);

    const uint16_t desiredMTU = 9200;
    uint16_t desiredDesc;
    try {
        desiredDesc = amon::util::numeric::safeCast<uint16_t>(config.mempoolSize / 2);
    } catch (const std::exception& ex) {
        desiredDesc = std::numeric_limits<uint16_t>::max();
    }

    const auto& portInfo = port.devInfo();
    amon::dpdk::PortConfig portConfig;
    portConfig.rx
        = {.nQueues = queueCnt,
           .nDescriptors
           = std::clamp(desiredDesc, portInfo.rx.nDescriptorsMin, portInfo.rx.nDescriptorsMax),
           .mempoolProvider = mempoolProvider};
    portConfig.mtu = std::clamp(desiredMTU, portInfo.mtuMin, portInfo.mtuMax);

    port.configure(portConfig);

    return port;
}

static void run(const Config& config)
{
    // Mempool setup
    const amon::dpdk::SocketID socketId = getDeviceSocket(config.devName);
    auto mempoolProvider = setupMempoolProvider(config.mempoolSize, socketId);

    // LCorePool setup
    auto corePool = amon::dpdk::GlobalLCorePool::create();

    // Port setup
    const uint16_t queueCnt = determineQueueCnt(config, *corePool);
    auto port = configureDevicePort(config, queueCnt, mempoolProvider);

    // StatsManager setup
    std::unique_ptr<OutputPrinter> printer = std::make_unique<TerminalPrinter>(config.enableParser);
    StatsManager statsManager(queueCnt, std::move(printer));

    // Stats worker init
    auto statsWorker = corePool->acquire(port.socketId());
    const auto statFunction = [&](const std::atomic_bool& stopFlag) -> void {
        statThread(statsManager, config.statsInterval, stopFlag);
    };
    statsWorker.start(statFunction);

    // Receive workers init
    std::vector<amon::dpdk::LCoreHandle> receiveWorkers;
    receiveWorkers.reserve(queueCnt);
    for (uint16_t i = 0; i < queueCnt; i++) {
        auto tmpHandle = corePool->acquire(port.socketId());

        const auto receiveFunction = [&, i](const std::atomic_bool& stopFlag) -> void {
            workerThread(port.rxQueue(i), statsManager.getAtomic(i), config, stopFlag);
        };
        tmpHandle.start(receiveFunction);

        receiveWorkers.emplace_back(std::move(tmpHandle));
    }

    port.up();

    // Shutdown
    amon::SignalHandler::waitForSignals();
    for (auto& receiveWorker : receiveWorkers) {
        receiveWorker.requestStop();
    }
    for (auto& receiveWorker : receiveWorkers) {
        receiveWorker.join();
    }

    statsWorker.requestStop();
    statsWorker.join();
}

int main(int argc, char** argv)
{
    auto logger = amon::LoggerProvider::get("main");
    amon::SignalHandler::install();

    try {
        Config config(argc, argv);

        if (!config.noHugepagesSetup) {
            amon::linuxos::Hugepages::setup(amon::linuxos::Hugepages::PAGESIZE_1GB, 4);
        }

        const amon::dpdk::EAL eal(config.ealArgs);
        run(config);
    } catch (const amon::Error& ex) {
        logger.critical("{}", ex.what());
        return EXIT_FAILURE;
    } catch (const std::exception& ex) {
        logger.critical("{}", ex.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
