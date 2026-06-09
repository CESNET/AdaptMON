/**
 * @file
 * @author Petr Macha <petr.macha@cesnet.cz>
 * @brief Layer counting class
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 */

#pragma once
#include "LayerNode.hpp"

#include <amon/Logger.hpp>
#include <amon/Packet.hpp>
#include <amon/util/Pcap.hpp>

#include <mutex>

/**
 * @brief Builds and maintains a tree of protocol layer occurrences.
 */
class LayerCounter {
public:
    /**
     * @brief Reads PcapPackets and transforms them into Packets. After counts all layers found
     * inside the packet.
     *
     * @param packet Packet to count.
     */
    void countAll(amon::Packet& packet);

    /**
     * @brief Prints current statistics into stdout with the class root used.
     */
    void printNodes();

    /**
     * @brief Recursively sorts the whole tree in descending order by count.
     */
    void sort();

private:
    /** Root of the layer tree; its children are the lowest parsed layers. */
    LayerNode m_rootNode {};
    /** Mutex for counting packets */
    std::mutex m_memberMutex;

    amon::Logger m_logger = amon::LoggerProvider::get("LayerCounter");

    /**
     * @brief Prints current statistics into stdout with the specified node as root.
     *
     * @param prefix Prefix to use for the current line.
     * @param root LayerNode whose children are printed.
     */
    std::string printNodes(std::string_view prefix, const LayerNode& root);
};
