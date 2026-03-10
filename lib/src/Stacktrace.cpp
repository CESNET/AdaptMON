/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Stacktrace functionality implementation
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "amon/util/CharBuffer.hpp"

#include <amon/Stacktrace.hpp>

#include <cstring>
#include <iomanip>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <backtrace.h>
#include <cxxabi.h>

namespace amon {

namespace stacktrace_detail {

namespace {

struct CallbackContext {
    backtrace_state* state;

    std::vector<StacktraceFrame>* frames;

    std::optional<int> errorNum;
    util::CharBuffer<256> errorMsg;

    std::exception_ptr exc = nullptr;
};

} // namespace

static void demangle(std::string& name)
{
    if (name.starts_with("_Z")) {
        int status;
        const std::unique_ptr<char, void (*)(void*)> res {
            abi::__cxa_demangle(name.c_str(), nullptr, nullptr, &status),
            std::free};
        if (status == 0) {
            name = res.get();
        }
    }
}

static void backtraceErrorCallback(void* data, const char* msg, int errNum) noexcept
{
    CallbackContext& ctx = *reinterpret_cast<CallbackContext*>(data);

    ctx.errorMsg.append(std::string_view {msg, std::strlen(msg)});
    ctx.errorNum = errNum;
}

static void backtraceSyminfoCallback(
    void* data,
    uintptr_t pc,
    const char* symName,
    uintptr_t symVal,
    uintptr_t /*symSize*/) noexcept
{
    CallbackContext& ctx = *reinterpret_cast<CallbackContext*>(data);

    try {
        StacktraceFrame& frame = ctx.frames->back();
        if (symName != nullptr) {
            frame.symbol = symName;
            demangle(frame.symbol);
            frame.offsetFromSymbol = pc - symVal;
        }
    } catch (...) {
        ctx.exc = std::current_exception();
    }
}
static int backtraceFullCallback(
    void* data,
    uintptr_t pc,
    const char* filename,
    int line,
    const char* function) noexcept
{
    CallbackContext& ctx = *reinterpret_cast<CallbackContext*>(data);

    try {
        StacktraceFrame& frame = ctx.frames->emplace_back();

        backtrace_syminfo(ctx.state, pc, backtraceSyminfoCallback, backtraceErrorCallback, data);

        if (function != nullptr) {
            frame.function = function;
            demangle(frame.function);
        }

        if (filename != nullptr) {
            frame.filename = filename;
            frame.line = line;
        }

        frame.pc = pc;

    } catch (...) {
        // C callback - exceptions cannot be thrown!
        ctx.exc = std::current_exception();
    }

    if (ctx.exc != nullptr) {
        return 1; // Stop early
    }

    return 0;
}

} // namespace stacktrace_detail

namespace dump_detail {

namespace {

struct CallbackContext {
    util::CharBuffer<>* printBuf;
    size_t frameCount = 0;
    backtrace_state* state;
};

} // namespace

static void backtraceErrorCallback(void* data, const char* msg, int errNum) noexcept
{
    auto& ctx = *reinterpret_cast<CallbackContext*>(data);
    ctx.printBuf->append("backtrace_full() failed: ", msg, "(code: ", errNum, ")\n");
}

static void backtraceSyminfoCallback(
    void* data,
    uintptr_t pc,
    const char* symName,
    uintptr_t symVal,
    uintptr_t /*symSize*/) noexcept
{
    auto& ctx = *reinterpret_cast<CallbackContext*>(data);
    if (symName != nullptr) {
        ctx.printBuf->append(symName, "+0x");
        ctx.printBuf->append(pc - symVal, 16);
    } else {
        ctx.printBuf->append("???+0x0");
    }
}

static int backtraceFullCallback(
    void* data,
    uintptr_t pc,
    const char* filename,
    int line,
    const char* function) noexcept
{
    auto& ctx = *reinterpret_cast<CallbackContext*>(data);

    ctx.frameCount++;
    ctx.printBuf->append(ctx.frameCount, 10, 2, ' ');
    ctx.printBuf->append(") ");

    ctx.printBuf->append("0x");
    ctx.printBuf->append(pc, 16, 16);
    ctx.printBuf->append(" ");
    backtrace_syminfo(ctx.state, pc, backtraceSyminfoCallback, backtraceErrorCallback, data);
    ctx.printBuf->append("\n");

    ctx.printBuf->append("    in ");
    if (function != nullptr) {
        ctx.printBuf->append(function, "()");
    } else {
        ctx.printBuf->append("???", "()");
    }

    ctx.printBuf->append(" at ");
    if (filename != nullptr) {
        ctx.printBuf->append(filename, ":", line);
    } else {
        ctx.printBuf->append("???:0");
    }
    ctx.printBuf->append("\n");

    return 0;
}

} // namespace dump_detail

StacktraceProvider& StacktraceProvider::instance()
{
    static StacktraceProvider instance;
    return instance;
}

StacktraceProvider::StacktraceProvider()
{
    stacktrace_detail::CallbackContext ctx;
    m_backtraceState
        = backtrace_create_state(nullptr, 1, stacktrace_detail::backtraceErrorCallback, &ctx);
    // There is no backtrace_state_free. This should be called once per program and reused.
    if (m_backtraceState == nullptr) {
        if (!ctx.errorNum) {
            throw std::runtime_error("backtrace_create_state() failed: unknown error");
        }
        throw std::runtime_error(
            "backtrace_create_state() failed: "
            + std::string {ctx.errorMsg.data(), ctx.errorMsg.size()} + " (code "
            + std::to_string(*ctx.errorNum) + ")");
    }
}

Stacktrace StacktraceProvider::createStacktrace(int skipFrames)
{
    Stacktrace stacktrace;

    stacktrace_detail::CallbackContext ctx;
    ctx.state = reinterpret_cast<backtrace_state*>(m_backtraceState);
    ctx.frames = &stacktrace.m_frames;
    backtrace_full(
        reinterpret_cast<backtrace_state*>(m_backtraceState),
        skipFrames + 1,
        &stacktrace_detail::backtraceFullCallback,
        &stacktrace_detail::backtraceErrorCallback,
        &ctx);

    if (ctx.exc) {
        std::rethrow_exception(ctx.exc);
    }

    if (ctx.errorNum) {
        throw std::runtime_error(
            "backtrace_full() failed: " + std::string {ctx.errorMsg.data(), ctx.errorMsg.size()}
            + " (code " + std::to_string(*ctx.errorNum) + ")");
    }

    return stacktrace;
}

void StacktraceProvider::dump(util::CharBuffer<>& printBuf, int skipFrames)
{
    dump_detail::CallbackContext ctx;
    ctx.state = reinterpret_cast<backtrace_state*>(m_backtraceState);
    ctx.printBuf = &printBuf;
    backtrace_full(
        reinterpret_cast<backtrace_state*>(m_backtraceState),
        skipFrames + 1,
        &dump_detail::backtraceFullCallback,
        &dump_detail::backtraceErrorCallback,
        &ctx);
    if (ctx.frameCount == 0) {
        ctx.printBuf->append("    N/A\n");
    }
}

std::string Stacktrace::str() const
{
    size_t frameCount = 0;
    std::stringstream ss;
    for (const auto& frame : frames()) {
        ss << std::dec << std::setfill(' ') << std::setw(2) << (frameCount + 1) << ") ";
        ss << "0x" << std::hex << std::setfill('0') << std::setw(16) << frame.pc << " "
           << frame.symbol << "+0x" << frame.offsetFromSymbol << std::dec << "\n";
        ss << "    in " << (!frame.function.empty() ? frame.function : "???") << "()" << " at "
           << (!frame.filename.empty() ? frame.filename : "???") << ":" << frame.line << "\n";
        frameCount++;
    }

    return ss.str();
}

} // namespace amon
