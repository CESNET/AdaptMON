/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief Logging interface for AdaptMON.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon/Logger.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <spdlog/cfg/env.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#pragma GCC diagnostic pop

namespace amon {

constexpr static LoggerLevel DEFAULT_LOGGER_LEVEL
    = LoggerLevel::INFO; ///< Default logging level for init
constexpr static const char* LOGGER_PATTERN = "[%T] [%^%L%$] %n: %v"; ///< Pattern for log messages

// Provided data is: user message, file name, line number
constexpr static const char* MESSAGE_PATTERN
    = "{} ({}:{})"; ///< Pattern for log messages with code location

constexpr static std::string_view pathToFilename(std::string_view path)
{
    const std::size_t pos = path.find_last_of('/');
    return (pos == std::string::npos) ? path : path.substr(pos + 1);
}

static spdlog::level::level_enum toSpdLogLevel(LoggerLevel level)
{
    switch (level) {
    case LoggerLevel::TRACE:
        return spdlog::level::trace;
    case LoggerLevel::DEBUG:
        return spdlog::level::debug;
    case LoggerLevel::INFO:
        return spdlog::level::info;
    case LoggerLevel::WARN:
        return spdlog::level::warn;
    case LoggerLevel::ERR:
        return spdlog::level::err;
    case LoggerLevel::CRITICAL:
        return spdlog::level::critical;
    case LoggerLevel::OFF:
        return spdlog::level::off;
    }
    throw std::logic_error(
        "Invalid log level provided: " + std::to_string(static_cast<int>(level)));
}

void Logger::setLevel(LoggerLevel level)
{
    m_logger->set_level(toSpdLogLevel(level));
}

Logger::Logger(const std::string& name)
{
    m_logger = spdlog::get(name);
    if (!m_logger) {
        m_logger = spdlog::stdout_color_mt(name);
    }
}

void Logger::log(LoggerLevel level, LoggerFmtString fmt, std::format_args&& args)
{
    auto message = std::vformat(fmt.m_fmt, args);
    std::string_view filename = pathToFilename(fmt.m_loc.file_name());

    switch (level) {
    case LoggerLevel::TRACE:
        m_logger->trace(MESSAGE_PATTERN, message, filename, fmt.m_loc.line());
        return;
    case LoggerLevel::DEBUG:
        m_logger->debug(MESSAGE_PATTERN, message, filename, fmt.m_loc.line());
        return;
    case LoggerLevel::INFO:
        m_logger->info(MESSAGE_PATTERN, message, filename, fmt.m_loc.line());
        return;
    case LoggerLevel::WARN:
        m_logger->warn(MESSAGE_PATTERN, message, filename, fmt.m_loc.line());
        return;
    case LoggerLevel::ERR:
        m_logger->error(MESSAGE_PATTERN, message, filename, fmt.m_loc.line());
        return;
    case LoggerLevel::CRITICAL:
        m_logger->critical(MESSAGE_PATTERN, message, filename, fmt.m_loc.line());
        return;
    case LoggerLevel::OFF:
        break;
    }

    throw std::logic_error(
        "Invalid log level provided: " + std::to_string(static_cast<int>(level)));
}

Logger LoggerProvider::get(const std::string& name)
{
    return LoggerProvider::instance().getImpl(name);
}

void LoggerProvider::setGlobalLogLevel(LoggerLevel level)
{
    LoggerProvider::instance().setGlobalLogLevelImpl(level);
}

LoggerProvider::LoggerProvider()
{
    spdlog::set_pattern(LOGGER_PATTERN);
    setGlobalLogLevel(DEFAULT_LOGGER_LEVEL);
}

// NOLINTBEGIN(readability-convert-member-functions-to-static)
Logger LoggerProvider::getImpl(const std::string& name)
{
    return Logger(name);
}

void LoggerProvider::setGlobalLogLevelImpl(LoggerLevel level)
{
    spdlog::set_level(toSpdLogLevel(level));
}
// NOLINTEND(readability-convert-member-functions-to-static)

} // namespace amon
