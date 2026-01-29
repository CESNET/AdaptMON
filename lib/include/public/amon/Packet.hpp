/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief Packet structure and API for parsed network packets.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/layers/Layer.hpp>
#include <amon/types/InplaceVector.hpp>
#include <amon/types/Timestamp.hpp>

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <variant>

namespace amon {

/// Maximum number of layers in a packet.
constexpr static const unsigned MAX_LAYERS = 32;

/**
 * @brief Structure representing error packet layer.
 */
struct ErrorLayer {
    layers::LayerType type; ///< Type of the layer.
    layers::LayerError error; ///< Error status for this layer.
    uint16_t offset; ///< Offset of the layer in the packet data.
};

/**
 * @brief Structure representing a parsed layer within a packet.
 */
struct PacketLayer {
    layers::LayerType type; ///< Type of the layer.
    uint16_t offset; ///< Offset of the layer in the packet data.
};

/// Variant type to hold either a valid PacketLayer or an ErrorLayer used in packet representation.
using LayerVariant = std::variant<PacketLayer, ErrorLayer>;

/**
 * @brief Structure representing the layout of key protocol layers in a packet.
 *        Members hold offsets to layers vector inside the packet object.
 */
struct PacketLayout {
    std::optional<uint8_t> l2; ///< Offset of the L2 layer in the layers vector.
    std::optional<uint8_t> vlan; ///< Offset of the VLAN layer in the layers vector.
    std::optional<uint8_t> l3; ///< Offset of the L3 layer in the layers vector.
    std::optional<uint8_t> l4; ///< Offset of the L4 layer in the layers vector.
    std::optional<uint8_t> l7; ///< Offset of the L7 layer in the layers vector.
};

/**
 * @brief Struct representing a network packet with parsed layers and metadata.
 *
 * Provides methods to find and access protocol layers, as well as store packet data and timestamp.
 */
struct Packet {
    /**
     * @brief Find the first layer of the specified type.
     * @param layerType Type of the layer to find.
     * @param startPos Starting index in the layers vector.
     * @return Variant containing PacketLayer or ErrorLayer if found, otherwise empty variant.
     */
    std::optional<LayerVariant> findLayer(layers::LayerType layerType, uint8_t startPos = 0) const
    {
        return findLayer(layerType, startPos, static_cast<uint8_t>(layers.size()));
    }

    /**
     * @brief Find the first layer of the specified type within a range of indexes.
     * @param layerType Type of the layer to find.
     * @param startPos Starting index in the layers vector.
     * @param endPos Ending index in the layers vector (exclusive).
     * @return Variant containing PacketLayer or ErrorLayer if found, otherwise empty variant.
     */
    std::optional<LayerVariant>
    findLayer(layers::LayerType layerType, uint8_t startPos, uint8_t endPos) const
    {
        if (endPos > layers.size() || startPos > endPos) {
            throw std::out_of_range("Bad start or end position.");
        }
        const auto* begin = layers.begin() + startPos;
        const auto* end = layers.begin() + endPos;
        const auto* it = std::ranges::find_if(begin, end, [&layerType](const auto& layerOrError) {
            const layers::LayerType type
                = std::visit([](auto&& arg) { return arg.type; }, layerOrError);
            return type == layerType;
        });
        if (it != end) {
            return *it;
        }
        return {};
    }

    /**
     * @brief Get a layer by its index in the layers vector.
     * @param index Index of the layer to retrieve.
     * @return Variant containing PacketLayer or ErrorLayer if index is valid, otherwise empty
     * variant.
     */
    std::optional<LayerVariant> getLayer(uint8_t index) const noexcept
    {
        if (index < layers.size()) {
            return layers[index];
        }
        return {};
    }

    /**
     * @brief Get a parsed layer object from PacketLayer info.
     * @tparam T Layer view type.
     * @param layerInfo Information about the layer.
     * @return Optional layer view object if valid and successfully parsed, otherwise empty.
     */
    template <typename T>
    std::optional<T> getLayerView(const PacketLayer& layerInfo) const noexcept
    {
        if (data.size() > layerInfo.offset) [[likely]] {
            auto layerOrError = T::create(data.subspan(layerInfo.offset));
            if (layerOrError) {
                return *layerOrError;
            }
        }
        return {};
    }

    /**
     * @brief Get a parsed layer object by type. First layer in packet of the type is returned.
     * @tparam T Layer view type.
     * @return Optional layer view object if found and successfully parsed, otherwise empty.
     */
    template <typename T>
    std::optional<T> getLayerView() const noexcept
    {
        auto layerOrError = findLayer(T::LAYER_TYPE);
        if (layerOrError) {
            if (const auto layerInfo = std::get_if<PacketLayer>(&layerOrError.value())) {
                return getLayerView<T>(*layerInfo);
            }
        }
        return {};
    }

    std::span<std::byte> data; ///< Raw packet data.
    types::Timestamp timestamp; ///< Packet timestamp.
    std::optional<uint8_t> ingressInterface; ///< Network port ID from which is packet received.
    PacketLayout layout; ///< Layout contains indexes to layers vector.
    types::InplaceVector<LayerVariant, MAX_LAYERS>
        layers; ///< Parsed protocol layers in the packet.
};

} // namespace amon
