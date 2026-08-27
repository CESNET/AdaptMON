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

/// Maximum number of encapsulation layers to store in PacketLayout.
constexpr static const unsigned ENCAPSULATION_SIZE = 2;

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
 *
 * Members hold offsets to layers vector inside the packet object.
 *
 * @par Example accessing layer from packet structure by layout member:
 * @code
 * if (layout.l3) {
 *   LayerVariant layerOrError = packet.layers[layout.l3];
 * }
 * @endcode
 */
struct PacketLayout {
    std::optional<uint8_t> l2; ///< Offset of the L2 layer in the layers vector.
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
    /// Raw packet data.
    std::span<std::byte> data;

    /// Packet timestamp.
    types::Timestamp timestamp;

    /// Network port ID from which is packet received.
    std::optional<uint8_t> ingressInterface;

    /**
     * @brief Layouts of protocol layers in the packet.
     *
     * Each layout corresponds to one encapsulation level, up to ENCAPSULATION_SIZE entries.
     *
     * @par Layout indexing:
     * - **layout[0]**: Outermost (first) encapsulation level. For a standard packet without
     *   tunneling, this contains offsets to the L2/L3/L4/L7 layers. For tunneled packets, it
     *   contains offsets to the layers before the tunneled protocol begins (e.g., Ethernet (L2)
     *   and outer IP layer (L3) for an IP-in-IP tunnel).
     * - **layout[1]**: Second encapsulation level, present when tunneling occurs. For example,
     *   in an IP-in-IP tunnel, this contains offsets to the inner IP layer (L3) and its
     *   L4/L7 protocols.
     *
     * @par Encapsulation overflow behavior:
     * When a packet contains more than ENCAPSULATION_SIZE encapsulation levels (e.g., nested
     * tunnels exceeding 2 levels), the layout vector becomes full and no additional layouts are
     * stored. In this case, the last layout is NOT the innermost encapsulation level.
     *
     * @note All protocol layers remain accessible via the `layers` vector regardless of layout
     *       overflow. The layout only provides quick access to the first ENCAPSULATION_SIZE
     *       encapsulation levels.
     *
     * @par Example - Standard packet (no tunneling):
     * @code
     * // Packet: Ethernet -> IPv4 -> TCP -> HTTP
     * // layout.size() == 1
     * // layout[0].l2 = 0 (Ethernet at index 0)
     * // layout[0].l3 = 1 (IPv4 at index 1)
     * // layout[0].l4 = 2 (TCP at index 2)
     * // layout[0].l7 = 3 (HTTP payload at index 3)
     * @endcode
     *
     * @par Example - IP-in-IP tunnel:
     * @code
     * // Packet: Ethernet -> IPv4 (outer) -> IPv4 (inner) -> TCP -> HTTP
     * // layout.size() == 2
     * // layout[0].l2 = 0 (Ethernet)
     * // layout[0].l3 = 1 (outer IPv4)
     * // layout[1].l3 = 2 (inner IPv4)
     * // layout[1].l4 = 3 (TCP)
     * // layout[1].l7 = 4 (HTTP payload)
     * @endcode
     *
     * @par Example - IP-in-IP + GRE tunnel (overflow):
     * @code
     * // Packet: Ethernet -> IPv4 (outer) -> IPv4 (middle) -> GRE -> IPv4 (inner) -> TCP -> HTTP
     * // layout.size() == 2 (overflow - inner IPv4 not stored in layout)
     * // layout[0].l2 = 0 (Ethernet)
     * // layout[0].l3 = 1 (outer IPv4)
     * // layout[1].l3 = 2 (middle IPv4)
     * // layout[1].l4 = 3 (GRE)
     * @endcode
     */
    types::InplaceVector<PacketLayout, ENCAPSULATION_SIZE> layout;

    /// Parsed protocol layers in the packet.
    types::InplaceVector<LayerVariant, MAX_LAYERS> layers;

    /**
     * @brief Find the first layer of the specified type.
     * @param layerType Type of the layer to find.
     * @param startPos Starting index in the layers vector.
     * @return Variant containing PacketLayer or ErrorLayer if found, otherwise empty variant.
     */
    std::optional<LayerVariant> findLayer(layers::LayerType layerType, uint8_t startPos = 0) const;

    /**
     * @brief Find the first layer of the specified type within a range of indexes.
     * @param layerType Type of the layer to find.
     * @param startPos Starting index in the layers vector.
     * @param endPos Ending index in the layers vector (exclusive).
     * @return Variant containing PacketLayer or ErrorLayer if found, otherwise empty variant.
     */
    std::optional<LayerVariant>
    findLayer(layers::LayerType layerType, uint8_t startPos, uint8_t endPos) const;

    /**
     * @brief Get a layer by its index in the layers vector.
     * @param index Index of the layer to retrieve.
     * @return Variant containing PacketLayer or ErrorLayer if index is valid, otherwise empty
     * variant.
     */
    std::optional<LayerVariant> getLayer(uint8_t index) const noexcept;

    /**
     * @brief Get a parsed layer object from PacketLayer info.
     * @tparam T Layer view type.
     * @param layerInfo Information about the layer.
     * @return Optional layer view object if valid and successfully parsed, otherwise empty.
     */
    template <typename T>
    std::optional<T> getLayerView(const PacketLayer& layerInfo) const noexcept;

    /**
     * @brief Get a parsed layer object by type. First layer in packet of the type is returned.
     * @tparam T Layer view type.
     * @return Optional layer view object if found and successfully parsed, otherwise empty.
     */
    template <typename T>
    std::optional<T> getLayerView() const noexcept;
};

inline std::optional<LayerVariant>
Packet::findLayer(layers::LayerType layerType, uint8_t startPos) const
{
    return findLayer(layerType, startPos, static_cast<uint8_t>(layers.size()));
}

inline std::optional<LayerVariant>
Packet::findLayer(layers::LayerType layerType, uint8_t startPos, uint8_t endPos) const
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

inline std::optional<LayerVariant> Packet::getLayer(uint8_t index) const noexcept
{
    if (index < layers.size()) {
        return layers[index];
    }
    return {};
}

template <typename T>
std::optional<T> Packet::getLayerView(const PacketLayer& layerInfo) const noexcept
{
    if (data.size() > layerInfo.offset) [[likely]] {
        auto layerOrError = T::create(data.subspan(layerInfo.offset));
        if (layerOrError) {
            return *layerOrError;
        }
    }
    return {};
}

template <typename T>
std::optional<T> Packet::getLayerView() const noexcept
{
    for (const auto& layerOrError : layers) {
        const PacketLayer* layerInfo = std::get_if<PacketLayer>(&layerOrError);
        if (!layerInfo || layerInfo->type != T::LAYER_TYPE) {
            continue;
        }
        return getLayerView<T>(*layerInfo);
    }
    return {};
}

} // namespace amon
