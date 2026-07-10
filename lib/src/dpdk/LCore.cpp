/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Implementation of the DPDK lcore handle.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon/Logger.hpp>
#include <amon/amon.hpp>
#include <amon/dpdk/Error.hpp>
#include <amon/dpdk/LCore.hpp>

#include <format>

#include <rte_errno.h>
#include <rte_launch.h>
#include <rte_lcore.h>

namespace amon::dpdk {

static void warnIfNotMainLcore(Logger& logger, std::string_view function)
{
    if (rte_lcore_id() != rte_get_main_lcore()) {
        logger.warn(
            "LCoreHandle::{}() called from non-main lcore! This is not allowed and may lead to "
            "undefined behavior.",
            function);
    }
}

void format(const LCore& value, util::Formatter& fmt)
{
    fmt.format("LCore(socketId={}, coreId={})", value.socketId, value.coreId);
}

LCoreHandle LCorePool::acquire(SocketID socketId)
{
    return {acquireCore(socketId), shared_from_this()};
}

void LCoreHandle::Lease::Release::operator()(const LCoreHandle::Lease& lease) const noexcept
{
    lease.owner->releaseCore(lease.core);
}

LCoreHandle::LCoreHandle(LCore core, std::shared_ptr<LCorePool> pool)
    : m_lease({.core = core, .owner = std::move(pool)})
    , m_logger(amon::LoggerProvider::get(std::format("{}", *this)))
{
    m_logger.debug("LCore acquired");
}

LCoreHandle::~LCoreHandle() noexcept // NOLINT(bugprone-exception-escape)
{
    warnIfNotMainLcore(m_logger, "~LCoreHandle");

    if (m_lease.hasValue() && m_launchCtx) {
        // any task was started
        m_logger.warn("Stopping unjoined task in ~LCoreHandle");
        requestStop();
        rte_eal_wait_lcore(m_lease->core.coreId);
    }
}

void LCoreHandle::start(TargetFn target)
{
    warnIfNotMainLcore(m_logger, "start");

    if (m_launchCtx) {
        throw DpdkError("rte_eal_remote_launch already running and/or join method was not called");
    }

    // TODO: Allocate this on the socket ID of the LCore where this will run. The target function
    // will constantly poll the stop flag, so it should be in its own memory.
    m_launchCtx = std::make_unique<LaunchContext>(std::move(target), m_lease->core, m_logger);

    auto launchFunc = [](void* arg) noexcept -> int {
        auto* ctx = reinterpret_cast<LaunchContext*>(arg);

        try {
            ctx->function(ctx->stopFlag);

        } catch (...) {
            ctx->logger.critical(
                "Unhandled exception occured while running target function on LCore!");
            std::terminate();
        }
        ctx->running = false;

        // We don't want to use 0 here because of how rte_eal_wait_lcore works. It propagates the
        // return code we returned on completion OR returns 0 in case the launch was never called.
        return 1;
    };

    m_logger.debug("Running target function on LCore.");
    const int ret = rte_eal_remote_launch(launchFunc, m_launchCtx.get(), m_lease->core.coreId);
    if (ret != 0) {
        m_launchCtx.reset();
        throw RteErrnoError(-ret, "rte_eal_remote_launch", m_lease->core.coreId);
    }
}

void LCoreHandle::requestStop()
{
    if (m_launchCtx) {
        m_launchCtx->stopFlag = true;
    }
}

void LCoreHandle::join()
{
    warnIfNotMainLcore(m_logger, "join");

    const int ret = rte_eal_wait_lcore(m_lease->core.coreId);
    if (ret == 0) {
        throw DpdkError(
            "rte_eal_wait_lcore({}) on core that was not started",
            m_lease->core.coreId);
    }
    m_launchCtx.reset();
}

bool LCoreHandle::isRunning() const
{
    return m_launchCtx && m_launchCtx->running;
}

void LCoreHandle::format(amon::util::Formatter& fmt) const
{
    fmt.format("LCoreHandle(socketId={}, coreId={})", m_lease->core.socketId, m_lease->core.coreId);
}

} // namespace amon::dpdk
