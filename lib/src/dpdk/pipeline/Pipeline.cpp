/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Packet processing pipeline implementation.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon/dpdk/pipeline/Pipeline.hpp>

#include <csignal>

namespace amon::dpdk::pipeline {

Pipeline::Pipeline(Blueprint blueprint, PipelineConfig config)
    : m_config(std::move(config))
    , m_logger(LoggerProvider::get(std::format("Pipeline({})", m_config.name)))
{
    if (m_config.numStreams == 0) {
        throw amon::Error("cannot create pipeline with 0 streams");
    }

    // Acquire lcores
    m_lcores.reserve(m_config.numStreams);
    for (uint16_t i = 0; i < m_config.numStreams; ++i) {
        auto lcore = m_config.lcorePool->acquire(m_config.socketId);
        m_lcores.emplace_back(std::move(lcore));
    }

    // Instantiate stages
    m_stages.reserve(blueprint.m_stageFactories.size());
    for (auto& factory : blueprint.m_stageFactories) {
        m_stages.emplace_back(factory());
        m_stages.back()->createStreams(m_lcores);
    }
}

void Pipeline::start()
{
    m_logger.info("Pipeline is starting ({} workers) ...", m_config.numStreams);

    // Run
    uint16_t workerId = 0;
    for (auto& lcore : m_lcores) {
        m_logger.debug("Starting worker on {}", lcore);
        lcore.start([workerId, this](const std::atomic_bool& stopFlag) noexcept {
            this->lcoreMain(workerId, stopFlag);
        });
        ++workerId;
    }
}

void Pipeline::requestStop()
{
    m_logger.info("Pipeline stop requested");
    for (auto& lcore : m_lcores) {
        lcore.requestStop();
    }
}

void Pipeline::join()
{
    for (auto& lcore : m_lcores) {
        lcore.join();
    }
}

const std::exception_ptr& Pipeline::activeException()
{
    const std::lock_guard<std::mutex> _ {m_activeExceptionMutex};
    return m_activeExceptionPtr;
}

void Pipeline::exceptionHandler(uint16_t workerId, const std::exception_ptr& ex) noexcept
{
    try {
        std::rethrow_exception(ex);

    } catch (const amon::Error& ex) {
        m_logger.critical(
            "Uncaught exception in pipeline worker #{}: {}\n{}",
            workerId,
            ex.what(),
            ex.stacktrace().str());
    } catch (const std::exception& ex) {
        m_logger.critical("Uncaught exception in pipeline worker #{}: {}", workerId, ex.what());
    } catch (...) {
        m_logger.critical("Uncaught exception in pipeline worker #{}", workerId);
    }

    {
        const std::lock_guard<std::mutex> _ {m_activeExceptionMutex};
        m_activeExceptionPtr = ex;
    }

    ::kill(::getpid(), SIGUSR1);
}

void Pipeline::lcoreMain(uint16_t workerId, const std::atomic_bool& stopFlag) noexcept
{
    try {
        auto burst = PktBurst<DpdkPacket> {m_config.burstSize};
        while (!stopFlag.load(std::memory_order_relaxed)) {
            for (auto& stage : m_stages) {
                stage->process(workerId, burst);
            }
            burst.clear();
        }

    } catch (...) { // NOLINT(bugprone-empty-catch)
        exceptionHandler(workerId, std::current_exception());
    }
}

} // namespace amon::dpdk::pipeline
