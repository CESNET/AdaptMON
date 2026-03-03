/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Packet statistics
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 */

#include "Stats.hpp"

#include <print>

void Stats::updatePacketSizeHistogram(size_t pktSize)
{
    if (pktSize <= 63) {
        n0to63++;
    } else if (pktSize <= 127) {
        n64to127++;
    } else if (pktSize <= 255) {
        n128to255++;
    } else if (pktSize <= 511) {
        n256to511++;
    } else if (pktSize <= 1023) {
        n512to1023++;
    } else if (pktSize <= 1518) {
        n1024to1518++;
    } else if (pktSize <= 2047) {
        n1519to2047++;
    } else if (pktSize <= 4095) {
        n2048to4095++;
    } else if (pktSize <= 8191) {
        n4096to8191++;
    } else {
        n8192toMax++;
    }
}

void Stats::print() const
{
    std::println("Statistics:");
    std::println("    Total packets:              {}", nPackets);
    std::println("    Errornous packets:          {}", nErrPackets);
    std::println("    Packets with VLAN:          {}", nVLAN);
    std::println("    Packets with MPLS:          {}", nMPLS);
    std::println("    Packets with GRE:           {}", nGRE);
    std::println("    Packets with IPv4:          {}", nIPv4);
    std::println("    Packets with IPv6:          {}", nIPv6);
    std::println("    Packets with ICMP:          {}", nICMP);
    std::println("    Packets with ICMPv6:        {}", nICMPv6);
    std::println("    Packets with UDP:           {}", nUDP);
    std::println("    Packets with TCP:           {}", nTCP);
    std::println("    Total bytes:                {}", nBytes);
    std::println("");
    std::println("Packet sizes:");
    std::println("    0 - 63:                     {}", n0to63);
    std::println("    64 - 127:                   {}", n64to127);
    std::println("    128 - 255:                  {}", n128to255);
    std::println("    256 - 511:                  {}", n256to511);
    std::println("    512 - 1023:                 {}", n512to1023);
    std::println("    1024 - 1518:                {}", n1024to1518);
    std::println("    1519 - 2047:                {}", n1519to2047);
    std::println("    2048 - 4095:                {}", n2048to4095);
    std::println("    4096 - 8191:                {}", n4096to8191);
    std::println("    8192+:                      {}", n8192toMax);
}
