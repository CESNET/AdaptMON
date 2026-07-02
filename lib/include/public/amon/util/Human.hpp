/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Helpers for human-readable formatting
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <cstddef>
#include <format>
#include <string_view>

namespace amon::util {

/**
 * @brief Styles for formatting human-readable amounts.
 *
 * DECIMAL: Use SI units (k, M, G, T).
 * BINARY: Use binary units (ki, Mi, Gi, Ti).
 */
enum class HumanReadableAmountStyle {
    DECIMAL, ///< SI units (powers of 1000)
    BINARY, ///< Binary units (powers of 1024)
};

/**
 * @brief Alias for HumanReadableAmountStyle.
 */
using HumanReadableByteAmountStyle = HumanReadableAmountStyle;

/**
 * @brief Compile-time string wrapper used for human-readable amount suffixes.
 *
 * This helper allows string literals such as "B" or "b" to be used as non-type
 * template parameters for HumanReadableAmount.
 *
 * @tparam N The size of the string literal including the null terminator.
 */
template <std::size_t N>
struct HumanReadableAmountSuffix {
    /**
     * @brief Constructs the suffix from a string literal.
     *
     * @param suffix The string literal, including its null terminator.
     */
    constexpr HumanReadableAmountSuffix(const char (&suffix)[N])
    {
        for (std::size_t i = 0; i < N; ++i) {
            value[i] = suffix[i];
        }
    }

    /**
     * @brief Returns the suffix as a string view without the null terminator.
     * @return The string literal.
     */
    constexpr std::string_view view() const { return {value, N - 1}; }

    char value[N]; ///< The copied string literal including the null terminator.
};

/**
 * @brief Represents a human-readable value.
 *
 * This class stores a value and allows formatting it in a human-readable way
 * using SI units (k, M, G, T) or binary units (ki, Mi, Gi, Ti) via std::formatter
 * specialization.
 *
 * @tparam Number The numeric type used to store the value.
 * @tparam Style The style for formatting (DECIMAL for SI units, BINARY for binary units).
 * @tparam Suffix The suffix appended to the unit prefix.
 */
template <
    typename Number,
    HumanReadableAmountSuffix Suffix = "",
    HumanReadableAmountStyle Style = HumanReadableAmountStyle::DECIMAL>
class HumanReadableAmount {
public:
    /**
     * @brief Constructs a HumanReadableAmount object with the given value.
     *
     * @param value The numeric value.
     */
    explicit HumanReadableAmount(Number value)
        : m_value(value)
    {
    }

private:
    template <typename, typename>
    friend struct std::formatter; ///< Allows std::formatter access to private members.
    Number m_value; ///< The stored value.
};

/**
 * @brief Represents a human-readable byte value.
 */
template <typename Number, HumanReadableAmountStyle Style = HumanReadableAmountStyle::DECIMAL>
class HumanReadableByteAmount : public HumanReadableAmount<Number, "B", Style> {
public:
    /**
     * @brief Constructs a HumanReadableByteAmount object with the given value.
     *
     * @param value The numeric value.
     */
    explicit HumanReadableByteAmount(Number value)
        : HumanReadableAmount<Number, "B", Style>(value)
    {
    }
};

/**
 * @brief Represents a human-readable bit value.
 */
template <typename Number, HumanReadableAmountStyle Style = HumanReadableAmountStyle::DECIMAL>
class HumanReadableBitAmount : public HumanReadableAmount<Number, "b", Style> {
public:
    /**
     * @brief Constructs a HumanReadableBitAmount object with the given value.
     *
     * @param value The numeric value.
     */
    explicit HumanReadableBitAmount(Number value)
        : HumanReadableAmount<Number, "b", Style>(value)
    {
    }
};

/// @brief Alias for HumanReadableByteAmount using base 2 (binary units, e.g., KiB, MiB, GiB, TiB).
/// @tparam Number The numeric type used to store the byte value.
template <typename Number>
using HumanReadableByteAmount1024
    = HumanReadableByteAmount<Number, HumanReadableAmountStyle::BINARY>;

/// @brief Alias for HumanReadableBitAmount using base 2 (binary units, e.g., Kib, Mib, Gib, Tib).
/// @tparam Number The numeric type used to store the bit value.
template <typename Number>
using HumanReadableBitAmount1024 = HumanReadableBitAmount<Number, HumanReadableAmountStyle::BINARY>;

} // namespace amon::util

/// @cond DO_NOT_DOCUMENT
template <typename Number, amon::util::HumanReadableAmountSuffix Suffix>
struct std::formatter<
    amon::util::HumanReadableAmount<Number, Suffix, amon::util::HumanReadableAmountStyle::DECIMAL>>
    : std::formatter<std::string_view> {
    auto format(
        amon::util::
            HumanReadableAmount<Number, Suffix, amon::util::HumanReadableAmountStyle::DECIMAL>
                humanAmount,
        std::format_context& ctx) const
    {
        constexpr auto suffix = Suffix.view();

        if (humanAmount.m_value < 1'000) {
            if constexpr (suffix.empty()) {
                return std::format_to(ctx.out(), "{}", humanAmount.m_value);
            } else {
                return std::format_to(ctx.out(), "{} {}", humanAmount.m_value, suffix);
            }
        }

        auto value = static_cast<double>(humanAmount.m_value);
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
        return std::format_to(ctx.out(), "{:.2f} {}{}", value, unit, suffix);
    }
};

template <typename Number, amon::util::HumanReadableAmountSuffix Suffix>
struct std::formatter<
    amon::util::HumanReadableAmount<Number, Suffix, amon::util::HumanReadableAmountStyle::BINARY>>
    : std::formatter<std::string_view> {
    auto format(
        amon::util::
            HumanReadableAmount<Number, Suffix, amon::util::HumanReadableAmountStyle::BINARY>
                humanAmount,
        std::format_context& ctx) const
    {
        constexpr auto suffix = Suffix.view();

        if (humanAmount.m_value < 1024) {
            if constexpr (suffix.empty()) {
                return std::format_to(ctx.out(), "{}", humanAmount.m_value);
            } else {
                return std::format_to(ctx.out(), "{} {}", humanAmount.m_value, suffix);
            }
        }

        auto value = static_cast<double>(humanAmount.m_value);
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
        return std::format_to(ctx.out(), "{:.2f} {}{}", value, unit, suffix);
    }
};

template <typename Number, amon::util::HumanReadableAmountStyle Style>
struct std::formatter<amon::util::HumanReadableByteAmount<Number, Style>>
    : std::formatter<amon::util::HumanReadableAmount<Number, "B", Style>> {
    auto format(
        amon::util::HumanReadableByteAmount<Number, Style> humanBytes,
        std::format_context& ctx) const
    {
        return std::formatter<amon::util::HumanReadableAmount<Number, "B", Style>>::format(
            humanBytes,
            ctx);
    }
};

template <typename Number, amon::util::HumanReadableAmountStyle Style>
struct std::formatter<amon::util::HumanReadableBitAmount<Number, Style>>
    : std::formatter<amon::util::HumanReadableAmount<Number, "b", Style>> {
    auto format(
        amon::util::HumanReadableBitAmount<Number, Style> humanBits,
        std::format_context& ctx) const
    {
        return std::formatter<amon::util::HumanReadableAmount<Number, "b", Style>>::format(
            humanBits,
            ctx);
    }
};
/// @endcond
