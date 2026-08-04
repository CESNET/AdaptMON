/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Argument parser utility for command-line interfaces.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Provides classes and functions for defining, parsing, and handling
 * positional and keyword arguments, including automatic help generation.
 *
 * Example use:
 * @code{.cpp}
 * static const auto argParser = ArgParser(
 *     {
 *         PosArg("INPUT_FILE")
 *             .required(true)
 *             .help("The input file")
 *     },
 *     {
 *         KwArg('o', "output")
 *             .required(true)
 *             .metaVar("FILE")
 *             .help("The output file"),
 *         KwArg('k', "key")
 *             .metaVar("KEY")
 *             .defaultValue("ABCDEFGHIJKLMNOPQRSTUVWXYZ012345")
 *             .help("The anonymization key, 32 characters long"),
 *         KwArg('p', "port")
 *             .metaVar("PORT")
 *             .defaultValue("4242")
 *             .help("The listen port"),
 *             .constraint(ArgConstraint::number<uint16_t>())
 *         KwArg('n', "number-of-iterations")
 *             .metaVar("N")
 *             .defaultValue("1000")
 *             .help("The number of iterations"),
 *             .constraint(ArgConstraint::numberInRange<unsigned>(1, 10000))
 *         KwArg('h', "help")
 *             .required(false)
 *             .help("Print this help")
 *             .special(true),
 *     });
 *
 * auto args = argParser.parse(argc, argv);
 * std::string output = *args["--output"];
 * std::string key = *args["--key"];
 * uint16_t port = args.get<uint16_t>("--port");
 * unsigned nIterations = args.get<unsigned>("-n");
 * // ...
 * @endcode
 */

#pragma once

#include <amon/Error.hpp>
#include <amon/util/getopt.hpp>
#include <amon/util/string.hpp>

#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace amon::util {

/**
 * @brief Exception type for argument parsing errors.
 */
class ArgParserError : public Error {
public:
    using Error::Error;
};

/**
 * @brief Represents a constraint that can be applied to argument values during parsing.
 *
 * Constraints are used to validate that values provided for an argument meet certain
 * criteria, such as being within a specified numeric range.
 */
class ArgConstraint {
public:
    /**
     * @brief Create a numeric constraint for a specific integral type.
     *
     * The constraint validates that the argument value, when parsed, can be
     * successfully converted to the specified integral type T.
     *
     * @tparam T The integral type to validate against.
     * @return An ArgConstraint that validates numeric values of type T.
     */
    template <typename T>
        requires(std::integral<T> && !std::is_same_v<T, bool>)
    static ArgConstraint number()
    {
        return {std::make_shared<ArgConstraint::Number<T>>()};
    }

    /**
     * @brief Create an integer range constraint.
     *
     * The constraint validates that the argument value, when parsed as an integer,
     * falls within the inclusive range [min, max].
     *
     * @tparam T The integral type used.
     * @param min The minimum allowed value (inclusive).
     * @param max The maximum allowed value (inclusive).
     * @return An ArgConstraint that validates integer values within the range.
     */
    template <typename T = int64_t>
        requires(std::integral<T> && !std::is_same_v<T, bool>)
    static ArgConstraint numberInRange(T min, T max)
    {
        return {std::make_shared<ArgConstraint::NumberInRange<T>>(min, max)};
    }

private:
    friend class ArgParser;

    class ConstraintImpl {
    public:
        virtual void validate(const std::string& value) const = 0;
        virtual std::string helpStr() const = 0;
    };

    template <typename T>
    class Number : public ConstraintImpl {
    public:
        void validate(const std::string& value) const override { string::parse<T>(value); }
        std::string helpStr() const override { return "number"; }
    };

    template <typename T>
    struct NumberInRange : public ConstraintImpl {
    public:
        NumberInRange(T low, T high)
            : m_low(low)
            , m_high(high)
        {
            if (low > high) {
                throw std::invalid_argument {
                    "invalid ArgConstraint range: lower bound must be less or equal higher bound"};
            }
        }
        void validate(const std::string& value) const override
        {
            T num;
            try {
                num = string::parse<T>(value);
            } catch (const std::exception& e) {
                throw std::runtime_error("not a valid number");
            }

            if (num < m_low || num > m_high) {
                throw std::runtime_error(std::format("not a number in range {}-{}", m_low, m_high));
            }
        }
        std::string helpStr() const override { return std::format("number {}-{}", m_low, m_high); }

    private:
        T m_low;
        T m_high;
    };

    ArgConstraint(std::shared_ptr<ConstraintImpl> impl)
        : m_impl(std::move(impl))
    {
    }
    std::shared_ptr<ConstraintImpl> m_impl;
    void validate(const std::string& value) const { m_impl->validate(value); }
    std::string helpStr() const { return m_impl->helpStr(); }
};

/**
 * @brief Represents a positional argument for the argument parser.
 *
 * By default the parameter is required and does not allow to be specified multiple times.
 */
class PosArg {
public:
    /**
     * @brief Construct a positional argument with a name.
     * @param name The name of the positional argument.
     */
    PosArg(std::string name)
        : m_name(std::move(name))
    {
        if (m_name.starts_with('-')) {
            throw amon::Error("positional argument name cannot start with \"-\"");
        }
    }

    /**
     * @brief Set whether the positional argument is required.
     * @param value True if required, false otherwise.
     * @return Reference to this PosArg.
     */
    PosArg& required(bool value = true)
    {
        m_required = value;
        return *this;
    }

    /**
     * @brief Sets whether this positional argument can be specified multiple times.
     * @param value If true, allows multiple occurences for this argument.
     * @return Reference to this PosArg for chaining.
     */
    PosArg& allowMultiple(bool value = true)
    {
        m_allowMultiple = value;
        return *this;
    }

    /**
     * @brief Set the default value for the positional argument.
     * Marks the argument as not required.
     * @param value The default value.
     * @return Reference to this PosArg.
     */
    PosArg& defaultValue(std::string value)
    {
        m_defaultValue = std::move(value);
        m_required = false;
        return *this;
    }

    /**
     * @brief Set the help text for the positional argument.
     * @param value The help text.
     * @return Reference to this PosArg.
     */
    PosArg& help(std::string value)
    {
        m_help = std::move(value);
        return *this;
    }

    /**
     * @brief Add a constraint to validate the argument value.
     *
     * The constraint is applied to the value provided for this argument during parsing,
     * ensuring it meets the specified criteria (e.g. within a numeric range).
     *
     * @param constaint The constraint to apply.
     * @return Reference to this PosArg for chaining.
     */
    PosArg& constraint(const ArgConstraint& constaint)
    {
        m_constraints.push_back(constaint);
        return *this;
    }

private:
    friend class ArgParser;
    friend class ParsedArgs;

    std::string m_name;
    std::optional<std::string> m_defaultValue = std::nullopt;
    bool m_required = true;
    bool m_allowMultiple = false;
    std::string m_help;
    std::vector<ArgConstraint> m_constraints;
};

/**
 * @brief Represents a keyword (optional or required) argument for the argument parser.
 *
 * By default, the argument is optional (with the default value being an empty string), not allowed
 * to be specified multiple times, and does not have an argument.
 */
class KwArg {
public:
    /**
     * @brief Construct a keyword argument with short and long options.
     * @param shortOpt The short option character.
     * @param longOpt The long option string.
     */
    KwArg(char shortOpt, std::string longOpt)
        : m_shortOpt(shortOpt)
        , m_longOpt(std::move(longOpt))
    {
    }

    /**
     * @brief Construct a keyword argument with only a short option.
     * @param shortOpt The short option character.
     */
    KwArg(char shortOpt)
        : m_shortOpt(shortOpt)
    {
    }

    /**
     * @brief Construct a keyword argument with only a long option.
     * @param longOpt The long option string.
     */
    KwArg(const std::string& longOpt)
        : m_longOpt(longOpt)
    {
    }

    /**
     * @brief Set whether the keyword argument is required.
     * @param value True if required, false otherwise.
     * @return Reference to this KwArg.
     */
    KwArg& required(bool value = true)
    {
        m_required = value;
        return *this;
    }

    /**
     * @brief Sets whether this positional argument can be specified multiple times.
     * @param value If true, allows multiple occurences for this argument.
     * @return Reference to this PosArg for chaining.
     */
    KwArg& allowMultiple(bool value = true)
    {
        m_allowMultiple = value;
        return *this;
    }

    /**
     * @brief Set whether the keyword argument takes a value.
     * @param value True if argument takes a value, false otherwise.
     * @return Reference to this KwArg.
     */
    KwArg& hasArg(bool value = true)
    {
        m_hasArg = value;
        return *this;
    }

    /**
     * @brief Set the meta variable name for the argument value.
     * Implies that the argument takes a value.
     * @param value The meta variable name.
     * @return Reference to this KwArg.
     */
    KwArg& metaVar(std::string value)
    {
        m_metaVar = std::move(value);
        m_hasArg = true;
        return *this;
    }

    /**
     * @brief Set the default value for the keyword argument.
     * Marks the argument as not required and as taking a value.
     * @param value The default value.
     * @return Reference to this KwArg.
     */
    KwArg& defaultValue(std::string value)
    {
        m_defaultValue = std::move(value);
        m_required = false;
        m_hasArg = true;
        return *this;
    }

    /**
     * @brief Set the help text for the keyword argument.
     * @param value The help text.
     * @return Reference to this KwArg.
     */
    KwArg& help(std::string value)
    {
        m_help = std::move(value);
        return *this;
    }

    /**
     * @brief Set the special flag for the keyword argument.
     *
     * When an option with this flag set is encountered in parsing, parsing does not end with an
     * error even if an error condition (e.g. missing required option) happens. This is intended for
     * special flags like --version or --help that should take priority.
     *
     * @param value The value.
     * @return Reference to this KwArg.
     */
    KwArg& special(bool value)
    {
        m_special = value;
        return *this;
    }

    /**
     * @brief Add a constraint to validate the argument value.
     *
     * The constraint is applied to the value provided for this argument during parsing,
     * ensuring it meets the specified criteria (e.g. within a numeric range).
     *
     * @param constaint The constraint to apply.
     * @return Reference to this KwArg for chaining.
     */
    KwArg& constraint(const ArgConstraint& constaint)
    {
        m_constraints.push_back(constaint);
        return *this;
    }

private:
    friend class ArgParser;
    friend class ParsedArgs;

    std::optional<char> m_shortOpt = std::nullopt;
    std::optional<std::string> m_longOpt = std::nullopt;
    bool m_required = false;
    bool m_hasArg = false;
    bool m_allowMultiple = false;
    bool m_special = false;
    std::optional<std::string> m_metaVar = std::nullopt;
    std::optional<std::string> m_defaultValue = std::nullopt;
    std::string m_help;
    std::vector<ArgConstraint> m_constraints;
};

/**
 * @brief Holds the parsed arguments after parsing.
 */
class ParsedArgs {
public:
    /**
     * @brief Get the value of an option.
     * @param option The option to search for.
     * If looking for shortopt, prefix the character by "-", e.g. "-e".
     * If looking for longopt, prefix the name by "--", e.g. "--example".
     * If looking for posarg, provide just its name, e.g. "EXAMPLE".
     * @return String containing the value option argument provided.
     * In case the option takes no argument, an empty string is returned.
     * If the option was not present, std::nullopt is returned.
     * If the option was present multiple times, the last occurence is returned.
     */
    std::optional<std::string> operator[](const std::string& option) const;

    /**
     * @brief Get the value of an option as a specific type.
     * @tparam ResultType The type to convert to.
     * @param option The option to search for.
     * @return Value containing the value option argument provided as the specified type.
     * @throw std::out_of_range if the option was not present.
     * @throw std::invalid_argument if the conversion failed due to parsing.
     * @throw std::range_error if the parsed value exceeds the representable range.
     *
     * See @ref amon::util::ParsedArgs::operator[] for details.
     */
    template <typename ResultType>
    ResultType get(const std::string& option) const;

    /**
     * @brief Get the value of an option as a specific type.
     * @tparam ResultType The type to convert to.
     * @param option The option to search for.
     * @param[out] result Value containing the value option argument provided as the specified type.
     * @throw std::out_of_range if the option was not present.
     * @throw std::invalid_argument if the conversion failed due to parsing.
     * @throw std::range_error if the parsed value exceeds the representable range.
     *
     * See @ref amon::util::ParsedArgs::operator[] for details.
     */
    template <typename ResultType>
    void getInto(const std::string& option, ResultType& result) const;

    /**
     * @brief Counts how many times the specified option was provided.
     * @param option The option to search for.
     * @return The count of occurrences of the option.
     * @see ParsedArgs::operator[] for detailed description.
     */
    size_t count(const std::string& option) const;

    /**
     * @brief Get the value of an option.
     * @param option The option to search for
     * @return Vector containing the values option argument provided.
     * @see ParsedArgs::operator[] for detailed description.
     */
    std::vector<std::string> getAll(const std::string& option) const;

private:
    friend class ArgParser;

    std::vector<std::pair<KwArg, std::optional<std::string>>> m_kwArgs;
    std::vector<std::pair<PosArg, std::string>> m_posArgs;
};

namespace detail {
template <typename T>
concept HasStringParseFn = requires(const std::string& value) {
    { amon::util::string::parse<T>(value) } -> std::same_as<T>;
};
} // namespace detail

template <typename ResultType>
ResultType ParsedArgs::get(const std::string& option) const
{
    auto value = (*this)[option];
    if (!value) {
        throw std::out_of_range {std::format("option \"{}\" not found", option)};
    }

    if constexpr (std::is_same_v<ResultType, std::string>) {
        return *value;

    } else if constexpr (detail::HasStringParseFn<ResultType>) {
        return string::parse<ResultType>(*value);

    } else {
        static_assert(false, "unsupported type");
    }
}

template <typename ResultType>
void ParsedArgs::getInto(const std::string& option, ResultType& result) const
{
    result = get<ResultType>(option);
}

/**
 * @brief Argument parser for command-line interfaces.
 */
class ArgParser {
public:
    /**
     * @brief Construct an argument parser with positional and keyword arguments.
     * @param posArgs List of positional arguments.
     * @param kwArgs List of keyword arguments.
     */
    ArgParser(std::initializer_list<PosArg> posArgs, std::initializer_list<KwArg> kwArgs);

    /**
     * @brief Construct an argument parser with only positional arguments.
     * @param posArgs List of positional arguments.
     */
    ArgParser(std::initializer_list<PosArg> posArgs);

    /**
     * @brief Construct an argument parser with only keyword arguments.
     * @param kwArgs List of keyword arguments.
     */
    ArgParser(std::initializer_list<KwArg> kwArgs);

    /**
     * @brief Enable or disable automatic help argument.
     * @param value True to enable, false to disable.
     * @return Reference to this ArgParser.
     *
     * For description of the "auto help" functionality see @ref ArgParser::parse()
     */
    ArgParser& setAutoHelp(bool value)
    {
        m_autoHelp = value;
        return *this;
    }

    /**
     * @brief Sets the prologue text to display before the usage/help message.
     * @param prologue The prologue string.
     * @return Reference to this ArgParser for chaining.
     */
    ArgParser& setPrologue(std::string prologue)
    {
        m_prologue = std::move(prologue);
        return *this;
    }

    /**
     * @brief Sets the epilogue text to display after the usage/help message.
     * @param epilogue The epilogue string.
     * @return Reference to this ArgParser for chaining.
     */
    ArgParser& setEpilogue(std::string epilogue)
    {
        m_epilogue = std::move(epilogue);
        return *this;
    }

    /**
     * @brief Sets the version string for the argument parser and enables "auto-version".
     *
     * @param version The version string to set.
     * @return Reference to this ArgParser instance.
     */
    ArgParser& setVersion(std::string version)
    {
        m_version = std::move(version);
        return *this;
    }

    /**
     * @brief Parse command-line arguments.
     *
     * When auto-help is enabled (default):
     *   - If '-h' or '--help' is provided, prints usage text and exits with status 0
     *   - On parsing errors, prints the error + usage and exits with status 1
     *   - Adds implicit help option if not explicitly defined
     *
     * When auto-help is disabled:
     *   - Help options become regular arguments
     *   - Parsing errors throw ArgParserError instead of exiting
     *
     * When version is set:
     *   - Options '-V' and '--version' are automatically added if not present.
     *   - If such option is provided, it is automatically handled by printing the version and
     *     exiting with status 0
     *
     * @param argc Argument count from main()
     * @param argv Argument vector from main()
     * @return ParsedArgs container with validated arguments
     * @throws ArgParserError Only when auto-help is disabled and parsing fails
     */
    ParsedArgs parse(int argc, const char* const* argv) const;

    /**
     * @brief Generate usage/help text for the program.
     * @param programName Name of the program.
     * @return Usage string.
     */
    std::string usage(const std::string& programName) const;

    /**
     * @brief Returns the version string.
     *
     * If the version is not set, returns an empty string.
     *
     * @return std::string The version string or empty string if not set.
     */
    std::string version() const { return m_version.value_or(""); }

private:
    std::vector<PosArg> m_posArgs;
    std::vector<KwArg> m_kwArgs;
    bool m_autoHelp = true;
    std::optional<std::string> m_prologue;
    std::optional<std::string> m_epilogue;
    std::optional<std::string> m_version;

    std::vector<KwArg> getKwArgsWithAutoArgs() const;

    static ParsedArgs parseWithGetOpt(
        int argc,
        const char* const* argv,
        const std::vector<KwArg>& kwArgs,
        const std::vector<PosArg>& posArgs);

    static ParsedArgs doParse(
        int argc,
        const char* const* argv,
        const std::vector<KwArg>& kwArgs,
        const std::vector<PosArg>& posArgs);

    static std::string optsStr(const KwArg& kwArg);
    static std::string constraintsStr(const std::vector<ArgConstraint>& constraints);
    static void checkConstraints(const PosArg& posArg, const std::string& value);
    static void checkConstraints(const KwArg& kwArg, const std::string& value);
};

} // namespace amon::util
