/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Stacktrace functionality implementation
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/util/CharBuffer.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace amon {

/**
 * @brief Represents a single frame in a stack trace.
 */
struct StacktraceFrame {
    uintptr_t pc = 0; ///< Program counter (instruction address).

    std::string symbol; ///< Symbol name (if available).
    uintptr_t offsetFromSymbol = 0; ///< Offset from the symbol address.

    std::string function; ///< Demangled function name (if available).
    std::string filename; ///< Filename of the source file.
    int line = 0; ///< Line number in the source file.
};

/**
 * @brief Represents a captured stack trace.
 *
 * Holds a sequence of stack frames and provides string conversion.
 */
class Stacktrace {
public:
    /**
     * @brief Default constructor.
     */
    Stacktrace() = default;

    /**
     * @brief Returns a string representation of the stack trace.
     * @return The stack trace as a string.
     */
    std::string str() const;

    /**
     * @brief Get the list of stack trace frames.
     * @return Reference to the vector of frames.
     */
    const std::vector<StacktraceFrame>& frames() const { return m_frames; }

private:
    friend class StacktraceProvider;

    std::vector<StacktraceFrame> m_frames; ///< List of stack trace frames.
};

/**
 * @brief Provides stack trace capturing functionality.
 *        Implements a singleton pattern.
 */
class StacktraceProvider {
public:
    /**
     * @brief Print the current stack trace to the provided PrintBuffer.
     * @param printBuf Buffer to write the stack trace output.
     * @param skipFrames Number of frames to skip from the top (default: 0).
     *
     * This method is signal-safe and can be called from a signal handler.
     * It avoids allocations and non-signal-safe functions.
     * For non-signal contexts, prefer creating a stacktrace and converting it to string.
     */
    void dump(util::CharBuffer<>& printBuf, int skipFrames = 0);

    /**
     * @brief Returns the singleton instance of StacktraceProvider.
     * @return Reference to the singleton instance.
     */
    static StacktraceProvider& instance();

    /**
     * @brief Captures a stack trace.
     * @param skipFrames Number of frames to skip from the top.
     * @return The captured stack trace.
     */
    Stacktrace createStacktrace(int skipFrames = 0);

    StacktraceProvider(const StacktraceProvider&) = delete;
    StacktraceProvider(StacktraceProvider&&) = delete;
    StacktraceProvider& operator=(const StacktraceProvider&) = delete;
    StacktraceProvider& operator=(StacktraceProvider&&) = delete;

private:
    /**
     * @brief Construct a StacktraceProvider (private).
     */
    StacktraceProvider();

    void* m_backtraceState; ///< Internal state for backtrace operations.
};

} // namespace amon
