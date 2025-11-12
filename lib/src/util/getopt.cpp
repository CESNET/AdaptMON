/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief A sane wrapper around getopt_long
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/Test.hpp>
#include <amon/util/ArgvList.hpp>
#include <amon/util/getopt.hpp>

#include <algorithm>
#include <cassert>
#include <mutex>
#include <stdexcept>

#include <getopt.h>

namespace amon::util::getopt {

static bool isValidShortOpt(char c)
{
    return static_cast<bool>(std::isgraph(c)) && c != ':' && c != '-' && c != ';';
}

static void validateOpts(const std::vector<Opt>& opts)
{
    for (const Opt& opt : opts) {
        if (!opt.longOpt && !opt.shortOpt) {
            throw amon::Error(
                "opt cannot have no long opt and no short opt, at least one of them must be set");
        }

        if (opt.shortOpt && std::ranges::count(opts, opt.shortOpt, &Opt::shortOpt) > 1) {
            throw amon::Error("option with short opt '{}' specified multiple times", *opt.shortOpt);
        }
        if (opt.longOpt && std::ranges::count(opts, opt.longOpt, &Opt::longOpt) > 1) {
            throw amon::Error("option with long opt \"{}\" specified multiple times", *opt.longOpt);
        }
        if (opt.shortOpt && !isValidShortOpt(*opt.shortOpt)) {
            throw amon::Error(
                "'{}' (chr {}) is not a valid short opt",
                *opt.shortOpt,
                int(*opt.shortOpt));
        }
    }
}

static std::pair<std::string, std::vector<struct option>>
makeGetoptParams(const std::vector<Opt>& opts)
{
    validateOpts(opts);

    int nextNonchr = 256;
    std::string getoptShortOpts = ":";
    std::vector<struct option> getoptLongOpts;

    for (const Opt& opt : opts) {
        struct option& getoptLongOpt = getoptLongOpts.emplace_back();
        getoptLongOpt.name = opt.longOpt.has_value() ? opt.longOpt->c_str() : nullptr;
        getoptLongOpt.has_arg = (opt.hasArg == REQUIRED_ARG) ? required_argument
            : (opt.hasArg == OPTIONAL_ARG)                   ? optional_argument
                                                             : no_argument;
        getoptLongOpt.flag = nullptr;
        getoptLongOpt.val = nextNonchr++;

        if (opt.shortOpt) {
            getoptShortOpts += *opt.shortOpt;
            getoptShortOpts += (opt.hasArg == REQUIRED_ARG) ? ":"
                : (opt.hasArg == OPTIONAL_ARG)              ? "::"
                                                            : "";
        }
    }
    getoptLongOpts.emplace_back(nullptr, 0, nullptr, 0);

    return {getoptShortOpts, getoptLongOpts};
}

static std::vector<std::string> collectArgv(int argc, const char* const* argv)
{
    std::vector<std::string> res;
    res.reserve(static_cast<std::size_t>(std::max<int>(0, argc)));
    for (int i = 0; i < argc; i++) {
        res.emplace_back(argv[i]);
    }
    return res;
}

Result getOpt(const std::vector<Opt>& opts, int argc, const char* const* argv)
{
    auto argvCopy = ArgvList {collectArgv(argc, argv)};

    auto [getoptShortOpts, getoptLongOpts] = makeGetoptParams(opts);

    static std::mutex getoptMutex;
    const std::lock_guard<std::mutex> guard {getoptMutex}; // getopt_long is not thread safe

    Result res;
    optind = 0;
    for (;;) {
        // NOLINTBEGIN(concurrency-mt-unsafe)
        const int ret = getopt_long(
            argvCopy.argc(),
            argvCopy.argv(),
            getoptShortOpts.c_str(),
            getoptLongOpts.data(),
            nullptr);
        // NOLINTEND(concurrency-mt-unsafe)
        if (ret == -1) {
            break;
        }
        if (ret == '?') {
            throw UnknownOpt {argvCopy.argv()[optind - 1]};
        }

        const int val = (ret != ':' ? ret : optopt);
        const Opt* opt;
        if (val >= 256) {
            // Long opt val where the index is directly encoded as a non-char
            assert(static_cast<size_t>(val - 256) < opts.size());
            opt = &opts[static_cast<size_t>(val - 256)];
        } else {
            // Short opt val that contains the actual option character
            auto it = std::ranges::find(opts, val, &Opt::shortOpt);
            assert(it != opts.end());
            opt = &(*it);
        }

        if (ret == ':') {
            throw MissingOptArg {*opt};
        }

        res.kwArgs.emplace_back(
            *opt,
            optarg != nullptr ? std::optional<std::string> {optarg} : std::nullopt);
    }

    for (int i = optind; i < argvCopy.argc(); i++) {
        res.posArgs.emplace_back(argvCopy.argv()[i]);
    }

    return res;
}

TEST_CASE("getOpt parses short options")
{
    const std::vector<Opt> opts
        = {{.shortOpt = 'a', .longOpt = "alpha", .hasArg = NO_ARG},
           {.shortOpt = 'b', .longOpt = "beta", .hasArg = REQUIRED_ARG}};
    const char* argv[] = {"prog", "-a", "-b", "val"};
    auto result = getOpt(opts, 4, argv);

    REQUIRE(result.kwArgs.size() == 2);
    CHECK(result.kwArgs[0].first.shortOpt == 'a');
    CHECK(!result.kwArgs[0].second.has_value());
    CHECK(result.kwArgs[1].first.shortOpt == 'b');
    CHECK(result.kwArgs[1].second == "val");
}

TEST_CASE("getOpt parses long options")
{
    const std::vector<Opt> opts
        = {{.shortOpt = 'a', .longOpt = "alpha", .hasArg = NO_ARG},
           {.shortOpt = 'b', .longOpt = "beta", .hasArg = REQUIRED_ARG}};
    const char* argv[] = {"prog", "--alpha", "--beta", "val"};
    auto result = getOpt(opts, 4, argv);

    REQUIRE(result.kwArgs.size() == 2);
    CHECK(result.kwArgs[0].first.longOpt == "alpha");
    CHECK(!result.kwArgs[0].second.has_value());
    CHECK(result.kwArgs[1].first.longOpt == "beta");
    CHECK(result.kwArgs[1].second == "val");
}

TEST_CASE("getOpt parses with omitted options")
{
    const std::vector<Opt> opts
        = {{.shortOpt = std::nullopt, .longOpt = "alpha", .hasArg = REQUIRED_ARG},
           {.shortOpt = 'b', .longOpt = std::nullopt, .hasArg = REQUIRED_ARG},
           {.shortOpt = 'c', .longOpt = "gamma", .hasArg = REQUIRED_ARG}};
    const char* argv[] = {"prog", "--alpha", "a", "-b", "b", "-c", "c"};
    auto result = getOpt(opts, 7, argv);

    REQUIRE(result.kwArgs.size() == 3);

    CHECK(result.kwArgs[0].first.longOpt == "alpha");
    CHECK(result.kwArgs[0].second == "a");

    CHECK(result.kwArgs[1].first.shortOpt == 'b');
    CHECK(result.kwArgs[1].second == "b");

    CHECK(result.kwArgs[2].first.shortOpt == 'c');
    CHECK(result.kwArgs[2].second == "c");
}

TEST_CASE("getOpt throws UnknownOpt")
{
    std::vector<Opt> opts;
    const char* argv[] = {"prog", "-x"};

    opts = {{.shortOpt = 'a', .longOpt = "alpha", .hasArg = NO_ARG}};
    CHECK_THROWS_AS(getOpt(opts, 2, argv), UnknownOpt);

    opts = {{.shortOpt = std::nullopt, .longOpt = "alpha", .hasArg = NO_ARG}};
    CHECK_THROWS_AS(getOpt(opts, 2, argv), UnknownOpt);

    opts = {{.shortOpt = 'a', .longOpt = std::nullopt, .hasArg = NO_ARG}};
    CHECK_THROWS_AS(getOpt(opts, 2, argv), UnknownOpt);

    const char* argv2[] = {"prog", "-a", "-x"};
    opts = {{.shortOpt = 'a', .longOpt = std::nullopt, .hasArg = NO_ARG}};
    CHECK_THROWS_AS(getOpt(opts, 3, argv2), UnknownOpt);
}

TEST_CASE("getOpt throws MissingOptArg")
{
    std::vector<Opt> opts;
    const char* argv[] = {"prog", "-b"};

    opts = {{.shortOpt = 'b', .longOpt = "beta", .hasArg = REQUIRED_ARG}};
    CHECK_THROWS_AS(getOpt(opts, 2, argv), MissingOptArg);

    opts = {{.shortOpt = 'b', .longOpt = std::nullopt, .hasArg = REQUIRED_ARG}};
    CHECK_THROWS_AS(getOpt(opts, 2, argv), MissingOptArg);
}

TEST_CASE("getOpt detects invalid options")
{
    std::vector<Opt> opts;
    const char* argv[] = {"prog"};

    opts
        = {{.shortOpt = 'b', .longOpt = "beta", .hasArg = REQUIRED_ARG},
           {.shortOpt = std::nullopt, .longOpt = "beta", .hasArg = REQUIRED_ARG}};
    CHECK_THROWS_AS(getOpt(opts, 1, argv), Error);

    opts
        = {{.shortOpt = 'b', .longOpt = "beta", .hasArg = REQUIRED_ARG},
           {.shortOpt = 'b', .longOpt = std::nullopt, .hasArg = REQUIRED_ARG}};
    CHECK_THROWS_AS(getOpt(opts, 1, argv), Error);

    opts
        = {{.shortOpt = 'b', .longOpt = "beta", .hasArg = REQUIRED_ARG},
           {.shortOpt = std::nullopt, .longOpt = std::nullopt, .hasArg = REQUIRED_ARG}};
    CHECK_THROWS_AS(getOpt(opts, 1, argv), Error);
}

}; // namespace amon::util::getopt
