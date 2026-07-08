/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief Representation of MAC (Media Access Control) address.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/types/ByteArray.hpp>
#include <amon/types/UIntBE.hpp>
#include <amon/util/Formatter.hpp>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <span>
#include <stdexcept>
#include <utility>

namespace amon::types {

/// Length of a MAC address in bytes.
constexpr static const unsigned MAC_ADDRESS_LEN = 6;

/**
 * @brief Class representing a MAC (Media Access Control) address.
 *
 * Provides constructors for initialization from string or byte array,
 * and inherits utility methods from ByteArray.
 */
class [[gnu::packed]] MACAddress : public ByteArray<MAC_ADDRESS_LEN> {
public:
    /**
     * @brief Default constructor. MAC address has uninitialized value.
     */
    MACAddress() = default;

    /**
     * @brief Construct MAC address from string in format "xx:xx:xx:xx:xx:xx".
     * @param addressStr MAC address string.
     * @throws std::invalid_argument if string format or length is invalid.
     */
    MACAddress(const std::string& addressStr)
    {
        if (addressStr.size() != 17) {
            throw std::invalid_argument("MAC address string must be 17 characters long.");
        }
        auto* dataAsUChar = reinterpret_cast<unsigned char*>(data());
        if (std::cmp_not_equal(
                std::sscanf(
                    addressStr.c_str(),
                    "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                    &dataAsUChar[0],
                    &dataAsUChar[1],
                    &dataAsUChar[2],
                    &dataAsUChar[3],
                    &dataAsUChar[4],
                    &dataAsUChar[5]),
                MAC_ADDRESS_LEN)) {
            throw std::invalid_argument("Bad MAC address string format.");
        }
    }

    /**
     * @brief Construct MAC address from a span of 6 bytes.
     * @param address Span of bytes containing the MAC address.
     * @throws std::invalid_argument if span size is not 6.
     */
    MACAddress(std::span<const uint8_t> address)
    {
        if (address.size() != MAC_ADDRESS_LEN) {
            throw std::invalid_argument("Mac address must be 6 bytes long.");
        }
        std::memcpy(data(), address.data(), MAC_ADDRESS_LEN);
    }
};

/**
 * @brief Formatter for MACAddress to print as "xx:xx:xx:xx:xx:xx".
 * @param addr MACAddress to format.
 * @param fmt Formatter to write to.
 */
inline void format(const MACAddress& addr, amon::util::Formatter& fmt)
{
    fmt.format(
        "{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}",
        static_cast<unsigned>(addr[0]),
        static_cast<unsigned>(addr[1]),
        static_cast<unsigned>(addr[2]),
        static_cast<unsigned>(addr[3]),
        static_cast<unsigned>(addr[4]),
        static_cast<unsigned>(addr[5]));
}

} // namespace amon::types
