/**
 * @file
 * @author Lukas Hutak <hutak@cesnet.cz>
 * @brief Utils for string operations.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/Test.hpp>
#include <amon/util/string.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <limits>
#include <ranges>
#include <string>

namespace amon::util::string {

static bool isWhitespace(char letter) noexcept
{
    return std::isspace(letter) != 0;
}

static bool isNotWhitepace(char letter) noexcept
{
    return !isWhitespace(letter);
}

std::string_view trimWhitespaceStart(std::string_view str)
{
    const char* firstNonSpace = std::ranges::find_if(str, isNotWhitepace);
    const auto offset = std::distance(str.begin(), firstNonSpace);
    return str.substr(static_cast<size_t>(offset));
}

std::string_view trimWhitespaceEnd(std::string_view str)
{
    const auto lastNonSpace = std::ranges::find_if(std::ranges::reverse_view(str), isNotWhitepace);
    const auto offset = std::distance(str.begin(), lastNonSpace.base());
    return str.substr(0, static_cast<size_t>(offset));
}

std::string_view trimWhitespace(std::string_view str)
{
    return trimWhitespaceStart(trimWhitespaceEnd(str));
}

std::string_view trimPrefix(std::string_view str, std::string_view prefix)
{
    return stripPrefix(str, prefix).value_or(str);
}

std::string_view trimSuffix(std::string_view str, std::string_view suffix)
{
    return stripSuffix(str, suffix).value_or(str);
}

std::optional<std::string_view> stripPrefix(std::string_view str, std::string_view prefix)
{
    if (!str.starts_with(prefix)) {
        return std::nullopt;
    }

    return str.substr(prefix.size());
}

std::optional<std::string_view> stripSuffix(std::string_view str, std::string_view suffix)
{
    if (!str.ends_with(suffix)) {
        return std::nullopt;
    }

    return str.substr(0, str.size() - suffix.size());
}

std::optional<std::pair<std::string_view, std::string_view>>
splitOnce(std::string_view str, std::string_view delimiter)
{
    if (delimiter.empty()) {
        throw std::invalid_argument("delimiter cannot be empty");
    }

    const std::size_t pos = str.find(delimiter);
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }

    return std::make_pair(str.substr(0, pos), str.substr(pos + delimiter.length()));
}

std::optional<std::pair<std::string_view, std::string_view>>
rsplitOnce(std::string_view str, std::string_view delimiter)
{
    if (delimiter.empty()) {
        throw std::invalid_argument("delimiter cannot be empty");
    }

    const std::size_t pos = str.rfind(delimiter);
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }

    return std::make_pair(str.substr(0, pos), str.substr(pos + delimiter.length()));
}

std::vector<std::string_view>
split(std::string_view str, std::string_view delimiter, std::optional<size_t> maxSplits)
{
    std::vector<std::string_view> result;

    if (delimiter.empty()) {
        throw std::invalid_argument("delimiter cannot be empty");
    }

    if (str.empty()) {
        result.emplace_back(str);
        return result;
    }

    for (auto substr : std::views::split(str, delimiter)) {
        if (maxSplits && result.size() + 1 > maxSplits.value()) {
            result.emplace_back(substr.begin(), str.end());
            break;
        }

        result.emplace_back(substr);
    }

    return result;
}

std::vector<std::string_view> splitWhitespace(std::string_view str, std::optional<size_t> maxSplits)
{
    std::vector<std::string_view> result;
    str = trimWhitespaceStart(str);

    while (!str.empty()) {
        // Stop if we have reached the maximum number of splits
        if (maxSplits && result.size() + 1 > maxSplits.value()) {
            result.emplace_back(str);
            break;
        }

        // Find the next whitespace character
        const char* firstSpace = std::ranges::find_if(str, isWhitespace);
        const auto offset = static_cast<size_t>(std::distance(str.begin(), firstSpace));
        // Store prior substring to the result
        result.emplace_back(str.begin(), offset);

        str = trimWhitespaceStart(str.substr(offset));
    }

    return result;
}

bool isEqualCaseInsensitive(std::string_view lhs, std::string_view rhs)
{
    const auto cmp = [](char lhsLetter, char rhsLetter) {
        return std::tolower(lhsLetter) == std::tolower(rhsLetter);
    };

    return std::ranges::equal(lhs, rhs, cmp);
}

TEST_CASE("trimWhitespaceStart")
{
    CHECK_EQ(std::string(""), trimWhitespaceStart(""));
    CHECK_EQ(std::string(""), trimWhitespaceStart(" \n\r\t  \n"));
    CHECK_EQ(std::string("a"), trimWhitespaceStart("a"));
    CHECK_EQ(std::string("abc"), trimWhitespaceStart("abc"));
    CHECK_EQ(std::string("abc abc"), trimWhitespaceStart("abc abc"));

    CHECK_EQ(std::string("abc"), trimWhitespaceStart(" abc"));
    CHECK_EQ(std::string("abc"), trimWhitespaceStart("\tabc"));
    CHECK_EQ(std::string("abc"), trimWhitespaceStart("\nabc"));
    CHECK_EQ(std::string("abc"), trimWhitespaceStart("\rabc"));

    CHECK_EQ(std::string("abc "), trimWhitespaceStart(" abc "));
    CHECK_EQ(std::string("abc\t"), trimWhitespaceStart("\tabc\t"));
    CHECK_EQ(std::string("abc\n"), trimWhitespaceStart("\nabc\n"));
    CHECK_EQ(std::string("abc\r"), trimWhitespaceStart("\rabc\r"));

    CHECK_EQ(std::string("abc abc "), trimWhitespaceStart(" abc abc "));
    CHECK_EQ(std::string("abc abc\t"), trimWhitespaceStart("\tabc abc\t"));
    CHECK_EQ(std::string("abc abc\n"), trimWhitespaceStart("\nabc abc\n"));
    CHECK_EQ(std::string("abc abc\r"), trimWhitespaceStart("\rabc abc\r"));

    CHECK_EQ(std::string("abc abc"), trimWhitespaceStart("   \t \r abc abc"));
}

TEST_CASE("trimWhitespaceEnd")
{
    CHECK_EQ(std::string(""), trimWhitespaceEnd(""));
    CHECK_EQ(std::string(""), trimWhitespaceEnd(" \n\r\t  \n"));
    CHECK_EQ(std::string("a"), trimWhitespaceEnd("a"));
    CHECK_EQ(std::string("abc"), trimWhitespaceEnd("abc"));
    CHECK_EQ(std::string("abc abc"), trimWhitespaceEnd("abc abc"));

    CHECK_EQ(std::string("abc"), trimWhitespaceEnd("abc "));
    CHECK_EQ(std::string("abc"), trimWhitespaceEnd("abc\t"));
    CHECK_EQ(std::string("abc"), trimWhitespaceEnd("abc\n"));
    CHECK_EQ(std::string("abc"), trimWhitespaceEnd("abc\r"));

    CHECK_EQ(std::string(" abc"), trimWhitespaceEnd(" abc "));
    CHECK_EQ(std::string("\tabc"), trimWhitespaceEnd("\tabc\t"));
    CHECK_EQ(std::string("\nabc"), trimWhitespaceEnd("\nabc\n"));
    CHECK_EQ(std::string("\rabc"), trimWhitespaceEnd("\rabc\r"));

    CHECK_EQ(std::string(" abc abc"), trimWhitespaceEnd(" abc abc "));
    CHECK_EQ(std::string("\tabc abc"), trimWhitespaceEnd("\tabc abc\t"));
    CHECK_EQ(std::string("\nabc abc"), trimWhitespaceEnd("\nabc abc\n"));
    CHECK_EQ(std::string("\rabc abc"), trimWhitespaceEnd("\rabc abc\r"));

    CHECK_EQ(std::string("abc abc"), trimWhitespaceEnd("abc abc    \t \r "));
}

TEST_CASE("trimWhitespace")
{
    CHECK_EQ(std::string(""), trimWhitespace(""));
    CHECK_EQ(std::string(""), trimWhitespace(" \n\r\t  \n"));
    CHECK_EQ(std::string("a"), trimWhitespace("a"));
    CHECK_EQ(std::string("abc"), trimWhitespace("abc"));
    CHECK_EQ(std::string("abc abc"), trimWhitespace("abc abc"));

    CHECK_EQ(std::string("abc"), trimWhitespace(" abc"));
    CHECK_EQ(std::string("abc"), trimWhitespace("\tabc"));
    CHECK_EQ(std::string("abc"), trimWhitespace("\nabc"));
    CHECK_EQ(std::string("abc"), trimWhitespace("\rabc"));

    CHECK_EQ(std::string("abc"), trimWhitespace("abc "));
    CHECK_EQ(std::string("abc"), trimWhitespace("abc\t"));
    CHECK_EQ(std::string("abc"), trimWhitespace("abc\n"));
    CHECK_EQ(std::string("abc"), trimWhitespace("abc\r"));

    CHECK_EQ(std::string("abc"), trimWhitespace(" abc "));
    CHECK_EQ(std::string("abc"), trimWhitespace("\tabc\t"));
    CHECK_EQ(std::string("abc"), trimWhitespace("\nabc\n"));
    CHECK_EQ(std::string("abc"), trimWhitespace("\rabc\r"));

    CHECK_EQ(std::string("abc abc"), trimWhitespace(" abc abc "));
    CHECK_EQ(std::string("abc abc"), trimWhitespace("\tabc abc\t"));
    CHECK_EQ(std::string("abc abc"), trimWhitespace("\nabc abc\n"));
    CHECK_EQ(std::string("abc abc"), trimWhitespace("\rabc abc\r"));

    CHECK_EQ(std::string("abc abc"), trimWhitespace("    \t \r abc abc    \t \r "));
}

TEST_CASE("trimPrefix")
{
    CHECK_EQ(std::string(""), trimPrefix("", "hello"));
    CHECK_EQ(std::string(""), trimPrefix("hello", "hello"));
    CHECK_EQ(std::string(" world"), trimPrefix("hello world", "hello"));
}

TEST_CASE("trimSuffix")
{
    CHECK_EQ(std::string(""), trimSuffix("", "hello"));
    CHECK_EQ(std::string(""), trimSuffix("hello", "hello"));
    CHECK_EQ(std::string("hello "), trimSuffix("hello world", "world"));
}

TEST_CASE("stripPrefix")
{
    CHECK_EQ(std::nullopt, stripPrefix("", "hello"));
    CHECK_EQ(std::string(""), stripPrefix("hello", "hello"));
    CHECK_EQ(std::string(" world"), stripPrefix("hello world", "hello"));
}

TEST_CASE("stripSuffix")
{
    CHECK_EQ(std::nullopt, stripSuffix("", "hello"));
    CHECK_EQ(std::string(""), stripSuffix("hello", "hello"));
    CHECK_EQ(std::string("hello "), stripSuffix("hello world", "world"));
}

TEST_CASE("splitOnce")
{
    std::optional<std::pair<std::string_view, std::string_view>> result;

    // NOLINTBEGIN(bugprone-unchecked-optional-access)
    result = splitOnce("abc=def=ghi", '=');
    REQUIRE(result.has_value());
    CHECK_EQ(std::string("abc"), result->first);
    CHECK_EQ(std::string("def=ghi"), result->second);

    result = splitOnce("a=b", '=');
    REQUIRE(result.has_value());
    CHECK_EQ(std::string("a"), result->first);
    CHECK_EQ(std::string("b"), result->second);

    result = splitOnce("=", '=');
    REQUIRE(result.has_value());
    CHECK_EQ(std::string(""), result->first);
    CHECK_EQ(std::string(""), result->second);

    result = splitOnce("", ':');
    CHECK_EQ(std::nullopt, result);

    result = splitOnce("a=b", '-');
    CHECK_EQ(std::nullopt, result);

    result = splitOnce("Hello^_^C++^_^20^_^!", "^_^");
    REQUIRE(result.has_value());
    CHECK_EQ(std::string("Hello"), result->first);
    CHECK_EQ(std::string("C++^_^20^_^!"), result->second);
    // NOLINTEND(bugprone-unchecked-optional-access)

    // Empty delimiter
    CHECK_THROWS_AS(splitOnce("abc=def=ghi", "");, std::invalid_argument);
}

TEST_CASE("rsplitOnce")
{
    std::optional<std::pair<std::string_view, std::string_view>> result;

    // NOLINTBEGIN(bugprone-unchecked-optional-access)
    result = rsplitOnce("abc=def=ghi", '=');
    REQUIRE(result.has_value());
    CHECK_EQ(std::string("abc=def"), result->first);
    CHECK_EQ(std::string("ghi"), result->second);

    result = rsplitOnce("a=b", '=');
    REQUIRE(result.has_value());
    CHECK_EQ(std::string("a"), result->first);
    CHECK_EQ(std::string("b"), result->second);

    result = rsplitOnce("=", '=');
    REQUIRE(result.has_value());
    CHECK_EQ(std::string(""), result->first);
    CHECK_EQ(std::string(""), result->second);

    result = rsplitOnce("", ':');
    CHECK_EQ(std::nullopt, result);

    result = rsplitOnce("a=b", '-');
    CHECK_EQ(std::nullopt, result);

    result = rsplitOnce("Hello^_^C++^_^20^_^!", "^_^");
    REQUIRE(result.has_value());
    CHECK_EQ(std::string("Hello^_^C++^_^20"), result->first);
    CHECK_EQ(std::string("!"), result->second);
    // NOLINTEND(bugprone-unchecked-optional-access)

    // Empty delimiter
    CHECK_THROWS_AS(rsplitOnce("abc=def=ghi", "");, std::invalid_argument);
}

TEST_CASE("split")
{
    std::vector<std::string_view> result;

    // Empty or only delimiters input
    result = split("", ',');
    REQUIRE_EQ(1, result.size());
    CHECK_EQ(0, result[0].size());

    result = split(",,", ',');
    REQUIRE_EQ(3, result.size());
    CHECK_EQ(0, result[0].size());
    CHECK_EQ(0, result[1].size());
    CHECK_EQ(0, result[2].size());

    // Typical (unlimited) split
    result = split(" a, b ,c ", ',');
    REQUIRE_EQ(3, result.size());
    CHECK_EQ(std::string(" a"), result[0]);
    CHECK_EQ(std::string(" b "), result[1]);
    CHECK_EQ(std::string("c "), result[2]);

    result = split("a = b,c,, d", ',');
    REQUIRE_EQ(4, result.size());
    CHECK_EQ(std::string("a = b"), result[0]);
    CHECK_EQ(std::string("c"), result[1]);
    CHECK_EQ(0, result[2].size());
    CHECK_EQ(std::string(" d"), result[3]);

    // Splits with limitations
    result = split("a = b,c,, d", ',', 0);
    REQUIRE_EQ(1, result.size());
    CHECK_EQ(std::string("a = b,c,, d"), result[0]);

    result = split("a = b,c,, d", ',', 1);
    REQUIRE_EQ(2, result.size());
    CHECK_EQ(std::string("a = b"), result[0]);
    CHECK_EQ(std::string("c,, d"), result[1]);

    result = split("a = b,c,, d", ',', 2);
    REQUIRE_EQ(3, result.size());
    CHECK_EQ(std::string("a = b"), result[0]);
    CHECK_EQ(std::string("c"), result[1]);
    CHECK_EQ(std::string(", d"), result[2]);

    result = split("a = b,c,, d", ',', 3);
    REQUIRE_EQ(4, result.size());
    CHECK_EQ(std::string("a = b"), result[0]);
    CHECK_EQ(std::string("c"), result[1]);
    CHECK_EQ(0, result[2].size());
    CHECK_EQ(std::string(" d"), result[3]);

    result = split("a = b,c,, d", ',', 4);
    REQUIRE_EQ(4, result.size());
    CHECK_EQ(std::string("a = b"), result[0]);
    CHECK_EQ(std::string("c"), result[1]);
    CHECK_EQ(0, result[2].size());
    CHECK_EQ(std::string(" d"), result[3]);

    // Delimiter with multiple characters
    result = split("Hello^_^C++^_^20^_^!", "^_^");
    REQUIRE_EQ(4, result.size());
    CHECK_EQ(std::string("Hello"), result[0]);
    CHECK_EQ(std::string("C++"), result[1]);
    CHECK_EQ(std::string("20"), result[2]);
    CHECK_EQ(std::string("!"), result[3]);

    // Empty delimiter
    CHECK_THROWS_AS(split("a,b", ""), std::invalid_argument);
}

TEST_CASE("splitWhitespace")
{
    std::vector<std::string_view> result;

    // Empty or whitespaces only
    result = splitWhitespace("");
    CHECK(result.empty());
    result = splitWhitespace("   ");
    CHECK(result.empty());
    result = splitWhitespace("", 1);
    CHECK(result.empty());
    result = splitWhitespace(" \t\v\n\f");
    CHECK(result.empty());
    result = splitWhitespace("  ", 0);
    CHECK(result.empty());
    result = splitWhitespace("  ", 1);
    CHECK(result.empty());

    // Nothing to split
    result = splitWhitespace("Hello!");
    REQUIRE_EQ(1, result.size());
    CHECK_EQ(std::string("Hello!"), result[0]);

    // Typical (unlimited) split
    result = splitWhitespace("  Hello World!  ");
    REQUIRE_EQ(2, result.size());
    CHECK_EQ(std::string("Hello"), result[0]);
    CHECK_EQ(std::string("World!"), result[1]);

    result = splitWhitespace(" a b\t c ");
    REQUIRE_EQ(3, result.size());
    CHECK_EQ(std::string("a"), result[0]);
    CHECK_EQ(std::string("b"), result[1]);
    CHECK_EQ(std::string("c"), result[2]);

    // Splits with limitations
    result = splitWhitespace("a b\t c", 0);
    REQUIRE_EQ(1, result.size());
    CHECK_EQ(std::string("a b\t c"), result[0]);

    result = splitWhitespace("a b\t c", 1);
    REQUIRE_EQ(2, result.size());
    CHECK_EQ(std::string("a"), result[0]);
    CHECK_EQ(std::string("b\t c"), result[1]);

    result = splitWhitespace("a b\t c", 2);
    REQUIRE_EQ(3, result.size());
    CHECK_EQ(std::string("a"), result[0]);
    CHECK_EQ(std::string("b"), result[1]);
    CHECK_EQ(std::string("c"), result[2]);

    result = splitWhitespace("a b\t c", 3);
    REQUIRE_EQ(3, result.size());
    CHECK_EQ(std::string("a"), result[0]);
    CHECK_EQ(std::string("b"), result[1]);
    CHECK_EQ(std::string("c"), result[2]);

    // Leading whitespaces removed
    result = splitWhitespace("   my name is   John Doe  ");
    REQUIRE_EQ(5, result.size());
    CHECK_EQ(std::string("my"), result[0]);
    CHECK_EQ(std::string("Doe"), result[4]);

    result = splitWhitespace("   my name is   John Doe  ", 0);
    REQUIRE_EQ(1, result.size());
    CHECK_EQ(std::string("my name is   John Doe  "), result[0]);

    result = splitWhitespace("   my name is   John Doe  ", 1);
    REQUIRE_EQ(2, result.size());
    CHECK_EQ(std::string("my"), result[0]);
    CHECK_EQ(std::string("name is   John Doe  "), result[1]);
}

TEST_CASE("isEqualCaseInsensitive")
{
    CHECK(isEqualCaseInsensitive("", ""));
    CHECK(isEqualCaseInsensitive("a", "a"));
    CHECK(isEqualCaseInsensitive("abc", "abc"));
    CHECK(isEqualCaseInsensitive("abc abc", "abc abc"));

    CHECK(isEqualCaseInsensitive("a", "A"));
    CHECK(isEqualCaseInsensitive("abc", "ABC"));
    CHECK(isEqualCaseInsensitive("abc abc", "aBc AbC"));

    CHECK(isEqualCaseInsensitive("true", "True"));
    CHECK(isEqualCaseInsensitive("true", "tRue"));
    CHECK(isEqualCaseInsensitive("true", "trUe"));
    CHECK(isEqualCaseInsensitive("true", "truE"));
    CHECK(isEqualCaseInsensitive("False", "false"));
    CHECK(isEqualCaseInsensitive("FAlse", "false"));
    CHECK(isEqualCaseInsensitive("FALse", "false"));
    CHECK(isEqualCaseInsensitive("FALSe", "false"));
    CHECK(isEqualCaseInsensitive("FALSE", "false"));

    CHECK_FALSE(isEqualCaseInsensitive("true", " true"));
    CHECK_FALSE(isEqualCaseInsensitive("true", "true "));
    CHECK_FALSE(isEqualCaseInsensitive("true", "eurt"));
    CHECK_FALSE(isEqualCaseInsensitive("true", "false"));

    CHECK_FALSE(isEqualCaseInsensitive("true ", "true"));
    CHECK_FALSE(isEqualCaseInsensitive(" true", "true"));
}

TEST_CASE("parse string")
{
    CHECK(parse<std::string>("") == "");
    CHECK(parse<std::string>("hello there") == "hello there");
}

TEST_CASE("parse bool")
{
    CHECK(parse<bool>("1"));
    CHECK(parse<bool>("true"));
    CHECK(parse<bool>("True"));
    CHECK(parse<bool>("TRUE"));
    CHECK(parse<bool>("yes"));
    CHECK(parse<bool>("Yes"));
    CHECK(parse<bool>("YES"));
    CHECK(parse<bool>("on"));
    CHECK(parse<bool>("On"));
    CHECK(parse<bool>("ON"));

    CHECK_FALSE(parse<bool>("0"));
    CHECK_FALSE(parse<bool>("false"));
    CHECK_FALSE(parse<bool>("False"));
    CHECK_FALSE(parse<bool>("FALSE"));
    CHECK_FALSE(parse<bool>("no"));
    CHECK_FALSE(parse<bool>("No"));
    CHECK_FALSE(parse<bool>("NO"));
    CHECK_FALSE(parse<bool>("off"));
    CHECK_FALSE(parse<bool>("Off"));
    CHECK_FALSE(parse<bool>("OFF"));

    CHECK_THROWS_AS(parse<bool>(""), std::invalid_argument);
    CHECK_THROWS_AS(parse<bool>("sure"), std::invalid_argument);
    CHECK_THROWS_AS(parse<bool>("none"), std::invalid_argument);
    CHECK_THROWS_AS(parse<bool>(" 1"), std::invalid_argument);
    CHECK_THROWS_AS(parse<bool>("0 "), std::invalid_argument);
    CHECK_THROWS_AS(parse<bool>(" True"), std::invalid_argument);
    CHECK_THROWS_AS(parse<bool>("false "), std::invalid_argument);
}

TEST_CASE("parse integer (unsigned, base 10)")
{
    CHECK_EQ(parse<uint8_t>("0"), 0);
    CHECK_EQ(parse<uint8_t>("1"), 1);
    CHECK_EQ(parse<uint8_t>("075"), 75);
    CHECK_EQ(parse<uint8_t>("32"), 32);
    CHECK_EQ(parse<uint8_t>("255"), std::numeric_limits<uint8_t>::max());
    CHECK_THROWS_AS(parse<uint8_t>(""), std::invalid_argument);
    CHECK_THROWS_AS(parse<uint8_t>("abc"), std::invalid_argument);
    CHECK_THROWS_AS(parse<uint8_t>("-1"), std::invalid_argument);
    CHECK_THROWS_AS(parse<uint8_t>("256"), std::range_error);
    CHECK_THROWS_AS(parse<uint8_t>(" 1"), std::invalid_argument);
    CHECK_THROWS_AS(parse<uint8_t>("1 "), std::invalid_argument);
    CHECK_THROWS_AS(parse<uint8_t>(" 1 "), std::invalid_argument);
    CHECK_THROWS_AS(parse<uint8_t>("1 ns"), std::invalid_argument);

    CHECK_EQ(parse<uint64_t>("0"), 0);
    CHECK_EQ(parse<uint64_t>("1"), 1);
    CHECK_EQ(parse<uint64_t>("0004827545"), 4827545);
    CHECK_EQ(parse<uint64_t>("56466545687132"), 56466545687132);
    CHECK_EQ(parse<uint64_t>("18446744073709551615"), std::numeric_limits<uint64_t>::max());
    CHECK_THROWS_AS(parse<uint64_t>(""), std::invalid_argument);
    CHECK_THROWS_AS(parse<uint64_t>("abc"), std::invalid_argument);
    CHECK_THROWS_AS(parse<uint64_t>("-1"), std::invalid_argument);
    CHECK_THROWS_AS(parse<uint64_t>("18446744073709551616"), std::range_error);
    CHECK_THROWS_AS(parse<uint64_t>(" 1"), std::invalid_argument);
    CHECK_THROWS_AS(parse<uint64_t>("1 "), std::invalid_argument);
    CHECK_THROWS_AS(parse<uint64_t>(" 1 "), std::invalid_argument);
    CHECK_THROWS_AS(parse<uint64_t>("1 ns"), std::invalid_argument);
}

TEST_CASE("parse integer (signed, base 10)")
{
    CHECK_EQ(parse<int8_t>("0"), 0);
    CHECK_EQ(parse<int8_t>("-1"), -1);
    CHECK_EQ(parse<int8_t>("1"), 1);
    CHECK_EQ(parse<int8_t>("-56"), -56);
    CHECK_EQ(parse<int8_t>("32"), 32);
    CHECK_EQ(parse<int8_t>("075"), 75);
    CHECK_EQ(parse<int8_t>("-128"), std::numeric_limits<int8_t>::min());
    CHECK_EQ(parse<int8_t>("127"), std::numeric_limits<int8_t>::max());
    CHECK_THROWS_AS(parse<int8_t>(""), std::invalid_argument);
    CHECK_THROWS_AS(parse<int8_t>("abc"), std::invalid_argument);
    CHECK_THROWS_AS(parse<int8_t>("-129"), std::range_error);
    CHECK_THROWS_AS(parse<int8_t>("128"), std::range_error);
    CHECK_THROWS_AS(parse<int8_t>(" 1"), std::invalid_argument);
    CHECK_THROWS_AS(parse<int8_t>("1 "), std::invalid_argument);
    CHECK_THROWS_AS(parse<int8_t>(" 1 "), std::invalid_argument);
    CHECK_THROWS_AS(parse<int8_t>("1 ns"), std::invalid_argument);

    CHECK_EQ(parse<int64_t>("0"), 0);
    CHECK_EQ(parse<int64_t>("-1"), -1);
    CHECK_EQ(parse<int64_t>("1"), 1);
    CHECK_EQ(parse<int64_t>("-9871654618710"), -9871654618710);
    CHECK_EQ(parse<int64_t>("56466545687132"), 56466545687132);
    CHECK_EQ(parse<int64_t>("-0004827545"), -4827545);
    CHECK_EQ(parse<int64_t>("-9223372036854775808"), std::numeric_limits<int64_t>::min());
    CHECK_EQ(parse<int64_t>("9223372036854775807"), std::numeric_limits<int64_t>::max());
    CHECK_THROWS_AS(parse<int64_t>(""), std::invalid_argument);
    CHECK_THROWS_AS(parse<int64_t>("abc"), std::invalid_argument);
    CHECK_THROWS_AS(parse<int64_t>("-9223372036854775809"), std::range_error);
    CHECK_THROWS_AS(parse<int64_t>("9223372036854775808"), std::range_error);
    CHECK_THROWS_AS(parse<int64_t>(" 1"), std::invalid_argument);
    CHECK_THROWS_AS(parse<int64_t>("1 "), std::invalid_argument);
    CHECK_THROWS_AS(parse<int64_t>(" 1 "), std::invalid_argument);
    CHECK_THROWS_AS(parse<int64_t>("1 ns"), std::invalid_argument);
}

TEST_CASE("parse integer (base 2)")
{
    CHECK_EQ(parse<uint8_t>("0b0"), 0);
    CHECK_EQ(parse<uint8_t>("0b1"), 1);
    CHECK_EQ(parse<uint8_t>("0b100"), 4);
    CHECK_EQ(parse<uint8_t>("0b01001111"), 79);
    CHECK_EQ(parse<uint8_t>("0b11111111"), std::numeric_limits<uint8_t>::max());
    CHECK_THROWS_AS(parse<uint8_t>(""), std::invalid_argument);
    CHECK_THROWS_AS(parse<uint8_t>("0b100000000"), std::range_error);
    CHECK_THROWS_AS(parse<uint8_t>("0b"), std::invalid_argument);
    CHECK_THROWS_AS(parse<uint8_t>("0b12"), std::invalid_argument);
    CHECK_THROWS_AS(parse<uint8_t>("0b001c1000"), std::invalid_argument);
    CHECK_THROWS_AS(parse<uint8_t>(" 0b1100"), std::invalid_argument);
    CHECK_THROWS_AS(parse<uint8_t>("0b0011 "), std::invalid_argument);

    CHECK_EQ(
        parse<uint64_t>("0b0000000000000000000000000000000000000000000000000000000000000000"),
        0);
    CHECK_EQ(
        parse<uint64_t>("0b1111111111111111111111111111111111111111111111111111111111111111"),
        std::numeric_limits<uint64_t>::max());

    // Unsupported conversion with negative values
    CHECK_THROWS_AS(parse<int8_t>("-0b1"), std::invalid_argument);
    CHECK_THROWS_AS(parse<uint8_t>("-0b1"), std::invalid_argument);
    CHECK_THROWS_AS(parse<int64_t>("-0b177"), std::invalid_argument);
    CHECK_THROWS_AS(parse<uint64_t>("-0b177"), std::invalid_argument);
    CHECK_THROWS_AS(parse<int8_t>("0b-0011"), std::invalid_argument);
    CHECK_THROWS_AS(parse<int8_t>("0b+0011"), std::invalid_argument);
}

TEST_CASE("parse integer (base 16)")
{
    CHECK_EQ(parse<uint8_t>("0x0"), 0);
    CHECK_EQ(parse<uint8_t>("0x1"), 1);
    CHECK_EQ(parse<uint8_t>("0x0A"), 0x0A);
    CHECK_EQ(parse<uint8_t>("0x1a"), 0x1A);
    CHECK_EQ(parse<uint8_t>("0x1A"), 0x1A);
    CHECK_EQ(parse<uint8_t>("0xFF"), std::numeric_limits<uint8_t>::max());
    CHECK_THROWS_AS(parse<uint8_t>("0x100"), std::range_error);
    CHECK_THROWS_AS(parse<uint8_t>("0x"), std::invalid_argument);
    CHECK_THROWS_AS(parse<uint8_t>("0x1g"), std::invalid_argument);
    CHECK_THROWS_AS(parse<uint8_t>(" 0xff"), std::invalid_argument);
    CHECK_THROWS_AS(parse<uint8_t>("0xff "), std::invalid_argument);
    CHECK_THROWS_AS(parse<uint8_t>("0XFF"), std::invalid_argument);

    CHECK_EQ(parse<uint64_t>("0x0"), 0);
    CHECK_EQ(parse<uint64_t>("0x1"), 1);
    CHECK_EQ(parse<uint64_t>("0x00BBCCDD"), 0x00BBCCDD);
    CHECK_EQ(parse<uint64_t>("0xA5A5A5A5A5A5A5A5"), 0xA5A5A5A5A5A5A5A5ULL);
    CHECK_EQ(parse<uint64_t>("0xFFFFFFFFFFFFFFFF"), std::numeric_limits<uint64_t>::max());
    CHECK_THROWS_AS(parse<uint64_t>(""), std::invalid_argument);
    CHECK_THROWS_AS(parse<uint64_t>("0x10000000000000000"), std::range_error);

    // Unsupported conversion with negative values
    CHECK_THROWS_AS(parse<int8_t>("-0x1"), std::invalid_argument);
    CHECK_THROWS_AS(parse<uint8_t>("-0x1"), std::invalid_argument);
    CHECK_THROWS_AS(parse<int64_t>("-0x177"), std::invalid_argument);
    CHECK_THROWS_AS(parse<uint64_t>("-0x177"), std::invalid_argument);
    CHECK_THROWS_AS(parse<int8_t>("0x-1"), std::invalid_argument);
}

TEST_CASE("parse float")
{
    const std::string fmin = std::to_string(std::numeric_limits<float>::min());
    const std::string fmax = std::to_string(std::numeric_limits<float>::max());
    const std::string fden = std::to_string(std::numeric_limits<float>::denorm_min());

    CHECK_EQ(parse<float>("0"), doctest::Approx(0.0F));
    CHECK_EQ(parse<float>("-1.5"), doctest::Approx(-1.5F));
    CHECK_EQ(parse<float>("2.75"), doctest::Approx(2.75F));
    CHECK_EQ(parse<float>("1e3"), doctest::Approx(1000.0F));
    CHECK_EQ(parse<float>("-1.003e-12"), doctest::Approx(-1.003e-12));

    CHECK_EQ(parse<float>(fmin), doctest::Approx(std::numeric_limits<float>::min()));
    CHECK_EQ(parse<float>(fmax), doctest::Approx(std::numeric_limits<float>::max()));
    CHECK_EQ(parse<float>(fden), doctest::Approx(std::numeric_limits<float>::denorm_min()));

    CHECK(doctest::IsNaN(parse<float>("nan")));
    CHECK(doctest::IsNaN(parse<float>("NaN")));
    CHECK(std::isinf(parse<float>("inf")));
    CHECK(std::isinf(parse<float>("INF")));
    CHECK(std::isinf(parse<float>("inf")));
    CHECK(std::isinf(parse<float>("-inf")));

    CHECK_THROWS_AS(parse<float>("1e39"), std::range_error);
    CHECK_THROWS_AS(parse<float>("1e-46"), std::range_error);
    CHECK_THROWS_AS(parse<float>(""), std::invalid_argument);
    CHECK_THROWS_AS(parse<float>(" 1.0"), std::invalid_argument);
    CHECK_THROWS_AS(parse<float>("1.0 "), std::invalid_argument);
    CHECK_THROWS_AS(parse<float>("1.0abc"), std::invalid_argument);
    CHECK_THROWS_AS(parse<float>("abc"), std::invalid_argument);
    CHECK_THROWS_AS(parse<float>("inf "), std::invalid_argument);
    CHECK_THROWS_AS(parse<float>(" nan"), std::invalid_argument);
}

TEST_CASE("parse double")
{
    const std::string dmin = std::to_string(std::numeric_limits<double>::min());
    const std::string dmax = std::to_string(std::numeric_limits<double>::max());
    const std::string dden = std::to_string(std::numeric_limits<double>::denorm_min());

    CHECK_EQ(parse<double>("0"), doctest::Approx(0.0));
    CHECK_EQ(parse<double>("-1.5"), doctest::Approx(-1.5));
    CHECK_EQ(parse<double>("2.75"), doctest::Approx(2.75));
    CHECK_EQ(parse<double>("1e3"), doctest::Approx(1000.0));
    CHECK_EQ(parse<double>("-1.003e-12"), doctest::Approx(-1.003e-12));

    CHECK_EQ(parse<double>(dmin), doctest::Approx(std::numeric_limits<double>::min()));
    CHECK_EQ(parse<double>(dmax), doctest::Approx(std::numeric_limits<double>::max()));
    CHECK_EQ(parse<double>(dden), doctest::Approx(std::numeric_limits<double>::denorm_min()));

    CHECK(doctest::IsNaN(parse<double>("nan")));
    CHECK(doctest::IsNaN(parse<double>("NaN")));
    CHECK(std::isinf(parse<double>("inf")));
    CHECK(std::isinf(parse<double>("INF")));
    CHECK(std::isinf(parse<double>("inf")));
    CHECK(std::isinf(parse<double>("-inf")));

    CHECK_THROWS_AS(parse<double>("1e309"), std::range_error);
    CHECK_THROWS_AS(parse<double>("1e-324"), std::range_error);
    CHECK_THROWS_AS(parse<double>(""), std::invalid_argument);
    CHECK_THROWS_AS(parse<double>(" 1.0"), std::invalid_argument);
    CHECK_THROWS_AS(parse<double>("1.0 "), std::invalid_argument);
    CHECK_THROWS_AS(parse<double>("1.0abc"), std::invalid_argument);
    CHECK_THROWS_AS(parse<double>("abc"), std::invalid_argument);
    CHECK_THROWS_AS(parse<double>("inf "), std::invalid_argument);
    CHECK_THROWS_AS(parse<double>(" nan"), std::invalid_argument);
}

} // namespace amon::util::string
