/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief Types representing big-endian (network order) with implicit conversions.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <bit>
#include <cstdint>
#include <type_traits>

namespace amon::types {

/**
 * @brief Byte-swaps the given integral value.
 *
 * This function swaps the byte order of the input value, effectively converting
 * between little-endian and big-endian representations. For 1-byte values, the
 * input is returned unchanged. For 2, 4, and 8-byte values, the appropriate
 * built-in byte swap function is used. For other sizes, compilation will fail.
 *
 * @tparam T Integral type to be byte-swapped.
 * @param value The value to byte-swap.
 * @return The byte-swapped value.
 * @note Requires T to be an integral type.
 */
template <typename T>
constexpr T byteswap(T value) noexcept
{
#if defined(__clang__) && (__clang_major__ < 23)
    static_assert(std::is_integral_v<T>, "byteswap requires integral type");

    if constexpr (sizeof(T) == 1) {
        return value;
    } else if constexpr (sizeof(T) == 2) {
        return __builtin_bswap16(value);
    } else if constexpr (sizeof(T) == 4) {
        return __builtin_bswap32(value);
    } else if constexpr (sizeof(T) == 8) {
        return __builtin_bswap64(value);
    }
#else
    return std::byteswap(value);
#endif
}

/**
 * @brief Template class representing an unsigned integer stored in big-endian format.
 *
 * This class wraps an unsigned integer type and stores its value in big-endian (network byte
 * order). It provides implicit conversion operators to and from the host-endian representation,
 * handling byte swapping automatically if the host is little-endian.
 *
 * @tparam UIntT The underlying unsigned integer type (e.g., uint16_t, uint32_t, uint64_t).
 *
 * Example usage:
 * @code
 * UInt16BE value;
 * value = 0x1234; // Stored as big-endian
 * uint16_t hostValue = value; // Converted to host-endian
 * @endcode
 */
template <typename UIntT>
class [[gnu::packed]] UIntBE {
public:
    /**
     * @brief Assigns a value in host-endian format to the big-endian storage.
     * @param valueHostEndian Value in host-endian format.
     * @return Reference to this object.
     */
    UIntBE& operator=(UIntT valueHostEndian) noexcept
    {
        if constexpr (std::endian::native == std::endian::little) {
            // Convert from host to big-endian
            m_value = byteswap(valueHostEndian);
        } else {
            // Already in big-endian
            m_value = valueHostEndian;
        }
        return *this;
    }

    /**
     * @brief Implicit conversion to the underlying type in host-endian format.
     * @return Value converted to host-endian format.
     */
    operator UIntT() const noexcept
    {
        if constexpr (std::endian::native == std::endian::little) {
            return byteswap(m_value); // Convert from big-endian to host
        } else {
            return m_value; // Already in host-endian
        }
    }

private:
    UIntT m_value; ///< The underlying value stored in big-endian format.
};

using UInt8 = uint8_t; ///< Alias for 8-bit unsigned integer (no endianness conversion needed).
using UInt16BE = UIntBE<uint16_t>; ///< Alias for 16-bit unsigned integer in big-endian format.
using UInt32BE = UIntBE<uint32_t>; ///< Alias for 32-bit unsigned integer in big-endian format.
using UInt64BE = UIntBE<uint64_t>; ///< Alias for 64-bit unsigned integer in big-endian format.

} // namespace amon::types
