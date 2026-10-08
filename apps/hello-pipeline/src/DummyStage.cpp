/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Custom stage example.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "DummyStage.hpp"

void DummyStream::process(amon::dpdk::PktBurst<amon::dpdk::DpdkPacket>& burst)
{
    m_pktsProcessed += burst.size();
}

void DummyStage::createStream(amon::dpdk::pipeline::StreamContext ctx)
{
    addStream<DummyStream>(ctx);
}

uint64_t DummyStage::pktsProcessedTotal() const
{
    uint64_t sum = 0;
    for (const auto& stream : streams()) {
        sum += dynamic_cast<DummyStream*>(stream.get())->pktsProcessed();
    }
    return sum;
}
