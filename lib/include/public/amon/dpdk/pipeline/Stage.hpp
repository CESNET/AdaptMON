/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Base class for packet processing pipeline stages.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/dpdk/DpdkPacket.hpp>
#include <amon/dpdk/LCore.hpp>
#include <amon/dpdk/PktBurst.hpp>
#include <amon/dpdk/pipeline/Stream.hpp>

#include <memory>
#include <vector>

namespace amon::dpdk::pipeline {

/**
 * @brief Collection of equivalent per-worker packet processing streams.
 *
 * Derived stages create one Stream instance for every pipeline worker. Packet bursts are routed to
 * the stream whose ID matches the worker ID.
 */
class Stage {
public:
    /** @brief Destroy the stage and its streams. */
    virtual ~Stage() = default;

    /**
     * @brief Create all per-worker streams for this stage.
     * @param lcores Lcores to create the streams on.
     */
    void createStreams(std::vector<LCoreHandle>& lcores);

    /**
     * @brief Process a burst using a selected stream.
     * @param streamId ID of the stream that processes the burst.
     * @param burst Packet burst passed through the stage.
     */
    void process(uint16_t streamId, PktBurst<DpdkPacket>& burst);

    /**
     * @brief Get streams the stage holds.
     * @return The streams.
     */
    const std::vector<std::unique_ptr<Stream>>& streams() const { return m_streams; }

protected:
    /**
     * @brief Create and register one stream implementation.
     * @param ctx Context of the stream to create.
     */
    virtual void createStream(StreamContext ctx) = 0;

    /**
     * @brief Construct and append a stream to this stage.
     * @tparam StreamType Stream implementation to construct. Must derive from Stream.
     * @tparam StreamArgs Constructor argument types following StreamContext.
     * @param ctx Context passed as the first constructor argument.
     * @param args Remaining arguments passed to the stream constructor.
     */
    template <typename StreamType, typename... StreamArgs>
    void addStream(StreamContext ctx, StreamArgs&&... args)
    {
        m_streams.push_back(std::make_unique<StreamType>(ctx, std::forward<StreamArgs>(args)...));
    }

private:
    std::vector<std::unique_ptr<Stream>> m_streams;
};

} // namespace amon::dpdk::pipeline
