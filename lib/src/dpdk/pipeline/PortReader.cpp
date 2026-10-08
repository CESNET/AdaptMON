/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief DPDK port reader stage implementation.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon/dpdk/pipeline/PortReader.hpp>

namespace amon::dpdk::pipeline {

PortReaderStream::PortReaderStream(StreamContext ctx, PortRxQueue queue)
    : Stream(ctx)
    , m_queue(std::move(queue))
{
}

void PortReaderStream::process(PktBurst<DpdkPacket>& burst)
{
    m_queue.receive(burst);
}

PortReader::PortReader(PortHandle port)
    : m_port(std::move(port))
{
}

void PortReader::createStream(StreamContext ctx)
{
    addStream<PortReaderStream>(ctx, m_port.rxQueue(ctx.id));
}

} // namespace amon::dpdk::pipeline
