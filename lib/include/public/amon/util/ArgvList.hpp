/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Utility class to manage argc/argv style argument lists.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <string>
#include <vector>

namespace amon::util {

/**
 * @brief Utility class to manage argc/argv style argument lists.
 *
 * Stores arguments as strings and provides access to argc and argv
 * in a format compatible with C-style APIs.
 */
class ArgvList {
public:
    /**
     * @brief Constructs an ArgvList from a vector of arguments.
     * @param args Vector of argument strings.
     */
    ArgvList(const std::vector<std::string>& args);

    /**
     * @brief Returns the number of arguments.
     * @return Argument count (argc).
     */
    int argc() const { return static_cast<int>(m_storage.size()); }

    /**
     * @brief Returns the argument vector (argv).
     * @return Pointer to array of C-string arguments.
     */
    char** argv() { return m_argv.data(); }

private:
    std::vector<std::string> m_storage;
    std::vector<char*> m_argv;
};

} // namespace amon::util
