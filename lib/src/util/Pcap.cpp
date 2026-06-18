/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Pcap file reading and writing utilities
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon/Error.hpp>
#include <amon/Logger.hpp>
#include <amon/util/Pcap.hpp>

#include <cassert>
#include <cstdio>
#include <iostream>

#include <pcap/pcap.h>

namespace amon::util {

namespace {

struct FClose {
    void operator()(std::FILE* file) const noexcept { std::fclose(file); }
};

} // namespace

static constexpr const uint64_t MICROSECS_IN_SEC = 1'000'000;
static constexpr const uint64_t NANOSECS_IN_SEC = 1'000'000'000;

void format(PcapTimestampPrecision value, Formatter& fmt)
{
    switch (value) {
    case amon::util::PcapTimestampPrecision::MICRO:
        fmt.format("{}", "PcapTimestampPrecision::MICRO");
        break;
    case amon::util::PcapTimestampPrecision::NANO:
        fmt.format("{}", "PcapTimestampPrecision::NANO");
        break;
    }
}

static types::Timestamp timestampFromTimevalWithNanos(const timeval& tv)
{
    if (tv.tv_sec < 0 || tv.tv_usec < 0) {
        throw amon::Error("Unexpected negative timestamp in packet");
    }

    const auto secU64 = numeric::safeCast<uint64_t>(tv.tv_sec);
    const auto nsecU64 = numeric::safeCast<uint64_t>(tv.tv_usec);

    uint64_t nanos;
    nanos = types::Timestamp::from<types::TimeUnit::SECONDS>(secU64).nanoseconds();
    nanos = numeric::safeAdd(nanos, nsecU64);
    return amon::types::Timestamp {nanos};
}

static timeval timestampToTimevalWithNanos(const types::Timestamp& ts)
{
    const uint64_t nanos = ts.nanoseconds();

    timeval tv;
    tv.tv_sec = numeric::safeCast<decltype(tv.tv_sec)>(nanos / NANOSECS_IN_SEC);
    tv.tv_usec = numeric::safeCast<decltype(tv.tv_usec)>(nanos % NANOSECS_IN_SEC);
    return tv;
}

static uint64_t ftellEndPos(FILE* fp)
{
    int ret;
    auto savePos = std::ftell(fp);
    if (savePos < 0) {
        throw ErrnoError("ftell() failed");
    }
    ret = std::fseek(fp, 0, SEEK_END);
    if (ret < 0) {
        throw ErrnoError("fseek() failed");
    }
    auto endPos = ftello64(fp);
    if (endPos < 0) {
        throw ErrnoError("ftello64() failed");
    }
    ret = std::fseek(fp, savePos, SEEK_SET);
    if (ret < 0) {
        throw ErrnoError("fseek() failed");
    }

    return numeric::safeCast<uint64_t>(endPos);
}

static uint64_t ftellCurrPos(FILE* fp)
{
    auto pos = ftello64(fp);
    if (pos < 0) {
        throw ErrnoError("ftello64() failed");
    }
    return numeric::safeCast<uint64_t>(pos);
}

PcapReader PcapReader::fromFile(const std::string& filename, PcapTimestampPrecision precision)
{
    PcapReader reader {precision};
    char errBuf[PCAP_ERRBUF_SIZE];
    reader.m_pcap.reset(pcap_open_offline_with_tstamp_precision(
        filename.c_str(),
        static_cast<u_int>(precision),
        errBuf));
    if (reader.m_pcap == nullptr) {
        throw Error(
            "pcap_open_offline_with_tstamp_precision(\"{}\", {}) failed: {}",
            filename,
            precision,
            errBuf);
    }
    return reader;
}

PcapReader PcapReader::fromInterface(
    const std::string& interface,
    const PcapInterfaceOptions& options,
    PcapTimestampPrecision precision)
{
    PcapReader reader {precision};
    char errBuf[PCAP_ERRBUF_SIZE];
    reader.m_pcap.reset(pcap_create(interface.c_str(), errBuf));
    if (reader.m_pcap == nullptr) {
        throw Error("pcap_create(\"{}\") failed: {}", interface, errBuf);
    }

    int ret;

    ret = pcap_set_immediate_mode(reader.m_pcap.get(), static_cast<int>(options.immediateMode));
    if (ret < 0) {
        throw Error(
            "pcap_set_immediate_mode({}) failed: {}",
            options.immediateMode,
            pcap_statustostr(ret));
    }

    auto bufferTimeout = numeric::safeCast<int>(options.bufferTimeout);
    if (bufferTimeout <= 0) {
        throw Error("pcap_set_timeout() invalid value: {}", bufferTimeout);
    }
    ret = pcap_set_timeout(reader.m_pcap.get(), bufferTimeout);
    if (ret < 0) {
        throw Error(
            "pcap_set_timeout({}) failed: {}",
            options.bufferTimeout,
            pcap_statustostr(ret));
    }

    ret = pcap_set_promisc(reader.m_pcap.get(), static_cast<int>(options.promiscuousMode));
    if (ret < 0) {
        throw Error(
            "pcap_set_promisc({}) failed: {}",
            options.promiscuousMode,
            pcap_statustostr(ret));
    }

    ret = pcap_set_snaplen(reader.m_pcap.get(), numeric::safeCast<int>(options.snapshotLength));
    if (ret < 0) {
        throw Error(
            "pcap_set_snaplen({}) failed: {}",
            options.snapshotLength,
            pcap_statustostr(ret));
    }

    ret = pcap_set_buffer_size(reader.m_pcap.get(), numeric::safeCast<int>(options.bufferSize));
    if (ret < 0) {
        throw Error(
            "pcap_set_buffer_size({}) failed: {}",
            options.bufferSize,
            pcap_statustostr(ret));
    }

    ret = pcap_set_tstamp_precision(reader.m_pcap.get(), static_cast<int>(precision));
    if (ret < 0) {
        throw Error("pcap_set_tstamp_precision({}) failed: {}", precision, pcap_statustostr(ret));
    }

    ret = pcap_activate(reader.m_pcap.get());
    if (ret < 0) {
        throw Error("pcap_activate() failed: {}", pcap_statustostr(ret));
    }

    // warning checking from activate
    if (ret == PCAP_WARNING) {
        amon::LoggerProvider::get("PcapReader")
            .warn("pcap_activate(): {}", pcap_geterr(reader.m_pcap.get()));
    } else if (ret > 0) {
        amon::LoggerProvider::get("PcapReader").warn("pcap_activate(): {}", pcap_statustostr(ret));
    }

    return reader;
}

std::optional<PcapPacket> PcapReader::readPacket()
{
    struct pcap_pkthdr* hdr;
    const u_char* data;

    const int ret = pcap_next_ex(m_pcap.get(), &hdr, &data);
    if (ret == PCAP_ERROR_BREAK) { // No more packets in file or break loop called
        m_eof = true;
        return std::nullopt;
    }
    if (ret == 0) { // Packet buffer timeout expired
        return std::nullopt;
    }
    if (ret == PCAP_ERROR) {
        const char* err = pcap_geterr(m_pcap.get());
        throw Error("pcap_next_ex() failed: {}", err);
    }
    if (ret != 1) {
        // Other errors should never happen in case of a file or interface
        throw amon::Error("pcap_next_ex() returned an unexpected value {}", ret);
    }

    PcapPacket packet;

    if (m_precision == PcapTimestampPrecision::MICRO) {
        packet.timestamp = amon::types::Timestamp::fromTimeval(hdr->ts);

    } else {
        assert(m_precision == PcapTimestampPrecision::NANO);
        // In case precision is NANO, the tv_usec part represents nanoseconds despite its
        // name...
        packet.timestamp = timestampFromTimevalWithNanos(hdr->ts);
    }

    packet.data
        = std::span<const std::byte> {reinterpret_cast<const std::byte*>(data), hdr->caplen};
    packet.lengthOnWire = hdr->len;

    return packet;
}

int PcapReader::linkLayerType() const
{
    const int ret = pcap_datalink(m_pcap.get());
    // We open the file in constructor, a valid PcapReader
    // should never have the file not open
    assert(ret != PCAP_ERROR_NOT_ACTIVATED);
    return ret;
}

uint64_t PcapReader::tell() const
{
    FILE* file = pcap_file(m_pcap.get());
    if (file == nullptr) {
        return 0;
    }

    return ftellCurrPos(file);
}

uint64_t PcapReader::tellEnd() const
{
    FILE* file = pcap_file(m_pcap.get());
    if (file == nullptr) {
        return std::numeric_limits<uint64_t>::max();
    }

    return ftellEndPos(file);
}

PcapWriter::PcapWriter(
    const std::string& filename,
    PcapTimestampPrecision precision,
    int linkLayerType)
    : m_precision(precision)
{
    // We need to use pcap_dump_fopen to be able to check for write errors.
    // Unfortunately pcap_dump does not return any value to indicate errors, so we have to check
    // the FILE* directly to detect write errors. fopen is done by us, fclose is done by
    // pcap_dump_close.
    std::unique_ptr<std::FILE, FClose> fp(std::fopen(filename.c_str(), "wb"));
    if (!fp) {
        throw ErrnoError("fopen", filename);
    }

    m_pcap.reset(pcap_open_dead_with_tstamp_precision(
        linkLayerType,
        std::numeric_limits<uint16_t>::max(),
        static_cast<u_int>(m_precision)));
    if (!m_pcap) {
        throw Error("pcap_open_dead_with_tstamp_precision() failed");
    }

    m_pcapDumper.reset(pcap_dump_fopen(m_pcap.get(), fp.get()));
    if (!m_pcapDumper) {
        const char* err = pcap_geterr(m_pcap.get());
        throw Error("pcap_dump_fopen() failed: {}", err);
    }
    m_fp = fp.release();
}

void PcapWriter::writePacket(
    std::span<const std::byte> data,
    types::Timestamp timestamp,
    std::optional<uint32_t> lengthOnWire)
{
    pcap_pkthdr hdr;
    hdr.caplen = numeric::safeCast<bpf_u_int32>(data.size());
    hdr.len = numeric::safeCast<bpf_u_int32>(lengthOnWire.value_or(data.size()));

    if (m_precision == PcapTimestampPrecision::MICRO) {
        hdr.ts = timestamp.toTimeval();
    } else {
        assert(m_precision == PcapTimestampPrecision::NANO);
        // In case precision is NANO, the tv_usec part represents nanoseconds despite its
        // name...
        hdr.ts = timestampToTimevalWithNanos(timestamp);
    }

    pcap_dump(
        reinterpret_cast<u_char*>(m_pcapDumper.get()),
        &hdr,
        reinterpret_cast<const u_char*>(data.data()));
    if (std::ferror(m_fp) != 0) {
        throw Error("pcap_dump() failed: error writing to file");
    }
}

} // namespace amon::util
