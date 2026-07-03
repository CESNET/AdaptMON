/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Generic wrapper around std::formatter for easier formatting.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <concepts>
#include <format>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>

namespace amon::util {

/**
 * @brief Formatter passed to the `format(value, formatter)` function.
 *
 * To make a type formattable, define either a free format function in the same namespace as the
 * type:
 *
 * @code{.cpp}
 * struct Endpoint {
 *     IPAddress address;
 *     uint16_t port;
 * };
 *
 * void format(const Endpoint& endpoint, amon::util::Formatter& fmt)
 * {
 *     fmt.format("{}:{}", endpoint.address, endpoint.port);
 * }
 * @endcode
 *
 * Or define a member format function:
 *
 * @code{.cpp}
 * class Counter {
 * public:
 *     void format(amon::util::Formatter& fmt) const
 *     {
 *         fmt.format("{} packets", m_value);
 *     }
 *
 * private:
 *     uint64_t m_value = 0;
 * };
 * @endcode
 *
 * The resulting type can then be used with std::format, std::println or in loggers:
 *
 * @code{.cpp}
 * auto text = std::format("{:>20}", endpoint);
 * @endcode
 *
 * @code{.cpp}
 * std::println("{:>20}", endpoint);
 * @endcode
 *
 * @code{.cpp}
 * auto logger = amon::LoggerProvider::get("example");
 * logger.info("{:>20}", endpoint);
 * @endcode
 */
class Formatter {
public:
    /**
     * @brief Format a string using a format string and arguments.
     *
     * This function appends the formatted result to the formatter's internal
     * output buffer.
     *
     * @tparam Args The types of the format arguments.
     * @param fmt A compile-time format string.
     * @param args The arguments to format.
     */
    template <typename... Args>
    void format(std::format_string<Args...> fmt, Args&&... args)
    {
        std::format_to(std::back_inserter(m_output), fmt, std::forward<Args>(args)...);
    }

private:
    template <typename, typename>
    friend struct std::formatter;

    std::string_view output() const { return m_output; }

    std::string m_output;
};

/// @cond DO_NOT_DOCUMENT
namespace detail {

template <typename T>
concept HasFreeFormatFunction = requires(const T& value, Formatter& formatter) {
    { format(value, formatter) } -> std::same_as<void>;
};

template <typename T>
concept HasMemberFormatFunction = requires(const T& value, Formatter& formatter) {
    { value.format(formatter) } -> std::same_as<void>;
};

template <typename T>
concept CFormattable = detail::HasFreeFormatFunction<T> || detail::HasMemberFormatFunction<T>;

template <typename T>
void formatValue(const T& value, Formatter& formatter)
{
    if constexpr (HasFreeFormatFunction<T>) {
        format(value, formatter);
    } else {
        value.format(formatter);
    }
}

} // namespace detail
/// @endcond

} // namespace amon::util

/// @cond DO_NOT_DOCUMENT
template <amon::util::detail::CFormattable T>
struct std::formatter<T, char> {
    constexpr auto parse(std::format_parse_context& ctx) { return m_formatter.parse(ctx); }

    auto format(const T& value, std::format_context& ctx) const
    {
        auto formatter = amon::util::Formatter {};
        amon::util::detail::formatValue(value, formatter);
        return m_formatter.format(formatter.output(), ctx);
    }

private:
    std::formatter<std::string_view> m_formatter;
};
/// @endcond
