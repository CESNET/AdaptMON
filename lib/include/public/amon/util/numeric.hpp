/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Utils for numeric operations.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <concepts>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>

namespace amon::util::numeric {

/**
 * @brief Concept that matches integral types except bool.
 * @tparam T The type to check.
 */
template <typename T>
concept Integral = std::integral<T> && !std::same_as<bool, T>;

/**
 * @brief Safely add two uint64_t values, throwing on overflow.
 * @param a First value.
 * @param b Second value.
 * @return Sum of a and b.
 * @throws std::overflow_error if addition would overflow.
 */
inline uint64_t safeAdd(uint64_t a, uint64_t b)
{
    if (a > std::numeric_limits<uint64_t>::max() - b) {
        throw std::overflow_error("addition would overflow");
    }
    return a + b;
}

/**
 * @brief Safely multiply two uint64_t values, throwing on overflow.
 * @param a First value.
 * @param b Second value.
 * @return Product of a and b.
 * @throws std::overflow_error if multiplication would overflow.
 */
inline uint64_t safeMul(uint64_t a, uint64_t b)
{
    if (b != 0 && a > std::numeric_limits<uint64_t>::max() / b) {
        throw std::overflow_error("multiplication would overflow");
    }
    return a * b;
}

/**
 * @brief Safely casts an integral value from one type to another.
 *
 * Performs compile-time checks to avoid unnecessary runtime checks when possible.
 * Throws std::overflow_error if the value cannot be represented in the target type.
 *
 * @tparam OutputT The target integral type.
 * @tparam InputT The source integral type.
 * @param value The value to cast.
 * @return OutputT The safely casted value.
 * @throws std::overflow_error If the value overflows or underflows the target type.
 */
template <typename OutputT, typename InputT>
    requires(std::integral<OutputT> && !std::same_as<bool, OutputT>)
    && (std::integral<InputT> && !std::same_as<bool, InputT>)
inline OutputT safeCast(InputT value)
{
    // Constexpr-time optimizations to ensure that no runtime checks happen if not necessary
    if constexpr (
        std::unsigned_integral<InputT> && std::unsigned_integral<OutputT>
        && sizeof(InputT) <= sizeof(OutputT)) {
        return static_cast<OutputT>(value);
    }

    if constexpr (
        std::signed_integral<InputT> && std::signed_integral<OutputT>
        && sizeof(InputT) <= sizeof(OutputT)) {
        return static_cast<OutputT>(value);
    }

    if constexpr (
        std::unsigned_integral<InputT>
        && std::cmp_greater_equal(
            std::numeric_limits<OutputT>::max(),
            std::numeric_limits<InputT>::max())) {
        return static_cast<OutputT>(value);
    }

    if (std::cmp_greater(value, std::numeric_limits<OutputT>::max())) {
        throw std::overflow_error("cast would overflow");
    }

    if (std::cmp_less(value, std::numeric_limits<OutputT>::min())) {
        throw std::overflow_error("cast would underflow");
    }

    return static_cast<OutputT>(value);
}

/**
 * @brief Computes the absolute difference between two integral values.
 * @tparam Integer An integral type.
 * @param a The first value.
 * @param b The second value.
 * @return The absolute difference between a and b.
 */
template <Integral Integer>
inline Integer absDiff(Integer a, Integer b)
{
    return a > b ? a - b : b - a;
}

} // namespace amon::util::numeric
