/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Helpers for human-readable formatting
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <format>
#include <string_view>

namespace amon::util {

/**
 * @brief Represents a human-readable byte value.
 *
 * This class stores a byte value and allows formatting it in a human-readable way
 * using SI units (kB, MB, GB, TB) via std::formatter specialization.
 *
 * @tparam Number The numeric type used to store the byte value.
 */
template <typename Number>
class HumanReadableByteAmount {
public:
    /**
     * @brief Constructs a HumanReadableByteAmount object with the given value.
     *
     * @param value The numeric value representing the number of bytes.
     */
    explicit HumanReadableByteAmount(Number value)
        : m_value(value)
    {
    }

private:
    template <typename, typename>
    friend struct std::formatter; ///< Allows std::formatter access to private members.
    Number m_value; ///< The stored byte value.
};

} // namespace amon::util

/// @cond DO_NOT_DOCUMENT
template <typename Number>
struct std::formatter<amon::util::HumanReadableByteAmount<Number>>
    : std::formatter<std::string_view> {
    auto
    format(amon::util::HumanReadableByteAmount<Number> humanBytes, std::format_context& ctx) const
    {
        if (humanBytes.m_value < 1'000) {
            return std::format_to(ctx.out(), "{} B", humanBytes.m_value);
        }

        auto value = static_cast<double>(humanBytes.m_value);
        std::string_view unit;
        if (value > 1'000'000'000'000) {
            value /= 1'000'000'000'000;
            unit = "T";
        } else if (value > 1'000'000'000) {
            value /= 1'000'000'000;
            unit = "G";
        } else if (value > 1'000'000) {
            value /= 1'000'000;
            unit = "M";
        } else if (value > 1'000) {
            value /= 1'000;
            unit = "k";
        }
        return std::format_to(ctx.out(), "{:.2f} {}B", value, unit);
    }
};
/// @endcond
