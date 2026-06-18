/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Wrapper around strerror_r.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// #include <amon/Strerror.hpp> missing on purpose

// Make sure that XSI-version is used.
#undef _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L // NOLINT

#include <cstring>

namespace amon {

// Leave this alone in a separate cpp or bad things happen...

int strerror(int errNum, char* buf, size_t bufLen) // NOLINT(misc-use-internal-linkage)
{
    return ::strerror_r(errNum, buf, bufLen);
}

} // namespace amon
