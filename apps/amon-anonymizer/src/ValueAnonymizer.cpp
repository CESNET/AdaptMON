/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Value anonymizer class
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 */

#include "ValueAnonymizer.hpp"

#include <amon/Error.hpp>
#include <amon/types/UIntBE.hpp>

ValueAnonymizer::ValueAnonymizer(const std::string& key)
{
    if (key.size() != 32) {
        throw amon::Error("key size must be 32, but got key of size {}", key.size());
    }

    int ret;

    ret = m_rijndael.init(
        cryptopan::ECB,
        cryptopan::Encrypt,
        reinterpret_cast<const uint8_t*>(key.data()),
        cryptopan::Key32Bytes,
        nullptr);
    if (ret != 0) {
        throw amon::Error("rijndael cipher initialization failed: {}", ret);
    }

    ret = m_panonymizer.init(reinterpret_cast<const uint8_t*>(key.data()));
    if (ret != 0) {
        throw amon::Error("panonymizer initialization failed: {}", ret);
    }
}

void ValueAnonymizer::anonymize(amon::types::IPv4& ipv4)
{
    static_assert(ipv4.size() == sizeof(amon::types::UInt32BE));

    amon::types::UInt32BE ipv4BE;
    std::memcpy(&ipv4BE, ipv4.data(), ipv4.size());
    ipv4BE = m_panonymizer.anonymize(ipv4BE);
    std::memcpy(ipv4.data(), &ipv4BE, ipv4.size());
}

void ValueAnonymizer::anonymize(amon::types::IPv6& ipv6)
{
    static_assert(ipv6.size() == sizeof(uint64_t[2]));

    uint64_t ipv6Orig[2];
    uint64_t ipv6Anon[2];
    std::memcpy(ipv6Orig, ipv6.data(), ipv6.size());
    m_panonymizer.anonymizeV6(ipv6Orig, ipv6Anon);
    std::memcpy(ipv6.data(), ipv6Anon, ipv6.size());
}

void ValueAnonymizer::anonymize(amon::types::MACAddress& mac)
{
    anonymize({mac.data(), mac.size()});
}

void ValueAnonymizer::anonymize(std::span<std::byte> bytes)
{
    const size_t n16ByteBlocks = bytes.size() / 16;
    const size_t nRemainingBytes = bytes.size() % 16;
    auto* p = reinterpret_cast<uint8_t*>(bytes.data());
    int ret;

    ret = m_rijndael.blockEncrypt(
        p,
        /* length in bits = */ static_cast<int>(n16ByteBlocks * 16 * 8),
        p);
    if (ret < 0) {
        throw amon::Error("anonymization failed: rijndael cipher error: {}", ret);
    }
    p += n16ByteBlocks * 16;

    uint8_t buffer[32]; // padEncrypt requires output buffer to be at least `len + 16` bytes long
    ret = m_rijndael.padEncrypt(
        p,
        /* length in octets = */ static_cast<int>(nRemainingBytes),
        buffer);
    if (ret < 0) {
        throw amon::Error("anonymization failed: rijndael cipher error: {}", ret);
    }
    std::memcpy(p, buffer, nRemainingBytes);
}
