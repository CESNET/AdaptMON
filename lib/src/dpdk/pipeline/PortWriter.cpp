/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief DPDK port writer stage implementation.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon/dpdk/pipeline/PortWriter.hpp>

namespace amon::dpdk::pipeline {

PortWriterStream::PortWriterStream(StreamContext ctx, PortTxQueue queue)
    : Stream(ctx)
    , m_queue(std::move(queue))
{
}

void PortWriterStream::process(PktBurst<DpdkPacket>& burst)
{
    m_queue.transmit(burst);
}

PortWriter::PortWriter(PortHandle port)
    : m_port(std::move(port))
{
}

void PortWriter::createStream(StreamContext ctx)
{
    addStream<PortWriterStream>(ctx, m_port.txQueue(ctx.id));
}

} // namespace amon::dpdk::pipeline
