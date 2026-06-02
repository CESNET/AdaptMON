/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Porthooks for the I40E driver.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/dpdk/dpdk.hpp>
#include <amon/Error.hpp>
#include <amon/amon.hpp>
#include <amon/dpdk/Port.hpp>
#include <amon/dpdk/porthooks/I40E.hpp>

#include <string_view>

namespace amon::dpdk::porthooks {

static constexpr unsigned RSS_KEY_MAX_LEN
    = 255; // length is stored as uint8_t, therefore it cannot exceed 255

static void setupFallbackRule(uint16_t portId, unsigned nRxQueues)
{
    const rte_flow_attr attr
        = {.group = 0, .priority = 0, .ingress = 1, .egress = 0, .transfer = 0, .reserved = 0};

    const rte_flow_item pattern[] = {
        {.type = RTE_FLOW_ITEM_TYPE_END, .spec = nullptr, .last = nullptr, .mask = nullptr},
    };

    uint16_t queues[RTE_MAX_QUEUES_PER_PORT];
    for (unsigned i = 0; i < nRxQueues; ++i) {
        queues[i] = static_cast<uint16_t>(i);
    }

    const rte_flow_action_rss rssActionConf = {
        .func = RTE_ETH_HASH_FUNCTION_DEFAULT,
        .level = 0,
        .types = 0, // queues region can not be configured with types
        .key_len = 0,
        .queue_num = nRxQueues,
        .key = nullptr,
        .queue = queues,
    };
    const rte_flow_action action[] = {
        {.type = RTE_FLOW_ACTION_TYPE_RSS, .conf = &rssActionConf},
        {.type = RTE_FLOW_ACTION_TYPE_END, .conf = nullptr},
    };

    flowCreate(portId, &attr, pattern, action);
}

static void setupSymmetricRssRule(
    uint16_t portId,
    std::span<const uint8_t> rssKey,
    std::initializer_list<rte_flow_item_type> patternItems,
    uint64_t rssType)
{
    const rte_flow_attr attr
        = {.group = 0, .priority = 0, .ingress = 1, .egress = 0, .transfer = 0, .reserved = 0};

    std::array<rte_flow_item, 32> pattern {};
    AMON_ASSERT(
        patternItems.size() <= pattern.size(),
        "matchPattern size too big, expected at most {}",
        pattern.size());
    unsigned i = 0;
    for (const auto flowItem : patternItems) {
        pattern[i++].type = flowItem;
    }

    rte_flow_action_rss rssActionConf = {
        .func = RTE_ETH_HASH_FUNCTION_SYMMETRIC_TOEPLITZ,
        .level = 0,
        .types = rssType,
        .key_len = static_cast<uint32_t>(rssKey.size()),
        .queue_num = 0,
        .key = rssKey.data(),
        .queue = nullptr,
    };
    rte_flow_action action[] = {
        {.type = RTE_FLOW_ACTION_TYPE_RSS, .conf = &rssActionConf},
        {.type = RTE_FLOW_ACTION_TYPE_END, .conf = nullptr},
    };

    flowCreate(portId, &attr, pattern.data(), action);
}

bool I40E::isEligible(uint16_t portId, const rte_eth_dev_info& devInfo, const PortConfig& config)
{
    (void) portId;
    (void) config;
    return devInfo.driver_name != nullptr && std::string_view {devInfo.driver_name} == "net_i40e";
}

PortHookAction I40E::preDevConfigure(
    uint16_t portId,
    const PortConfig& config,
    uint16_t& nRxQueues,
    uint16_t& nTxQueues,
    rte_eth_conf& dpdkConfig)
{
    (void) portId;
    (void) nTxQueues;
    (void) nRxQueues;

    if (!config.rx) {
        return PortHookAction::CONTINUE;
    }

    dpdkConfig.rx_adv_conf.rss_conf
        = {.rss_key = nullptr,
           .rss_key_len = 0,
           .rss_hf = 0,
           .algorithm = RTE_ETH_HASH_FUNCTION_DEFAULT};

    return PortHookAction::CONTINUE;
}

void I40E::postDevStart(uint16_t portId, const PortConfig& config)
{
    if (!config.rx) {
        return;
    }

    // Get the current RSS key and use it
    // This seems important for rte_flow configuration to not fail...
    rte_eth_rss_conf rssConf = {};
    std::array<uint8_t, RSS_KEY_MAX_LEN> rssKey; // Safe buffer size
    rssConf.rss_key = rssKey.data();
    rssConf.rss_key_len = rssKey.size();
    if (rte_eth_dev_rss_hash_conf_get(portId, &rssConf) != 0) {
        throw amon::Error("Failed to fetch current RSS hardware configuration");
    }

    setupFallbackRule(portId, config.rx->nQueues);

    setupSymmetricRssRule(
        portId,
        rssKey,
        {RTE_FLOW_ITEM_TYPE_ETH, RTE_FLOW_ITEM_TYPE_IPV4, RTE_FLOW_ITEM_TYPE_END},
        RTE_ETH_RSS_IPV4);
    setupSymmetricRssRule(
        portId,
        rssKey,
        {RTE_FLOW_ITEM_TYPE_ETH,
         RTE_FLOW_ITEM_TYPE_IPV4,
         RTE_FLOW_ITEM_TYPE_UDP,
         RTE_FLOW_ITEM_TYPE_END},
        RTE_ETH_RSS_IPV4);
    setupSymmetricRssRule(
        portId,
        rssKey,
        {RTE_FLOW_ITEM_TYPE_ETH,
         RTE_FLOW_ITEM_TYPE_IPV4,
         RTE_FLOW_ITEM_TYPE_TCP,
         RTE_FLOW_ITEM_TYPE_END},
        RTE_ETH_RSS_IPV4);
    setupSymmetricRssRule(
        portId,
        rssKey,
        {RTE_FLOW_ITEM_TYPE_ETH,
         RTE_FLOW_ITEM_TYPE_IPV4,
         RTE_FLOW_ITEM_TYPE_SCTP,
         RTE_FLOW_ITEM_TYPE_END},
        RTE_ETH_RSS_IPV4);
    setupSymmetricRssRule(
        portId,
        rssKey,
        {RTE_FLOW_ITEM_TYPE_ETH, RTE_FLOW_ITEM_TYPE_IPV6, RTE_FLOW_ITEM_TYPE_END},
        RTE_ETH_RSS_IPV6);
    setupSymmetricRssRule(
        portId,
        rssKey,
        {RTE_FLOW_ITEM_TYPE_ETH,
         RTE_FLOW_ITEM_TYPE_IPV6,
         RTE_FLOW_ITEM_TYPE_UDP,
         RTE_FLOW_ITEM_TYPE_END},
        RTE_ETH_RSS_IPV6);
    setupSymmetricRssRule(
        portId,
        rssKey,
        {RTE_FLOW_ITEM_TYPE_ETH,
         RTE_FLOW_ITEM_TYPE_IPV6,
         RTE_FLOW_ITEM_TYPE_TCP,
         RTE_FLOW_ITEM_TYPE_END},
        RTE_ETH_RSS_IPV6);
    setupSymmetricRssRule(
        portId,
        rssKey,
        {RTE_FLOW_ITEM_TYPE_ETH,
         RTE_FLOW_ITEM_TYPE_IPV6,
         RTE_FLOW_ITEM_TYPE_SCTP,
         RTE_FLOW_ITEM_TYPE_END},
        RTE_ETH_RSS_IPV6);
}

} // namespace amon::dpdk::porthooks
