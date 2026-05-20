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
 * @brief Styles for formatting human-readable byte amounts.
 *
 * DECIMAL: Use SI units (kB, MB, GB, TB).
 * BINARY: Use binary units (KiB, MiB, GiB, TiB).
 */
enum class HumanReadableByteAmountStyle {
    DECIMAL, ///< SI units (powers of 1000)
    BINARY, ///< Binary units (powers of 1024)
};

/**
 * @brief Represents a human-readable byte value.
 *
 * This class stores a byte value and allows formatting it in a human-readable way
 * using SI units (kB, MB, GB, TB) or binary units (KiB, MiB, GiB, TiB) via std::formatter
 * specialization.
 *
 * @tparam Number The numeric type used to store the byte value.
 * @tparam Style The style for formatting (DECIMAL for SI units, BINARY for binary units).
 */
template <
    typename Number,
    HumanReadableByteAmountStyle Style = HumanReadableByteAmountStyle::DECIMAL>
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

/// @brief Alias for HumanReadableByteAmount using base 2 (binary units, e.g., KiB, MiB, GiB, TiB).
/// @tparam Number The numeric type used to store the byte value.
template <typename Number>
using HumanReadableByteAmount1024
    = HumanReadableByteAmount<Number, HumanReadableByteAmountStyle::BINARY>;

} // namespace amon::util

/// @cond DO_NOT_DOCUMENT
template <typename Number>
struct std::formatter<
    amon::util::HumanReadableByteAmount<Number, amon::util::HumanReadableByteAmountStyle::DECIMAL>>
    : std::formatter<std::string_view> {
    auto format(
        amon::util::HumanReadableByteAmount<
            Number,
            amon::util::HumanReadableByteAmountStyle::DECIMAL> humanBytes,
        std::format_context& ctx) const
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

template <typename Number>
struct std::formatter<
    amon::util::HumanReadableByteAmount<Number, amon::util::HumanReadableByteAmountStyle::BINARY>>
    : std::formatter<std::string_view> {
    auto format(
        amon::util::HumanReadableByteAmount<
            Number,
            amon::util::HumanReadableByteAmountStyle::BINARY> humanBytes,
        std::format_context& ctx) const
    {
        if (humanBytes.m_value < 1024) {
            return std::format_to(ctx.out(), "{} B", humanBytes.m_value);
        }

        auto value = static_cast<double>(humanBytes.m_value);
        std::string_view unit;
        if (value > 1024ULL * 1024ULL * 1024ULL * 1024ULL) {
            value /= 1024ULL * 1024ULL * 1024ULL * 1024ULL;
            unit = "Ti";
        } else if (value > 1024ULL * 1024ULL * 1024ULL) {
            value /= 1024ULL * 1024ULL * 1024ULL;
            unit = "Gi";
        } else if (value > 1024ULL * 1024ULL) {
            value /= 1024ULL * 1024ULL;
            unit = "Mi";
        } else if (value > 1024ULL) {
            value /= 1024ULL;
            unit = "ki";
        }
        return std::format_to(ctx.out(), "{:.2f} {}B", value, unit);
    }
};
/// @endcond
