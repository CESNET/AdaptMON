/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Configuration parser error type
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 */

#pragma once

#include <amon/Error.hpp>
#include <amon/conf/Node.hpp>

#include <format>
#include <memory>
#include <string>
#include <utility>

namespace amon::conf {

/**
 * @brief Exception type for configuration errors.
 *
 * Stores the node where the error occurred for detailed error reporting.
 */
class ConfigError : public Error {
public:
    /**
     * @brief Construct a ConfigError with a node and formatted error message.
     * @tparam Args Types of the arguments for formatting the error message.
     * @param node The node where the error occurred.
     * @param fmt The format string for the error message.
     * @param args Arguments to be formatted into the error message.
     */
    template <typename... Args>
    ConfigError(const Node& node, std::format_string<Args...> fmt, Args&&... args)
        : Error(fmt, std::forward<Args...>(args)...)
        , m_line(node.line())
        , m_col(node.col())
        , m_source(node.source())
    {
    }

    /**
     * @brief Construct a ConfigError with line, column, source, and formatted error message.
     * @tparam Args Types of the arguments for formatting the error message.
     * @param line The line number where the error occurred.
     * @param col The column number where the error occurred.
     * @param source The source string where the error occurred.
     * @param fmt The format string for the error message.
     * @param args Arguments to be formatted into the error message.
     */
    template <typename... Args>
    ConfigError(
        unsigned line,
        unsigned col,
        const std::shared_ptr<std::string>& source,
        std::format_string<Args...> fmt,
        Args&&... args)
        : Error(fmt, std::forward<Args...>(args)...)
        , m_line(line)
        , m_col(col)
        , m_source(source)
    {
    }

    /**
     * @brief Format a pretty error message for a configuration error.
     * @return The formatted string.
     */
    std::string toString() const;

private:
    unsigned m_line;
    unsigned m_col;
    std::shared_ptr<std::string> m_source;
};

} // namespace amon::conf
