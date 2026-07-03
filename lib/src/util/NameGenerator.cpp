/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Name generator implementation.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon/amon.hpp>
#include <amon/util/NameGenerator.hpp>

namespace amon::util {

NameGenerator::NameGenerator(std::format_string<uint64_t> fmt)
    : m_fmt(fmt)
{
    AMON_ASSERT(m_fmt.get().contains("{}"), "fmt must contain '{{}}'");
}

} // namespace amon::util
