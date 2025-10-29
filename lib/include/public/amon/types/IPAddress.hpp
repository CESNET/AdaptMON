/**
 * @file
 * @author Lukáš Huták <hutak@cesnet.cz>
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief IP address and network representation and manipulation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/types/ByteArray.hpp>

#include <cstdint>
#include <format>
#include <string>

namespace amon::types {

/**
 * @class IPv4
 * @brief Represents an IPv4 address.
 *
 * Inherits from ByteArray<4>.
 */
class [[gnu::packed]] IPv4 : public ByteArray<4> {};

/**
 * @class IPv6
 * @brief Represents an IPv6 address.
 *
 * Inherits from ByteArray<16>.
 */
class [[gnu::packed]] IPv6 : public ByteArray<16> {};

/**
 * @struct IPAddress
 * @brief Represents an IP address (IPv4 or IPv6).
 *
 * Provides multiple representations and utility methods for IP address manipulation.
 */
class IPAddress : public ByteArray<16> {
public:
    /**
     * @brief Constructs a zero-initialized IP address.
     */
    IPAddress() noexcept;

    /**
     * @brief Constructs an IP address from a string.
     * @param ip String representation of the IP address.
     * @throws std::invalid_argument If the string cannot be converted to an IP address.
     */
    explicit IPAddress(const std::string& ip);

    /**
     * @brief Constructs an IP address from an IPv4 address.
     * @param ip IPv4 address.
     *
     * @note The IPv4 address must be stored in network byte order.
     */
    explicit IPAddress(const IPv4& ip) noexcept;

    /**
     * @brief Constructs an IP address from an IPv6 address.
     * @param ip IPv6 address.
     *
     * @note The IPv6 address must be stored in network byte order.
     */
    explicit IPAddress(const IPv6& ip) noexcept;

    /**
     * @brief Checks if the IP address is IPv4.
     * @return True if the address is IPv4, false otherwise.
     */
    bool isIPv4() const noexcept;

    /**
     * @brief Checks if the IP address is IPv6.
     * @return True if the address is IPv6, false otherwise.
     */
    bool isIPv6() const noexcept;

    /**
     * @brief Converts the IP address to its string representation.
     * @return String representation of the IP address.
     * @throws std::runtime_error If the conversion fails.
     */
    std::string toString() const;

private:
    /* Get pointer to the address as array of 32-bit unsigned integers. */
    using U32x4 = uint32_t[4];
    U32x4& u32() { return *reinterpret_cast<U32x4*>(data()); }
    const U32x4& u32() const { return *reinterpret_cast<const U32x4*>(data()); }

    /* Get pointer to the address as array of 64-bit unsigned integers. */
    using U64x2 = uint64_t[2];
    U64x2& u64() { return *reinterpret_cast<U64x2*>(data()); }
    const U64x2& u64() const { return *reinterpret_cast<const U64x2*>(data()); }
};

} // namespace amon::types

// (suppress inconsistency doxygen error)
/// @cond
/**
 * @brief Formatter for IPAddress to print as standard string representation.
 */
template <>
struct std::formatter<amon::types::IPAddress> : std::formatter<std::string_view> {
    /**
     * @brief Format IPAddress for output.
     * @param addr IPAddress to format.
     * @param ctx Format context.
     * @return Iterator to the end of the formatted output.
     */
    auto format(const amon::types::IPAddress& addr, std::format_context& ctx) const
    {
        return std::formatter<std::string_view>::format(addr.toString(), ctx);
    }
};
/// @endcond
