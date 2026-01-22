/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Configuration parser - value parsers
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 */

#pragma once

#include <amon/Error.hpp>
#include <amon/conf/Scalar.hpp>
#include <amon/util/numeric.hpp>

#include <string>

namespace amon::conf {

/**
 * @brief Generic parser template for converting strings to values.
 * Specializations should implement static T parse(const std::string&).
 * @tparam T The type to parse into.
 */
template <typename T>
struct Parser {};

/**
 * @brief Parser specialization for std::string.
 */
template <>
struct Parser<std::string> {
    /**
     * @brief Parses the input string and returns it as is.
     *
     * @param scalar The input string to parse.
     * @return The same string as input.
     */
    static std::string parse(const Scalar& scalar) { return scalar.asString(); }
};

/**
 * @brief Parser specialization for signed integral types.
 *
 * Parses a scalar as a signed integer type T.
 * Throws amon::Error if parsing fails.
 *
 * @tparam T Signed integral type (e.g., int32_t, int64_t).
 */
template <typename T>
    requires(std::signed_integral<T> && sizeof(T) <= sizeof(int64_t))
struct Parser<T> {
    /**
     * @brief Parses a scalar as a signed integer of type T.
     * @param scalar The scalar to parse.
     * @return Parsed value as type T.
     * @throws amon::Error if parsing fails.
     */
    static T parse(const Scalar& scalar)
    {
        int64_t value;

        try {
            value = scalar.asInt();
        } catch (const std::exception& err) {
            throw amon::Error(
                "could not convert value '{}' to integer: {}",
                scalar.asString(),
                err.what());
        }

        try {
            return util::numeric::safeCast<T>(value);
        } catch (const std::overflow_error& err) {
            throw amon::Error(
                "could not convert value '{}' to integer ({}b): {}",
                scalar.asString(),
                sizeof(T) * 8,
                err.what());
        }
    }
};

/**
 * @brief Parser specialization for unsigned integral types (excluding bool).
 *
 * Parses a scalar as an unsigned integer type T.
 * Throws amon::Error if parsing fails.
 *
 * @tparam T Unsigned integral type (e.g., uint32_t, uint64_t), but not bool.
 */
template <typename T>
    requires(std::unsigned_integral<T> && sizeof(T) <= sizeof(uint64_t) && !std::same_as<bool, T>)
struct Parser<T> {
    /**
     * @brief Parses a scalar as an unsigned integer of type T.
     * @param scalar The scalar to parse.
     * @return Parsed value as type T.
     * @throws amon::Error if parsing fails.
     */
    static T parse(const Scalar& scalar)
    {
        uint64_t value;

        try {
            value = scalar.asUInt();
        } catch (const std::exception& err) {
            throw amon::Error(
                "could not convert value '{}' to unsigned integer: {}",
                scalar.asString(),
                err.what());
        }

        try {
            return util::numeric::safeCast<T>(value);
        } catch (const std::overflow_error& err) {
            throw amon::Error(
                "could not convert value '{}' to unsigned integer ({}b): {}",
                scalar.asString(),
                sizeof(T) * 8,
                err.what());
        }
    }
};

/**
 * @brief Parser specialization for floating-point types.
 *
 * Parses a scalar as a floating-point type T.
 * Throws amon::Error if parsing fails.
 *
 * @tparam T Floating-point type (e.g., float, double).
 */
template <typename T>
    requires(std::floating_point<T> && sizeof(T) <= sizeof(double))
struct Parser<T> {
    /**
     * @brief Parses a scalar as a floating-point value of type T.
     * @param scalar The scalar to parse.
     * @return Parsed value as type T.
     * @throws amon::Error if parsing fails.
     */
    static T parse(const Scalar& scalar)
    {
        double value;

        try {
            value = scalar.asDouble();
        } catch (const std::exception& err) {
            throw amon::Error(
                "could not convert value '{}' to floating point: {}",
                scalar.asString(),
                err.what());
        }

        return static_cast<T>(value);
    }
};

/**
 * @brief Parser specialization for bool type.
 *
 * Parses a scalar as a boolean value
 * Throws amon::Error if parsing fails.
 */
template <>
struct Parser<bool> {
    /**
     * @brief Parses a scalar as a boolean value.
     * @param scalar The scalar to parse.
     * @return Parsed value as bool.
     * @throws amon::Error if parsing fails.
     */
    static bool parse(const Scalar& scalar)
    {
        try {
            return scalar.asBool();
        } catch (const std::exception& err) {
            throw amon::Error(
                "could not convert value '{}' to boolean: {}",
                scalar.asString(),
                err.what());
        }
    }
};

} // namespace amon::conf
