/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Pipeline stream implementation.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon/dpdk/pipeline/Stream.hpp>

namespace amon::dpdk::pipeline {

Stream::Stream(StreamContext context)
    : m_context(context)
{
}

} // namespace amon::dpdk::pipeline
