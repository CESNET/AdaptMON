/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Error handling classes and utilities for the amon library.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/Stacktrace.hpp>

#include <exception>
#include <format>
#include <sstream>
#include <string_view>

namespace amon {

/**
 * @brief Basic error class for representing error messages.
 *
 * Inherits from std::exception and provides constructors for
 * simple and formatted error messages.
 */
class BasicError : public std::exception {
public:
    /**
     * @brief Constructs a BasicError with a C-string message.
     * @param message Error message.
     */
    BasicError(const char* message)
        : m_message(message)
    {
    }

    /**
     * @brief Constructs a BasicError with a formatted message.
     *
     * @tparam Args Types of arguments to format into the message.
     * @param fmt Format string compatible with std::format.
     * @param args Arguments to be formatted into the message.
     */
    template <typename... Args>
    BasicError(std::format_string<Args...> fmt, Args&&... args)
        : m_message(std::format(fmt, std::forward<Args>(args)...))
    {
    }

    /**
     * @brief Returns the error message.
     * @return C-string containing the error message.
     */
    const char* what() const noexcept override { return m_message.c_str(); }

private:
    std::string m_message;
};

/**
 * @brief Error class that includes stacktrace information.
 *
 * Inherits from BasicError and adds stacktrace capturing.
 */
class Error : public BasicError {
public:
    /**
     * @brief Constructs an Error with a C-string message.
     * @param message Error message.
     */
    Error(const char* message)
        : BasicError(message)
        , m_stacktrace(StacktraceProvider::createStacktrace(1))
    {
    }

    /**
     * @brief Constructs an Error with a formatted message.
     *
     * This constructor uses a format string and arguments to create
     * a detailed error message.
     *
     * @tparam Args Types of arguments to format into the message.
     * @param fmt Format string compatible with std::format.
     * @param args Arguments to be formatted into the message.
     */
    template <typename... Args>
    Error(std::format_string<Args...> fmt, Args&&... args)
        : BasicError(fmt, std::forward<Args>(args)...)
        , m_stacktrace(StacktraceProvider::createStacktrace(1))
    {
    }

    /**
     * @brief Returns the captured stacktrace.
     * @return Reference to the stacktrace object.
     */
    const Stacktrace& stacktrace() const { return m_stacktrace; }

private:
    Stacktrace m_stacktrace;
};

/**
 * @brief Converts an error number to a human-readable string.
 * @param errNum Error number (errno).
 * @return String describing the error.
 */
std::string errnoToString(int errNum);

/// @cond DO_NOT_DOCUMENT
namespace detail {

template <typename Arg, typename... Args>
static void joinHelper(std::stringstream& ss, Arg&& arg, Args&&... args)
{
    if constexpr (std::is_convertible_v<Arg, std::string_view>) {
        ss << '"' << arg << '"';
    } else {
        ss << arg;
    }

    if constexpr (sizeof...(args) > 0) {
        ss << ", ";
        joinHelper(ss, std::forward<Args>(args)...);
    }
}

template <typename... Args>
static std::string join(Args&&... args)
{
    if constexpr (sizeof...(args) > 0) {
        std::stringstream ss;
        joinHelper(ss, std::forward<Args>(args)...);
        return ss.str();
    } else {
        return "";
    }
}

} // namespace detail

/// @endcond

/**
 * @brief Error class for representing errors related to errno values.
 *
 * Inherits from Error and constructs a message including the function name,
 * error string, and error number.
 */
class ErrnoError : public Error {
public:
    /**
     * @brief Constructs an ErrnoError with a specific error number.
     *
     * @tparam FunctionArgs Variadic arguments representing function parameters.
     * @param errNum The error number (errno value).
     * @param functionName The name of the function that failed.
     * @param functionArgs Arguments passed to the failed function.
     *
     * Formats the error message as: "{functionName}({functionArgs}) has failed: {errnoToString}
     * ({errNum})"
     */
    template <typename... FunctionArgs>
    ErrnoError(int errNum, std::string_view functionName, FunctionArgs&&... functionArgs)
        : Error(
              "{}({}) has failed: {} ({})",
              functionName,
              detail::join(std::forward<FunctionArgs>(functionArgs)...),
              errnoToString(errNum),
              errNum)
    {
    }

    /**
     * @brief Constructs an ErrnoError using the current errno value.
     *
     * @tparam FunctionArgs Variadic arguments representing function parameters.
     * @param functionName The name of the function that failed.
     * @param functionArgs Arguments passed to the failed function.
     *
     * Uses the current value of errno for the error number.
     */
    template <typename... FunctionArgs>
    ErrnoError(std::string_view functionName, FunctionArgs&&... functionArgs)
        : ErrnoError(errno, functionName, std::forward<FunctionArgs>(functionArgs)...)
    {
    }
};

} // namespace amon
