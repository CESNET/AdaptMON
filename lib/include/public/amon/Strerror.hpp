/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Wrapper around strerror_r.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * This exists to isolate the side effects of feature-test macros required
 * to guarantee the XSI-compliant (POSIX) version of strerror_r, which returns int.
 * Forcing _POSIX_C_SOURCE and undefining _GNU_SOURCE must happen before including
 * `<cstring>`. If this were done in a common header or a widely-used source file, it
 * would leak these macro changes into other parts of the codebase, potentially
 * changing the behavior of standard library functions or causing the GNU variant
 * of strerror_r (which returns char*) to be expected elsewhere. By confining the
 * macro definitions and the strerror_r call to Strerror.cpp, we safely contain
 * these platform-specific side effects.
 */

#pragma once

#include <cstddef>

namespace amon {

/**
 * @brief Wrapper around strerror_r.
 *
 * @param errNum Errno value to convert to string.
 * @param buf User supplied buffer for the message.
 * @param bufLen Size of the buffer.
 * @return 0 on success, otherwise failure.
 */
int strerror(int errNum, char* buf, size_t bufLen);

} // namespace amon
