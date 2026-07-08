/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Packet statistics
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 */

#pragma once

#include <cstddef>
#include <cstdint>

/**
 * @brief Collects statistics about network packets.
 *
 * This structure maintains counters for various packet types, errors, and size histograms.
 */
struct Stats {
    /** Total number of packets processed. */
    uint64_t nPackets = 0;
    /** Number of erroneous packets. */
    uint64_t nErrPackets = 0;
    /** Number of VLAN packets. */
    uint64_t nVLAN = 0;
    /** Number of MPLS packets. */
    uint64_t nMPLS = 0;
    /** Number of GRE packets. */
    uint64_t nGRE = 0;
    /** Number of IPv4 packets. */
    uint64_t nIPv4 = 0;
    /** Number of IPv6 packets. */
    uint64_t nIPv6 = 0;
    /** Number of ICMP packets. */
    uint64_t nICMP = 0;
    /** Number of ICMPv6 packets. */
    uint64_t nICMPv6 = 0;
    /** Number of UDP packets. */
    uint64_t nUDP = 0;
    /** Number of TCP packets. */
    uint64_t nTCP = 0;
    /** Number of ESP packet. */
    uint64_t nESP = 0;

    /** Total number of bytes processed. */
    uint64_t nBytes = 0;
    /** Number of packets with size 0-63 bytes. */
    uint64_t n0to63 = 0;
    /** Number of packets with size 64-127 bytes. */
    uint64_t n64to127 = 0;
    /** Number of packets with size 128-255 bytes. */
    uint64_t n128to255 = 0;
    /** Number of packets with size 256-511 bytes. */
    uint64_t n256to511 = 0;
    /** Number of packets with size 512-1023 bytes. */
    uint64_t n512to1023 = 0;
    /** Number of packets with size 1024-1518 bytes. */
    uint64_t n1024to1518 = 0;
    /** Number of packets with size 1519-2047 bytes. */
    uint64_t n1519to2047 = 0;
    /** Number of packets with size 2048-4095 bytes. */
    uint64_t n2048to4095 = 0;
    /** Number of packets with size 4096-8191 bytes. */
    uint64_t n4096to8191 = 0;
    /** Number of packets with size 8192 bytes up to maximum. */
    uint64_t n8192toMax = 0;

    /**
     * @brief Updates the packet size histogram based on the given packet size.
     * @param pktSize Size of the packet in bytes.
     */
    void updatePacketSizeHistogram(size_t pktSize);

    /**
     * @brief Prints the collected statistics to standard output.
     */
    void print() const;
};
