/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Pipeline stage for packet parsing.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon/dpdk/pipeline/PacketParser.hpp>

namespace amon::dpdk::pipeline {

PacketParserStream::PacketParserStream(
    StreamContext ctx,
    PacketParserConfig config,
    layers::LayerType firstLayer)
    : Stream(ctx)
    , m_packetParser(config)
    , m_firstLayer(firstLayer)
{
}

void PacketParserStream::process(PktBurst<DpdkPacket>& burst)
{
    for (auto& pkt : burst) {
        m_packetParser.parse(pkt->packet(), m_firstLayer);
    }
}

PacketParser::PacketParser(PacketParserConfig config, layers::LayerType firstLayer)
    : m_config(config)
    , m_firstLayer(firstLayer)
{
}

void PacketParser::createStream(StreamContext ctx)
{
    addStream<PacketParserStream>(ctx, m_config, m_firstLayer);
}

} // namespace amon::dpdk::pipeline
