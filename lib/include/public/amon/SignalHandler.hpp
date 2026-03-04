/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Signal handling utilities for the amon library.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/Stacktrace.hpp>
#include <amon/util/UniqueResource.hpp>

#include <csignal>

namespace amon {

/**
 * @brief Manages installation and handling of signals.
 *
 * Provides methods to install signal handlers and wait for signals.
 *
 * The signal handler is installed using the `SignalHandler::install()` method. This should be
 * called as early during the program execution as possible. Especially before any threads are
 * spawned. Threads spawned before this call will not have signal handlers set-up correctly.
 *
 * After the signal handler is installed, you can wait for incoming signals using the
 * `SignalHandler::waitForSignals()` method. This method blocks until a signal has arrived. Once a
 * signal is received, the method returns the signal number.
 *
 * Different signals are handled differently:
 *   "Configuration/termination" signals - {SIGTERM, SIGINT, SIGQUIT, SIGUSR1}:
 *   - Pass to the user to handle using the `waitForSignals()` mechanism.
 *
 *   "Deadly" signals - {SIGILL, SIGBUS, SIGFPE, SIGSEGV, SIGSYS}:
 *   - Prints current stacktrace of the handling thread and invoke the default signal handler.
 *
 *   "Debug" signals - {SIGUSR2}:
 *   - Prints current stacktrace of the handling thread and do nothing.
 *
 *   All other signals that have not been mentioned are handled using their default handler.
 *
 * Example use:
 * @code{.cpp}
 * int main()
 * {
 *     amon::SignalHandler::install();
 *
 *     // ... spawn threads and do work ...
 *
 *     for (;;) {
 *         int sig = amon::SignalHandler::waitForSignals();
 *         std::cout << "Received signal " << sig << "\n";
 *
 *         if (sig == SIGUSR1) {
 *             std::cout << "Reloading config ...\n";
 *             reloadConfig();
 *             continue;
 *         }
 *
 *         if (sig == SIGINT) {
 *             std::cout << "CTRL+C received, quitting...\n";
 *             break;
 *         }
 *     }
 *
 *     return 0;
 * }
 * @endcode
 *
 * Note: The `SignalHandler::waitForSignals()` method can be potentially called from any thread.
 * There is no limitation that would require it to be called from the main thread as in the example.
 */
class SignalHandler {
public:
    /**
     * @brief Get the singleton instance of SignalHandler.
     * @return Reference to the SignalHandler instance.
     */
    static SignalHandler& instance();

    /**
     * @brief Install signal handlers.
     */
    static void install();

    /**
     * @brief Wait for signals and process them.
     * @return Signal number received. One of {SIGTERM, SIGINT, SIGQUIT, SIGUSR1}.
     */
    static int waitForSignals();

private:
    friend void
    deadlySignalCallback(int sigNum, siginfo_t* /*sigInfo*/, void* /*context*/) noexcept;
    friend void
    returnSignalCallback(int sigNum, siginfo_t* /*sigInfo*/, void* /*context*/) noexcept;
    friend void debugSignalCallback(int sigNum, siginfo_t* /*sigInfo*/, void* /*context*/) noexcept;

    util::UniqueFd m_pipeRd; ///< Read end of the signal pipe.
    util::UniqueFd m_pipeWr; ///< Write end of the signal pipe.

    /**
     * @brief Construct a SignalHandler (private).
     */
    SignalHandler();
};

} // namespace amon
