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

#ifdef AMON_ENABLE_TESTS

TEST(AmonUtilString, Trimwhitespacestart)
{
    EXPECT_EQ(std::string(""), trimWhitespaceStart(""));
    EXPECT_EQ(std::string(""), trimWhitespaceStart(" \n\r\t  \n"));
    EXPECT_EQ(std::string("a"), trimWhitespaceStart("a"));
    EXPECT_EQ(std::string("abc"), trimWhitespaceStart("abc"));
    EXPECT_EQ(std::string("abc abc"), trimWhitespaceStart("abc abc"));

    EXPECT_EQ(std::string("abc"), trimWhitespaceStart(" abc"));
    EXPECT_EQ(std::string("abc"), trimWhitespaceStart("\tabc"));
    EXPECT_EQ(std::string("abc"), trimWhitespaceStart("\nabc"));
    EXPECT_EQ(std::string("abc"), trimWhitespaceStart("\rabc"));

    EXPECT_EQ(std::string("abc "), trimWhitespaceStart(" abc "));
    EXPECT_EQ(std::string("abc\t"), trimWhitespaceStart("\tabc\t"));
    EXPECT_EQ(std::string("abc\n"), trimWhitespaceStart("\nabc\n"));
    EXPECT_EQ(std::string("abc\r"), trimWhitespaceStart("\rabc\r"));

    EXPECT_EQ(std::string("abc abc "), trimWhitespaceStart(" abc abc "));
    EXPECT_EQ(std::string("abc abc\t"), trimWhitespaceStart("\tabc abc\t"));
    EXPECT_EQ(std::string("abc abc\n"), trimWhitespaceStart("\nabc abc\n"));
    EXPECT_EQ(std::string("abc abc\r"), trimWhitespaceStart("\rabc abc\r"));

    EXPECT_EQ(std::string("abc abc"), trimWhitespaceStart("   \t \r abc abc"));
}

TEST(AmonUtilString, Trimwhitespaceend)
{
    EXPECT_EQ(std::string(""), trimWhitespaceEnd(""));
    EXPECT_EQ(std::string(""), trimWhitespaceEnd(" \n\r\t  \n"));
    EXPECT_EQ(std::string("a"), trimWhitespaceEnd("a"));
    EXPECT_EQ(std::string("abc"), trimWhitespaceEnd("abc"));
    EXPECT_EQ(std::string("abc abc"), trimWhitespaceEnd("abc abc"));

    EXPECT_EQ(std::string("abc"), trimWhitespaceEnd("abc "));
    EXPECT_EQ(std::string("abc"), trimWhitespaceEnd("abc\t"));
    EXPECT_EQ(std::string("abc"), trimWhitespaceEnd("abc\n"));
    EXPECT_EQ(std::string("abc"), trimWhitespaceEnd("abc\r"));

    EXPECT_EQ(std::string(" abc"), trimWhitespaceEnd(" abc "));
    EXPECT_EQ(std::string("\tabc"), trimWhitespaceEnd("\tabc\t"));
    EXPECT_EQ(std::string("\nabc"), trimWhitespaceEnd("\nabc\n"));
    EXPECT_EQ(std::string("\rabc"), trimWhitespaceEnd("\rabc\r"));

    EXPECT_EQ(std::string(" abc abc"), trimWhitespaceEnd(" abc abc "));
    EXPECT_EQ(std::string("\tabc abc"), trimWhitespaceEnd("\tabc abc\t"));
    EXPECT_EQ(std::string("\nabc abc"), trimWhitespaceEnd("\nabc abc\n"));
    EXPECT_EQ(std::string("\rabc abc"), trimWhitespaceEnd("\rabc abc\r"));

    EXPECT_EQ(std::string("abc abc"), trimWhitespaceEnd("abc abc    \t \r "));
}

TEST(AmonUtilString, Trimwhitespace)
{
    EXPECT_EQ(std::string(""), trimWhitespace(""));
    EXPECT_EQ(std::string(""), trimWhitespace(" \n\r\t  \n"));
    EXPECT_EQ(std::string("a"), trimWhitespace("a"));
    EXPECT_EQ(std::string("abc"), trimWhitespace("abc"));
    EXPECT_EQ(std::string("abc abc"), trimWhitespace("abc abc"));

    EXPECT_EQ(std::string("abc"), trimWhitespace(" abc"));
    EXPECT_EQ(std::string("abc"), trimWhitespace("\tabc"));
    EXPECT_EQ(std::string("abc"), trimWhitespace("\nabc"));
    EXPECT_EQ(std::string("abc"), trimWhitespace("\rabc"));

    EXPECT_EQ(std::string("abc"), trimWhitespace("abc "));
    EXPECT_EQ(std::string("abc"), trimWhitespace("abc\t"));
    EXPECT_EQ(std::string("abc"), trimWhitespace("abc\n"));
    EXPECT_EQ(std::string("abc"), trimWhitespace("abc\r"));

    EXPECT_EQ(std::string("abc"), trimWhitespace(" abc "));
    EXPECT_EQ(std::string("abc"), trimWhitespace("\tabc\t"));
    EXPECT_EQ(std::string("abc"), trimWhitespace("\nabc\n"));
    EXPECT_EQ(std::string("abc"), trimWhitespace("\rabc\r"));

    EXPECT_EQ(std::string("abc abc"), trimWhitespace(" abc abc "));
    EXPECT_EQ(std::string("abc abc"), trimWhitespace("\tabc abc\t"));
    EXPECT_EQ(std::string("abc abc"), trimWhitespace("\nabc abc\n"));
    EXPECT_EQ(std::string("abc abc"), trimWhitespace("\rabc abc\r"));

    EXPECT_EQ(std::string("abc abc"), trimWhitespace("    \t \r abc abc    \t \r "));
}

TEST(AmonUtilString, Trimprefix)
{
    EXPECT_EQ(std::string(""), trimPrefix("", "hello"));
    EXPECT_EQ(std::string(""), trimPrefix("hello", "hello"));
    EXPECT_EQ(std::string(" world"), trimPrefix("hello world", "hello"));
}

TEST(AmonUtilString, Trimsuffix)
{
    EXPECT_EQ(std::string(""), trimSuffix("", "hello"));
    EXPECT_EQ(std::string(""), trimSuffix("hello", "hello"));
    EXPECT_EQ(std::string("hello "), trimSuffix("hello world", "world"));
}

TEST(AmonUtilString, Stripprefix)
{
    EXPECT_EQ(std::nullopt, stripPrefix("", "hello"));
    EXPECT_EQ(std::string(""), stripPrefix("hello", "hello"));
    EXPECT_EQ(std::string(" world"), stripPrefix("hello world", "hello"));
}

TEST(AmonUtilString, Stripsuffix)
{
    EXPECT_EQ(std::nullopt, stripSuffix("", "hello"));
    EXPECT_EQ(std::string(""), stripSuffix("hello", "hello"));
    EXPECT_EQ(std::string("hello "), stripSuffix("hello world", "world"));
}

TEST(AmonUtilString, Splitonce)
{
    std::optional<std::pair<std::string_view, std::string_view>> result;

    // NOLINTBEGIN(bugprone-unchecked-optional-access)
    result = splitOnce("abc=def=ghi", '=');
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::string("abc"), result->first);
    EXPECT_EQ(std::string("def=ghi"), result->second);

    result = splitOnce("a=b", '=');
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::string("a"), result->first);
    EXPECT_EQ(std::string("b"), result->second);

    result = splitOnce("=", '=');
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::string(""), result->first);
    EXPECT_EQ(std::string(""), result->second);

    result = splitOnce("", ':');
    EXPECT_EQ(std::nullopt, result);

    result = splitOnce("a=b", '-');
    EXPECT_EQ(std::nullopt, result);

    result = splitOnce("Hello^_^C++^_^20^_^!", "^_^");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::string("Hello"), result->first);
    EXPECT_EQ(std::string("C++^_^20^_^!"), result->second);
    // NOLINTEND(bugprone-unchecked-optional-access)

    // Empty delimiter
    EXPECT_THROW(splitOnce("abc=def=ghi", "");, std::invalid_argument);
}

TEST(AmonUtilString, Rsplitonce)
{
    std::optional<std::pair<std::string_view, std::string_view>> result;

    // NOLINTBEGIN(bugprone-unchecked-optional-access)
    result = rsplitOnce("abc=def=ghi", '=');
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::string("abc=def"), result->first);
    EXPECT_EQ(std::string("ghi"), result->second);

    result = rsplitOnce("a=b", '=');
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::string("a"), result->first);
    EXPECT_EQ(std::string("b"), result->second);

    result = rsplitOnce("=", '=');
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::string(""), result->first);
    EXPECT_EQ(std::string(""), result->second);

    result = rsplitOnce("", ':');
    EXPECT_EQ(std::nullopt, result);

    result = rsplitOnce("a=b", '-');
    EXPECT_EQ(std::nullopt, result);

    result = rsplitOnce("Hello^_^C++^_^20^_^!", "^_^");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::string("Hello^_^C++^_^20"), result->first);
    EXPECT_EQ(std::string("!"), result->second);
    // NOLINTEND(bugprone-unchecked-optional-access)

    // Empty delimiter
    EXPECT_THROW(rsplitOnce("abc=def=ghi", "");, std::invalid_argument);
}

TEST(AmonUtilString, Split)
{
    std::vector<std::string_view> result;

    // Empty or only delimiters input
    result = split("", ',');
    ASSERT_EQ(1, result.size());
    EXPECT_EQ(0, result[0].size());

    result = split(",,", ',');
    ASSERT_EQ(3, result.size());
    EXPECT_EQ(0, result[0].size());
    EXPECT_EQ(0, result[1].size());
    EXPECT_EQ(0, result[2].size());

    // Typical (unlimited) split
    result = split(" a, b ,c ", ',');
    ASSERT_EQ(3, result.size());
    EXPECT_EQ(std::string(" a"), result[0]);
    EXPECT_EQ(std::string(" b "), result[1]);
    EXPECT_EQ(std::string("c "), result[2]);

    result = split("a = b,c,, d", ',');
    ASSERT_EQ(4, result.size());
    EXPECT_EQ(std::string("a = b"), result[0]);
    EXPECT_EQ(std::string("c"), result[1]);
    EXPECT_EQ(0, result[2].size());
    EXPECT_EQ(std::string(" d"), result[3]);

    // Splits with limitations
    result = split("a = b,c,, d", ',', 0);
    ASSERT_EQ(1, result.size());
    EXPECT_EQ(std::string("a = b,c,, d"), result[0]);

    result = split("a = b,c,, d", ',', 1);
    ASSERT_EQ(2, result.size());
    EXPECT_EQ(std::string("a = b"), result[0]);
    EXPECT_EQ(std::string("c,, d"), result[1]);

    result = split("a = b,c,, d", ',', 2);
    ASSERT_EQ(3, result.size());
    EXPECT_EQ(std::string("a = b"), result[0]);
    EXPECT_EQ(std::string("c"), result[1]);
    EXPECT_EQ(std::string(", d"), result[2]);

    result = split("a = b,c,, d", ',', 3);
    ASSERT_EQ(4, result.size());
    EXPECT_EQ(std::string("a = b"), result[0]);
    EXPECT_EQ(std::string("c"), result[1]);
    EXPECT_EQ(0, result[2].size());
    EXPECT_EQ(std::string(" d"), result[3]);

    result = split("a = b,c,, d", ',', 4);
    ASSERT_EQ(4, result.size());
    EXPECT_EQ(std::string("a = b"), result[0]);
    EXPECT_EQ(std::string("c"), result[1]);
    EXPECT_EQ(0, result[2].size());
    EXPECT_EQ(std::string(" d"), result[3]);

    // Delimiter with multiple characters
    result = split("Hello^_^C++^_^20^_^!", "^_^");
    ASSERT_EQ(4, result.size());
    EXPECT_EQ(std::string("Hello"), result[0]);
    EXPECT_EQ(std::string("C++"), result[1]);
    EXPECT_EQ(std::string("20"), result[2]);
    EXPECT_EQ(std::string("!"), result[3]);

    // Empty delimiter
    EXPECT_THROW(split("a,b", ""), std::invalid_argument);
}

TEST(AmonUtilString, Splitwhitespace)
{
    std::vector<std::string_view> result;

    // Empty or whitespaces only
    result = splitWhitespace("");
    EXPECT_TRUE(result.empty());
    result = splitWhitespace("   ");
    EXPECT_TRUE(result.empty());
    result = splitWhitespace("", 1);
    EXPECT_TRUE(result.empty());
    result = splitWhitespace(" \t\v\n\f");
    EXPECT_TRUE(result.empty());
    result = splitWhitespace("  ", 0);
    EXPECT_TRUE(result.empty());
    result = splitWhitespace("  ", 1);
    EXPECT_TRUE(result.empty());

    // Nothing to split
    result = splitWhitespace("Hello!");
    ASSERT_EQ(1, result.size());
    EXPECT_EQ(std::string("Hello!"), result[0]);

    // Typical (unlimited) split
    result = splitWhitespace("  Hello World!  ");
    ASSERT_EQ(2, result.size());
    EXPECT_EQ(std::string("Hello"), result[0]);
    EXPECT_EQ(std::string("World!"), result[1]);

    result = splitWhitespace(" a b\t c ");
    ASSERT_EQ(3, result.size());
    EXPECT_EQ(std::string("a"), result[0]);
    EXPECT_EQ(std::string("b"), result[1]);
    EXPECT_EQ(std::string("c"), result[2]);

    // Splits with limitations
    result = splitWhitespace("a b\t c", 0);
    ASSERT_EQ(1, result.size());
    EXPECT_EQ(std::string("a b\t c"), result[0]);

    result = splitWhitespace("a b\t c", 1);
    ASSERT_EQ(2, result.size());
    EXPECT_EQ(std::string("a"), result[0]);
    EXPECT_EQ(std::string("b\t c"), result[1]);

    result = splitWhitespace("a b\t c", 2);
    ASSERT_EQ(3, result.size());
    EXPECT_EQ(std::string("a"), result[0]);
    EXPECT_EQ(std::string("b"), result[1]);
    EXPECT_EQ(std::string("c"), result[2]);

    result = splitWhitespace("a b\t c", 3);
    ASSERT_EQ(3, result.size());
    EXPECT_EQ(std::string("a"), result[0]);
    EXPECT_EQ(std::string("b"), result[1]);
    EXPECT_EQ(std::string("c"), result[2]);

    // Leading whitespaces removed
    result = splitWhitespace("   my name is   John Doe  ");
    ASSERT_EQ(5, result.size());
    EXPECT_EQ(std::string("my"), result[0]);
    EXPECT_EQ(std::string("Doe"), result[4]);

    result = splitWhitespace("   my name is   John Doe  ", 0);
    ASSERT_EQ(1, result.size());
    EXPECT_EQ(std::string("my name is   John Doe  "), result[0]);

    result = splitWhitespace("   my name is   John Doe  ", 1);
    ASSERT_EQ(2, result.size());
    EXPECT_EQ(std::string("my"), result[0]);
    EXPECT_EQ(std::string("name is   John Doe  "), result[1]);
}

TEST(AmonUtilString, Isequalcaseinsensitive)
{
    EXPECT_TRUE(isEqualCaseInsensitive("", ""));
    EXPECT_TRUE(isEqualCaseInsensitive("a", "a"));
    EXPECT_TRUE(isEqualCaseInsensitive("abc", "abc"));
    EXPECT_TRUE(isEqualCaseInsensitive("abc abc", "abc abc"));

    EXPECT_TRUE(isEqualCaseInsensitive("a", "A"));
    EXPECT_TRUE(isEqualCaseInsensitive("abc", "ABC"));
    EXPECT_TRUE(isEqualCaseInsensitive("abc abc", "aBc AbC"));

    EXPECT_TRUE(isEqualCaseInsensitive("true", "True"));
    EXPECT_TRUE(isEqualCaseInsensitive("true", "tRue"));
    EXPECT_TRUE(isEqualCaseInsensitive("true", "trUe"));
    EXPECT_TRUE(isEqualCaseInsensitive("true", "truE"));
    EXPECT_TRUE(isEqualCaseInsensitive("False", "false"));
    EXPECT_TRUE(isEqualCaseInsensitive("FAlse", "false"));
    EXPECT_TRUE(isEqualCaseInsensitive("FALse", "false"));
    EXPECT_TRUE(isEqualCaseInsensitive("FALSe", "false"));
    EXPECT_TRUE(isEqualCaseInsensitive("FALSE", "false"));

    EXPECT_FALSE(isEqualCaseInsensitive("true", " true"));
    EXPECT_FALSE(isEqualCaseInsensitive("true", "true "));
    EXPECT_FALSE(isEqualCaseInsensitive("true", "eurt"));
    EXPECT_FALSE(isEqualCaseInsensitive("true", "false"));

    EXPECT_FALSE(isEqualCaseInsensitive("true ", "true"));
    EXPECT_FALSE(isEqualCaseInsensitive(" true", "true"));
}

TEST(AmonUtilString, ParseString)
{
    EXPECT_TRUE(parse<std::string>("") == "");
    EXPECT_TRUE(parse<std::string>("hello there") == "hello there");
}

TEST(AmonUtilString, ParseBool)
{
    EXPECT_TRUE(parse<bool>("1"));
    EXPECT_TRUE(parse<bool>("true"));
    EXPECT_TRUE(parse<bool>("True"));
    EXPECT_TRUE(parse<bool>("TRUE"));
    EXPECT_TRUE(parse<bool>("yes"));
    EXPECT_TRUE(parse<bool>("Yes"));
    EXPECT_TRUE(parse<bool>("YES"));
    EXPECT_TRUE(parse<bool>("on"));
    EXPECT_TRUE(parse<bool>("On"));
    EXPECT_TRUE(parse<bool>("ON"));

    EXPECT_FALSE(parse<bool>("0"));
    EXPECT_FALSE(parse<bool>("false"));
    EXPECT_FALSE(parse<bool>("False"));
    EXPECT_FALSE(parse<bool>("FALSE"));
    EXPECT_FALSE(parse<bool>("no"));
    EXPECT_FALSE(parse<bool>("No"));
    EXPECT_FALSE(parse<bool>("NO"));
    EXPECT_FALSE(parse<bool>("off"));
    EXPECT_FALSE(parse<bool>("Off"));
    EXPECT_FALSE(parse<bool>("OFF"));

    EXPECT_THROW(parse<bool>(""), std::invalid_argument);
    EXPECT_THROW(parse<bool>("sure"), std::invalid_argument);
    EXPECT_THROW(parse<bool>("none"), std::invalid_argument);
    EXPECT_THROW(parse<bool>(" 1"), std::invalid_argument);
    EXPECT_THROW(parse<bool>("0 "), std::invalid_argument);
    EXPECT_THROW(parse<bool>(" True"), std::invalid_argument);
    EXPECT_THROW(parse<bool>("false "), std::invalid_argument);
}

TEST(AmonUtilString, ParseIntegerUnsignedBase10)
{
    EXPECT_EQ(parse<uint8_t>("0"), 0);
    EXPECT_EQ(parse<uint8_t>("1"), 1);
    EXPECT_EQ(parse<uint8_t>("075"), 75);
    EXPECT_EQ(parse<uint8_t>("32"), 32);
    EXPECT_EQ(parse<uint8_t>("255"), std::numeric_limits<uint8_t>::max());
    EXPECT_THROW(parse<uint8_t>(""), std::invalid_argument);
    EXPECT_THROW(parse<uint8_t>("abc"), std::invalid_argument);
    EXPECT_THROW(parse<uint8_t>("-1"), std::invalid_argument);
    EXPECT_THROW(parse<uint8_t>("256"), std::range_error);
    EXPECT_THROW(parse<uint8_t>(" 1"), std::invalid_argument);
    EXPECT_THROW(parse<uint8_t>("1 "), std::invalid_argument);
    EXPECT_THROW(parse<uint8_t>(" 1 "), std::invalid_argument);
    EXPECT_THROW(parse<uint8_t>("1 ns"), std::invalid_argument);

    EXPECT_EQ(parse<uint64_t>("0"), 0);
    EXPECT_EQ(parse<uint64_t>("1"), 1);
    EXPECT_EQ(parse<uint64_t>("0004827545"), 4827545);
    EXPECT_EQ(parse<uint64_t>("56466545687132"), 56466545687132);
    EXPECT_EQ(parse<uint64_t>("18446744073709551615"), std::numeric_limits<uint64_t>::max());
    EXPECT_THROW(parse<uint64_t>(""), std::invalid_argument);
    EXPECT_THROW(parse<uint64_t>("abc"), std::invalid_argument);
    EXPECT_THROW(parse<uint64_t>("-1"), std::invalid_argument);
    EXPECT_THROW(parse<uint64_t>("18446744073709551616"), std::range_error);
    EXPECT_THROW(parse<uint64_t>(" 1"), std::invalid_argument);
    EXPECT_THROW(parse<uint64_t>("1 "), std::invalid_argument);
    EXPECT_THROW(parse<uint64_t>(" 1 "), std::invalid_argument);
    EXPECT_THROW(parse<uint64_t>("1 ns"), std::invalid_argument);
}

TEST(AmonUtilString, ParseIntegerSignedBase10)
{
    EXPECT_EQ(parse<int8_t>("0"), 0);
    EXPECT_EQ(parse<int8_t>("-1"), -1);
    EXPECT_EQ(parse<int8_t>("1"), 1);
    EXPECT_EQ(parse<int8_t>("-56"), -56);
    EXPECT_EQ(parse<int8_t>("32"), 32);
    EXPECT_EQ(parse<int8_t>("075"), 75);
    EXPECT_EQ(parse<int8_t>("-128"), std::numeric_limits<int8_t>::min());
    EXPECT_EQ(parse<int8_t>("127"), std::numeric_limits<int8_t>::max());
    EXPECT_THROW(parse<int8_t>(""), std::invalid_argument);
    EXPECT_THROW(parse<int8_t>("abc"), std::invalid_argument);
    EXPECT_THROW(parse<int8_t>("-129"), std::range_error);
    EXPECT_THROW(parse<int8_t>("128"), std::range_error);
    EXPECT_THROW(parse<int8_t>(" 1"), std::invalid_argument);
    EXPECT_THROW(parse<int8_t>("1 "), std::invalid_argument);
    EXPECT_THROW(parse<int8_t>(" 1 "), std::invalid_argument);
    EXPECT_THROW(parse<int8_t>("1 ns"), std::invalid_argument);

    EXPECT_EQ(parse<int64_t>("0"), 0);
    EXPECT_EQ(parse<int64_t>("-1"), -1);
    EXPECT_EQ(parse<int64_t>("1"), 1);
    EXPECT_EQ(parse<int64_t>("-9871654618710"), -9871654618710);
    EXPECT_EQ(parse<int64_t>("56466545687132"), 56466545687132);
    EXPECT_EQ(parse<int64_t>("-0004827545"), -4827545);
    EXPECT_EQ(parse<int64_t>("-9223372036854775808"), std::numeric_limits<int64_t>::min());
    EXPECT_EQ(parse<int64_t>("9223372036854775807"), std::numeric_limits<int64_t>::max());
    EXPECT_THROW(parse<int64_t>(""), std::invalid_argument);
    EXPECT_THROW(parse<int64_t>("abc"), std::invalid_argument);
    EXPECT_THROW(parse<int64_t>("-9223372036854775809"), std::range_error);
    EXPECT_THROW(parse<int64_t>("9223372036854775808"), std::range_error);
    EXPECT_THROW(parse<int64_t>(" 1"), std::invalid_argument);
    EXPECT_THROW(parse<int64_t>("1 "), std::invalid_argument);
    EXPECT_THROW(parse<int64_t>(" 1 "), std::invalid_argument);
    EXPECT_THROW(parse<int64_t>("1 ns"), std::invalid_argument);
}

TEST(AmonUtilString, ParseIntegerBase2)
{
    EXPECT_EQ(parse<uint8_t>("0b0"), 0);
    EXPECT_EQ(parse<uint8_t>("0b1"), 1);
    EXPECT_EQ(parse<uint8_t>("0b100"), 4);
    EXPECT_EQ(parse<uint8_t>("0b01001111"), 79);
    EXPECT_EQ(parse<uint8_t>("0b11111111"), std::numeric_limits<uint8_t>::max());
    EXPECT_THROW(parse<uint8_t>(""), std::invalid_argument);
    EXPECT_THROW(parse<uint8_t>("0b100000000"), std::range_error);
    EXPECT_THROW(parse<uint8_t>("0b"), std::invalid_argument);
    EXPECT_THROW(parse<uint8_t>("0b12"), std::invalid_argument);
    EXPECT_THROW(parse<uint8_t>("0b001c1000"), std::invalid_argument);
    EXPECT_THROW(parse<uint8_t>(" 0b1100"), std::invalid_argument);
    EXPECT_THROW(parse<uint8_t>("0b0011 "), std::invalid_argument);

    EXPECT_EQ(
        parse<uint64_t>("0b0000000000000000000000000000000000000000000000000000000000000000"),
        0);
    EXPECT_EQ(
        parse<uint64_t>("0b1111111111111111111111111111111111111111111111111111111111111111"),
        std::numeric_limits<uint64_t>::max());

    // Unsupported conversion with negative values
    EXPECT_THROW(parse<int8_t>("-0b1"), std::invalid_argument);
    EXPECT_THROW(parse<uint8_t>("-0b1"), std::invalid_argument);
    EXPECT_THROW(parse<int64_t>("-0b177"), std::invalid_argument);
    EXPECT_THROW(parse<uint64_t>("-0b177"), std::invalid_argument);
    EXPECT_THROW(parse<int8_t>("0b-0011"), std::invalid_argument);
    EXPECT_THROW(parse<int8_t>("0b+0011"), std::invalid_argument);
}

TEST(AmonUtilString, ParseIntegerBase16)
{
    EXPECT_EQ(parse<uint8_t>("0x0"), 0);
    EXPECT_EQ(parse<uint8_t>("0x1"), 1);
    EXPECT_EQ(parse<uint8_t>("0x0A"), 0x0A);
    EXPECT_EQ(parse<uint8_t>("0x1a"), 0x1A);
    EXPECT_EQ(parse<uint8_t>("0x1A"), 0x1A);
    EXPECT_EQ(parse<uint8_t>("0xFF"), std::numeric_limits<uint8_t>::max());
    EXPECT_THROW(parse<uint8_t>("0x100"), std::range_error);
    EXPECT_THROW(parse<uint8_t>("0x"), std::invalid_argument);
    EXPECT_THROW(parse<uint8_t>("0x1g"), std::invalid_argument);
    EXPECT_THROW(parse<uint8_t>(" 0xff"), std::invalid_argument);
    EXPECT_THROW(parse<uint8_t>("0xff "), std::invalid_argument);
    EXPECT_THROW(parse<uint8_t>("0XFF"), std::invalid_argument);

    EXPECT_EQ(parse<uint64_t>("0x0"), 0);
    EXPECT_EQ(parse<uint64_t>("0x1"), 1);
    EXPECT_EQ(parse<uint64_t>("0x00BBCCDD"), 0x00BBCCDD);
    EXPECT_EQ(parse<uint64_t>("0xA5A5A5A5A5A5A5A5"), 0xA5A5A5A5A5A5A5A5ULL);
    EXPECT_EQ(parse<uint64_t>("0xFFFFFFFFFFFFFFFF"), std::numeric_limits<uint64_t>::max());
    EXPECT_THROW(parse<uint64_t>(""), std::invalid_argument);
    EXPECT_THROW(parse<uint64_t>("0x10000000000000000"), std::range_error);

    // Unsupported conversion with negative values
    EXPECT_THROW(parse<int8_t>("-0x1"), std::invalid_argument);
    EXPECT_THROW(parse<uint8_t>("-0x1"), std::invalid_argument);
    EXPECT_THROW(parse<int64_t>("-0x177"), std::invalid_argument);
    EXPECT_THROW(parse<uint64_t>("-0x177"), std::invalid_argument);
    EXPECT_THROW(parse<int8_t>("0x-1"), std::invalid_argument);
}

TEST(AmonUtilString, ParseFloat)
{
    const auto approx = [](float expected) { return std::max(1.0F, std::fabs(expected)) * 1e-12F; };

    const std::string fmin = std::to_string(std::numeric_limits<float>::min());
    const std::string fmax = std::to_string(std::numeric_limits<float>::max());
    const std::string fden = std::to_string(std::numeric_limits<float>::denorm_min());

    EXPECT_FLOAT_EQ(parse<float>("0"), 0.0F);
    EXPECT_FLOAT_EQ(parse<float>("-1.5"), -1.5F);
    EXPECT_FLOAT_EQ(parse<float>("2.75"), 2.75F);
    EXPECT_FLOAT_EQ(parse<float>("1e3"), 1000.0F);
    EXPECT_FLOAT_EQ(parse<float>("-1.003e-12"), -1.003e-12F);

    EXPECT_NEAR(
        parse<float>(fmin),
        std::numeric_limits<float>::min(),
        approx(std::numeric_limits<float>::min()));
    EXPECT_NEAR(
        parse<float>(fmax),
        std::numeric_limits<float>::max(),
        approx(std::numeric_limits<float>::max()));
    EXPECT_NEAR(
        parse<float>(fden),
        std::numeric_limits<float>::denorm_min(),
        approx(std::numeric_limits<float>::denorm_min()));

    EXPECT_TRUE(std::isnan(parse<float>("nan")));
    EXPECT_TRUE(std::isnan(parse<float>("NaN")));
    EXPECT_TRUE(std::isinf(parse<float>("inf")));
    EXPECT_TRUE(std::isinf(parse<float>("INF")));
    EXPECT_TRUE(std::isinf(parse<float>("inf")));
    EXPECT_TRUE(std::isinf(parse<float>("-inf")));

    EXPECT_THROW(parse<float>("1e39"), std::range_error);
    EXPECT_THROW(parse<float>("1e-46"), std::range_error);
    EXPECT_THROW(parse<float>(""), std::invalid_argument);
    EXPECT_THROW(parse<float>(" 1.0"), std::invalid_argument);
    EXPECT_THROW(parse<float>("1.0 "), std::invalid_argument);
    EXPECT_THROW(parse<float>("1.0abc"), std::invalid_argument);
    EXPECT_THROW(parse<float>("abc"), std::invalid_argument);
    EXPECT_THROW(parse<float>("inf "), std::invalid_argument);
    EXPECT_THROW(parse<float>(" nan"), std::invalid_argument);
}

TEST(AmonUtilString, ParseDouble)
{
    const auto approx = [](double expected) { return std::max(1.0, std::fabs(expected)) * 1e-12; };

    const std::string dmin = std::to_string(std::numeric_limits<double>::min());
    const std::string dmax = std::to_string(std::numeric_limits<double>::max());
    const std::string dden = std::to_string(std::numeric_limits<double>::denorm_min());

    EXPECT_DOUBLE_EQ(parse<double>("0"), 0.0);
    EXPECT_DOUBLE_EQ(parse<double>("-1.5"), -1.5);
    EXPECT_DOUBLE_EQ(parse<double>("2.75"), 2.75);
    EXPECT_DOUBLE_EQ(parse<double>("1e3"), 1000.0);
    EXPECT_DOUBLE_EQ(parse<double>("-1.003e-12"), -1.003e-12);

    EXPECT_NEAR(
        parse<double>(dmin),
        std::numeric_limits<double>::min(),
        approx(std::numeric_limits<double>::min()));
    EXPECT_NEAR(
        parse<double>(dmax),
        std::numeric_limits<double>::max(),
        approx(std::numeric_limits<double>::max()));
    EXPECT_NEAR(
        parse<double>(dden),
        std::numeric_limits<double>::denorm_min(),
        approx(std::numeric_limits<double>::denorm_min()));

    EXPECT_TRUE(std::isnan(parse<double>("nan")));
    EXPECT_TRUE(std::isnan(parse<double>("NaN")));
    EXPECT_TRUE(std::isinf(parse<double>("inf")));
    EXPECT_TRUE(std::isinf(parse<double>("INF")));
    EXPECT_TRUE(std::isinf(parse<double>("inf")));
    EXPECT_TRUE(std::isinf(parse<double>("-inf")));

    EXPECT_THROW(parse<double>("1e309"), std::range_error);
    EXPECT_THROW(parse<double>("1e-324"), std::range_error);
    EXPECT_THROW(parse<double>(""), std::invalid_argument);
    EXPECT_THROW(parse<double>(" 1.0"), std::invalid_argument);
    EXPECT_THROW(parse<double>("1.0 "), std::invalid_argument);
    EXPECT_THROW(parse<double>("1.0abc"), std::invalid_argument);
    EXPECT_THROW(parse<double>("abc"), std::invalid_argument);
    EXPECT_THROW(parse<double>("inf "), std::invalid_argument);
    EXPECT_THROW(parse<double>(" nan"), std::invalid_argument);
}

#endif

} // namespace amon::util::string
