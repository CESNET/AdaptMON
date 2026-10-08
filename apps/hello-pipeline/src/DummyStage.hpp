/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Custom stage example.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/dpdk/pipeline/Stage.hpp>
#include <amon/dpdk/pipeline/Stream.hpp>

/// @brief Instance that gets initialized per-worker.
class DummyStream : public amon::dpdk::pipeline::Stream {
public:
    using amon::dpdk::pipeline::Stream::Stream;

    /// @copydoc amon::dpdk::pipeline::Stream::process
    void process(amon::dpdk::PktBurst<amon::dpdk::DpdkPacket>& burst) override;

    /// @brief Get number of processed packets.
    /// @return Number of processed packets by this stream.
    uint64_t pktsProcessed() const { return m_pktsProcessed; }

private:
    std::atomic_uint64_t m_pktsProcessed;
};

/// @brief Instance that gets initialized once.
class DummyStage : public amon::dpdk::pipeline::Stage {
public:
    using amon::dpdk::pipeline::Stage::Stage;

    /// @copydoc amon::dpdk::pipeline::Stage::createStream
    void createStream(amon::dpdk::pipeline::StreamContext ctx) override;

    /// @brief Get number of processed packets by all streams.
    /// @return Number of processed packets by all streams.
    uint64_t pktsProcessedTotal() const;
};
