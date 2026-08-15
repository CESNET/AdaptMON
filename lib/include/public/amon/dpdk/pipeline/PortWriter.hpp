/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Pipeline stages for transmitting packets through DPDK ports.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/dpdk/DpdkPacket.hpp>
#include <amon/dpdk/PktBurst.hpp>
#include <amon/dpdk/Port.hpp>
#include <amon/dpdk/pipeline/Stage.hpp>
#include <amon/dpdk/pipeline/Stream.hpp>

#include <utility>

namespace amon::dpdk::pipeline {

/** @brief Per-stream packet processor that transmits bursts through a port TX queue. */
class PortWriterStream : public Stream {
public:
    /**
     * @brief Construct a port writer stream.
     * @param ctx Context identifying the owning stage and stream.
     * @param queue TX queue through which packets are transmitted.
     */
    PortWriterStream(StreamContext ctx, PortTxQueue queue);

    /**
     * @brief Transmit a packet burst.
     *
     * The burst is consumed by this operation - transmitted packets are released from the burst.
     * Packets that could not be transmitted remain.
     *
     * @param burst Burst to transmit.
     */
    void process(PktBurst<DpdkPacket>& burst) override;

private:
    PortTxQueue m_queue;
};

/** @brief Pipeline stage that assigns a port TX queue to each stream. */
class PortWriter : public Stage {
public:
    /**
     * @brief Construct a port writer stage.
     * @param port Port whose TX queues are used by the stage streams.
     */
    PortWriter(PortHandle port);

    /**
     * @brief Create a writer backed by the TX queue matching the stream ID.
     * @param ctx Context of the stream to create.
     */
    void createStream(StreamContext ctx) override;

private:
    PortHandle m_port;
};

} // namespace amon::dpdk::pipeline
