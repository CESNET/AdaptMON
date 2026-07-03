/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Simple wrappers around DPDK rte_ functions.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include <rte_ethdev.h>

namespace amon::dpdk {

/**
 * @brief Convert a single DPDK RSS offload flag to its constant name.
 * @param mask RSS offload flag.
 * @return Constant name, or an empty string for an unknown flag.
 */
std::string_view rteEthRssOffloadToString(uint64_t mask);

/**
 * @brief Convert a single DPDK hash function flag to its constant name.
 * @param algorithm Hash function.
 * @return Constant name, or an empty string for an unknown flag.
 */
std::string_view rteEthHashFunctionToString(uint32_t algorithm);

/**
 * @brief Format DPDK RSS offload flags as a human-readable list.
 * @param offloadsBitset RSS offload flags bitset.
 * @return Human-readable list of known RSS offload flags.
 */
std::string rteEthRssOffloadsToString(uint64_t offloadsBitset);

/**
 * @brief Format DPDK hash function flags as a human-readable list.
 * @param hashFunctions Hash function flags bitset.
 * @return Human-readable list of known hash function flags.
 */
std::string rteEthHashFunctionsToString(uint32_t hashFunctions);

/**
 * @brief Wrapper around rte_eth_dev_info_get().
 * @param portId DPDK port ID.
 * @return DPDK Ethernet device info.
 * @throws RteErrnoError On DPDK error.
 */
rte_eth_dev_info ethDevInfoGet(uint16_t portId);

/**
 * @brief Wrapper around rte_eth_dev_start().
 * @param portId DPDK port ID.
 * @throws RteErrnoError On DPDK error.
 */
void ethDevStart(uint16_t portId);

/**
 * @brief Wrapper around rte_eth_dev_stop().
 * @param portId DPDK port ID.
 * @throws RteErrnoError On DPDK error.
 */
void ethDevStop(uint16_t portId);

/**
 * @brief Wrapper around rte_eth_dev_adjust_nb_rx_tx_desc().
 * @param portId DPDK port ID.
 * @param[in,out] nRxDesc Number of RX descriptors.
 * @param[in,out] nTxDesc Number of TX descriptors.
 * @throws RteErrnoError On DPDK error.
 */
void ethDevAdjustRxTxDesc(uint16_t portId, uint16_t& nRxDesc, uint16_t& nTxDesc);

/**
 * @brief Wrapper around rte_eth_dev_configure().
 * @param portId DPDK port ID.
 * @param nRxQueues Number of RX queues.
 * @param nTxQueues Number of TX queues.
 * @param conf DPDK Ethernet device configuration.
 * @throws RteErrnoError On DPDK error.
 */
void ethDevConfigure(
    uint16_t portId,
    uint16_t nRxQueues,
    uint16_t nTxQueues,
    const rte_eth_conf* conf);

/**
 * @brief Wrapper around rte_eth_rx_queue_setup().
 * @param portId DPDK port ID.
 * @param rxQueueId RX queue ID.
 * @param nRxDesc Number of RX descriptors.
 * @param socketId NUMA socket ID.
 * @param rxConf DPDK RX queue configuration.
 * @param mempool Mempool used by the RX queue.
 * @throws RteErrnoError On DPDK error.
 */
void ethRxQueueSetup(
    uint16_t portId,
    uint16_t rxQueueId,
    uint16_t nRxDesc,
    unsigned int socketId,
    const rte_eth_rxconf* rxConf,
    rte_mempool* mempool);

/**
 * @brief Wrapper around rte_eth_tx_queue_setup().
 * @param portId DPDK port ID.
 * @param txQueueId TX queue ID.
 * @param nTxDesc Number of TX descriptors.
 * @param socketId NUMA socket ID.
 * @param txConf DPDK TX queue configuration.
 * @throws RteErrnoError On DPDK error.
 */
void ethTxQueueSetup(
    uint16_t portId,
    uint16_t txQueueId,
    uint16_t nTxDesc,
    unsigned int socketId,
    const rte_eth_txconf* txConf);

/**
 * @brief Wrapper around rte_eth_dev_set_link_down().
 * @param portId DPDK port ID.
 * @throws RteErrnoError On DPDK error.
 */
void ethDevLinkDown(uint16_t portId);

/**
 * @brief Wrapper around rte_eth_dev_set_link_up().
 * @param portId DPDK port ID.
 * @throws RteErrnoError On DPDK error.
 */
void ethDevLinkUp(uint16_t portId);

/**
 * @brief Wrapper around rte_eth_promiscuous_enable() or rte_eth_promiscuous_disable().
 * @param portId DPDK port ID.
 * @param enable Enable promiscuous mode when true, disable it when false.
 * @throws RteErrnoError On DPDK error.
 */
void ethPromiscuousSet(uint16_t portId, bool enable);

/**
 * @brief Wrapper around rte_flow_create().
 * @param portId DPDK port ID.
 * @param attr Flow rule attributes.
 * @param pattern Flow pattern array.
 * @param actions Flow actions array.
 * @return Created DPDK flow.
 * @throws DpdkError On DPDK error.
 */
rte_flow* flowCreate(
    uint16_t portId,
    const rte_flow_attr* attr,
    const rte_flow_item pattern[],
    const rte_flow_action actions[]);

} // namespace amon::dpdk
