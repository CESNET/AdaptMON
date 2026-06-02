/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Name generator implementation.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <atomic>
#include <format>
#include <string>

namespace amon::util {

/**
 * @brief Name generator with increasing atomic counter.
 *
 * This is useful for situations when you need unique names, for example DPDK resources which take a
 * "name" argument. In such case you will want to hold one instance of NameGenerator (e.g. as a
 * static variable) and get a new name every time you create a new object.
 *
 * @warning If you are using this to generate names for DPDK components, make sure the name is short
 * enough! The general limit is 32 characters but for certain components can be as low as ~27 due to
 * certain automatically added prefixes. This can lead to very weird and hard to debug errors when
 * instantiating the components.
 */
class NameGenerator {
public:
    /**
     * @brief Construct a NameGenerator instance.
     * @param fmt The format string used for name construction. Must contain '{}' which will be
     * substituted for the ID.
     */
    NameGenerator(std::format_string<uint64_t> fmt);

    /**
     * @brief Generate the next name.
     * @return The name according to the specified format and the next ID in line.
     */
    std::string next() { return std::format(m_fmt, m_counter.fetch_add(1)); }

private:
    std::format_string<uint64_t> m_fmt;
    std::atomic<uint64_t> m_counter {0};
};

} // namespace amon::util
