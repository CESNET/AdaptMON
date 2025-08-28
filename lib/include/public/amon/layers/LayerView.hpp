/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief Accessor for single packet layer. Read only.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <cstddef>
#include <span>

namespace amon::layers {

/**
 * @brief Provides read-only access to a specific layer in a packet.
 * @tparam HeaderStruct The structure representing the layer header.
 * Packet bytes (big-endian) is casted to this structure.
 */
template <typename HeaderStruct>
class LayerView {
public:
    static_assert(
        std::is_standard_layout<HeaderStruct>(),
        "LayerView header template type must be in standard layout");
    static_assert(
        std::is_trivially_default_constructible<HeaderStruct>(),
        "LayerView header template type must be default constructible");

    /**
     * @brief Constructs a LayerView from raw packet data.
     * @param data Raw packet data.
     */
    LayerView(std::span<const std::byte> data) noexcept
        : data(data) {};

    /**
     * @brief Returns a reference to the layer header structure.
     * @return Reference to the layer header structure.
     */
    const HeaderStruct& header() const noexcept
    {
        return *reinterpret_cast<const HeaderStruct*>(data.data());
    }

    std::span<const std::byte> data; ///< Data of the layer view.
};

} // namespace amon::layers
