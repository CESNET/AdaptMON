/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief DPDK Port handling.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/dpdk/dpdk.hpp>
#include <amon/Logger.hpp>
#include <amon/Packet.hpp>
#include <amon/amon.hpp>
#include <amon/dpdk/Error.hpp>
#include <amon/dpdk/Port.hpp>
#include <amon/dpdk/PortHooks.hpp>
#include <amon/dpdk/XStats.hpp>
#include <amon/types/Singleton.hpp>

#include <algorithm>
#include <initializer_list>
#include <random>
#include <unordered_map>

#include <rte_bus.h>
#include <rte_ethdev.h>

static constexpr unsigned RSS_KEY_MAX_LEN
    = 255; // length is stored as uint8_t, therefore it cannot exceed 255

namespace amon::dpdk {

///// Port ////
class Port {
public:
    Port(uint16_t portId);
    ~Port();
    void configure(const PortConfig& config, std::vector<std::unique_ptr<PortHooks>> hooks);
    SocketID socketId() const;
    void up();
    void down();
    PortTxQueue txQueue(uint16_t id);
    PortRxQueue rxQueue(uint16_t id);
    XStats xstats();

private:
    struct Configured {
        PortConfig config;
        PortHooksDispatcher dispatcher;
    };

    uint16_t m_portId;
    Logger m_logger;
    std::mutex m_mutex;
    std::optional<Configured> m_configured;

    void adjustRxTxDesc(uint16_t& nRxDesc, uint16_t& nTxDesc);
    void devConfigure(
        const PortHooksDispatcher& dispatcher,
        const PortConfig& config,
        uint16_t& nRxQueues,
        uint16_t& nTxQueues);
    void setupQueues(
        const PortHooksDispatcher& dispatcher,
        const PortConfig& config,
        uint16_t nRxQueues,
        uint16_t nTxQueues,
        uint16_t nRxDesc,
        uint16_t nTxDesc);
};

static std::string portLoggerName(uint16_t portId)
{
    const rte_eth_dev_info devInfo = ethDevInfoGet(portId);
    return std::format("Port(id={}, dev={})", portId, rte_dev_name(devInfo.device));
}

Port::Port(uint16_t portId)
    : m_portId(portId)
    , m_logger(LoggerProvider::get(portLoggerName(portId)))
{
    if (rte_eth_dev_is_valid_port(m_portId) != 1) {
        throw DpdkError("invalid port id {}", portId);
    }
}

Port::~Port()
{
    if (m_configured) {
        m_logger.info("Stopping eth dev");
        try {
            ethDevStop(m_portId);
        } catch (const std::exception& ex) { // NOLINT
            // Cannot throw in destructor
            m_logger.error("Port::~Port() failed: {}", ex.what());
        }
    }
}

static void genSymmetricRssKey(std::span<uint8_t> keyBuf)
{
    // Generate symmetric key. This is a special key that causes (srcip, dstip) map to the same
    // queue id as (dstip, srcip) when the toeplitz algorithm is used.
    size_t i = 0;
    for (auto& b : keyBuf) {
        b = (i++ % 2 == 0) ? 0x6D : 0x5A;
    }
}

static void genRandomRssKey(std::span<uint8_t> keyBuf)
{
    std::random_device rd;
    for (auto& b : keyBuf) {
        b = static_cast<uint8_t>(rd());
    }
}

static void setRssConf(
    const rte_eth_dev_info& devInfo,
    rte_eth_conf& conf,
    std::array<uint8_t, RSS_KEY_MAX_LEN>& keyBuf,
    Logger& logger)
{
    struct HashFunctionOption {
        rte_eth_hash_function id;
        std::function<void(std::span<uint8_t>)> keyGen;
    };

    // RSS algorithm options sorted by preference (first one = most preferrable).
    static const auto hfOptions = std::initializer_list<HashFunctionOption> {
        {.id = RTE_ETH_HASH_FUNCTION_SYMMETRIC_TOEPLITZ_SORT, .keyGen = genRandomRssKey},
        {.id = RTE_ETH_HASH_FUNCTION_SYMMETRIC_TOEPLITZ, .keyGen = genRandomRssKey},
        {.id = RTE_ETH_HASH_FUNCTION_SIMPLE_XOR, .keyGen = genRandomRssKey},
        {.id = RTE_ETH_HASH_FUNCTION_TOEPLITZ, .keyGen = genSymmetricRssKey},
    };

    // Default option that gets picked in case none of the preferences match
    auto hfOption
        = HashFunctionOption {.id = RTE_ETH_HASH_FUNCTION_DEFAULT, .keyGen = genSymmetricRssKey};

    for (const auto& opt : hfOptions) {
        const auto mask = RTE_ETH_HASH_ALGO_TO_CAPA(static_cast<unsigned>(opt.id));
        if ((devInfo.rss_algo_capa & mask) == mask) {
            hfOption = opt;
            break;
        }
    }

    hfOption.keyGen({keyBuf.data(), devInfo.hash_key_size});
    conf.rxmode.mq_mode = RTE_ETH_MQ_RX_RSS;
    conf.rx_adv_conf.rss_conf.algorithm = hfOption.id;
    conf.rx_adv_conf.rss_conf.rss_hf = RTE_ETH_RSS_IP;
    conf.rx_adv_conf.rss_conf.rss_key = keyBuf.data();
    conf.rx_adv_conf.rss_conf.rss_key_len = devInfo.hash_key_size;

    if ((devInfo.flow_type_rss_offloads & RTE_ETH_RSS_IP) != RTE_ETH_RSS_IP) {
        logger.warn(
            "Card does not support all RSS IP offloads, RSS might not work correctly (it usually "
            "will)");

        logger.info(
            "Supported rss offloads: {:064b}\n{}",
            devInfo.flow_type_rss_offloads,
            rteEthRssOffloadsToString(devInfo.flow_type_rss_offloads));
        logger.info(
            "Wanted rss offloads   : {:064b}\n{}",
            conf.rx_adv_conf.rss_conf.rss_hf,
            rteEthRssOffloadsToString(conf.rx_adv_conf.rss_conf.rss_hf));

        conf.rx_adv_conf.rss_conf.rss_hf &= devInfo.flow_type_rss_offloads;

        logger.warn(
            "Adjusted offloads     : {:064b}\n{}",
            conf.rx_adv_conf.rss_conf.rss_hf,
            rteEthRssOffloadsToString(conf.rx_adv_conf.rss_conf.rss_hf));
    }
}

void Port::adjustRxTxDesc(uint16_t& nRxDesc, uint16_t& nTxDesc)
{
    const uint16_t nRxDescOrig = nRxDesc;
    const uint16_t nTxDescOrig = nTxDesc;
    m_logger.info("Adjusting RX/TX descriptors: {}/{}", nRxDesc, nTxDesc);
    ethDevAdjustRxTxDesc(m_portId, nRxDesc, nTxDesc);

    if (nRxDescOrig != nRxDesc || nTxDescOrig != nTxDesc) {
        m_logger.warn(
            "RX/TX descriptor count adjusted from {}/{} to {}/{}",
            nRxDescOrig,
            nTxDescOrig,
            nRxDesc,
            nTxDesc);
    }
}

void Port::devConfigure(
    const PortHooksDispatcher& dispatcher,
    const PortConfig& config,
    uint16_t& nRxQueues,
    uint16_t& nTxQueues)
{
    rte_eth_conf conf {};
    std::array<uint8_t, RSS_KEY_MAX_LEN> rssKeyBuf;
    const rte_eth_dev_info devInfo = ethDevInfoGet(m_portId);

    if (config.rx) {
        conf.rxmode.mtu = config.mtu;
        conf.rxmode.offloads = config.rx->offloads;

        if (nRxQueues > 1) {
            setRssConf(devInfo, conf, rssKeyBuf, m_logger);
        }
    }

    if (config.tx) {
        conf.txmode.offloads = config.tx->offloads;
    }

    m_logger.info("Running eth dev configure");
    const PortHookAction action = dispatcher.dispatchPre(
        &PortHooks::preDevConfigure,
        m_portId,
        config,
        nRxQueues,
        nTxQueues,
        conf);
    if (action == PortHookAction::CONTINUE) {
        ethDevConfigure(m_portId, nRxQueues, nTxQueues, &conf);
    }
    dispatcher
        .dispatchPost(&PortHooks::postDevConfigure, m_portId, config, nRxQueues, nTxQueues, conf);
}

void Port::setupQueues(
    const PortHooksDispatcher& dispatcher,
    const PortConfig& config,
    uint16_t nRxQueues,
    uint16_t nTxQueues,
    uint16_t nRxDesc,
    uint16_t nTxDesc)
{
    const rte_eth_dev_info devInfo = ethDevInfoGet(m_portId);

    m_logger.info("Setting up queues (RX desc={}, TX desc={})", nRxDesc, nTxDesc);

    for (uint16_t i = 0; i < nRxQueues; i++) {
        m_logger.info("Setting up RX queue #{} ({}/{})", i, i + 1, nRxQueues);

        rte_eth_rxconf conf = devInfo.default_rxconf;

        // offloads already set for port in devConfigure, no need to repeat
        // see https://doc.dpdk.org/api/rte__ethdev_8h.html#a796c2f20778984c6f41b271e36bae50e
        conf.offloads = 0;

        // These are copied because the hook can adjust them
        uint16_t nRxDescTmp = nRxDesc; // NOLINT(misc-const-correctness)
        unsigned sockIdTmp = socketId(); // NOLINT(misc-const-correctness)

        rte_mempool* mempool = nullptr;
        // This check is relevant in case nRxQueues is increased by a hook even though the config
        // didn't have rx set. In this case, mempool remains null for the hook to fill.
        if (config.rx) {
            const PktMempoolHandle mempoolHandle = config.rx->mempoolProvider->get(i);
            mempool = mempoolHandle.get();
        }

        const PortHookAction action = dispatcher.dispatchPre(
            &PortHooks::preRxQueueSetup,
            m_portId,
            i,
            config,
            nRxDescTmp,
            sockIdTmp,
            conf,
            mempool);
        if (action == PortHookAction::CONTINUE) {
            ethRxQueueSetup(m_portId, i, nRxDescTmp, sockIdTmp, &conf, mempool);
        }
        dispatcher.dispatchPost(
            &PortHooks::postRxQueueSetup,
            m_portId,
            i,
            config,
            nRxDescTmp,
            sockIdTmp,
            conf,
            mempool);
    }
    for (uint16_t i = 0; i < nTxQueues; i++) {
        m_logger.info("Setting up TX queue #{} ({}/{})", i, i + 1, nTxQueues);

        rte_eth_txconf conf = devInfo.default_txconf;

        // offloads already set for port in devConfigure, no need to repeat
        // see https://doc.dpdk.org/api/rte__ethdev_8h.html#a796c2f20778984c6f41b271e36bae50e
        conf.offloads = 0;

        // These are copied because the hook can adjust them
        uint16_t nTxDescTmp = nTxDesc; // NOLINT(misc-const-correctness)
        unsigned sockIdTmp = socketId(); // NOLINT(misc-const-correctness)

        const PortHookAction action = dispatcher.dispatchPre(
            &PortHooks::preTxQueueSetup,
            m_portId,
            i,
            config,
            nTxDescTmp,
            sockIdTmp,
            conf);
        if (action == PortHookAction::CONTINUE) {
            ethTxQueueSetup(m_portId, i, nTxDescTmp, sockIdTmp, &conf);
        }
        dispatcher.dispatchPost(
            &PortHooks::postTxQueueSetup,
            m_portId,
            i,
            config,
            nTxDescTmp,
            sockIdTmp,
            conf);
    }
}

static void validatePortConfig(const PortConfig& config)
{
    if (!config.rx && !config.tx) {
        throw amon::Error {"invalid PortConfig: no rx nor tx set"};
    }

    if (config.rx && config.rx->mempoolProvider == nullptr) {
        throw amon::Error {"invalid PortConfig: rx is missing mempoolProvider"};
    }

    if (config.rx && config.rx->nQueues == 0) {
        throw amon::Error {"invalid PortConfig: number of rx queues cannot be 0"};
    }

    if (config.rx && config.rx->nDescriptors == 0) {
        throw amon::Error {"invalid PortConfig: number of rx descriptors cannot be 0"};
    }

    if (config.tx && config.tx->nQueues == 0) {
        throw amon::Error {"invalid PortConfig: number of tx queues cannot be 0"};
    }

    if (config.tx && config.tx->nDescriptors == 0) {
        throw amon::Error {"invalid PortConfig: number of tx descriptors cannot be 0"};
    }
}

template <typename Call>
static void warnIfNotSupported(Call call, const Logger& logger)
{
    try {
        call();
    } catch (const amon::dpdk::RteErrnoError& err) {
        if (err.errNum() == ENOTSUP) {
            logger.warn("{}", err.what());
        } else {
            throw;
        }
    }
}

void Port::configure(const PortConfig& config, std::vector<std::unique_ptr<PortHooks>> hooks)
{
    validatePortConfig(config);

    // Lock the port handle and check that the port has not been configured yet
    const std::lock_guard<std::mutex> _ {m_mutex};
    if (m_configured) {
        throw DpdkError("port {} is already configured", m_portId);
    }

    // Begin configuration
    m_logger.info("Starting to configure port");

    const rte_eth_dev_info devInfo = ethDevInfoGet(m_portId);
    const auto valueOrMissing = [](const char* val) { return val != nullptr ? val : "N/A"; };
    m_logger.info("Port info:");
    m_logger.info("  Driver   : {}", valueOrMissing(devInfo.driver_name));
    m_logger.info("  Dev name : {}", valueOrMissing(rte_dev_name(devInfo.device)));
    m_logger.info("  Bus info : {}", valueOrMissing(rte_dev_bus_info(devInfo.device)));
    m_logger.info(
        "  Socket ID: {}",
        socketId() == SocketID::ANY ? "N/A" : std::to_string(static_cast<unsigned>(socketId())));

    auto dispatcher = PortHooksDispatcher {std::move(hooks), m_portId, devInfo, config};
    PortHookAction action;

    uint16_t nRxDesc = config.rx ? config.rx->nDescriptors : 0;
    uint16_t nTxDesc = config.tx ? config.tx->nDescriptors : 0;
    adjustRxTxDesc(nRxDesc, nTxDesc);

    uint16_t nRxQueues = config.rx ? config.rx->nQueues : 0;
    uint16_t nTxQueues = config.tx ? config.tx->nQueues : 0;
    devConfigure(dispatcher, config, nRxQueues, nTxQueues);

    setupQueues(dispatcher, config, nRxQueues, nTxQueues, nRxDesc, nTxDesc);

    // Set promiscuous mode if rx. If it fails because not supported, only a warning is logged, the
    // configuration continues.
    if (config.rx) {
        warnIfNotSupported(
            [&]() { ethPromiscuousSet(m_portId, config.rx->promiscuous); },
            m_logger);
    }

    // Set link down. If it fails because not supported, only a warning is logged, the configuration
    // continues.
    m_logger.info("Setting link down");
    action = dispatcher.dispatchPre(&PortHooks::preLinkDown, m_portId, config);
    if (action == PortHookAction::CONTINUE) {
        warnIfNotSupported([&]() { ethDevLinkDown(m_portId); }, m_logger);
    }
    dispatcher.dispatchPost(&PortHooks::postLinkDown, m_portId, config);

    // Start the port
    m_logger.info("Starting eth dev");
    action = dispatcher.dispatchPre(&PortHooks::preDevStart, m_portId, config);
    if (action == PortHookAction::CONTINUE) {
        ethDevStart(m_portId);
    }
    dispatcher.dispatchPost(&PortHooks::postDevStart, m_portId, config);

    // Finish configuration and save the config parameters.
    m_configured = {
        .config = config,
        .dispatcher = std::move(dispatcher),
    };
}

SocketID Port::socketId() const
{
    const int res = rte_eth_dev_socket_id(m_portId);
    if (res == SOCKET_ID_ANY) {
        return SocketID::ANY;
    }
    return SocketID {static_cast<unsigned>(res)};
}

void Port::down()
{
    const std::lock_guard<std::mutex> _ {m_mutex};

    if (!m_configured) {
        throw amon::Error {"port must be configured first"};
    }

    const PortHookAction action = m_configured->dispatcher.dispatchPre(
        &PortHooks::preLinkDown,
        m_portId,
        m_configured->config);
    if (action == PortHookAction::CONTINUE) {
        ethDevLinkDown(m_portId);
    }
    m_configured->dispatcher.dispatchPost(&PortHooks::postLinkDown, m_portId, m_configured->config);
}

void Port::up()
{
    const std::lock_guard<std::mutex> _ {m_mutex};

    if (!m_configured) {
        throw amon::Error {"port must be configured first"};
    }

    const PortHookAction action = m_configured->dispatcher.dispatchPre(
        &PortHooks::preLinkUp,
        m_portId,
        m_configured->config);
    if (action == PortHookAction::CONTINUE) {
        ethDevLinkUp(m_portId);
    }
    m_configured->dispatcher.dispatchPost(&PortHooks::postLinkUp, m_portId, m_configured->config);
}

PortTxQueue Port::txQueue(uint16_t id) // NOLINT(readability-make-member-function-const)
{
    const std::lock_guard<std::mutex> _ {m_mutex};
    if (!m_configured) {
        throw DpdkError {"cannot get tx queue, port {} is not configured", id};
    }
    if (!m_configured->config.tx) {
        throw DpdkError {"cannot get tx queue, port {} has no tx configured", m_portId};
    }
    if (id >= m_configured->config.tx->nQueues) {
        throw DpdkError {
            "cannot get tx queue {}, port {} has only {} tx queues",
            id,
            m_portId,
            m_configured->config.tx->nQueues};
    }
    return PortTxQueue {PortHandle(m_portId), id};
}

PortRxQueue Port::rxQueue(uint16_t id) // NOLINT(readability-make-member-function-const)
{
    const std::lock_guard<std::mutex> _ {m_mutex};
    if (!m_configured) {
        throw DpdkError {"cannot get rx queue, port {} is not configured", m_portId};
    }
    if (!m_configured->config.rx) {
        throw DpdkError {"cannot get rx queue, port {} has no rx configured", m_portId};
    }
    if (id >= m_configured->config.rx->nQueues) {
        throw DpdkError {
            "cannot get rx queue {}, port {} has only {} rx queues",
            id,
            m_portId,
            m_configured->config.rx->nQueues};
    }
    return PortRxQueue {PortHandle(m_portId), id};
}

XStats Port::xstats()
{
    const std::lock_guard<std::mutex> _ {m_mutex};
    if (!m_configured) {
        throw DpdkError {"cannot get statistics, port {} is not configured", m_portId};
    }
    return {m_portId};
}

///// PortRegistry ////
namespace {
class PortRegistry : public types::Singleton<PortRegistry> {
public:
    std::shared_ptr<Port> get(uint16_t portId);

private:
    friend class types::Singleton<PortRegistry>;

    std::mutex m_mutex;
    std::unordered_map<uint16_t, std::weak_ptr<Port>> m_ports;
};

} // namespace

std::shared_ptr<Port> PortRegistry::get(uint16_t portId)
{
    const std::lock_guard<std::mutex> _ {m_mutex};

    std::shared_ptr<Port> p;
    auto it = m_ports.find(portId);
    if (it == m_ports.end()
        || !(p = it->second.lock())) { // NOLINT(bugprone-assignment-in-if-condition)
        p = std::make_shared<Port>(portId);
        m_ports.insert_or_assign(portId, p);
    }

    return p;
}
////////////////////

///// PortHandle ////
PortHandle::PortHandle(uint16_t portId)
    : m_portId(portId)
    , m_port(PortRegistry::instance().get(portId))
{
}

uint16_t PortHandle::id() const
{
    return m_portId;
}

std::string PortHandle::devName() const
{
    const rte_eth_dev_info info = ethDevInfoGet(m_portId);
    return std::string {rte_dev_name(info.device)};
}

std::string PortHandle::driverName() const
{
    const rte_eth_dev_info info = ethDevInfoGet(m_portId);
    return std::string {info.driver_name};
}

PortDevInfo PortHandle::devInfo() const
{
    const rte_eth_dev_info info = ethDevInfoGet(m_portId);

    const PortDevInfo::Rx rx {
        .nQueuesMax = info.max_rx_queues,
        .nQueuesCurrent = info.nb_rx_queues,
        .nDescriptorsMin = info.rx_desc_lim.nb_min,
        .nDescriptorsMax = info.rx_desc_lim.nb_max,
        .offloadsCapa = info.rx_offload_capa};
    const PortDevInfo::Tx tx {
        .nQueuesMax = info.max_tx_queues,
        .nQueuesCurrent = info.nb_tx_queues,
        .nDescriptorsMin = info.tx_desc_lim.nb_min,
        .nDescriptorsMax = info.tx_desc_lim.nb_max,
        .offloadsCapa = info.tx_offload_capa};
    const PortDevInfo portInfo {.rx = rx, .tx = tx, .mtuMin = info.min_mtu, .mtuMax = info.max_mtu};

    return portInfo;
}

SocketID PortHandle::socketId() const
{
    return m_port->socketId();
}

std::vector<PortHandle> PortHandle::allPorts()
{
    std::vector<PortHandle> portIds;

    uint16_t portId;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
    RTE_ETH_FOREACH_DEV(portId)
#pragma GCC diagnostic pop
    {
        portIds.emplace_back(portId);
    }

    return portIds;
}

PortHandle PortHandle::fromDevName(std::string_view devName)
{
    uint16_t value;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
    RTE_ETH_FOREACH_DEV(value)
#pragma GCC diagnostic pop
    {
        PortHandle portId {value};

        if (portId.devName() == devName) {
            return portId;
        }
    }

    throw DpdkError("port \"{}\" not found", devName);
}

void PortHandle::configure(const PortConfig& config)
{
    configure(config, makeDefaultPortHooks());
}

void PortHandle::configure(const PortConfig& config, std::vector<std::unique_ptr<PortHooks>> hooks)
{
    m_port->configure(config, std::move(hooks));
}

void PortHandle::down() // NOLINT(readability-make-member-function-const)
{
    m_port->down();
}

void PortHandle::up() // NOLINT(readability-make-member-function-const)
{
    m_port->up();
}

PortTxQueue PortHandle::txQueue(uint16_t id)
{
    return m_port->txQueue(id);
}

PortRxQueue PortHandle::rxQueue(uint16_t id)
{
    return m_port->rxQueue(id);
}

XStats PortHandle::xstats()
{
    return m_port->xstats();
}

////////////////////

///// PortTxQueue ////
PortTxQueue::PortTxQueue(PortHandle port, uint16_t queueId)
    : m_queueId(queueId)
    , m_port(std::move(port))
{
}

std::span<rte_mbuf*> PortTxQueue::transmit(std::span<rte_mbuf*> burst) noexcept
{
    const uint16_t nTransmitted = rte_eth_tx_burst(
        m_port.id(),
        m_queueId,
        burst.data(),
        static_cast<uint16_t>(burst.size()));
    return burst.subspan(nTransmitted);
}

void PortTxQueue::transmit(PktBurst<rte_mbuf>& burst) noexcept
{
    const uint16_t nTransmitted = rte_eth_tx_burst(
        m_port.id(),
        m_queueId,
        burst.data(),
        static_cast<uint16_t>(burst.size()));
    burst.release(burst.begin(), burst.begin() + nTransmitted);
}

void PortTxQueue::transmit(PktBurst<DpdkPacket>& burst) noexcept
{
    const uint16_t nTransmitted = rte_eth_tx_burst(
        m_port.id(),
        m_queueId,
        reinterpret_cast<rte_mbuf**>(burst.data()),
        static_cast<uint16_t>(burst.size()));
    burst.release(burst.begin(), burst.begin() + nTransmitted);
}

void PortTxQueue::transmitAllBlocking(std::span<rte_mbuf*> burst) noexcept
{
    while (!burst.empty()) {
        burst = transmit(burst);
    }
}

void PortTxQueue::transmitAllBlocking(PktBurst<rte_mbuf>& burst) noexcept
{
    transmitAllBlocking(std::span<rte_mbuf*> {burst.begin(), burst.end()});
    burst.release(burst.begin(), burst.end());
}

void PortTxQueue::transmitAllBlocking(PktBurst<DpdkPacket>& burst) noexcept
{
    transmitAllBlocking(
        std::span<rte_mbuf*> {
            reinterpret_cast<rte_mbuf**>(burst.begin()),
            reinterpret_cast<rte_mbuf**>(burst.end())});
    burst.release(burst.begin(), burst.end());
}
////////////////////

///// PortRxQueue ////
static void initDpdkPackets(std::span<DpdkPacket*> packets)
{
    for (auto* packet : packets) {
        std::construct_at<Packet>(&packet->packet());
        std::byte* data = rte_pktmbuf_mtod(packet->mbuf(), std::byte*);
        const auto dataLen = rte_pktmbuf_data_len(packet->mbuf());
        packet->packet().data = {data, data + dataLen};
    }
}

PortRxQueue::PortRxQueue(PortHandle port, uint16_t queueId)
    : m_queueId(queueId)
    , m_port(std::move(port))
{
}

std::span<rte_mbuf*> PortRxQueue::receive(std::span<rte_mbuf*> burst) noexcept
{
    const uint16_t nReceived = rte_eth_rx_burst(
        m_port.id(),
        m_queueId,
        burst.data(),
        static_cast<uint16_t>(burst.size()));

    return burst.subspan(0, nReceived);
}

void PortRxQueue::receive(PktBurst<rte_mbuf>& burst) noexcept
{
    burst.fill([&](std::span<rte_mbuf*> mbufs) { return receive(mbufs).size(); });
}

void PortRxQueue::receive(PktBurst<DpdkPacket>& burst) noexcept
{
    burst.fill([&](std::span<DpdkPacket*> packets) {
        const uint16_t nReceived = rte_eth_rx_burst(
            m_port.id(),
            m_queueId,
            reinterpret_cast<rte_mbuf**>(packets.data()),
            static_cast<uint16_t>(packets.size()));
        initDpdkPackets(packets.subspan(0, nReceived));

        return nReceived;
    });
}

void PortRxQueue::receive(PktBurst<rte_mbuf>& burst, size_t maxPkts) noexcept
{
    burst.fill([&](std::span<rte_mbuf*> mbufs) {
        mbufs = mbufs.subspan(0, std::min<size_t>(maxPkts, mbufs.size()));
        return receive(mbufs).size();
    });
}

void PortRxQueue::receive(PktBurst<DpdkPacket>& burst, size_t maxPkts) noexcept
{
    burst.fill([&](std::span<DpdkPacket*> packets) {
        packets = packets.subspan(0, std::min<size_t>(maxPkts, packets.size()));

        const uint16_t nReceived = rte_eth_rx_burst(
            m_port.id(),
            m_queueId,
            reinterpret_cast<rte_mbuf**>(packets.data()),
            static_cast<uint16_t>(packets.size()));
        initDpdkPackets(packets.subspan(0, nReceived));

        return nReceived;
    });
}
////////////////////

} // namespace amon::dpdk
