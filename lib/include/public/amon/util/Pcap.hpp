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

    /**
     * @brief Flush any not yet written packets to the output file.
     *
     * @throws amon::Error on `pcap_dump_flush()` fail.
     */
    void flush() const;

    /**
     * @brief Returns the current position in the underlying file stream.
     * @return Current position in the file stream.
     * @throws amon::ErrnoError on ftello64 failure.
     */
    uint64_t tell() const;

private:
    std::unique_ptr<pcap_t, detail::PcapClose> m_pcap;
    std::unique_ptr<pcap_dumper_t, detail::PcapDumpClose> m_pcapDumper;
    PcapTimestampPrecision m_precision;
    std::FILE* m_fp;
};

/**
 * @brief Options for creating PcapReader from interface.
 */
struct PcapInterfaceOptions {
    /// Accept all packets arriving on the interface (default: true).
    bool promiscuousMode = true;
    /// Do not buffer, deliver packets as soon as they arrive (default: true).
    bool immediateMode = true;
    /// Packet buffer timeout in milliseconds (default: 1000ms, 0 = throws (otherwise undefined
    /// behaviour)).
    unsigned int bufferTimeout = 1000;
    /// Size of the buffer that holds the packets before they're delivered (default: 0 (unset))
    unsigned int bufferSize = 0;
    /// Maximum size of packet before it gets truncated (default: 256KiB).
    unsigned int snapshotLength = 262144;
};

/**
 * @brief Reads packets from a pcap file.
 */
class PcapReader {
public:
    /**
     * @brief Creates and returns a PcapReader for the given interface.
     * @param interface Network interface to start listening on.
     * @param options Options for live capture
     * @param precision Timestamp precision (default: MICRO).
     * @return PcapReader instance
     */
    static PcapReader fromInterface(
        const std::string& interface,
        const PcapInterfaceOptions& options = {},
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
     * @brief Reads the next packet from the pcap source (file or live interface).
     * @return Optional packet, std::nullopt if end of file or timeout occurred.
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
     * @return Current position in the file stream, returns std::numeric_limits<uint64_t>::max() if
     * it's an interface
     */
    uint64_t tell() const;

    /**
     * @brief Returns the position at the end of the underlying file stream.
     * @return End position of the file stream, returns 0 if it's an interface
     */
    uint64_t tellEnd() const;

    /**
     * @brief Returns if reader hit an EOF.
     * @return Boolean value for EOF.
     */
    bool eof() const { return m_eof; }

private:
    explicit PcapReader(PcapTimestampPrecision precision)
        : m_precision(precision)
    {
    }

    std::unique_ptr<pcap_t, detail::PcapClose> m_pcap;
    PcapTimestampPrecision m_precision;
    bool m_eof = false; // Used for checking if reading hit an end
};

} // namespace amon::util
