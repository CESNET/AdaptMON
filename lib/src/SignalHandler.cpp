/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Signal handling utilities for the amon library.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon/Error.hpp>
#include <amon/SignalHandler.hpp>
#include <amon/util/CharBuffer.hpp>

#include <cassert>
#include <csignal>
#include <cstring>

#include <cxxabi.h>
#include <linux/limits.h>

namespace amon {

static constexpr std::string_view sigToStr(int sigNum) noexcept
{
    constexpr const std::array signalNames = {
        "",          "SIGHUP",  "SIGINT",    "SIGQUIT", "SIGILL",   "SIGTRAP", "SIGABRT", "SIGBUS",
        "SIGFPE",    "SIGKILL", "SIGUSR1",   "SIGSEGV", "SIGUSR2",  "SIGPIPE", "SIGALRM", "SIGTERM",
        "SIGSTKFLT", "SIGCHLD", "SIGCONT",   "SIGSTOP", "SIGTSTP",  "SIGTTIN", "SIGTTOU", "SIGURG",
        "SIGXCPU",   "SIGXFSZ", "SIGVTALRM", "SIGPROF", "SIGWINCH", "SIGIO",   "SIGPWR",  "SIGSYS",
    };
    return sigNum > 0 && static_cast<size_t>(sigNum) < signalNames.size()
        ? signalNames[static_cast<size_t>(sigNum)]
        : "<unknown>";
}

static constexpr std::string_view sigCodeToStr(int sigNum, int sigCode) noexcept
{
    // Based on: https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/signal.h.html

    // Signal specific codes
    switch (sigNum) {
    case SIGILL:
        switch (sigCode) {
        case ILL_ILLOPC:
            return "ILL_ILLOPC - Illegal opcode";
        case ILL_ILLOPN:
            return "ILL_ILLOPN - Illegal operand";
        case ILL_ILLADR:
            return "ILL_ILLADR - Illegal addressing mode";
        case ILL_ILLTRP:
            return "ILL_ILLTRP - Illegal trap";
        case ILL_PRVOPC:
            return "ILL_PRVOPC - Privileged opcode";
        case ILL_PRVREG:
            return "ILL_PRVREG - Privileged register";
        case ILL_COPROC:
            return "ILL_COPROC - Coprocessor error";
        case ILL_BADSTK:
            return "ILL_BADSTK - Internal stack error";
        default:
            break;
        }
        break;
    case SIGFPE:
        switch (sigCode) {
        case FPE_INTDIV:
            return "FPE_INTDIV - Integer divide by zero";
        case FPE_INTOVF:
            return "FPE_INTOVF - Integer overflow";
        case FPE_FLTDIV:
            return "FPE_FLTDIV - Floating-point divide by zero";
        case FPE_FLTOVF:
            return "FPE_FLTOVF - Floating-point overflow";
        case FPE_FLTUND:
            return "FPE_FLTUND - Floating-point underflow";
        case FPE_FLTRES:
            return "FPE_FLTRES - Floating-point inexact result";
        case FPE_FLTINV:
            return "FPE_FLTINV - Invalid floating-point operation";
        case FPE_FLTSUB:
            return "FPE_FLTSUB - Subscript out of range";
        default:
            break;
        }
        break;
    case SIGSEGV:
        switch (sigCode) {
        case SEGV_MAPERR:
            return "SEGV_MAPERR - Address not mapped to object";
        case SEGV_ACCERR:
            return "SEGV_ACCERR - Invalid permissions for mapped object";
        default:
            break;
        }
        break;
    case SIGBUS:
        switch (sigCode) {
        case BUS_ADRALN:
            return "BUS_ADRALN - Invalid address alignment";
        case BUS_ADRERR:
            return "BUS_ADRERR - Nonexistent physical address";
        case BUS_OBJERR:
            return "BUS_OBJERR - Object-specific hardware error";
        default:
            break;
        }
        break;
    case SIGTRAP:
        switch (sigCode) {
        case TRAP_BRKPT:
            return "TRAP_BRKPT - Process breakpoint";
        case TRAP_TRACE:
            return "TRAP_TRACE - Process trace trap";
        default:
            break;
        }
        break;
    case SIGCLD:
        switch (sigCode) {
        case CLD_EXITED:
            return "CLD_EXITED - Child has exited";
        case CLD_KILLED:
            return "CLD_KILLED - Child has terminated abnormally and did not create a core file";
        case CLD_DUMPED:
            return "CLD_DUMPED - Child has terminated abnormally and created a core file";
        case CLD_TRAPPED:
            return "CLD_TRAPPED - Traced child has trapped";
        case CLD_STOPPED:
            return "CLD_STOPPED - Child has stopped";
        case CLD_CONTINUED:
            return "CLD_CONTINUED - Stopped child has continued";
        default:
            break;
        }
        break;
    case SIGPOLL:
        switch (sigCode) {
        case POLL_IN:
            return "POLL_IN - Data input available";
        case POLL_OUT:
            return "POLL_OUT - Output buffers available";
        case POLL_MSG:
            return "POLL_MSG - Input message available";
        case POLL_ERR:
            return "POLL_ERR - I/O error";
        case POLL_PRI:
            return "POLL_PRI - High priority input available";
        case POLL_HUP:
            return "POLL_HUP - Device disconnected";
        default:
            break;
        }
        break;
    default:
        break;
    }

    // Applicable to any signal
    switch (sigCode) {
    case SI_USER:
        return "SI_USER - Signal sent by kill()";
    case SI_KERNEL:
        return "SI_KERNEL - Signal sent by kernel";
    case SI_QUEUE:
        return "SI_QUEUE - Signal sent by sigqueue()";
    case SI_TIMER:
        return "SI_TIMER - Signal generated by expiration of a timer set by timer_settime()";
    case SI_ASYNCIO:
        return "SI_ASYNCIO - Signal generated by completion of an asynchronous I/O request";
    case SI_MESGQ:
        return "SI_MESGQ - Signal generated by arrival of a message on an empty message queue";
    default:
        break;
    }

    return "<unknown>";
}

static void rethrowSignal(int sigNum) noexcept
{
    struct sigaction action {};
    action.sa_handler = SIG_DFL;

    if (sigaction(sigNum, &action, nullptr) != 0) {
        util::CharBuffer printBuf;
        printBuf.append("FATAL: Failed to rethrow signal\n");
        write(STDERR_FILENO, printBuf.data(), printBuf.size());
        std::_Exit(EXIT_FAILURE);
    }

    kill(getpid(), sigNum);
}

struct __attribute__((packed)) SignalInfo {
    int sigNum;
};

static void dumpSigInfo(int sigNum, siginfo_t* sigInfo, util::CharBuffer<>& printBuf)
{
    printBuf.append("Signal: ", sigToStr(sigNum), " (", sigNum, ")\n");
    printBuf
        .append("Code:   ", sigCodeToStr(sigNum, sigInfo->si_code), " (", sigInfo->si_code, ")\n");
    if (sigInfo->si_errno != 0) {
        printBuf.append("Errno:  ", sigInfo->si_errno, "\n");
    }
    if (sigInfo->si_addr != nullptr) {
        printBuf.append("Addr:   0x");
        printBuf.append(reinterpret_cast<uintptr_t>(sigInfo->si_addr), 16);
        printBuf.append("\n");
    }
    if (sigInfo->si_pid != 0) {
        printBuf.append("PID:    ", sigInfo->si_pid, "\n");
    }
    if (sigInfo->si_uid != 0) {
        printBuf.append("UID:    ", sigInfo->si_uid, "\n");
    }
}

void deadlySignalCallback(int sigNum, siginfo_t* sigInfo, void* /*context*/) noexcept
{
    util::CharBuffer<4096> printBuf;
    printBuf.append(
        "--- SIGNAL RECEIVED ------------------------------------------------------------\n");
    dumpSigInfo(sigNum, sigInfo, printBuf);
    printBuf.append("\n");
    printBuf.append("Stacktrace:\n");
    StacktraceProvider::instance().dump(printBuf, /*skipFrames=*/1);
    printBuf.append("\n");
    write(STDERR_FILENO, printBuf.data(), printBuf.size());
    rethrowSignal(sigNum);
}

void debugSignalCallback(int sigNum, siginfo_t* sigInfo, void* /*context*/) noexcept
{
    util::CharBuffer<4096> printBuf;
    printBuf.append(
        "--- SIGNAL RECEIVED ------------------------------------------------------------\n");
    dumpSigInfo(sigNum, sigInfo, printBuf);
    printBuf.append("\n");
    printBuf.append("Stacktrace:\n");
    StacktraceProvider::instance().dump(printBuf, /*skipFrames=*/1);
    printBuf.append("\n");
    write(STDERR_FILENO, printBuf.data(), printBuf.size());
}

void returnSignalCallback(int sigNum, siginfo_t* /*sigInfo*/, void* /*context*/) noexcept
{
    SignalInfo info;
    info.sigNum = sigNum;

    // POSIX.1 says that writes of less than PIPE_BUF bytes must be atomic: the output data
    // is written to the pipe as a contiguous sequence.
    static_assert(sizeof(SignalInfo) <= PIPE_BUF);
    for (;;) {
        const ssize_t nWrote = write(SignalHandler::instance().m_pipeWr.get(), &info, sizeof(info));
        if (nWrote < 0) {
            if (errno == EAGAIN || errno == EINTR) {
                continue;
            }
            util::CharBuffer printBuf;
            printBuf.append("SIGNAL HANDLER ERROR: write() failed: ", errno, "\n");
            write(STDERR_FILENO, printBuf.data(), printBuf.size());
        }
        break;
    }
}

static void terminateHandler [[noreturn]] () noexcept
{
    util::CharBuffer printBuf;
    printBuf.append(
        "--- TERMINATE RECEIVED ---------------------------------------------------------\n");
    if (std::current_exception() != nullptr) {
        printBuf.append("std::terminate() called with an uncaught exception:\n");
        try {
            std::rethrow_exception(std::current_exception());
        } catch (const std::exception& exc) {
            printBuf.append("    ", exc.what(), "\n");
        } catch (...) {
            printBuf.append("    <unknown exception>\n");
        }
    } else {
        printBuf.append("std::terminate() called with no active exception\n");
    }
    printBuf.append("\n");
    printBuf.append("Stacktrace:\n");
    StacktraceProvider::instance().dump(printBuf, /*skipFrames=*/1);
    printBuf.append("\n");
    write(STDERR_FILENO, printBuf.data(), printBuf.size());
    std::abort();
}

SignalHandler& SignalHandler::instance()
{
    static SignalHandler instance;
    return instance;
}

SignalHandler::SignalHandler()
{
    int pipeFds[2];
    const int ret = pipe(pipeFds);
    if (ret != 0) {
        throw ErrnoError("pipe");
    }
    m_pipeRd = pipeFds[0];
    m_pipeWr = pipeFds[1];
}

int SignalHandler::waitForSignals()
{
    SignalInfo info;
    for (;;) {
        const ssize_t nBytes = read(instance().m_pipeRd.get(), &info, sizeof(info));
        if (nBytes < 0) {
            if (errno == EAGAIN || errno == EINTR) {
                continue;
            }
            throw ErrnoError("read", sizeof(info));
        }
        assert(nBytes == sizeof(info));
        break;
    }
    return info.sigNum;
}

void SignalHandler::installSigAction(int signal, SigActionCallback callback)
{
    struct sigaction action {};
    action.sa_handler = callback;

    const int ret = sigaction(signal, &action, nullptr);
    if (ret != 0) {
        throw ErrnoError("sigaction");
    }
}

void SignalHandler::installSigAction(int signal, SigActionExtendedCallback callback)
{
    struct sigaction action {};
    action.sa_flags = SA_SIGINFO;
    action.sa_sigaction = callback;

    const int ret = sigaction(signal, &action, nullptr);
    if (ret != 0) {
        throw ErrnoError("sigaction");
    }
}

void SignalHandler::installSigAction(std::initializer_list<int> signals, SigActionCallback callback)
{
    for (const int signal : signals) {
        installSigAction(signal, callback);
    }
}

void SignalHandler::installSigAction(
    std::initializer_list<int> signals,
    SigActionExtendedCallback callback)
{
    for (const int signal : signals) {
        installSigAction(signal, callback);
    }
}

void SignalHandler::install(bool willUseWaitForSignals)
{
    int ret;

    // Unblock all signals
    sigset_t sigSet;
    ret = sigemptyset(&sigSet);
    if (ret != 0) {
        throw ErrnoError("sigemptyset");
    }
    ret = pthread_sigmask(SIG_SETMASK, &sigSet, nullptr);
    if (ret != 0) {
        throw Error("pthread_sigmask() failed: {}", ret);
    }

    if (willUseWaitForSignals) {
        // Termination/configuration signals
        // (signals that we only want the main thread to handle)
        installSigAction({SIGTERM, SIGINT, SIGQUIT, SIGUSR1}, returnSignalCallback);
    }

    // Deadly signals
    // (signals that we want the specific thread to handle)
    installSigAction({SIGILL, SIGBUS, SIGFPE, SIGSEGV, SIGSYS}, deadlySignalCallback);

    // Special debug signal
    // (signal that we want the specific thread to handle)
    installSigAction(SIGUSR2, debugSignalCallback);

    // Terminate handler
    // (technically not a signal, but it fits here)
    std::set_terminate(terminateHandler);
}

} // namespace amon
