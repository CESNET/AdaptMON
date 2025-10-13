/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief A timestamp class
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/util/numeric.hpp>

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <sys/time.h>

namespace amon::types {

using namespace util::numeric;

/**
 * @brief Time units for timestamp representation and conversion.
 */
enum class TimeUnit : int {
    NANOSECONDS = 1,
    MICROSECONDS = 2,
    MILLISECONDS = 3,
    SECONDS = 4,
};

/**
 * @brief Class providing a timestamp representation with operators and additional functionality.
 *
 * Represents a timestamp in nanoseconds and provides conversion, arithmetic, and formatting
 * utilities. Timestamp is in UNIX Epoch format, nanoseconds from 1 January 1970.
 */
class Timestamp {
public:
    /**
     * @brief Construct a new zero timestamp object.
     */
    Timestamp() = default;

    /**
     * @brief Construct a new Timestamp object.
     * @param nanosec Timestamp in nanoseconds (since the Unix epoch).
     */
    Timestamp(uint64_t nanosec)
        : m_nanosec(nanosec)
    {
    }

    /**
     * @brief Construct a new Timestamp object from a value of the specified time unit.
     * Value correspond to SourceUnit since the Unix epoch.
     * @tparam SourceUnit Time unit of the input value.
     * @param value Value in SourceUnit.
     * @return Timestamp object representing the value in nanoseconds.
     */
    template <TimeUnit SourceUnit>
    static Timestamp from(uint64_t value)
    {
        const uint64_t multiplier
            = unitsInSecond(TimeUnit::NANOSECONDS) / unitsInSecond(SourceUnit);
        const uint64_t nanosec = safeMul(value, multiplier);
        return {nanosec};
    }

    /**
     * @brief Construct a new Timestamp object from a timeval structure.
     * @param timeVal Timeval structure containing seconds and microseconds.
     * @return Timestamp object representing the total nanoseconds.
     */
    static Timestamp fromTimeval(timeval timeVal)
    {
        if (timeVal.tv_sec < 0 || timeVal.tv_usec < 0) {
            throw std::invalid_argument("Only positive timeval are accepted.");
        }
        const uint64_t totalNanoseconds = safeAdd(
            safeMul(static_cast<uint64_t>(timeVal.tv_sec), unitsInSecond(TimeUnit::NANOSECONDS)),
            safeMul(static_cast<uint64_t>(timeVal.tv_usec), 1000));
        return {totalNanoseconds};
    }

    /**
     * @brief Get timestamp in nanoseconds (since the Unix epoch).
     * @return Timestamp value in nanoseconds.
     */
    uint64_t nanoseconds() const { return m_nanosec; }

    /**
     * @brief Compare timestamps.
     * @param other Timestamp to compare with.
     * @return Comparison result.
     */
    auto operator<=>(const Timestamp& other) const = default;

    /**
     * @brief Convert timestamp to string in the specified unit.
     * @tparam PrintUnit Time unit for string representation.
     * @return String representation of the timestamp.
     */
    template <TimeUnit PrintUnit>
    std::string toString() const
    {
        std::stringstream ss;

        if constexpr (PrintUnit == TimeUnit::NANOSECONDS) {
            ss << m_nanosec;
        } else {
            const uint64_t divisor
                = unitsInSecond(TimeUnit::NANOSECONDS) / unitsInSecond(PrintUnit);

            const uint64_t value = m_nanosec;
            const uint64_t whole = value / divisor;
            const uint64_t decimal = value % divisor;
            const int decimalPlaces = (int(PrintUnit) - int(TimeUnit::NANOSECONDS)) * 3;

            ss << whole << "." << std::setfill('0') << std::setw(decimalPlaces) << decimal;
        }

        return ss.str();
    }

    /**
     * @brief Convert timestamp to the specified time unit.
     * @tparam PrintUnit Target time unit for conversion.
     * @return Timestamp value in the specified unit.
     */
    template <TimeUnit PrintUnit>
    constexpr uint64_t convertTo() const noexcept
    {
        if constexpr (PrintUnit == TimeUnit::NANOSECONDS) {
            return m_nanosec;
        }
        constexpr uint64_t divisor
            = unitsInSecond(TimeUnit::NANOSECONDS) / unitsInSecond(PrintUnit);
        return m_nanosec / divisor;
    }

    /**
     * @brief Convert timestamp to the specified time unit.
     * @param unit Target time unit for conversion.
     * @return Timestamp value in the specified unit.
     */
    uint64_t convertTo(TimeUnit unit) const noexcept
    {
        if (unit == TimeUnit::NANOSECONDS) {
            return m_nanosec;
        }
        const uint64_t divisor = unitsInSecond(TimeUnit::NANOSECONDS) / unitsInSecond(unit);
        return m_nanosec / divisor;
    }

private:
    uint64_t m_nanosec = 0; ///< Nanoseconds since the Unix epoch

    /**
     * @brief Get the number of units in one second for the given time unit.
     * @param unit Time unit (e.g., NANOSECONDS, MICROSECONDS).
     * @return Number of units in one second.
     */
    static constexpr uint64_t unitsInSecond(TimeUnit unit)
    {
        switch (unit) {
        case TimeUnit::SECONDS:
            return 1;
        case TimeUnit::MILLISECONDS:
            return 1'000;
        case TimeUnit::MICROSECONDS:
            return 1'000'000;
        case TimeUnit::NANOSECONDS:
            return 1'000'000'000;
        }

        std::unreachable();
    }
};

} // namespace amon::types
