/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Porthooks for the I40E driver.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/dpdk/PortHooks.hpp>

namespace amon::dpdk::porthooks {

/**
 * @brief PortHooks for the i40e driver.
 *
 * This hook sets up symmetric RSS using the rte_flow API instead of the default legacy RSS.
 * This allows us to use the symmetric sort toeplitz algorithm, which is for some reason not
 * available in the legacy RSS API.
 */
class I40E : public PortHooks {
public:
    /// @copydoc amon::dpdk::PortHooks::name
    std::string_view name() const override { return "I40E"; }

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

    /// @copydoc amon::dpdk::PortHooks::postDevStart
    void postDevStart(uint16_t portId, const PortConfig& config) override;
};

} // namespace amon::dpdk::porthooks
