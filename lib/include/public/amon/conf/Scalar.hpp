/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Configuration parser implementation - abstract scalar class
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 */

#pragma once

#include <cstdint>
#include <string>

namespace amon::conf {

/**
 * @brief Abstract base class representing a scalar value.
 *
 * Provides interface methods to retrieve the scalar value in various types.
 */
class Scalar {
public:
    /**
     * @brief Returns the scalar value as a string.
     * @return String representation of the scalar value.
     */
    virtual std::string asString() const = 0;

    /**
     * @brief Returns the scalar value as a boolean.
     * @return Boolean representation of the scalar value.
     */
    virtual bool asBool() const = 0;

    /**
     * @brief Returns the scalar value as an unsigned 64-bit integer.
     * @return Unsigned 64-bit integer representation of the scalar value.
     */
    virtual uint64_t asUInt() const = 0;

    /**
     * @brief Returns the scalar value as a signed 64-bit integer.
     * @return Signed 64-bit integer representation of the scalar value.
     */
    virtual int64_t asInt() const = 0;

    /**
     * @brief Returns the scalar value as a double.
     * @return Double-precision floating point representation of the scalar value.
     */
    virtual double asDouble() const = 0;

    /**
     * @brief Virtual destructor.
     */
    virtual ~Scalar() = default;
};

} // namespace amon::conf
