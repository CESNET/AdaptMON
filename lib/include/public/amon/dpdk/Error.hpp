/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Dpdk-specific error types.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/Error.hpp>

#include <rte_errno.h>

namespace amon::dpdk {

/**
 * @brief Error class for representing errors related to DPDK.
 */
class DpdkError : public amon::Error {
public:
    using amon::Error::Error;
};

/**
 * @brief Error class for representing errors related to rte errno values.
 *
 * Inherits from DpdkError and constructs a message including the function name,
 * error string, and error number.
 */
class RteErrnoError : public DpdkError {
public:
    /**
     * @brief Constructs an RteErrnoError with a specific error number.
     *
     * @tparam FunctionArgs Variadic arguments representing function parameters.
     * @param errNum The error number (rte_errno value).
     * @param functionName The name of the function that failed.
     * @param functionArgs Arguments passed to the failed function.
     *
     * Formats the error message as: "{functionName}({functionArgs}) has failed: {errnoToString}
     * ({errNum})"
     */
    template <typename... FunctionArgs>
    RteErrnoError(int errNum, std::string_view functionName, FunctionArgs&&... functionArgs)
        : DpdkError(
              "{}({}) has failed: {} ({})",
              functionName,
              amon::detail::join(std::forward<FunctionArgs>(functionArgs)...),
              rte_strerror(errNum),
              errNum)
        , m_errNum(errNum)
    {
    }

    /**
     * @brief Constructs an ErrnoError using the current errno value.
     *
     * @tparam FunctionArgs Variadic arguments representing function parameters.
     * @param functionName The name of the function that failed.
     * @param functionArgs Arguments passed to the failed function.
     *
     * Uses the current value of rte_errno for the error number.
     */
    template <typename... FunctionArgs>
    RteErrnoError(std::string_view functionName, FunctionArgs&&... functionArgs)
        : RteErrnoError(rte_errno, functionName, std::forward<FunctionArgs>(functionArgs)...)
    {
    }

    /**
     * @brief Get the underlying errno value.
     * @return The rte_errno value.
     */
    int errNum() const { return m_errNum; }

private:
    int m_errNum;
};

} // namespace amon::dpdk
