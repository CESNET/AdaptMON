/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief Logging interface for AdaptMON.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @note To simply get a logger instance, use `amon::LoggerProvider::get("loggerName")`.
 */

#pragma once

#include <amon/types/Singleton.hpp>

#include <format>
#include <memory>
#include <mutex>
#include <source_location>
#include <string>
#include <string_view>

// NOLINTBEGIN(readability-identifier-naming)
namespace spdlog {
/// Early definition of spdlog::logger to avoid including spdlog headers in the header file.
class logger;
} // namespace spdlog
// NOLINTEND(readability-identifier-naming)

namespace amon {

/**
 * @brief Logging levels supported by the AdaptMON logger.
 */
enum class LoggerLevel {
    TRACE, ///< Trace level (most verbose)
    DEBUG, ///< Debug level
    INFO, ///< Information level
    WARN, ///< Warning level
    ERR, ///< Error level
    CRITICAL, ///< Critical error level
    OFF, ///< Logging disabled
};

/**
 * @brief Format string wrapper for logging.
 *
 * This class is used to pass format strings to the logger methods.
 * It captures the format string and the source location and allows
 * to use variadic args in the logger methods.
 */
class LoggerFmtString {
public:
    /**
     * @brief Construct a LoggerFmtString with a format string and source location.
     * @param fmt Format string to use in logging.
     * @param loc Source location for logging (default is current location).
     */
    LoggerFmtString(const char* fmt, std::source_location loc = std::source_location::current())
        : m_fmt(fmt)
        , m_loc(loc)
    {
    }

private:
    friend class Logger;
    std::string_view m_fmt;
    std::source_location m_loc;
};

/**
 * @brief Logger class for AdaptMON, provides various logging methods.
 *
 * Use this class to log messages at different severity levels. Each log entry
 * includes the message, file name, and line number.
 */
class Logger {
public:
    /**
     * @brief Set the logging level for this logger.
     * @param level The minimum LogLevel to log.
     */
    void setLevel(LoggerLevel level);

    /**
     * @brief Log a trace-level message.
     *
     * @param fmt Format string for the log message.
     * @param args Arguments to format the message.
     *
     * Usage:
     * @code
     * logger.trace("Processing value: {}", value);
     * @endcode
     */
    template <typename... Args>
    void trace(LoggerFmtString fmt, Args&&... args) const
    {
        log(LoggerLevel::TRACE, fmt, std::make_format_args(args...));
    }

    /**
     * @brief Log an info-level message.
     *
     * @param fmt Format string for the log message.
     * @param args Arguments to format the message.
     *
     * Usage:
     * @code
     * logger.info("Started process: {}", processName);
     * @endcode
     */
    template <typename... Args>
    void info(LoggerFmtString fmt, Args&&... args) const
    {
        log(LoggerLevel::INFO, fmt, std::make_format_args(args...));
    }

    /**
     * @brief Log an error-level message.
     *
     * @param fmt Format string for the log message.
     * @param args Arguments to format the message.
     *
     * Usage:
     * @code
     * logger.error("Failed to open file: {}", filename);
     * @endcode
     */
    template <typename... Args>
    void error(LoggerFmtString fmt, Args&&... args) const
    {
        log(LoggerLevel::ERR, fmt, std::make_format_args(args...));
    }

    /**
     * @brief Log a debug-level message.
     *
     * @param fmt Format string for the log message.
     * @param args Arguments to format the message.
     *
     * Usage:
     * @code
     * logger.debug("Counter value: {}", counter);
     * @endcode
     */
    template <typename... Args>
    void debug(LoggerFmtString fmt, Args&&... args) const
    {
        log(LoggerLevel::DEBUG, fmt, std::make_format_args(args...));
    }

    /**
     * @brief Log a warning-level message.
     *
     * @param fmt Format string for the log message.
     * @param args Arguments to format the message.
     *
     * Usage:
     * @code
     * logger.warn("CPU {} was already used.", cpuNumber);
     * @endcode
     */
    template <typename... Args>
    void warn(LoggerFmtString fmt, Args&&... args) const
    {
        log(LoggerLevel::WARN, fmt, std::make_format_args(args...));
    }

    /**
     * @brief Log a critical-level message.
     *
     * @param fmt Format string for the log message.
     * @param args Arguments to format the message.
     *
     * Usage:
     * @code
     * logger.critical("System failure: {}", details);
     * @endcode
     */
    template <typename... Args>
    void critical(LoggerFmtString fmt, Args&&... args) const
    {
        log(LoggerLevel::CRITICAL, fmt, std::make_format_args(args...));
    }

private:
    /**
     * @brief Construct a Logger with a given name.
     * @param name Name of the logger instance.
     */
    explicit Logger(const std::string& name);

    /**
     * @brief General method to log messages at a specific level.
     * @param level Logging level to use.
     * @param fmt Format string for the message.
     * @param args Arguments to format the message.
     */
    void log(LoggerLevel level, LoggerFmtString fmt, std::format_args&& args) const;

    friend class LoggerProvider;
    std::shared_ptr<spdlog::logger> m_logger; ///< Underlying spdlog logger instance.
};

/**
 * @brief Singleton provider for Logger instances.
 *
 * Use this class to obtain named Logger instances and to set global logging options.
 */
class LoggerProvider final : private types::Singleton<LoggerProvider> {
public:
    /**
     * @brief Get a Logger instance by name.
     * @param name Name of the logger.
     * @return Logger object.
     */
    static Logger get(const std::string& name);

    /**
     * @brief Set the global logging level for all loggers.
     * @param level The minimum LogLevel to log.
     * @note This also affects loggers that have already been created.
     *       Even loggers with explicitly set levels.
     */
    static void setGlobalLogLevel(LoggerLevel level);

private:
    friend class types::Singleton<LoggerProvider>;

    LoggerProvider();
    Logger getImpl(const std::string& name);
    void setGlobalLogLevelImpl(LoggerLevel level);

    std::mutex m_registryMutex;
};

} // namespace amon
