/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief Utils for numeric operations.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstdint>
#include <limits>
#include <stdexcept>

namespace amon::util::numeric {

/**
 * @brief Safely add two uint64_t values, throwing on overflow.
 * @param a First value.
 * @param b Second value.
 * @return Sum of a and b.
 * @throws std::overflow_error if addition would overflow.
 */
inline uint64_t safeAdd(uint64_t a, uint64_t b)
{
    if (a > std::numeric_limits<uint64_t>::max() - b) {
        throw std::overflow_error("addition would overflow");
    }
    return a + b;
}

/**
 * @brief Safely multiply two uint64_t values, throwing on overflow.
 * @param a First value.
 * @param b Second value.
 * @return Product of a and b.
 * @throws std::overflow_error if multiplication would overflow.
 */
inline uint64_t safeMul(uint64_t a, uint64_t b)
{
    if (b != 0 && a > std::numeric_limits<uint64_t>::max() / b) {
        throw std::overflow_error("multiplication would overflow");
    }
    return a * b;
}

} // namespace amon::util::numeric
