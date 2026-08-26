/**
 * @file
 * @author Lukas Hutak <hutak@cesnet.cz>
 * @brief Utils for string operations.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <charconv>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace amon::util::string {

/**
 * @brief Removes leading ASCII whitespace characters from a string view.
 *
 * @param str The string view to trim.
 * @return A string view with leading whitespace removed.
 */
std::string_view trimWhitespaceStart(std::string_view str);

/**
 * @brief Removes trailing ASCII whitespace characters from a string view.
 *
 * @param str The string view to trim.
 * @return A string view with trailing whitespace removed.
 */
std::string_view trimWhitespaceEnd(std::string_view str);

/**
 * @brief Removes both leading and trailing ASCII whitespace characters from a string view.
 *
 * @param str The string view to trim.
 * @return A string view with both leading and trailing whitespace removed.
 */
std::string_view trimWhitespace(std::string_view str);

/**
 * @brief Removes an optional prefix from a string view.
 *
 * @param str The string view to trim.
 * @param prefix The prefix to remove.
 * @return A string view with the prefix removed. If the string view doesn't start
 * with the given @p prefix, the original string view is returned.
 */
std::string_view trimPrefix(std::string_view str, std::string_view prefix);

/**
 * @brief Removes an optional suffix from a string view.
 *
 * @param str The string view to trim.
 * @param suffix The suffix to remove.
 * @return A string view with the suffix removed. If the string view doesn't end
 * with the given @p suffix, the original string view is returned.
 */
std::string_view trimSuffix(std::string_view str, std::string_view suffix);

/**
 * @brief Removes a specified prefix from a string view.
 *
 * @param str The string view to trim.
 * @param prefix The prefix to remove.
 * @return A string view with the prefix removed. If the string view doesn't start
 * with the given @p prefix, std::nullopt is returned.
 */
std::optional<std::string_view> stripPrefix(std::string_view str, std::string_view prefix);

/**
 * @brief Removes a specified suffix from a string view.
 *
 * @param str The string view to trim.
 * @param suffix The suffix to remove.
 * @return A string view with the suffix removed. If the string view doesn't end
 * with the given @p suffix, std::nullopt is returned.
 */
std::optional<std::string_view> stripSuffix(std::string_view str, std::string_view suffix);

/**
 * @brief Splits a string view on the first occurrence of the specified delimiter
 * and returns prefix before delimiter and suffix after delimiter.
 *
 * @code
 * splitOnce("file.cpp:80:40", ":") -> ("file.cpp", "80:40")
 * @endcode
 *
 * @param str The string view to split.
 * @param delimiter The delimiter
 * @return A pair of string views representing the prefix and suffix.
 * If the delimiter is not found, std::nullopt is returned.
 */
std::optional<std::pair<std::string_view, std::string_view>>
splitOnce(std::string_view str, std::string_view delimiter);

/**
 * @copydoc amon::util::string::splitOnce
 */
inline std::optional<std::pair<std::string_view, std::string_view>>
splitOnce(std::string_view str, char delimiter)
{
    return splitOnce(str, {std::addressof(delimiter), 1});
}

/**
 * @brief Splits a string view on the last occurrence of the specified delimiter
 * and returns prefix before delimiter and suffix after delimiter.
 *
 * @code
 * rsplitOnce("file.cpp:80:40", ":") -> ("file.cpp:80", "40")
 * @endcode
 *
 * @param str The string view to split.
 * @param delimiter The delimiter
 * @return A pair of string views representing the prefix and suffix.
 * If the delimiter is not found, std::nullopt is returned.
 */
std::optional<std::pair<std::string_view, std::string_view>>
rsplitOnce(std::string_view str, std::string_view delimiter);

/**
 * @copydoc amon::util::string::rsplitOnce
 */
inline std::optional<std::pair<std::string_view, std::string_view>>
rsplitOnce(std::string_view str, char delimiter)
{
    return rsplitOnce(str, {std::addressof(delimiter), 1});
}

/**
 * @brief Splits a string view into substrings based on a specified delimiter.
 *
 * If @p maxSplits is provided, the function will produce at most `maxSplits + 1` substrings.
 * This means that after `maxSplits` splits, any remaining part of the string is returned as
 * a single substring without further splitting.
 *
 * The substrings can be empty if there are consecutive delimiters or if the input starts/ends
 * with the delimiter.
 *
 * Examples:
 * @code
 * split("", ',') -> [""]
 * split(" a, b ,c ", ',') -> [" a", " b ", "c "]
 * split("a = b,c,, d", ',') -> ["a = b", "c", "", " d"]
 * split("a = b,c,, d", ',', 1) -> ["a = b", "c,, d"]
 * @endcode
 *
 * @note If you need to avoid memory allocation or you need more precise control, consider
 * using foreach loop with `std::views::split(str, delimiter)` adaptor.
 *
 * @param str The string view to split into substrings.
 * @param delimiter The character used as a delimiter.
 * @param maxSplits An optional maximum number of splits to produce.
 * @return A vector of `std::string_view` objects, each representing a substring from the
 * original input.
 */
std::vector<std::string_view> split(
    std::string_view str,
    std::string_view delimiter,
    std::optional<size_t> maxSplits = std::nullopt);

/**
 * @copydoc amon::util::string::split
 */
inline std::vector<std::string_view>
split(std::string_view str, char delimiter, std::optional<size_t> maxSplits = std::nullopt)
{
    return split(str, {std::addressof(delimiter), 1}, maxSplits);
}

/**
 * @brief Splits a string view into substrings separated by ASCII whitespace character(s).
 *
 * This function splits input string view it into multiple non-empty substrings, each delimited
 * by one or more consecutive whitespace characters.
 *
 * If @p maxSplits is provided, the function will produce at most `maxSplits + 1` substrings.
 * This means that after `maxSplits` splits, any remaining part of the string is returned as
 * a single substring without further splitting.
 *
 * Examples:
 * @code
 * splitWhitespace("   ") -> []
 * splitWhitespace("  Hello World!  ") -> ["Hello", "World!"]
 * splitWhitespace("my name is   Jon Doe  ") -> ["my", "name", "is", "Jon", "Doe"]
 * splitWhitespace("my name is   Jon Doe  ", 1) -> ["my", "name is   Jon Doe  "]
 * @endcode
 *
 * @note If the maximum number of splits is zero, the original string view is returned with
 * removed leading whitespaces.
 *
 * @param str The string view to split into substrings.
 * @param maxSplits An optional maximum number of splits to produce.
 * @return A vector of `std::string_view` objects, each representing a trimmed non-empty
 * substring from the original input.
 */
std::vector<std::string_view>
splitWhitespace(std::string_view str, std::optional<size_t> maxSplits = std::nullopt);

/**
 * @brief Checks if two ASCII strings are equal, ignoring case.
 *
 * This function compares two strings in a case-insensitive manner.
 *
 * @param lhs The first string to compare.
 * @param rhs The second string to compare.
 * @return True if the strings are equal (ignoring case), false otherwise.
 */
bool isEqualCaseInsensitive(std::string_view lhs, std::string_view rhs);

/**
 * @brief Parse a string view to the given data type.
 *
 * @param str The string view to parse.
 * @return The parsed value of the given type.
 * @throw Exceptions such as std::invalid_argument or std::range_error if the
 * value cannot be converted.
 */
template <typename T>
T parse(std::string_view str);

/**
 * @brief Parses a string view into a string value.
 *
 * @param str The string view to parse.
 * @return The parsed string value.
 */
template <>
inline std::string parse<std::string>(std::string_view str)
{
    return std::string {str};
}

/**
 * @overload T parse(std::string_view str)
 * @tparam T Must be boolean type.
 * @brief Parses a string view into a boolean value.
 *
 * The following case insensitive expressions are supported:
 * yes/no, true/false, on/off, and 1/0.
 *
 * @note The string to convert must not contain any leading or tailing whitespaces,
 * otherwise the conversion fails. Use trimWhitespace() to remove them, if necessary.
 * @param str The string to parse.
 * @return The parsed boolean value.
 * @throw std::invalid_argument If the string cannot be parsed as a boolean.
 */
template <typename T>
    requires std::same_as<T, bool>
T parse(std::string_view str)
{
    for (const auto& item : {"yes", "true", "on", "1"}) {
        if (isEqualCaseInsensitive(str, item)) {
            return true;
        }
    }

    for (const auto& item : {"no", "false", "off", "0"}) {
        if (isEqualCaseInsensitive(str, item)) {
            return false;
        }
    }

    throw std::invalid_argument("'" + std::string(str) + "' is not boolean value");
}

/**
 * @overload T parse(std::string_view str)
 * @tparam T Must be a signed or unsigned integer.
 * @brief Parses a string view into an integer.
 *
 * For integer types, conversions use base 10 (decimal) by default.
 * The base is overridden if the string starts with:
 *   - "0x": uses base 16 (hexadecimal)
 *   - "0b": uses base 2 (binary)
 *
 * @note The string to convert must not contain any leading or tailing whitespaces,
 * otherwise the conversion fails. Use trimWhitespace() to remove them, if necessary.
 * @tparam T The target numeric type.
 * @param str The string to parse.
 * @return The parsed number.
 * @throw std::invalid_argument If the string cannot be parsed as a number.
 * @throw std::range_error If the parsed value cannot fit into the target type.
 */

template <typename T>
    requires(std::integral<T> && !std::is_same_v<T, bool>)
T parse(std::string_view str)
{
    const std::string_view original {str};
    int base = 10;
    T output;

    if (str.starts_with("0x")) {
        str = str.substr(2);
        base = 16;
    } else if (str.starts_with("0b")) {
        str = str.substr(2);
        base = 2;
    }

    auto isHexaChar = [](char ch) -> bool { return std::isxdigit(static_cast<unsigned char>(ch)); };

    if (base != 10 && !str.empty() && !isHexaChar(str.front())) {
        // Unexpected character for the given base (e.g. input like "0b-1111")
        const std::string value {original};
        throw std::invalid_argument("'" + value + "' is not a valid number");
    }

    const char* begin = str.data();
    const char* end = str.data() + str.size();

    auto [ptr, ec] {std::from_chars(begin, end, output, base)};
    if (ec == std::errc() && ptr == end) {
        return output;
    }

    const std::string value {str};

    if (ec == std::errc::result_out_of_range) {
        throw std::range_error("'" + value + "' is out of supported range");
    }

    if (ec == std::errc::invalid_argument) {
        throw std::invalid_argument("'" + value + "' is not a valid number");
    }

    throw std::invalid_argument(
        "'" + value + "' is not a valid number due to unexpected characters");
}

/**
 * @overload T parse(std::string_view str)
 * @tparam T Must be a floating point number.
 * @brief Parses a string view into a floating point number.
 *
 * @note The string to convert must not contain any leading or tailing whitespaces,
 * otherwise the conversion fails. Use trimWhitespace() to remove them, if necessary.
 * @tparam T The target numeric type.
 * @param str The string to parse.
 * @return The parsed number.
 * @throw std::invalid_argument If the string cannot be parsed as a number.
 * @throw std::range_error If the parsed value cannot fit into the target type.
 */
template <typename T>
    requires std::floating_point<T>
T parse(std::string_view str)
{
    const char* begin = str.data();
    const char* end = str.data() + str.size();
    T output;

    auto [ptr, ec] {std::from_chars(begin, end, output)};
    if (ec == std::errc() && ptr == end) {
        return output;
    }

    const std::string value {str};

    if (ec == std::errc::result_out_of_range) {
        throw std::range_error("'" + value + "' is out of supported range");
    }

    if (ec == std::errc::invalid_argument) {
        throw std::invalid_argument("'" + value + "' is not a valid number");
    }

    throw std::invalid_argument(
        "'" + value + "' is not a valid number due to unexpected characters");
}

/**
 * @brief Converts a string view to a specified type and stores the result.
 *
 * This helper function provides a consistent interface for type conversion by
 * leveraging the primary `parse<T>` template.
 *
 * @tparam T Target type for conversion. Must be supported by `parse<T>`.
 * @param str Input string view to parse. Content must be convertible to `T`.
 * @param[out] output Reference to store the parsed result. Overwritten on success.
 * @throws Propagates any exceptions thrown by `parse<T>`.
 */
template <typename T>
void parseInto(std::string_view str, T& output)
{
    output = parse<T>(str);
}

} // namespace amon::util::string
