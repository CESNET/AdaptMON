/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Pipeline stages for writing packet streams to PCAP files.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/dpdk/DpdkPacket.hpp>
#include <amon/dpdk/PktBurst.hpp>
#include <amon/dpdk/pipeline/Stage.hpp>
#include <amon/dpdk/pipeline/Stream.hpp>
#include <amon/util/Pcap.hpp>

namespace amon::dpdk::pipeline {

/** @brief Per-stream packet processor that writes packets to a PCAP file. */
class PcapWriterStream : public Stream {
public:
    /**
     * @brief Construct a PCAP writer stream.
     * @param ctx Context identifying the owning stage and stream.
     * @param filename Path of the PCAP file to write.
     */
    PcapWriterStream(StreamContext ctx, const std::string& filename);

    /**
     * @brief Write every packet in a burst to the output file.
     *
     * The burst is not consumed by this operation - all packets are kept in the burst.
     *
     * @param burst Packet burst to write.
     */
    void process(PktBurst<DpdkPacket>& burst) override;

private:
    util::PcapWriter m_writer;
};

/** @brief Pipeline stage that creates one PCAP output file per stream. */
class PcapWriter : public Stage {
public:
    /**
     * @brief Construct a PCAP writer stage.
     * @param outputFilePattern Format string used to produce each stream's output filename. The
     * stream ID is supplied as the first formatting argument.
     */
    PcapWriter(std::string outputFilePattern);

    /**
     * @brief Create the PCAP writer for a stream.
     * @param ctx Context of the stream to create.
     */
    void createStream(StreamContext ctx) override;

private:
    std::string m_outputFilePattern;
};

} // namespace amon::dpdk::pipeline
