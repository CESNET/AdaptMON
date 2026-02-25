/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Value anonymizer class
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 */

#pragma once

#include "3rdparty/cryptopan/panonymizer.h"
#include "3rdparty/cryptopan/rijndael.h"

#include <amon/types/IPAddress.hpp>
#include <amon/types/MACAddress.hpp>

#include <concepts>
#include <string>

/**
 * @brief Provides anonymization functions for various data types using CryptoPAN.
 */
class ValueAnonymizer {
public:
    /**
     * @brief Constructs an Anonymizer with the given key.
     * @param key The key used for anonymization.
     */
    ValueAnonymizer(const std::string& key);

    /**
     * @brief Anonymizes an IPv4 address.
     * @param ipv4 The IPv4 address to anonymize.
     */
    void anonymize(amon::types::IPv4& ipv4);

    /**
     * @brief Anonymizes an IPv6 address.
     * @param ipv6 The IPv6 address to anonymize.
     */
    void anonymize(amon::types::IPv6& ipv6);

    /**
     * @brief Anonymizes a MAC address.
     * @param mac The MAC address to anonymize.
     */
    void anonymize(amon::types::MACAddress& mac);

    /**
     * @brief Anonymizes a span of bytes.
     * @param bytes The bytes to anonymize.
     */
    void anonymize(std::span<std::byte> bytes);

    /**
     * @brief Anonymizes an integral value.
     * @tparam T Integral type.
     * @param value The value to anonymize.
     */
    template <std::integral T>
    void anonymize(T& value);

    /**
     * @brief Anonymizes a big-endian unsigned integer value.
     * @tparam T Underlying type.
     * @param value The big-endian unsigned integer to anonymize.
     */
    template <typename T>
    void anonymize(amon::types::UIntBE<T>& value);

private:
    cryptopan::Rijndael m_rijndael; ///< Rijndael cipher instance.
    cryptopan::Panonymizer m_panonymizer; ///< CryptoPAN anonymizer instance.
};

template <typename T>
void ValueAnonymizer::anonymize(amon::types::UIntBE<T>& value)
{
    T tmp = value;
    anonymize(tmp);
    value = tmp;
}

template <std::integral T>
void ValueAnonymizer::anonymize(T& value)
{
    anonymize({reinterpret_cast<std::byte*>(&value), sizeof(value)});
}
