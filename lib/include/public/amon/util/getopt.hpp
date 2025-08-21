/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief A sane wrapper around getopt_long
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/Error.hpp>

#include <format>
#include <optional>
#include <string>
#include <vector>

namespace amon::util::getopt {

/**
 * @brief Specifies whether an option takes an argument.
 */
enum class ArgReq {
    NO_ARG, ///< Option does not take an argument.
    REQUIRED_ARG, ///< Option requires an argument.
    OPTIONAL_ARG, ///< Option may take an optional argument.
};

static constexpr const ArgReq NO_ARG = ArgReq::NO_ARG; ///< Alias of ArgReq::NO_ARG
static constexpr const ArgReq REQUIRED_ARG
    = ArgReq::REQUIRED_ARG; ///< Alias of ArgReq::REQUIRED_ARG
static constexpr const ArgReq OPTIONAL_ARG
    = ArgReq::OPTIONAL_ARG; ///< Alias of ArgReq::OPTIONAL_ARG

/**
 * @brief Describes a command-line option.
 */
struct Opt {
    std::optional<char> shortOpt; ///< Short option character (e.g., 'h' for -h).
    std::optional<std::string> longOpt; ///< Long option string (e.g., "help" for --help).
    ArgReq hasArg; ///< Argument requirement for the option.
};

/**
 * @brief Holds parsed command-line arguments.
 */
struct Result {
    std::vector<std::pair<Opt, std::optional<std::string>>>
        kwArgs; ///< Parsed options and their arguments.
    std::vector<std::string> posArgs; ///< Positional arguments.
};

/**
 * @brief Represents an unknown command-line option.
 */
class UnknownOpt : public amon::Error {
public:
    /**
     * @brief Exception thrown when an unknown option is encountered.
     * @param opt The unknown option string.
     */
    UnknownOpt(const std::string& opt)
        : amon::Error("unknown option \"{}\"", opt)
        , m_opt(opt)
    {
    }

    /**
     * @brief Get the unknown option string.
     * @return The option string.
     */
    const std::string& opt() const { return m_opt; }

private:
    std::string m_opt;
};

/**
 * @brief Represents a missing argument for an option.
 */
class MissingOptArg : public amon::Error {
public:
    /**
     * @brief Exception thrown when a required argument for an option is missing.
     * @param opt The option for which the argument is missing.
     */
    MissingOptArg(const getopt::Opt& opt)
        : amon::Error(
              "missing argument for option \"{}\"",
              (opt.shortOpt && opt.longOpt) ? std::format("-{}, --{}", *opt.shortOpt, *opt.longOpt)
                  : (opt.shortOpt)          ? std::format("-{}", *opt.shortOpt)
                  : (opt.longOpt)           ? std::format("--{}", *opt.longOpt)
                                            : "")
        , m_opt(opt)
    {
    }

    /**
     * @brief Get the option missing its required argument.
     * @return The opt object.
     */
    const Opt& opt() const { return m_opt; }

private:
    Opt m_opt;
};

/**
 * @brief Parses command-line arguments according to the provided options.
 *
 * Example usage:
 * @code{.cpp}
 * auto parsedArgs = amon::util::getopt::getOpt(
 *    {
 *      {.shortOpt = 'i', .longOpt = "input", .hasArg = amon::util::getopt::REQUIRED_ARG},
 *      {.shortOpt = 'o', .longOpt = "output", .hasArg = amon::util::getopt::REQUIRED_ARG},
 *      {.shortOpt = 'h', .longOpt = "help", .hasArg = amon::util::getopt::NO_ARG},
 *    },
 *    argc,
 *    argv);
 *
 * for (const auto& arg : parsedArgs.kwArgs) {
 *     if (arg.second) {
 *         std::cout <<
 *             "Keyword argument " << *arg.first.longOpt << " is: " << *arg.second << "\n";
 *     } else {
 *         std::cout <<
 *             "Keyword argument " << *arg.first.longOpt << " is missing" << "\n";
 *     }
 * }
 *
 * size_t i = 0;
 * for (const auto& arg : parsedArgs.posArgs) {
 *     std::cout << "Positional argument " << i << " is: " << arg << "\n";
 *     i++;
 * }
 * @endcode
 *
 * @param opts List of supported options.
 * @param argc Argument count.
 * @param argv Argument array.
 * @return ParsedArgs containing options and positional arguments.
 * @throw UnknownOpt if an option not specified in opts is encountered
 * @throw MissingOptArg if the option requires an argument but it is not present
 * @warning The function uses an internal mutex because we use `getopt_long`, which is not
 * multithread safe.
 */
Result getOpt(const std::vector<Opt>& opts, int argc, const char* const* argv);

} // namespace amon::util::getopt
