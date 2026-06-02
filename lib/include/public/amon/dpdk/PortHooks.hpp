/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Port hooks interface definition
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/Logger.hpp>

#include <bitset>
#include <format>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <rte_ethdev.h>

namespace amon::dpdk {

struct PortConfig;

/**
 * @brief Action returned by pre-hooks.
 *
 * This is so that pre-hooks can skip problematic calls or replace them with their own call.
 *
 * For example, let's assume the following hook:
 *    ---> PortHooks::preDevConfigure()
 *    rte_eth_dev_configure()
 *    ---> PortHooks::postDevConfigure()
 *
 * In this case, if the pre-hook returns the CONTINUE action, rte_eth_dev_configure is called as
 * usual. If SKIP is returned, rte_eth_dev_configure is not called.
 */
enum class PortHookAction {
    CONTINUE = 0, ///< Continue with the operation.
    SKIP, ///< Skip the operation.
};

/**
 * @brief Base class for port configuration hooks.
 *
 * This mechanism allows you to hook into the port configuration process in various places to
 * perform driver-specific configuration. For example, RSS might need to be configured differently
 * on different cards.
 *
 * Order of hook calls during port configuration:
 *
 * ---> PortHooks::preDevConfigure()
 * rte_eth_dev_configure()
 * ---> PortHooks::postDevConfigure()
 *
 * For each RX queue:
 *     ---> PortHooks::preRxQueueSetup()
 *     rte_eth_rx_queue_setup()
 *     ---> PortHooks::postRxQueueSetup()
 *
 * For each TX queue:
 *     ---> PortHooks::preTxQueueSetup()
 *     rte_eth_tx_queue_setup()
 *     ---> PortHooks::postTxQueueSetup()
 *
 * ---> PortHooks::preLinkDown()
 * rte_eth_dev_set_link_down()
 * ---> PortHooks::postLinkDown()
 *
 * ---> PortHooks::preDevStart()
 * rte_eth_dev_start()
 * ---> PortHooks::postDevStart()
 */
class PortHooks {
public:
    /**
     * @brief Get the name the hooks instance identifies with.
     * @return Name of the hook.
     */
    virtual std::string_view name() const = 0;

    /**
     * @brief Check whether this hook applies to a port.
     *
     * If the return value is true, each hook will be called when configuring this port.
     * If the return value is false, the other hooks won't be called for this particular port
     * configuration.
     *
     * @param portId DPDK port ID.
     * @param devInfo DPDK Ethernet device info.
     * @param config Port configuration.
     * @return True when this hook should be used for the port.
     */
    virtual bool
    isEligible(uint16_t portId, const rte_eth_dev_info& devInfo, const PortConfig& config) = 0;

    /**
     * @brief Called before rte_eth_dev_configure().
     * @param portId DPDK port ID.
     * @param config Port configuration.
     * @param nRxQueues Number of RX queues; may be modified.
     * @param nTxQueues Number of TX queues; may be modified.
     * @param dpdkConfig DPDK Ethernet device configuration; may be modified.
     * @return Hook action.
     */
    virtual PortHookAction preDevConfigure(
        uint16_t portId,
        const PortConfig& config,
        uint16_t& nRxQueues,
        uint16_t& nTxQueues,
        rte_eth_conf& dpdkConfig);

    /**
     * @brief Called after rte_eth_dev_configure().
     * @param portId DPDK port ID.
     * @param config Port configuration.
     * @param nRxQueues Number of RX queues.
     * @param nTxQueues Number of TX queues.
     * @param dpdkConfig DPDK Ethernet device configuration.
     */
    virtual void postDevConfigure(
        uint16_t portId,
        const PortConfig& config,
        uint16_t nRxQueues,
        uint16_t nTxQueues,
        const rte_eth_conf& dpdkConfig);

    /**
     * @brief Called before rte_eth_rx_queue_setup().
     * @param portId DPDK port ID.
     * @param queueId RX queue ID.
     * @param config Port configuration.
     * @param nRxDesc Number of RX descriptors; may be modified.
     * @param sockId NUMA socket ID; may be modified.
     * @param dpdkConfig DPDK RX queue configuration; may be modified.
     * @param mempool RX mempool; may be modified.
     * @return Hook action.
     */
    virtual PortHookAction preRxQueueSetup(
        uint16_t portId,
        uint16_t queueId,
        const PortConfig& config,
        uint16_t& nRxDesc,
        unsigned int& sockId,
        rte_eth_rxconf& dpdkConfig,
        rte_mempool*& mempool);

    /**
     * @brief Called after rte_eth_rx_queue_setup().
     * @param portId DPDK port ID.
     * @param queueId RX queue ID.
     * @param config Port configuration.
     * @param nRxDesc Number of RX descriptors.
     * @param sockId NUMA socket ID.
     * @param dpdkConfig DPDK RX queue configuration.
     * @param mempool RX mempool.
     */
    virtual void postRxQueueSetup(
        uint16_t portId,
        uint16_t queueId,
        const PortConfig& config,
        uint16_t nRxDesc,
        unsigned int sockId,
        const rte_eth_rxconf& dpdkConfig,
        rte_mempool* mempool);

    /**
     * @brief Called before rte_eth_tx_queue_setup().
     * @param portId DPDK port ID.
     * @param queueId TX queue ID.
     * @param config Port configuration.
     * @param nTxDesc Number of TX descriptors; may be modified.
     * @param sockId NUMA socket ID; may be modified.
     * @param dpdkConfig DPDK TX queue configuration; may be modified.
     * @return Hook action.
     */
    virtual PortHookAction preTxQueueSetup(
        uint16_t portId,
        uint16_t queueId,
        const PortConfig& config,
        uint16_t& nTxDesc,
        unsigned int& sockId,
        rte_eth_txconf& dpdkConfig);

    /**
     * @brief Called after rte_eth_tx_queue_setup().
     * @param portId DPDK port ID.
     * @param queueId TX queue ID.
     * @param config Port configuration.
     * @param nTxDesc Number of TX descriptors.
     * @param sockId NUMA socket ID.
     * @param dpdkConfig DPDK TX queue configuration.
     */
    virtual void postTxQueueSetup(
        uint16_t portId,
        uint16_t queueId,
        const PortConfig& config,
        uint16_t nTxDesc,
        unsigned int sockId,
        const rte_eth_txconf& dpdkConfig);

    /**
     * @brief Called before rte_eth_dev_set_link_down().
     * @param portId DPDK port ID.
     * @param config Port configuration.
     * @return Hook action.
     */
    virtual PortHookAction preLinkDown(uint16_t portId, const PortConfig& config);

    /**
     * @brief Called after rte_eth_dev_set_link_down().
     * @param portId DPDK port ID.
     * @param config Port configuration.
     */
    virtual void postLinkDown(uint16_t portId, const PortConfig& config);

    /**
     * @brief Called before rte_eth_dev_set_link_up().
     * @param portId DPDK port ID.
     * @param config Port configuration.
     * @return Hook action.
     */
    virtual PortHookAction preLinkUp(uint16_t portId, const PortConfig& config);

    /**
     * @brief Called after rte_eth_dev_set_link_up().
     * @param portId DPDK port ID.
     * @param config Port configuration.
     */
    virtual void postLinkUp(uint16_t portId, const PortConfig& config);

    /**
     * @brief Called before rte_eth_dev_start().
     * @param portId DPDK port ID.
     * @param config Port configuration.
     * @return Hook action.
     */
    virtual PortHookAction preDevStart(uint16_t portId, const PortConfig& config);

    /**
     * @brief Called after rte_eth_dev_start().
     * @param portId DPDK port ID.
     * @param config Port configuration.
     */
    virtual void postDevStart(uint16_t portId, const PortConfig& config);

    /**
     * @brief Default destructor.
     */
    virtual ~PortHooks() = default;
};

/**
 * @brief Dispatches hook calls to eligible PortHooks instances.
 */
class PortHooksDispatcher {
public:
    /**
     * @brief Construct a dispatcher and evaluate hook eligibility.
     * @param instances Hook instances.
     * @param portId DPDK port ID.
     * @param devInfo DPDK Ethernet device info.
     * @param config Port configuration.
     */
    PortHooksDispatcher(
        std::vector<std::unique_ptr<PortHooks>> instances,
        uint16_t portId,
        const rte_eth_dev_info& devInfo,
        const PortConfig& config)
        : m_instances(std::move(instances))
        , m_logger(LoggerProvider::get(std::format("PortHooksDispatcher(portId={})", portId)))
    {
        if (m_instances.size() > m_eligible.size()) {
            throw std::invalid_argument {
                "maximum number of porthook instances is " + std::to_string(m_eligible.size())};
        }

        for (size_t i = 0; i < m_instances.size(); ++i) {
            auto* instance = m_instances[i].get();
            const bool isEligible = instance->isEligible(portId, devInfo, config);
            m_eligible.set(i, isEligible);

            m_logger.info(
                "Porthooks instance {} eligible: {}",
                instance->name(),
                isEligible ? "yes" : "no");
        }
    }

    /**
     * @brief Dispatch a post-hook to all eligible hooks.
     * @param method PortHooks post-hook method pointer.
     * @param args Arguments forwarded to the hook.
     */
    template <typename Method, typename... Args>
    void dispatchPost(Method method, Args&&... args) const
    {
        for (size_t i = 0; i < m_instances.size(); ++i) {
            if (m_eligible[i]) {
                std::invoke(method, *m_instances[i], args...);
            }
        }
    }

    /**
     * @brief Dispatch a pre-hook to eligible hooks.
     * @param method PortHooks pre-hook method pointer.
     * @param args Arguments forwarded to the hook.
     * @return SKIP if any hook blocks the operation, otherwise CONTINUE.
     */
    template <typename Method, typename... Args>
    [[nodiscard]] PortHookAction dispatchPre(Method method, Args&&... args) const
    {
        for (size_t i = 0; i < m_instances.size(); ++i) {
            if (m_eligible[i]) {
                const PortHookAction ret = std::invoke(method, *m_instances[i], args...);
                if (ret == PortHookAction::SKIP) {
                    return PortHookAction::SKIP;
                }
            }
        }
        return PortHookAction::CONTINUE;
    }

private:
    std::vector<std::unique_ptr<PortHooks>> m_instances;
    std::bitset<128> m_eligible;
    Logger m_logger;
};

/**
 * @brief Create the default set of port hooks.
 * @return Default port hook instances.
 */
std::vector<std::unique_ptr<PortHooks>> makeDefaultPortHooks();

} // namespace amon::dpdk
