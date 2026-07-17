/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Global LCore Pool.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/Logger.hpp>
#include <amon/dpdk/LCore.hpp>

#include <mutex>
#include <vector>

namespace amon::dpdk {

/**
 * @brief Global pool of all worker cores.
 */
class GlobalLCorePool : public LCorePool {
public:
    /**
     * @brief Create an instance of the global pool.
     * @return Shared pointer to the instance.
     */
    static std::shared_ptr<LCorePool> create();

private:
    std::mutex m_lock;
    std::vector<LCore> m_cores;
    Logger m_logger;

    GlobalLCorePool();
    LCore acquireCore(SocketID socketId) override;
    void releaseCore(LCore core) override;
};

} // namespace amon::dpdk
