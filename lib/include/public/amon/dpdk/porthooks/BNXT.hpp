/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Porthooks for the BNXT driver.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/dpdk/PktMempool.hpp>
#include <amon/dpdk/PortHooks.hpp>

namespace amon::dpdk::porthooks {

/**
 * @brief Porthooks for the bnxt driver.
 *
 * This hook deals with the oddity that BNXT cards require to have at least 1 RX queue, else the
 * configuration fails with an error. In case no RX configuration is provided, the hooks sets up at
 * least one RX queue and injects a dummy mempool that it owns.
 */
class BNXT : public PortHooks {
public:
    /// @copydoc amon::dpdk::PortHooks::name
    std::string_view name() const override { return "BNXT"; }

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

    /// @copydoc amon::dpdk::PortHooks::preRxQueueSetup
    PortHookAction preRxQueueSetup(
        uint16_t portId,
        uint16_t queueId,
        const PortConfig& config,
        uint16_t& nRxDesc,
        unsigned int& sockId,
        rte_eth_rxconf& dpdkConfig,
        rte_mempool*& mempool) override;

private:
    std::optional<PktMempoolHandle> m_dummyMempool;
};

} // namespace amon::dpdk::porthooks
