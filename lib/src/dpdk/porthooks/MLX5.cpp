/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Porthooks for the MLX5 driver.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon/dpdk/Port.hpp>
#include <amon/dpdk/porthooks/MLX5.hpp>

namespace amon::dpdk::porthooks {

bool MLX5::isEligible(uint16_t portId, const rte_eth_dev_info& devInfo, const PortConfig& config)
{
    (void) portId;
    (void) config;
    return devInfo.driver_name != nullptr && std::string_view {devInfo.driver_name} == "mlx5_pci";
}

PortHookAction MLX5::preDevConfigure(
    uint16_t portId,
    const PortConfig& config,
    uint16_t& nRxQueues,
    uint16_t& nTxQueues,
    rte_eth_conf& dpdkConfig)
{
    (void) portId;
    (void) nRxQueues;
    (void) dpdkConfig;

    if (!config.tx) {
        nTxQueues = 1;
    }
    return PortHookAction::CONTINUE;
}

PortHookAction MLX5::preTxQueueSetup(
    uint16_t portId,
    uint16_t queueId,
    const PortConfig& config,
    uint16_t& nTxDesc,
    unsigned int& sockId,
    rte_eth_txconf& dpdkConfig)
{
    (void) portId;
    (void) queueId;
    (void) sockId;
    (void) dpdkConfig;
    (void) nTxDesc;

    if (!config.tx) {
        return PortHookAction::SKIP;
    }

    return PortHookAction::CONTINUE;
}

} // namespace amon::dpdk::porthooks
