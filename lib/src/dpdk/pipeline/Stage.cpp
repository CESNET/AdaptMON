/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Pipeline stage implementation.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon/dpdk/pipeline/Stage.hpp>

namespace amon::dpdk::pipeline {

void Stage::createStreams(std::vector<LCoreHandle>& lcores)
{
    uint16_t id = 0;
    for (auto& lcore : lcores) {
        const auto ctx = StreamContext {
            .stage = *this,
            .id = id,
        };
        lcore.start([=, this](const std::atomic_bool&) { createStream(ctx); });
        ++id;
    }
    for (auto& lcore : lcores) {
        lcore.join();
    }
}

void Stage::process(uint16_t streamId, PktBurst<DpdkPacket>& burst)
{
    m_streams[streamId]->process(burst);
}

} // namespace amon::dpdk::pipeline
