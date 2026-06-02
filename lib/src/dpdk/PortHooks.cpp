/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Port hooks interface definition
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon/dpdk/PortHooks.hpp>
#include <amon/dpdk/porthooks/I40E.hpp>

namespace amon::dpdk {

PortHookAction PortHooks::preDevConfigure(
    uint16_t portId,
    const PortConfig& config,
    uint16_t& nRxQueues,
    uint16_t& nTxQueues,
    rte_eth_conf& dpdkConfig)
{
    (void) portId;
    (void) config;
    (void) nRxQueues;
    (void) nTxQueues;
    (void) dpdkConfig;
    return PortHookAction::CONTINUE;
}

void PortHooks::postDevConfigure(
    uint16_t portId,
    const PortConfig& config,
    uint16_t nRxQueues,
    uint16_t nTxQueues,
    const rte_eth_conf& dpdkConfig)
{
    (void) portId;
    (void) config;
    (void) nRxQueues;
    (void) nTxQueues;
    (void) dpdkConfig;
}

PortHookAction PortHooks::preRxQueueSetup(
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
    (void) config;
    (void) nRxDesc;
    (void) sockId;
    (void) dpdkConfig;
    (void) mempool;
    return PortHookAction::CONTINUE;
}

void PortHooks::postRxQueueSetup(
    uint16_t portId,
    uint16_t queueId,
    const PortConfig& config,
    uint16_t nRxDesc,
    unsigned int sockId,
    const rte_eth_rxconf& dpdkConfig,
    rte_mempool* mempool)
{
    (void) portId;
    (void) queueId;
    (void) config;
    (void) nRxDesc;
    (void) sockId;
    (void) dpdkConfig;
    (void) mempool;
}

PortHookAction PortHooks::preTxQueueSetup(
    uint16_t portId,
    uint16_t queueId,
    const PortConfig& config,
    uint16_t& nTxDesc,
    unsigned int& sockId,
    rte_eth_txconf& dpdkConfig)
{
    (void) portId;
    (void) queueId;
    (void) config;
    (void) nTxDesc;
    (void) sockId;
    (void) dpdkConfig;
    return PortHookAction::CONTINUE;
}

void PortHooks::postTxQueueSetup(
    uint16_t portId,
    uint16_t queueId,
    const PortConfig& config,
    uint16_t nTxDesc,
    unsigned int sockId,
    const rte_eth_txconf& dpdkConfig)
{
    (void) portId;
    (void) queueId;
    (void) config;
    (void) nTxDesc;
    (void) sockId;
    (void) dpdkConfig;
}

PortHookAction PortHooks::preLinkDown(uint16_t portId, const PortConfig& config)
{
    (void) portId;
    (void) config;
    return PortHookAction::CONTINUE;
}

void PortHooks::postLinkDown(uint16_t portId, const PortConfig& config)
{
    (void) portId;
    (void) config;
}

PortHookAction PortHooks::preLinkUp(uint16_t portId, const PortConfig& config)
{
    (void) portId;
    (void) config;
    return PortHookAction::CONTINUE;
}

void PortHooks::postLinkUp(uint16_t portId, const PortConfig& config)
{
    (void) portId;
    (void) config;
}

PortHookAction PortHooks::preDevStart(uint16_t portId, const PortConfig& config)
{
    (void) portId;
    (void) config;
    return PortHookAction::CONTINUE;
}

void PortHooks::postDevStart(uint16_t portId, const PortConfig& config)
{
    (void) portId;
    (void) config;
}

std::vector<std::unique_ptr<PortHooks>> makeDefaultPortHooks()
{
    std::vector<std::unique_ptr<PortHooks>> res;
    res.emplace_back(std::make_unique<porthooks::I40E>());
    return res;
}

} // namespace amon::dpdk
