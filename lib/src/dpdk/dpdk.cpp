/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Simple wrappers around dpdk rte_ functions.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/dpdk/dpdk.hpp>
#include <amon/dpdk/Error.hpp>

#include <format>
#include <string>

namespace amon::dpdk {

#define CASE_VAR2STR(var)                                                                          \
    case (var):                                                                                    \
        return #var;

std::string_view rteEthRssOffloadToString(uint64_t mask)
{
    switch (mask) {
        CASE_VAR2STR(RTE_ETH_RSS_IPV4)
        CASE_VAR2STR(RTE_ETH_RSS_FRAG_IPV4)
        CASE_VAR2STR(RTE_ETH_RSS_NONFRAG_IPV4_TCP)
        CASE_VAR2STR(RTE_ETH_RSS_NONFRAG_IPV4_UDP)
        CASE_VAR2STR(RTE_ETH_RSS_NONFRAG_IPV4_SCTP)
        CASE_VAR2STR(RTE_ETH_RSS_NONFRAG_IPV4_OTHER)
        CASE_VAR2STR(RTE_ETH_RSS_IPV6)
        CASE_VAR2STR(RTE_ETH_RSS_FRAG_IPV6)
        CASE_VAR2STR(RTE_ETH_RSS_NONFRAG_IPV6_TCP)
        CASE_VAR2STR(RTE_ETH_RSS_NONFRAG_IPV6_UDP)
        CASE_VAR2STR(RTE_ETH_RSS_NONFRAG_IPV6_SCTP)
        CASE_VAR2STR(RTE_ETH_RSS_NONFRAG_IPV6_OTHER)
        CASE_VAR2STR(RTE_ETH_RSS_L2_PAYLOAD)
        CASE_VAR2STR(RTE_ETH_RSS_IPV6_EX)
        CASE_VAR2STR(RTE_ETH_RSS_IPV6_TCP_EX)
        CASE_VAR2STR(RTE_ETH_RSS_IPV6_UDP_EX)
        CASE_VAR2STR(RTE_ETH_RSS_PORT)
        CASE_VAR2STR(RTE_ETH_RSS_VXLAN)
        CASE_VAR2STR(RTE_ETH_RSS_GENEVE)
        CASE_VAR2STR(RTE_ETH_RSS_NVGRE)
        CASE_VAR2STR(RTE_ETH_RSS_GTPU)
        CASE_VAR2STR(RTE_ETH_RSS_ETH)
        CASE_VAR2STR(RTE_ETH_RSS_S_VLAN)
        CASE_VAR2STR(RTE_ETH_RSS_C_VLAN)
        CASE_VAR2STR(RTE_ETH_RSS_ESP)
        CASE_VAR2STR(RTE_ETH_RSS_AH)
        CASE_VAR2STR(RTE_ETH_RSS_L2TPV3)
        CASE_VAR2STR(RTE_ETH_RSS_PFCP)
        CASE_VAR2STR(RTE_ETH_RSS_PPPOE)
        CASE_VAR2STR(RTE_ETH_RSS_ECPRI)
        CASE_VAR2STR(RTE_ETH_RSS_MPLS)
        CASE_VAR2STR(RTE_ETH_RSS_IPV4_CHKSUM)
        CASE_VAR2STR(RTE_ETH_RSS_L4_CHKSUM)
        CASE_VAR2STR(RTE_ETH_RSS_L2TPV2)
        CASE_VAR2STR(RTE_ETH_RSS_L3_SRC_ONLY)
        CASE_VAR2STR(RTE_ETH_RSS_L3_DST_ONLY)
        CASE_VAR2STR(RTE_ETH_RSS_L4_SRC_ONLY)
        CASE_VAR2STR(RTE_ETH_RSS_L4_DST_ONLY)
        CASE_VAR2STR(RTE_ETH_RSS_L2_SRC_ONLY)
        CASE_VAR2STR(RTE_ETH_RSS_L2_DST_ONLY)
        CASE_VAR2STR(RTE_ETH_RSS_L3_PRE32)
        CASE_VAR2STR(RTE_ETH_RSS_L3_PRE40)
        CASE_VAR2STR(RTE_ETH_RSS_L3_PRE48)
        CASE_VAR2STR(RTE_ETH_RSS_L3_PRE56)
        CASE_VAR2STR(RTE_ETH_RSS_L3_PRE64)
        CASE_VAR2STR(RTE_ETH_RSS_LEVEL_OUTERMOST)
        CASE_VAR2STR(RTE_ETH_RSS_LEVEL_INNERMOST)
#ifdef RTE_ETH_RSS_IPV6_FLOW_LABEL
        CASE_VAR2STR(RTE_ETH_RSS_IPV6_FLOW_LABEL)
#endif
#ifdef RTE_ETH_RSS_IB_BTH
        CASE_VAR2STR(RTE_ETH_RSS_IB_BTH)
#endif
    default:
        return "";
    }
}

std::string_view rteEthHashFunctionToString(uint32_t algorithm)
{
    switch (algorithm) {
        CASE_VAR2STR(RTE_ETH_HASH_FUNCTION_TOEPLITZ)
        CASE_VAR2STR(RTE_ETH_HASH_FUNCTION_SIMPLE_XOR)
        CASE_VAR2STR(RTE_ETH_HASH_FUNCTION_SYMMETRIC_TOEPLITZ)
        CASE_VAR2STR(RTE_ETH_HASH_FUNCTION_SYMMETRIC_TOEPLITZ_SORT)
    default:
        return "";
    }
}

static std::string_view yesNoStr(bool value)
{
    return value ? "yes" : "no";
}

std::string rteEthRssOffloadsToString(uint64_t offloadsBitset)
{
    std::string s;

    for (uint64_t i = 0; i < 64; i++) {
        const uint64_t offloadMask = uint64_t(1) << i;
        const bool isSet = ((offloadsBitset & offloadMask) == offloadMask);

        std::string offloadName {rteEthRssOffloadToString(offloadMask)};
        if (offloadName.empty() && !isSet) {
            // Does not exist and not set, we just iterate all the bits so this is expected
            continue;
        }
        if (offloadName.empty()) {
            // Unknown but set, so likely exists, we just dont know it
            offloadName = std::format("UNKNOWN<{}>", i);
        }

        std::format_to(std::back_inserter(s), "{:32} : {}\n", offloadName, yesNoStr(isSet));
    }

    return s;
}
std::string rteEthHashFunctionsToString(uint32_t hashFunctions)
{
    std::string s;

    for (uint32_t i = 0; i < 32; i++) {
        const uint32_t algoMask = RTE_ETH_HASH_ALGO_TO_CAPA(i);
        const bool isSet = ((hashFunctions & algoMask) == algoMask);

        std::string algoName {rteEthHashFunctionToString(i)};
        if (algoName.empty() && !isSet) {
            // Does not exist and not set, we just iterate all the bits so this is expected
            continue;
        }
        if (algoName.empty()) {
            // Unknown but set, so likely exists, we just dont know it
            algoName = std::format("UNKNOWN<{}>", i);
        }

        std::format_to(std::back_inserter(s), "{:32} : {}\n", algoName, yesNoStr(isSet));
    }

    return s;
}

rte_eth_dev_info ethDevInfoGet(uint16_t portId)
{
    rte_eth_dev_info info;
    const int ret = rte_eth_dev_info_get(portId, &info);
    if (ret != 0) {
        throw RteErrnoError(-ret, "rte_eth_dev_info_get", portId);
    }
    return info;
}

void ethDevStart(uint16_t portId)
{
    static constexpr unsigned maxAttempts = 5;

    int ret;
    for (unsigned i = 0; i < maxAttempts; ++i) {
        ret = rte_eth_dev_start(portId);
        if (ret != -EAGAIN) {
            break;
        }
    }

    if (ret != 0) {
        throw RteErrnoError(-ret, "rte_eth_dev_start", portId);
    }
}

void ethDevStop(uint16_t portId)
{
    const int ret = rte_eth_dev_stop(portId);
    if (ret != 0) {
        throw RteErrnoError(-ret, "rte_eth_dev_stop", portId);
    }
}

void ethDevAdjustRxTxDesc(uint16_t portId, uint16_t& nRxDesc, uint16_t& nTxDesc)
{
    const int ret = rte_eth_dev_adjust_nb_rx_tx_desc(portId, &nRxDesc, &nTxDesc);
    if (ret != 0) {
        throw RteErrnoError(-ret, "rte_eth_dev_adjust_nb_rx_tx_desc", portId, nRxDesc, nTxDesc);
    }
}
void ethDevConfigure(
    uint16_t portId,
    uint16_t nRxQueues,
    uint16_t nTxQueues,
    const rte_eth_conf* conf)
{
    const int ret = rte_eth_dev_configure(portId, nRxQueues, nTxQueues, conf);
    if (ret != 0) {
        throw RteErrnoError(-ret, "rte_eth_dev_configure", portId, nRxQueues, nTxQueues);
    }
}

void ethRxQueueSetup(
    uint16_t portId,
    uint16_t rxQueueId,
    uint16_t nRxDesc,
    unsigned int socketId,
    const rte_eth_rxconf* rxConf,
    rte_mempool* mempool)
{
    const int ret = rte_eth_rx_queue_setup(portId, rxQueueId, nRxDesc, socketId, rxConf, mempool);
    if (ret != 0) {
        throw RteErrnoError(-ret, "rte_eth_rx_queue_setup", portId, rxQueueId);
    }
}

void ethTxQueueSetup(
    uint16_t portId,
    uint16_t txQueueId,
    uint16_t nTxDesc,
    unsigned int socketId,
    const rte_eth_txconf* txConf)
{
    const int ret = rte_eth_tx_queue_setup(portId, txQueueId, nTxDesc, socketId, txConf);
    if (ret != 0) {
        throw RteErrnoError(-ret, "rte_eth_tx_queue_setup", portId, txQueueId);
    }
}

void ethDevLinkDown(uint16_t portId)
{
    const int ret = rte_eth_dev_set_link_down(portId);
    if (ret != 0) {
        throw RteErrnoError(-ret, "rte_eth_dev_set_link_down", portId);
    }
}

void ethDevLinkUp(uint16_t portId)
{
    const int ret = rte_eth_dev_set_link_up(portId);
    if (ret != 0) {
        throw RteErrnoError(-ret, "rte_eth_dev_set_link_up", portId);
    }
}

void ethPromiscuousSet(uint16_t portId, bool enable)
{
    if (enable) {
        const int ret = rte_eth_promiscuous_enable(portId);
        if (ret != 0) {
            throw RteErrnoError(-ret, "rte_eth_promiscuous_enable", portId);
        }
    } else {
        const int ret = rte_eth_promiscuous_disable(portId);
        if (ret != 0) {
            throw RteErrnoError(-ret, "rte_eth_promiscuous_disable", portId);
        }
    }
}

rte_flow* flowCreate(
    uint16_t portId,
    const rte_flow_attr* attr,
    const rte_flow_item pattern[],
    const rte_flow_action actions[])
{
    rte_flow* flow;
    rte_flow_error err;

    flow = rte_flow_create(portId, attr, pattern, actions, &err);
    if (flow == nullptr) {
        throw DpdkError(
            "rte_flow_create({}) has failed: {} ({}){}{} [type={}]",
            portId,
            rte_strerror(rte_errno),
            rte_errno,
            err.message == nullptr ? "" : ": ",
            err.message == nullptr ? "" : err.message,
            static_cast<int>(err.type));
    }

    return flow;
}

} // namespace amon::dpdk
