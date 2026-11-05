/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Pipeline stage for packet parsing.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/PacketParser.hpp>
#include <amon/dpdk/DpdkPacket.hpp>
#include <amon/dpdk/PktBurst.hpp>
#include <amon/dpdk/pipeline/Stage.hpp>
#include <amon/dpdk/pipeline/Stream.hpp>

namespace amon::dpdk::pipeline {

/// @brief Packet parser stream.
class PacketParserStream : public Stream {
public:
    /**
     * @brief Construct a packet parser stream.
     * @param ctx Context identifying the owning stage and stream.
     * @param config The config used for `amon::PacketParser` construction.
     * @param firstLayer The first layer of the packet (see `amon::PackerParser::parse` docs).
     */
    PacketParserStream(StreamContext ctx, PacketParserConfig config, layers::LayerType firstLayer);

    /**
     * @brief Parse every packet in a burst and populate its amon::Packet structure.
     *
     * The burst is not consumed by this operation - all packets are kept in the burst.
     *
     * @param burst Packet burst to process.
     */
    void process(PktBurst<DpdkPacket>& burst) override;

private:
    amon::PacketParser m_packetParser;
    layers::LayerType m_firstLayer;
};

/// @brief Packet parser stage.
class PacketParser : public Stage {
public:
    /**
     * @brief Construct the stage.
     * @param ctx Stage context.
     * @param config The config used for `amon::PacketParser` construction.
     * @param firstLayer The first layer of the packet (see `amon::PackerParser::parse` docs).
     */
    PacketParser(
        StageContext ctx,
        PacketParserConfig config = PacketParserConfig {},
        layers::LayerType firstLayer = layers::LayerType::ETHERNET);

    /**
     * @brief Create the stream.
     * @param ctx Context of the stream to create.
     */
    void createStream(StreamContext ctx) override;

private:
    PacketParserConfig m_config;
    layers::LayerType m_firstLayer;
};

} // namespace amon::dpdk::pipeline
