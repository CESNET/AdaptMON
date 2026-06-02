/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Packet anonymizer class
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 */

#pragma once

#include "Stats.hpp"
#include "ValueAnonymizer.hpp"

#include <amon/PacketParser.hpp>
#include <amon/layers/Layer.hpp>

#include <cstddef>
#include <string>
#include <vector>

/**
 * @brief Provides functionality to anonymize network packets.
 *
 * This class is responsible for processing and anonymizing packet data
 * to protect sensitive information before further analysis or storage.
 */
class PacketAnonymizer {
public:
    /**
     * @brief Constructs a PacketAnonymizer with the specified parameters.
     *
     * @param anonymizationKey The key used for anonymizing packets (must be exactly 32 characters).
     * @param firstLayerType The type of the first layer (used in parsing).
     * @param skipErroneous If true, skip packets with errors during anonymization.
     */
    PacketAnonymizer(
        const std::string& anonymizationKey,
        amon::layers::LayerType firstLayerType,
        bool skipErroneous);

    /**
     * @brief Anonymizes a network packet in-place.
     *
     * Parses the given packet data, updates statistics, and anonymizes each layer of the packet.
     * If parsing fails and skipErroneous is true, the packet is skipped.
     *
     * @param packetData The packet data to anonymize, as a vector of bytes.
     * @return true if the packet was successfully anonymized or processed; false if skipped due to
     * parse error.
     */
    bool anonymizePacket(std::vector<std::byte>& packetData);

    /**
     * @brief Get the statistics of processed packets.
     * @return Reference to the current Stats object.
     */
    const Stats& stats() const { return m_stats; }

private:
    ValueAnonymizer m_valueAnon;
    amon::layers::LayerType m_firstLayerType;
    bool m_skipErroneous;

    Stats m_stats;
    amon::PacketParser m_parser;
    std::vector<std::byte> m_buffer;

    void anonymizeLayer(std::span<std::byte> layerData, amon::layers::LayerType layerType);

    void anonymizeIPv6SegmentRouting(std::span<std::byte> data);
};
