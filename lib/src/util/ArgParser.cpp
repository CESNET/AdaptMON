/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Argument parser utility for command-line interfaces.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/Test.hpp>
#include <amon/util/ArgParser.hpp>

#include <cassert>
#include <format>
#include <iostream>

namespace amon::util {

std::vector<std::string> ParsedArgs::getAll(const std::string& option) const
{
    std::vector<std::string> res;

    if (option.starts_with("--")) {
        const auto& longOpt = option.substr(2);
        for (const auto& [kwArg, value] : m_kwArgs) {
            if (kwArg.m_longOpt == longOpt) {
                res.emplace_back(value.value_or(""));
            }
        }

    } else if (option.starts_with('-') && option.size() == 2) {
        const char shortOpt = option[1];
        for (const auto& [kwArg, value] : m_kwArgs) {
            if (kwArg.m_shortOpt == shortOpt) {
                res.emplace_back(value.value_or(""));
            }
        }

    } else {
        for (const auto& [posArg, value] : m_posArgs) {
            if (posArg.m_name == option) {
                res.emplace_back(value);
            }
        }
    }

    return res;
}

std::optional<std::string> ParsedArgs::operator[](const std::string& option) const
{
    auto res = getAll(option);
    return !res.empty() ? std::optional<std::string> {res[res.size() - 1]} : std::nullopt;
}

size_t ParsedArgs::count(const std::string& option) const
{
    return getAll(option).size();
}

ArgParser::ArgParser(std::initializer_list<PosArg> posArgs, std::initializer_list<KwArg> kwArgs)
    : m_posArgs(posArgs)
    , m_kwArgs(kwArgs)
{
    bool seenOptional = false;
    size_t i = 0;

    for (auto& posArg : m_posArgs) {
        if (!posArg.m_required) {
            seenOptional = true;
        }

        if (posArg.m_required && seenOptional) {
            throw amon::Error("required positional arguments must preceed all optional arguments");
        }

        if (posArg.m_allowMultiple && i != m_posArgs.size() - 1) {
            throw amon::Error("positional argument accepting multiple values must appear last");
        }

        i++;
    }
}
ArgParser::ArgParser(std::initializer_list<PosArg> posArgs)
    : ArgParser(posArgs, {})
{
}
ArgParser::ArgParser(std::initializer_list<KwArg> kwArgs)
    : ArgParser({}, kwArgs)
{
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
std::string ArgParser::usage(const std::string& programName) const
{
    std::string buffer;
    auto ins = std::back_inserter(buffer);

    // Print the first line
    std::format_to(ins, "Usage: {}", programName);
    if (!m_kwArgs.empty()) {
        std::format_to(ins, " ARGS");
    }
    for (const auto& posArg : m_posArgs) {
        if (posArg.m_required) {
            std::format_to(ins, " <{}{}>", posArg.m_name, posArg.m_allowMultiple ? " ..." : "");
        } else {
            std::format_to(ins, " [{}{}]", posArg.m_name, posArg.m_allowMultiple ? " ..." : "");
        }
    }
    std::format_to(ins, "\n");

    // Print prologue
    if (m_prologue) {
        std::format_to(ins, "\n{}\n", *m_prologue);
    }

    // Print positional arguments
    if (!m_posArgs.empty()) {
        std::format_to(ins, "\n");
        std::format_to(ins, "Positional arguments:\n");
        for (const auto& posArg : m_posArgs) {
            std::string tmp;
            {
                auto ins = std::back_inserter(tmp);
                std::format_to(ins, "  {}", posArg.m_name);
            }
            std::format_to(ins, "{:<40}{}", tmp, posArg.m_help);
            if (!posArg.m_required) {
                std::format_to(ins, " (default: {})", posArg.m_defaultValue.value_or(""));
            }
            std::format_to(ins, "\n");
        }
    }

    // Print keyword arguments
    auto kwArgs = getKwArgsWithAutoArgs();
    if (!kwArgs.empty()) {
        std::format_to(ins, "\n");
        std::format_to(ins, "Keyword arguments (ARGS):\n");
        for (const auto& kwArg : kwArgs) {
            std::string tmp;
            {
                auto ins = std::back_inserter(tmp);
                std::format_to(ins, "  ");
                if (kwArg.m_shortOpt) {
                    std::format_to(ins, "-{}", *kwArg.m_shortOpt);
                }
                if (kwArg.m_longOpt) {
                    if (kwArg.m_shortOpt) {
                        std::format_to(ins, ", ");
                    }
                    std::format_to(ins, "--{}", *kwArg.m_longOpt);
                }
                if (kwArg.m_hasArg) {
                    std::format_to(ins, " {}", kwArg.m_metaVar.value_or("ARG"));
                }
            }

            std::format_to(ins, "{:<40}{}", tmp, kwArg.m_help);
            if (kwArg.m_required) {
                std::format_to(ins, " (required)");
            }
            if (kwArg.m_defaultValue) {
                std::format_to(ins, " (default: {})", *kwArg.m_defaultValue);
            }
            std::format_to(ins, "\n");
        }
    }

    if (m_epilogue) {
        std::format_to(ins, "\n{}\n", *m_epilogue);
    }

    return buffer;
}

ParsedArgs ArgParser::parseWithGetOpt(
    int argc,
    const char* const* argv,
    const std::vector<KwArg>& kwArgs,
    const std::vector<PosArg>& posArgs)
{
    ParsedArgs res;

    // Setup definition for getOpts
    std::vector<getopt::Opt> opts;
    opts.reserve(kwArgs.size());
    for (const auto& kwArg : kwArgs) {
        opts.push_back({
            .shortOpt = kwArg.m_shortOpt,
            .longOpt = kwArg.m_longOpt,
            .hasArg = kwArg.m_hasArg ? getopt::REQUIRED_ARG : getopt::NO_ARG,
        });
    }

    // Collect keyword arguments
    const getopt::Result parsedOpts = getopt::getOpt(opts, argc, argv);

    for (const auto& [opt, value] : parsedOpts.kwArgs) {
        const KwArg* matchingArg = nullptr;
        for (const KwArg& kwArg : kwArgs) {
            if (kwArg.m_shortOpt && kwArg.m_shortOpt == opt.shortOpt) {
                matchingArg = &kwArg;
                break;
            }
            if (kwArg.m_longOpt && kwArg.m_longOpt == opt.longOpt) {
                matchingArg = &kwArg;
                break;
            }
        }
        assert(matchingArg != nullptr);
        if (matchingArg == nullptr) {
            std::unreachable();
        }
        res.m_kwArgs.emplace_back(*matchingArg, value);
    }

    // Collect positional arguments
    size_t nPosArg = 0;

    for (const auto& value : parsedOpts.posArgs) {
        if (nPosArg >= posArgs.size()) {
            throw ArgParserError("unexpected positional argument {}: \"{}\"", nPosArg, value);
        }

        if (posArgs[nPosArg].m_allowMultiple) {
            assert(nPosArg == posArgs.size() - 1); // Must be last
            // Collect remaining args
            for (size_t i = nPosArg; i < parsedOpts.posArgs.size(); i++) {
                res.m_posArgs.emplace_back(posArgs[nPosArg], parsedOpts.posArgs[i]);
            }
            break;
        }

        res.m_posArgs.emplace_back(posArgs[nPosArg], value);
        nPosArg++;
    }

    return res;
}

std::string ArgParser::optsStr(const KwArg& kwArg)
{
    const auto& sOpt = kwArg.m_shortOpt;
    const auto& lOpt = kwArg.m_longOpt;
    return (sOpt && lOpt) ? std::format("-{}, --{}", *sOpt, *lOpt)
        : (sOpt)          ? std::format("-{}", *sOpt)
        : (lOpt)          ? std::format("--{}", *lOpt)
                          : "";
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
ParsedArgs ArgParser::doParse(
    int argc,
    const char* const* argv,
    const std::vector<KwArg>& kwArgs,
    const std::vector<PosArg>& posArgs)
{
    ParsedArgs res;

    try {
        res = parseWithGetOpt(argc, argv, kwArgs, posArgs);
    } catch (const getopt::MissingOptArg& e) {
        throw ArgParserError(e.what());
    } catch (const getopt::UnknownOpt& e) {
        throw ArgParserError(e.what());
    }

    // Check if there is any parameter marked "special" provided. If so, skip errors.
    const bool skipErrors
        = std::ranges::any_of(res.m_kwArgs, [](const auto& p) { return p.first.m_special; });

    // Check that all requires kw args were provided, provide default values for optional
    for (const auto& kwArg : kwArgs) {
        const auto count = std::ranges::count_if(res.m_kwArgs, [&](const auto& p) {
            return (kwArg.m_shortOpt && p.first.m_shortOpt == kwArg.m_shortOpt)
                || (kwArg.m_longOpt && p.first.m_longOpt == kwArg.m_longOpt);
        });
        if (count == 0 && kwArg.m_required) {
            if (skipErrors) {
                continue;
            }
            throw ArgParserError("missing required option \"{}\"", optsStr(kwArg));
        }
        if (count > 1 && !kwArg.m_allowMultiple) {
            if (skipErrors) {
                continue;
            }
            throw ArgParserError("option \"{}\" defined multiple times", optsStr(kwArg));
        }
        if (count == 0 && kwArg.m_defaultValue) {
            res.m_kwArgs.emplace_back(kwArg, *kwArg.m_defaultValue);
        }
    }

    // Check that all positional arguments were provided
    size_t nPosArg = 0;
    for (const auto& posArg : posArgs) {
        // Skip over defined arguments
        if (res.m_posArgs.size() > nPosArg) {
            nPosArg++;
            continue;
        }

        // Handle undefined arguments
        if (posArg.m_required) {
            if (skipErrors) {
                continue;
            }
            throw ArgParserError("missing required positional argument \"{}\"", posArg.m_name);
        }
        if (!posArg.m_required && posArg.m_defaultValue) {
            res.m_posArgs.emplace_back(posArg, *posArg.m_defaultValue);
        }
    }

    return res;
}

std::vector<KwArg> ArgParser::getKwArgsWithAutoArgs() const
{
    auto kwArgsCopy = m_kwArgs;

    if (m_autoHelp) {
        auto it = std::ranges::find_if(m_kwArgs, [](const auto& kwArg) {
            return kwArg.m_shortOpt == 'h' || kwArg.m_longOpt == "help";
        });
        if (it == m_kwArgs.end()) {
            kwArgsCopy.push_back(
                KwArg('h', "help").required(false).special(true).help("Print this help message"));
        } else {
            if (it->m_hasArg || (it->m_longOpt && it->m_longOpt != "help")) {
                throw amon::Error("auto-help must be disabled when -h is not a help option");
            }
        }
    }

    if (m_version) {
        auto it = std::ranges::find_if(m_kwArgs, [](const auto& kwArg) {
            return kwArg.m_shortOpt == 'V' || kwArg.m_longOpt == "version";
        });
        if (it == m_kwArgs.end()) {
            kwArgsCopy.push_back(
                KwArg('V', "version").required(false).special(true).help("Print the version"));
        } else {
            if (it->m_hasArg || (it->m_longOpt && it->m_longOpt != "version")) {
                throw amon::Error("auto-version must be disabled when -V is not a version option");
            }
            if (!it->m_special) {
                throw amon::Error(
                    "auto-version must be disabled when version opt ({}) is not marked as "
                    "special",
                    optsStr(*it));
            }
        }
    }

    return kwArgsCopy;
}

ParsedArgs ArgParser::parse(int argc, const char* const* argv) const
{
    auto kwArgs = getKwArgsWithAutoArgs();

    try {
        ParsedArgs res = doParse(argc, argv, kwArgs, m_posArgs);

        if (m_autoHelp && res["-h"]) {
            std::cerr << usage(argv[0]) << std::flush;
            std::exit(0); // NOLINT(concurrency-mt-unsafe)
        }

        if (m_version && res["-V"]) {
            std::cerr << version() << "\n" << std::flush;
            std::exit(0); // NOLINT(concurrency-mt-unsafe)
        }

        return res;

    } catch (const ArgParserError& e) {
        if (!m_autoHelp) {
            throw;
        }
        std::cerr << "ERROR: " << e.what() << "\n";
        std::cerr << "\n";
        std::cerr << usage(argv[0]) << std::flush;
        std::exit(1); // NOLINT(concurrency-mt-unsafe)
    }
}

#ifdef AMON_ENABLE_TESTS

TEST(AmonUtilArgParser, ArgparserParsePositionalArguments)
{
    auto parser = ArgParser {
        {PosArg("input").required(true), PosArg("output").defaultValue("out.txt")},
        {}}.setAutoHelp(false);

    const char* const argv[] = {"prog", "file.txt"};
    auto args = parser.parse(sizeof(argv) / sizeof(const char*), (argv));

    EXPECT_EQ(args["input"], "file.txt");
    EXPECT_EQ(args["output"], "out.txt");
}

TEST(AmonUtilArgParser, ArgparserParsePositionalArgumentsWithMultipleValues)
{
    auto parser = ArgParser {{PosArg("input").allowMultiple(true)}, {}}.setAutoHelp(false);

    const char* const argv[] = {"prog", "file.txt", "file2.txt"};
    auto args = parser.parse(sizeof(argv) / sizeof(const char*), (argv));

    EXPECT_EQ(args.getAll("input"), (std::vector<std::string> {"file.txt", "file2.txt"}));
}

TEST(AmonUtilArgParser, ArgparserParseKeywordArguments)
{
    auto parser = ArgParser{
        {},
        {KwArg('v', "verbose").help("Enable verbose mode"),
         KwArg('o', "output").hasArg(true).defaultValue("default.txt")}}.setAutoHelp(false);

    const char* const argv[] = {"prog", "-v", "-o", "result.txt"};
    auto args = parser.parse(sizeof(argv) / sizeof(const char*), (argv));

    EXPECT_TRUE(args["--verbose"]);
    EXPECT_EQ(args["--output"], "result.txt");
}

TEST(AmonUtilArgParser, ArgparserParseKeywordArgumentsWithMultipleOccurences)
{
    auto parser
        = ArgParser {{}, {KwArg('v', "verbose").allowMultiple(true).help("Increase verbosity")}}
              .setAutoHelp(false);

    const char* const argv[] = {"prog", "-vvv"};
    auto args = parser.parse(sizeof(argv) / sizeof(const char*), (argv));

    EXPECT_EQ(args.count("-v"), 3);
}

TEST(AmonUtilArgParser, ArgparserParseKeywordArgumentsWithMultipleOccurencesAndValue)
{
    auto parser = ArgParser {{}, {KwArg('a', "alpha").hasArg(true).allowMultiple(true).help("")}}
                      .setAutoHelp(false);

    const char* const argv[] = {"prog", "-a", "1", "-a", "2", "--alpha", "3", "--alpha", "4"};
    auto args = parser.parse(sizeof(argv) / sizeof(const char*), (argv));

    EXPECT_EQ(
        args.getAll("-a"),
        (std::vector<std::string> {
            "1",
            "2",
            "3",
            "4",
        }));
}

TEST(AmonUtilArgParser, ArgparserParseKeywordArgumentsWithDifferentWaysOfSpecifyingThem)
{
    auto parser = ArgParser {{}, {KwArg('a', "alpha").hasArg(true).allowMultiple(true).help("")}}
                      .setAutoHelp(false);

    const char* const argv[]
        = {"prog", "-a1", "-a", "2", "--alpha", "3", "--alpha", "4", "-a", "5", "-a6", "--alpha=7"};
    auto args = parser.parse(12, (argv));

    EXPECT_EQ(args.getAll("-a"), (std::vector<std::string> {"1", "2", "3", "4", "5", "6", "7"}));
}

TEST(AmonUtilArgParser, ArgparserMissingRequiredPositionalArgumentThrows)
{
    auto parser = ArgParser {{PosArg("input").required(true)}, {}}.setAutoHelp(false);

    const char* const argv[] = {"prog"};
    EXPECT_THROW(parser.parse(sizeof(argv) / sizeof(const char*), argv), ArgParserError);
}

TEST(AmonUtilArgParser, ArgparserDefaultValuesForKeywordArguments)
{
    auto parser = ArgParser {{}, {KwArg('o', "output").hasArg(true).defaultValue("default.txt")}}
                      .setAutoHelp(false);

    const char* const argv[] = {"prog"};
    auto args = parser.parse(sizeof(argv) / sizeof(const char*), argv);
    EXPECT_TRUE(args["--output"] == "default.txt");
}

TEST(AmonUtilArgParser, ArgparserUnknownKeywordArgumentThrows)
{
    auto parser = ArgParser {{}, {KwArg('v', "verbose")}}.setAutoHelp(false);

    const char* const argv[] = {"prog", "--unknown"};
    EXPECT_THROW(parser.parse(sizeof(argv) / sizeof(const char*), argv), ArgParserError);
}

TEST(AmonUtilArgParser, ArgparserExtraPositionalArgumentThrows)
{
    auto parser = ArgParser {{PosArg("input").required(true)}, {}}.setAutoHelp(false);

    const char* const argv[] = {"prog", "file.txt", "extra.txt"};
    EXPECT_THROW(parser.parse(sizeof(argv) / sizeof(const char*), argv), ArgParserError);
}

TEST(AmonUtilArgParser, ArgparserExtraKeywordArgumentThrows)
{
    auto parser = ArgParser {{}, {KwArg('v', "verbose")}}.setAutoHelp(false);

    const char* const argv[] = {"prog", "-v", "-x"};
    EXPECT_THROW(parser.parse(sizeof(argv) / sizeof(const char*), argv), ArgParserError);
}

TEST(AmonUtilArgParser, ArgparserMissingRequiredKeywordArgumentThrows)
{
    auto parser = ArgParser {
        {},
        {
            KwArg('i').required(true),
            KwArg('h', "help").special(true),
        }}.setAutoHelp(false);

    const char* const argv[] = {"prog"};
    EXPECT_THROW(parser.parse(sizeof(argv) / sizeof(const char*), argv), ArgParserError);
}

TEST(AmonUtilArgParser, ArgparserPositionalArgumentWithMultipleValuesNotLastThrows)
{
    EXPECT_THROW(
        ([] {
            (void) ArgParser {
                {PosArg("input").allowMultiple(true), PosArg("output")},
                {},
            }
                .setAutoHelp(false);
        }()),
        amon::Error);
    EXPECT_THROW(
        ([] {
            (void) ArgParser {
                {PosArg("input").allowMultiple(true), PosArg("output").required(false)},
                {},
            }
                .setAutoHelp(false);
        }()),
        amon::Error);
}

TEST(AmonUtilArgParser, ParsedArgsGetConvertsValuesToRequestedType)
{
    auto parser = ArgParser {
        {PosArg("input")},
        {KwArg('n', "number").hasArg(true)},
    }.setAutoHelp(false);

    const char* const argv[] = {"prog", "--number", "42", "123"};
    const auto args = parser.parse(sizeof(argv) / sizeof(const char*), argv);

    EXPECT_EQ(args.get<int>("--number"), 42);
    EXPECT_EQ(args.get<unsigned>("input"), 123U);
}

TEST(AmonUtilArgParser, ParsedArgsGetReturnsStringValue)
{
    auto parser = ArgParser {{}, {KwArg('s', "string").hasArg(true)}}.setAutoHelp(false);

    const char* const argv[] = {"prog", "--string", "hello world"};
    const auto args = parser.parse(sizeof(argv) / sizeof(const char*), argv);

    EXPECT_EQ(args.get<std::string>("--string"), "hello world");
}

TEST(AmonUtilArgParser, ParsedArgsGetIntoConvertsAndAssignsValue)
{
    auto parser
        = ArgParser {{}, {KwArg('n', "number").hasArg(true).defaultValue("42")}}.setAutoHelp(false);

    const char* const argv[] = {"prog"};
    const auto args = parser.parse(sizeof(argv) / sizeof(const char*), argv);
    int value = 0;
    args.getInto("-n", value);

    EXPECT_EQ(value, 42);
}

TEST(AmonUtilArgParser, ParsedArgsGetParsesBooleanAndFloatingPointValues)
{
    auto parser = ArgParser {
        {},
        {
            KwArg('b', "boolean").hasArg(true),
            KwArg('f', "float").hasArg(true),
            KwArg('d', "double").hasArg(true),
        },
    }.setAutoHelp(false);

    const char* const argv[] = {"prog", "--boolean", "yes", "--float", "1.25", "--double", "-2.5"};
    const auto args = parser.parse(sizeof(argv) / sizeof(const char*), argv);

    EXPECT_TRUE(args.get<bool>("--boolean"));
    EXPECT_FLOAT_EQ(args.get<float>("--float"), 1.25F);
    EXPECT_DOUBLE_EQ(args.get<double>("--double"), -2.5);
}

TEST(AmonUtilArgParser, ParsedArgsGetThrowsForMissingOrInvalidValue)
{
    auto parser = ArgParser {{}, {KwArg('n', "number").hasArg(true)}}.setAutoHelp(false);

    const char* const argv[] = {"prog", "--number", "not-a-number"};
    const auto args = parser.parse(sizeof(argv) / sizeof(const char*), argv);

    EXPECT_THROW(args.get<int>("--missing"), std::out_of_range);
    EXPECT_THROW(args.get<int>("--number"), std::invalid_argument);
}

#endif

} // namespace amon::util
