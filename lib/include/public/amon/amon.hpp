/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Common amon utilities
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/Stacktrace.hpp>

#include <iostream>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace amon {

/// @cond DO_NOT_DOCUMENT
namespace detail {
template <typename... Fs>
struct Overloaded : Fs... {
    using Fs::operator()...; // inherit all operator()s
};

template <typename... Fs>
Overloaded(Fs...) -> Overloaded<Fs...>; // deduction guide

template <typename>
struct IsVariantHelper : std::false_type {};

template <typename... Ts>
struct IsVariantHelper<std::variant<Ts...>> : std::true_type {};
template <typename T>
concept Variant = detail::IsVariantHelper<std::remove_cvref_t<T>>::value;

} // namespace detail
/// @endcond

/**
 * @brief Applies the given callables to a variant using pattern matching.
 *
 * This function enables a concise and type-safe way to handle `std::variant` values.
 * Each callable should handle one or more types contained in the variant.
 * The function uses `std::visit` with an overloaded set of callables.
 *
 * @tparam Variant A type satisfying the `detail::Variant` concept (typically a `std::variant`).
 * @tparam Callables Callable objects (e.g., lambdas, function objects) for each variant
 * alternative.
 * @param variant The variant to match against.
 * @param callables The callables to apply, each handling a specific type or using a generic lambda.
 * @return The result of invoking the matching callable.
 *
 * @note If a type in the variant is not handled by any callable, a compile-time error will occur.
 *
 * @code
 * // Example 1: Specific-type lambdas
 * std::variant<int, std::string> v = 42;
 * auto result = match(v,
 *     [](int i) { return i * 2; },
 *     [](const std::string& s) { return s.size(); }
 * );
 * // result == 84
 *
 * // Example 2: Auto lambda for all types
 * std::variant<int, double, std::string> v = "hello";
 * auto result = match(v,
 *     [](auto&& value) { return std::to_string(value); }
 * );
 * // result == "hello"
 *
 * // Example 3: Mix of specific and generic lambdas resulting in a "default case" behavior
 * std::variant<int, std::string, float> v = 3.14f;
 * auto result = match(v,
 *     [](int i) { return std::string("int: ") + std::to_string(i); },
 *     [](const std::string& s) { return std::string("string: ") + s; },
 *     [](auto&& other) { return std::string("other type"); }
 * );
 * // result == "other type"
 *
 * // Example 4: Missing handler for a variant alternative (does not compile)
 * std::variant<int, std::string, double> v = 3.14;
 * auto result = match(v,
 *     [](int i) { return i; },
 *     [](const std::string& s) { return s.size(); }
 * );
 * @endcode
 */
template <detail::Variant Variant, typename... Callables>
auto match(Variant&& variant, Callables&&... callables)
{
    return std::visit(detail::Overloaded {callables...}, std::forward<decltype(variant)>(variant));
}

/// @cond DO_NOT_DOCUMENT
namespace detail {

template <typename... Args>
[[noreturn]] void assertFailImpl(
    std::string_view cond,
    std::format_string<Args...> fmt,
    Args&&... args) noexcept
{
    std::cerr
        << "--- ASSERTION FAILED -----------------------------------------------------------\n";
    std::cerr << "Condition: " << cond << "\n";
    if (!fmt.get().empty()) {
        try {
            std::cerr << "Message:   " << std::format(fmt, std::forward<Args>(args)...) << "\n";
        } catch (...) { // NOLINT(bugprone-empty-catch)
            std::cerr << "Message:   <failed to generate message>\n";
        }
    }
    std::cerr << "\n";
    std::cerr << "Stacktrace:\n";
    try {
        std::cerr << StacktraceProvider::createStacktrace(2).str() << "\n";
    } catch (...) { // NOLINT(bugprone-empty-catch)
        std::cerr << "<failed to generate stacktrace>\n";
    }
    std::abort();
}

template <typename... Args>
[[noreturn]] void
assertFail(std::string_view cond, std::format_string<Args...> fmt, Args&&... args) noexcept
{
    assertFailImpl(cond, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
[[noreturn]] void assertFail(std::string_view cond) noexcept
{
    assertFailImpl(cond, "");
}

} // namespace detail
/// @endcond

/**
 * @def AMON_ASSERT
 * @brief Asserts that a condition is true, otherwise prints diagnostic info and aborts.
 *
 * Unlike classical `assert`, AMON_ASSERT is intended to work in release mode as well.
 * The motivation is to replace the `if (!condition) throw std::logic_error(...);` pattern that
 * could not be replaced by a regular assert due to it being optimized away in release mode.
 *
 * AMON_ASSERT also prints a stack trace when triggered to aid in debugging.
 *
 * @param CONDITION The condition to assert.
 * @param ... Format string and arguments for the format string if you want to print a message.
 */
#define AMON_ASSERT(CONDITION, ...)                                                                \
    if (!(CONDITION)) [[unlikely]] {                                                               \
        amon::detail::assertFail(#CONDITION, ##__VA_ARGS__);                                       \
    }

} // namespace amon
