/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief Encapsulating Security Payload layer definitions.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon/layers/ESP.hpp>

namespace amon::layers {

std::expected<ESPView, LayerError> ESPView::create(std::span<const std::byte> data) noexcept
{
    if (data.size() < sizeof(ESPHeader)) {
        return std::unexpected(LayerError::HEADER_SHORT);
    }
    return ESPView(data);
};

} // namespace amon::layers
