/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Pcap file reading and writing utilities
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/types/Timestamp.hpp>
#include <amon/util/Formatter.hpp>

#include <cassert>
#include <cstdio>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <pcap/pcap.h>

namespace amon::util {

/// @cond
namespace detail {

struct PcapClose {
    void operator()(pcap_t* pcap) const noexcept { pcap_close(pcap); }
};

struct PcapDumpClose {
    void operator()(pcap_dumper_t* dumper) const noexcept { pcap_dump_close(dumper); }
};

} // namespace detail
/// @endcond

/**
 * @brief Timestamp precision options for pcap files.
 */
enum class PcapTimestampPrecision {
    MICRO = PCAP_TSTAMP_PRECISION_MICRO, ///< Microsecond precision
    NANO = PCAP_TSTAMP_PRECISION_NANO, ///< Nanosecond precision
};

/**
 * @brief Formatter for PcapTimestampPrecision.
 * @param value Value to format.
 * @param fmt Formatter to write to.
 */
void format(PcapTimestampPrecision value, Formatter& fmt);

/**
 * @brief Represents a single packet in a pcap file.
 */
struct PcapPacket {
    std::span<const std::byte> data; ///< Packet data
    types::Timestamp timestamp; ///< Packet timestamp
    uint32_t lengthOnWire; ///< Length of the packet as seen on the wire
};

/**
 * @brief Writes packets to a pcap file.
 */
class PcapWriter {
public:
    /**
     * @brief Constructs a PcapWriter for the given file.
     * @param filename Path to the pcap file to write.
     * @param precision Timestamp precision (default: MICRO).
     * @param linkLayerType The link layer type, aka "DLT". (default: IEEE 802.3 Ethernet). For more
     * about linkLayerType values, see https://www.tcpdump.org/linktypes.html.
     * @note DLT_EN10MB = IEEE 802.3 Ethernet (10Mb, 100Mb, 1000Mb, and up); the 10MB in the DLT_
     * name is historical.
     */
    PcapWriter(
        const std::string& filename,
        PcapTimestampPrecision precision = PcapTimestampPrecision::MICRO,
        int linkLayerType = DLT_EN10MB);

    /**
     * @brief Writes a packet to the pcap file.
     * @param data The packet data to write.
     * @param timestamp The timestamp associated with the packet.
     * @param lengthOnWire The original length of the packet on the wire. If not provided, uses the
     * size of data.
     */
    void writePacket(
        std::span<const std::byte> data,
        types::Timestamp timestamp,
        std::optional<uint32_t> lengthOnWire = std::nullopt);

private:
    std::unique_ptr<pcap_t, detail::PcapClose> m_pcap;
    std::unique_ptr<pcap_dumper_t, detail::PcapDumpClose> m_pcapDumper;
    PcapTimestampPrecision m_precision;
    std::FILE* m_fp;
};

/**
 * @brief Reads packets from a pcap file.
 */
class PcapReader {
public:
    /**
     * @brief Constructs a PcapReader for the given file.
     * @param filename Path to the pcap file to read.
     * @param precision Timestamp precision (default: MICRO).
     */
    PcapReader(
        const std::string& filename,
        PcapTimestampPrecision precision = PcapTimestampPrecision::MICRO);

    /**
     * @brief Creates and returns a PcapReader for the given file.
     * @param filename Path to a pcap file.
     * @param precision Timestamp precision (default: MICRO).
     * @return PcapReader instance
     */
    static PcapReader fromFile(
        const std::string& filename,
        PcapTimestampPrecision precision = PcapTimestampPrecision::MICRO);

    /**
     * @brief Reads the next packet from the pcap file.
     * @return Optional packet, std::nullopt if end of file.
     * @warning The data in the returned packet are valid only until next packet is read or until
     * this PcapReader instance is destructed.
     */
    std::optional<PcapPacket> readPacket();

    /**
     * @brief Get the link layer type of the pcap, aka DLT.
     * @return The link layer type (see https://www.tcpdump.org/linktypes.html).
     */
    int linkLayerType() const;

    /**
     * @brief Returns the current position in the underlying file stream.
     * @return The current position as an unsigned 64-bit integer.
     */
    uint64_t tell() const;

    /**
     * @brief Returns the position at the end of the underlying file stream.
     * @return The end position as an unsigned 64-bit integer.
     */
    uint64_t tellEnd() const;

private:
    explicit PcapReader(PcapTimestampPrecision precision)
        : m_precision(precision)
    {
    }

    std::unique_ptr<pcap_t, detail::PcapClose> m_pcap;
    PcapTimestampPrecision m_precision;
};

} // namespace amon::util
