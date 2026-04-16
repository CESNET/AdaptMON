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
#include <amon/types/Singleton.hpp>
#include <amon/util/UniqueResource.hpp>

#include <csignal>
#include <initializer_list>

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
 *
 * Example use 2:
 *
 * In case your application does not want to use the waitForSignals mechanism, for example in case
 * of simpler and/or single-threaded applications...
 * @code{.cpp}
 * std::atomic_bool g_stop;
 *
 * void stopSignalHandler(int signal) noexcept
 * {
 *     g_stop.store(true);
 * }
 *
 * int main()
 * {
 *     amon::SignalHandler::install(false);
 *     amon::SignalHandler::installSigAction({SIGINT, SIGTERM}, stopSignalHandler);
 *
 *     while (true) {
 *         if (g_stop.load(std::memory_order_relaxed)) {
 *             std::cout << "CTRL+C received, stopping early...\n";
 *             break;
 *         }
 *
 *         // ... do work ...
 *
 *     }
 *
 *     return 0;
 * }
 * @endcode
 *
 */
class SignalHandler final : private types::Singleton<SignalHandler> {
public:
    /**
     * @brief Type alias for a signal handler callback function.
     *
     * This callback is invoked when a signal is received.
     * @param sigNum The signal number.
     */
    using SigActionCallback = void (*)(int sigNum) noexcept;

    /**
     * @brief Type alias for an extended signal handler callback function.
     *
     * This callback is invoked when a signal is received and provides additional information.
     * @param sigNum The signal number.
     * @param sigInfo Pointer to a siginfo_t structure containing signal information.
     * @param context Pointer to context information.
     */
    using SigActionExtendedCallback
        = void (*)(int sigNum, siginfo_t* sigInfo, void* context) noexcept;

    /**
     * @brief Installs a signal handler for the specified signal number.
     *
     * Sets up a signal handler for the given signal number using the provided callback function.
     *
     * @param signal The signal number to handle (e.g., SIGINT, SIGTERM).
     * @param callback The callback function to be invoked when the signal is received.
     */
    static void installSigAction(int signal, SigActionCallback callback);

    /**
     * @brief Installs a signal handler for the specified signal number.
     *
     * This function sets up a signal handler using the provided callback for the given signal
     * number.
     *
     * @param signal The signal number to handle (e.g., SIGINT, SIGTERM).
     * @param callback The extended callback function to be invoked when the signal is received.
     */
    static void installSigAction(int signal, SigActionExtendedCallback callback);

    /**
     * @brief Installs a signal handler for the given signals.
     *
     * @param signals List of signal numbers to handle.
     * @param callback Function to call when the signal is received.
     */
    static void installSigAction(std::initializer_list<int> signals, SigActionCallback callback);

    /**
     * @brief Installs an extended signal handler for the given signals.
     *
     * @param signals List of signal numbers to handle.
     * @param callback Extended callback function to call when the signal is received.
     */
    static void
    installSigAction(std::initializer_list<int> signals, SigActionExtendedCallback callback);

    /**
     * @brief Installs signal handlers for the application.
     *
     * This function sets up the necessary signal handlers. If
     * willUseWaitForSignals is true, the handlers will be configured to
     * support waiting for signals in the application's event loop.
     *
     * In summary, set willUseForSignals = true if you intend to use the `waitForSignals()` method.
     *
     * @param willUseWaitForSignals If true, enables waiting for signals using the
     * `waitForSignals()` method; otherwise, installs only basic handlers providing more informative
     * output for certain signals.
     */
    static void install(bool willUseWaitForSignals = true);

    /**
     * @brief Wait for signals and process them.
     * @return Signal number received. One of {SIGTERM, SIGINT, SIGQUIT, SIGUSR1}.
     */
    static int waitForSignals();

private:
    friend class types::Singleton<SignalHandler>;
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
