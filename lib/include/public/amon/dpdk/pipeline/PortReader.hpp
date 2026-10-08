/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Pipeline stages for receiving packets from DPDK ports.
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

namespace amon::dpdk::pipeline {

/** @brief Per-stream packet processor that receives bursts from a port RX queue. */
class PortReaderStream : public Stream {
public:
    /**
     * @brief Construct a port reader stream.
     * @param ctx Context identifying the owning stage and stream.
     * @param queue RX queue from which packets are received.
     */
    PortReaderStream(StreamContext ctx, PortRxQueue queue);

    /**
     * @brief Receive available packets into a burst.
     * @param burst Burst populated with received packets.
     */
    void process(PktBurst<DpdkPacket>& burst) override;

private:
    PortRxQueue m_queue;
};

/** @brief Pipeline stage that assigns a port RX queue to each stream. */
class PortReader : public Stage {
public:
    /**
     * @brief Construct a port reader stage.
     * @param port Port whose RX queues are used by the stage streams.
     */
    PortReader(PortHandle port);

    /**
     * @brief Create a reader backed by the RX queue matching the stream ID.
     * @param ctx Context of the stream to create.
     */
    void createStream(StreamContext ctx) override;

private:
    PortHandle m_port;
};

} // namespace amon::dpdk::pipeline
