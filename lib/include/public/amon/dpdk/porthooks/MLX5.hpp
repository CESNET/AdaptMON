/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Porthooks for the MLX5 driver.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/dpdk/PortHooks.hpp>

namespace amon::dpdk::porthooks {

/**
 * @brief Porthooks for the MLX5 driver.
 *
 * This hook deals with the oddity that MLX5 cards require to have at least 1 TX queue, else the
 * configuration fails with an error. In case no TX configuration is provided, the hooks sets up at
 * least one TX queue.
 */
class MLX5 : public PortHooks {
public:
    /// @copydoc amon::dpdk::PortHooks::name
    std::string_view name() const override { return "MLX5"; }

    /// @copydoc amon::dpdk::PortHooks::isEligible
    bool
    isEligible(uint16_t portId, const rte_eth_dev_info& devInfo, const PortConfig& config) override;

    /// @copydoc amon::dpdk::PortHooks::preDevConfigure
    PortHookAction preDevConfigure(
        uint16_t portId,
        const PortConfig& config,
        uint16_t& nRxQueues,
        uint16_t& nTxQueues,
        rte_eth_conf& dpdkConfig) override;

    /// @copydoc amon::dpdk::PortHooks::preTxQueueSetup
    PortHookAction preTxQueueSetup(
        uint16_t portId,
        uint16_t queueId,
        const PortConfig& config,
        uint16_t& nTxDesc,
        unsigned int& sockId,
        rte_eth_txconf& dpdkConfig) override;
};

} // namespace amon::dpdk::porthooks
