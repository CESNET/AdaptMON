/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Base interface and context for pipeline streams.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/dpdk/DpdkPacket.hpp>
#include <amon/dpdk/PktBurst.hpp>

#include <cstdint>

namespace amon::dpdk::pipeline {

class Stage;

/** @brief Identifies a stream and its owning stage. */
struct StreamContext {
    /// Stage that owns the stream.
    Stage& stage;
    /// Zero-based stream identifier within the stage.
    uint16_t id;
};

/**
 * @brief Per-worker packet processing unit owned by a Stage.
 *
 * A concrete stream derives from Stream and implements process(). Its owning Stage constructs one
 * instance for each worker through addStream():
 * @code
 * class PacketSizeHistogramStream : public Stream {
 * public:
 *     using Stream::Stream;
 *
 *     void process(PktBurst<DpdkPacket>& burst) override
 *     {
 *         for (auto* packet : burst) {
 *             const auto size = rte_pktmbuf_pkt_len(packet->mbuf());
 *             const auto bucket = size <= 64    ? 0
 *                               : size <= 512   ? 1
 *                               : size <= 1'500 ? 2
 *                                               : 3;
 *             ++m_histogram[bucket];
 *         }
 *     }
 *
 *     const std::array<uint64_t, 4>& histogram() const { return m_histogram; }
 *
 * private:
 *     // Packet counts for sizes <=64, 65-512, 513-1500, and >1500 bytes.
 *     std::array<uint64_t, 4> m_histogram {};
 * };
 *
 * class PacketSizeHistogram : public Stage {
 * protected:
 *     void createStream(StreamContext ctx) override
 *     {
 *         addStream<PacketSizeHistogramStream>(ctx);
 *     }
 * };
 * @endcode
 */
class Stream {
public:
    /**
     * @brief Construct a stream.
     * @param context Owning stage and stream identifier.
     */
    Stream(StreamContext context);

    /**
     * @brief Process a packet burst.
     * @param burst Burst passed through the pipeline. Implementations may modify its contents.
     */
    virtual void process(PktBurst<DpdkPacket>& burst) = 0;

    /** @brief Destroy the stream. */
    virtual ~Stream() = default;

    /**
     * @brief Get the stage that owns this stream.
     * @return Reference to the owning stage.
     */
    Stage& stage() const { return m_context.stage; }

    /**
     * @brief Get the stream identifier.
     * @return Zero-based identifier within the owning stage.
     */
    uint16_t id() const { return m_context.id; }

private:
    StreamContext m_context;
};

} // namespace amon::dpdk::pipeline
