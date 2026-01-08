/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief Authentication Header layer definitions.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon/layers/AuthenticationHeader.hpp>

namespace amon::layers {

std::expected<AuthenticationHeaderView, LayerError>
AuthenticationHeaderView::create(std::span<const std::byte> data) noexcept
{
    if (data.size() < sizeof(AuthenticationHeader)) {
        return std::unexpected(LayerError::HEADER_SHORT);
    }
    auto view = AuthenticationHeaderView(data);
    if (data.size() < view.headerLength()) {
        return std::unexpected(LayerError::BAD_LEN_FIELD);
    }
    return view;
};

} // namespace amon::layers
