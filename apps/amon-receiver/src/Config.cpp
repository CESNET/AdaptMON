/**
 * @file
 * @author Samuel Luptak <luptak@cesnet.cz>
 * @brief Receiver configuration
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 */

#include "Config.hpp"

#include <amon/Error.hpp>
#include <amon/util/ArgParser.hpp>
#include <amon/util/string.hpp>

#include <optional>
#include <string_view>

static const auto argParser = amon::util::ArgParser(
    {},
    {
        amon::util::KwArg('e', "eal")
            .metaVar("EAL_PARAMS_STRING")
            .defaultValue("")
            .help("EAL parameters as string"),
        amon::util::KwArg('d', "dev")
            .metaVar("DEV_NAME[,driverArg1,driverArg2,...]")
            .required(true)
            .help("DPDK device name (most often PCI address) + driver arguments"),
        // Zero defaults to all available queues
        amon::util::KwArg('q', "queues")
            .metaVar("N")
            .defaultValue("0")
            .help("Number of RX queues")
            .constraint(amon::util::ArgConstraint::number<uint16_t>()),
        amon::util::KwArg('t', "interval")
            .metaVar("MS")
            .defaultValue("1000")
            .help("Statistics display interval in milliseconds")
            .constraint(amon::util::ArgConstraint::number<uint32_t>()),
        amon::util::KwArg('m', "mempool-size")
            .metaVar("N")
            .defaultValue("4096")
            .help("Number of packets in the DPDK mempool")
            .constraint(amon::util::ArgConstraint::number<unsigned>()),
        amon::util::KwArg('b', "burst-size")
            .metaVar("N")
            .defaultValue("64")
            .help("Maximum number of packets per receive burst")
            .constraint(amon::util::ArgConstraint::number<size_t>()),
        amon::util::KwArg("no-hugepages-setup")
            .hasArg(false)
            .help("Disable automatic Hugepages allocation on startup"),
        amon::util::KwArg("enable-parser")
            .hasArg(false)
            .help("Enable packet parsing in worker threads"),
    });

static std::vector<std::string>
parseEalArgs(std::string_view programName, const amon::util::ParsedArgs& args)
{
    const auto& ealArg = args["--eal"].value_or("");
    auto ealArgViews = amon::util::string::splitWhitespace(ealArg);

    // Convert string_views to owned strings
    std::vector<std::string> ealArgStrs(ealArgViews.begin(), ealArgViews.end());

    // Adds the -a flag with the command-line specified device as argument
    ealArgStrs.insert(
        ealArgStrs.begin(),
        {"-a", *args["--dev"]}); // NOLINT(bugprone-unchecked-optional-access)

    // DPDK EAL expects argv[0] to be the program name
    ealArgStrs.insert(ealArgStrs.begin(), std::string(programName));

    return ealArgStrs;
}

Config::Config(int argc, char** argv)
{
    auto parsedArgs = argParser.parse(argc, argv);

    ealArgs = parseEalArgs(argv[0], parsedArgs);

    const std::string dev = *parsedArgs["--dev"]; // NOLINT(bugprone-unchecked-optional-access)
    auto devParsed = amon::util::string::splitOnce(dev, ',');
    if (devParsed != std::nullopt) {
        devName = devParsed.value().first;
    } else {
        devName = dev;
    }

    rxQueues = parsedArgs.get<uint16_t>("--queues");
    statsInterval = parsedArgs.get<uint32_t>("--interval");
    mempoolSize = parsedArgs.get<unsigned>("--mempool-size");
    burstSize = parsedArgs.get<size_t>("--burst-size");

    noHugepagesSetup = static_cast<bool>(parsedArgs["--no-hugepages-setup"]);
    enableParser = static_cast<bool>(parsedArgs["--enable-parser"]);
}
