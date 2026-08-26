/**
 * @file
 * @author Samuel Luptak <luptak@cesnet.cz>
 * @brief Receiver configuration
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

/**
 * @brief Receiver configuration.
 *
 * Parses and holds settings for the packet receiver.
 */
class Config {
public:
    /**
     * @brief Constructs Config from command line arguments.
     * @param argc Number of command-line arguments.
     * @param argv Array of command-line argument strings.
     */
    Config(int argc, char** argv);

    std::vector<std::string> ealArgs; ///< DPDK EAL argument list.
    std::string devName; ///< DPDK device name.
    uint16_t rxQueues; ///< Number of RX queues.
    uint32_t statsInterval; ///< Statistics sampling interval in milliseconds.
    unsigned int mempoolSize; ///< Size of the DPDK mempool in packets.
    size_t burstSize; ///< Maximum number of packets per receive burst.
    bool noHugepagesSetup; ///< Do not set up DPDK hugepages.
    bool enableParser; ///< Enable packet parsing and L3 statistics.
};
