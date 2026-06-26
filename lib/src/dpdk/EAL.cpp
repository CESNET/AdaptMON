/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief RAII wrapper around rte_eal_init/rte_eal_cleanup.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon/Logger.hpp>
#include <amon/dpdk/EAL.hpp>

#include <ranges>
#include <tuple>

namespace amon::dpdk {

EAL::EAL(int& argc, char**& argv)
{
    std::tie(argc, argv) = init(argc, argv);
}

EAL::EAL(util::ArgvList& args)
{
    const auto [argc, argv] = init(args.argc(), args.argv());
    args = {std::ranges::subrange(argv, argv + argc) | std::ranges::to<std::vector<std::string>>()};
}

EAL::EAL(std::vector<std::string>& args)
{
    util::ArgvList argsList {args};
    const auto [newArgc, _] = init(argsList.argc(), argsList.argv());
    const auto nParsed = static_cast<ssize_t>(args.size()) - newArgc;
    // + 1 because we are keeping the first argument, which is the program name
    args.erase(args.begin() + 1, args.begin() + 1 + nParsed);
}

std::pair<int, char**> EAL::init(int argc, char** argv)
{
    const int ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        throw amon::dpdk::RteErrnoError {"rte_eal_init"};
    }
    m_handle.reset(Dummy {});
    return {argc - ret, argv + ret};
}

void EAL::Cleanup::operator()(Dummy unused) const noexcept
{
    (void) unused;
    const int ret = rte_eal_cleanup();
    if (ret != 0) {
        LoggerProvider::get("EAL::Cleanup").warn("rte_eal_cleanup() failed: {}", -ret);
    }
}

} // namespace amon::dpdk
