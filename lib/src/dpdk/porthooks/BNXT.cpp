/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Porthooks for the BNXT driver.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/dpdk/dpdk.hpp>
#include <amon/Error.hpp>
#include <amon/amon.hpp>
#include <amon/dpdk/Port.hpp>
#include <amon/dpdk/porthooks/BNXT.hpp>

#include <string_view>

namespace amon::dpdk::porthooks {

static constexpr uint16_t MIN_RX_DESC = 16;
static constexpr uint16_t MIN_MTU = 68;
static constexpr uint16_t MIN_DATAROOM_SIZE = 129;
static constexpr uint16_t MIN_PKTMBUFS = 128;

bool BNXT::isEligible(uint16_t portId, const rte_eth_dev_info& devInfo, const PortConfig& config)
{
    (void) portId;
    (void) config;
    return devInfo.driver_name != nullptr && std::string_view {devInfo.driver_name} == "net_bnxt";
}

PortHookAction BNXT::preDevConfigure(
    uint16_t portId,
    const PortConfig& config,
    uint16_t& nRxQueues,
    uint16_t& nTxQueues,
    rte_eth_conf& dpdkConfig)
{
    (void) portId;
    (void) nTxQueues;

    if (!config.rx) {
        nRxQueues = 1;
        dpdkConfig.rxmode.mtu = MIN_MTU;
    }
    return PortHookAction::CONTINUE;
}

PortHookAction BNXT::preRxQueueSetup(
    uint16_t portId,
    uint16_t queueId,
    const PortConfig& config,
    uint16_t& nRxDesc,
    unsigned int& sockId,
    rte_eth_rxconf& dpdkConfig,
    rte_mempool*& mempool)
{
    (void) portId;
    (void) queueId;
    (void) dpdkConfig;

    if (!config.rx) {
        AMON_ASSERT(queueId == 0);
        nRxDesc = MIN_RX_DESC;
        m_dummyMempool = PktMempoolHandle({
            .nPackets = MIN_PKTMBUFS,
            .dataRoomSize = MIN_DATAROOM_SIZE,
            .socketId
            = static_cast<int>(sockId) == SOCKET_ID_ANY ? SocketID::ANY : SocketID {sockId},
        });
        mempool = m_dummyMempool->get();
    }

    return PortHookAction::CONTINUE;
}

} // namespace amon::dpdk::porthooks
