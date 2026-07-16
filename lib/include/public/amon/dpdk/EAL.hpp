/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief RAII wrapper around rte_eal_init/rte_eal_cleanup.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/dpdk/Error.hpp>
#include <amon/util/ArgvList.hpp>
#include <amon/util/UniqueResource.hpp>

#include <string>
#include <utility>
#include <vector>

#include <rte_eal.h>

namespace amon::dpdk {

/**
 * @brief RAII wrapper around DPDK's Environment Abstraction Layer.
 *
 * Calls rte_eal_init() in the constructor and ensures rte_eal_cleanup()
 * is called on destruction via a unique resource handle.
 */
class EAL {
public:
    /**
     * @brief Construct an EAL instance directly from C-style arguments.
     *
     * argv[0] is expected to be the program name. It must be always present.
     *
     * argc and argv are adjusted to point to the remaining unparsed args. argv[0] remains the
     * program name after adjustment.
     *
     * @param[in,out] argc Argument count.
     * @param[in,out] argv Argument vector.
     */
    EAL(int& argc, char**& argv);

    /**
     * @brief Construct an EAL instance from an ArgvList.
     *
     * args[0] is expected to be the program name. It must be always present.
     *
     * args is modified to contain the remaining unparsed args. args[0] remains the program name.
     *
     * @param[in,out] args An ArgvList containing the command-line arguments.
     */
    EAL(util::ArgvList& args);

    /**
     * @brief Construct an EAL instance from a vector of strings.
     *
     * args[0] is expected to be the program name. It must be always present.
     *
     * args is modified to contain the remaining unparsed args. args[0] remains the program name.
     *
     * @param[in,out] args A vector of strings representing the arguments.
     */
    EAL(std::vector<std::string>& args);

private:
    struct Dummy {};

    struct Cleanup {
        void operator()(Dummy unused) const noexcept;
    };

    amon::util::UniqueResource<Dummy, Cleanup> m_handle;

    std::pair<int, char**> init(int argc, char** argv);
};

} // namespace amon::dpdk
