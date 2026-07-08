/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief DPDK NUMA socket ID representation.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/util/Formatter.hpp>
#include <amon/util/numeric.hpp>

namespace amon::dpdk {

/**
 * @brief Represents a DPDK NUMA socket ID.
 *
 * SocketID wraps the integer socket identifiers used by DPDK APIs and provides a
 * named value for the DPDK "any socket" sentinel. Regular socket identifiers are
 * non-negative values.
 */
class SocketID {
public:
    /**
     * @brief Sentinel value selecting any available NUMA socket.
     */
    static const SocketID ANY;

    /**
     * @brief Construct a concrete socket ID.
     * @param value NUMA socket identifier.
     */
    explicit SocketID(unsigned value)
        : SocketID(amon::util::numeric::safeCast<int>(value))
    {
    }

    /**
     * @brief Convert the socket ID to a signed integer value.
     * @return Socket ID value compatible with DPDK API.
     */
    operator int() const noexcept { return m_value; }

    /**
     * @brief Convert the socket ID to an unsigned integer value.
     * @return Socket ID value compatible with DPDK API.
     */
    operator unsigned() const noexcept { return static_cast<unsigned>(m_value); }

    /**
     * @brief Equal comparison operator.
     * @param other Value to compare against.
     * @return True or false.
     */
    bool operator==(const amon::dpdk::SocketID& other) const noexcept
    {
        return m_value == other.m_value;
    }

private:
    int m_value;

    SocketID(int value)
        : m_value(value)
    {
    }
};

/**
 * @brief Format SocketID for output.
 * @param value SocketID to format.
 * @param fmt Formatter to write to.
 */
inline void format(const SocketID& value, amon::util::Formatter& fmt)
{
    if (value == SocketID::ANY) {
        fmt.format("SocketID::ANY");
    } else {
        fmt.format("SocketID({})", static_cast<unsigned>(value));
    }
}

} // namespace amon::dpdk
