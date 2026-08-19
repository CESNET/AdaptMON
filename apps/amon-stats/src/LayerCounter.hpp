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
     * @param readerPosition Current position of PcapReader.
     */
    void countAll(amon::Packet& packet, uint64_t readerPosition);

    /**
     * @brief Prints current statistics into stdout with the class root used.
     *
     * @param showPosition Prints progress through the file at the end.
     *
     * @return Number of lines outputted
     */
    uint32_t printNodes(bool showPosition = true);

    /**
     * @brief Recursively sorts the whole tree in descending order by count.
     */
    void sort();

    /**
     * @brief Writes statistics into a file in YAML format.
     *
     * @param path Path to the file to write to.
     */
    void writeToFile(std::string_view path);

    /**
     * @brief Constructs LayerCounter.
     *
     * @param readerEnd Byte offset of the end of the input file, used to compute the
     * percent done while printing progress.
     * @param isInterface Specifies if counter is being ran on an interface.
     */
    LayerCounter(const uint64_t readerEnd, bool isInterface)
        : m_readerEnd(readerEnd)
        , m_isInterface(isInterface)
    {
    }

private:
    /** Byte offset of the end of the input file, used to compute the percent done. */
    uint64_t m_readerEnd;
    /** Current position of reader inside the file */
    uint64_t m_readerPos = 0;
    /** Root of the layer tree; its children are the lowest parsed layers. */
    LayerNode m_rootNode {};
    /** Mutex for counting packets */
    std::mutex m_memberMutex;
    /** See if its running on a network interface */
    bool m_isInterface;

    amon::Logger m_logger = amon::LoggerProvider::get("LayerCounter");

    /**
     * @brief Prints current statistics into stdout with the specified node as root.
     *
     * Output is redrawn in place using ANSI escape sequences. When moveBack is set the cursor jumps
     * to back to the first line
     *
     * @param prefix Prefix to use for the current line.
     * @param root LayerNode whose children are printed.
     *
     * @return Formatted node text in the form of a pair consisting of the number of lines and the
     * text itself.
     */
    std::pair<uint32_t, std::string> printNodes(std::string_view prefix, const LayerNode& root);
};
