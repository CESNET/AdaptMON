/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Pipeline stages for writing packet streams to PCAP files.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon/dpdk/pipeline/PcapWriter.hpp>

namespace amon::dpdk::pipeline {

PcapWriterStream::PcapWriterStream(StreamContext ctx, const std::string& filename)
    : Stream(ctx)
    , m_writer(filename)
{
}

void PcapWriterStream::process(PktBurst<DpdkPacket>& burst)
{
    for (const auto& pkt : burst) {
        m_writer.writePacket(pkt->packet().data, pkt->packet().timestamp);
    }
}

PcapWriter::PcapWriter(std::string outputFilePattern)
    : m_outputFilePattern(std::move(outputFilePattern))
{
}

void PcapWriter::createStream(StreamContext ctx)
{
    addStream<PcapWriterStream>(
        ctx,
        std::vformat(m_outputFilePattern, std::make_format_args(ctx.id)));
}

} // namespace amon::dpdk::pipeline
