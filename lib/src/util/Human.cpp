/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Helpers for human-readable formatting
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/Test.hpp>
#include <amon/util/Human.hpp>

#include <cstdint>
#include <format>
#include <string>

namespace amon::util {

#ifdef AMON_ENABLE_TESTS

TEST(AmonUtilHuman, HumanreadableamountFormatsWithoutSuffix)
{
    EXPECT_EQ(std::string("999"), std::format("{}", HumanReadableAmount<int> {999}));
    EXPECT_EQ(std::string("1.00 k"), std::format("{}", HumanReadableAmount<int> {1'001}));
    EXPECT_EQ(std::string("1.00 M"), std::format("{}", HumanReadableAmount<int> {1'000'001}));
    EXPECT_EQ(
        std::string("1.00 G"),
        std::format("{}", HumanReadableAmount<int64_t> {1'000'000'001}));
    EXPECT_EQ(
        std::string("1.00 T"),
        std::format("{}", HumanReadableAmount<int64_t> {1'000'000'000'001}));
}

TEST(AmonUtilHuman, HumanreadableamountFormatsWithCustomSuffix)
{
    EXPECT_EQ(std::string("999 pkt"), std::format("{}", HumanReadableAmount<int, "pkt"> {999}));
    EXPECT_EQ(std::string("1.00 kpkt"), std::format("{}", HumanReadableAmount<int, "pkt"> {1'001}));
    EXPECT_EQ(
        std::string("1.00 Mpkt"),
        std::format("{}", HumanReadableAmount<int, "pkt"> {1'000'001}));
}

TEST(AmonUtilHuman, HumanreadablebyteamountFormatsWithByteSuffix)
{
    EXPECT_EQ(std::string("999 B"), std::format("{}", HumanReadableByteAmount<int> {999}));
    EXPECT_EQ(std::string("1.00 kB"), std::format("{}", HumanReadableByteAmount<int> {1'001}));
    EXPECT_EQ(std::string("1.00 MB"), std::format("{}", HumanReadableByteAmount<int> {1'000'001}));
}

TEST(AmonUtilHuman, HumanreadablebitamountFormatsWithBitSuffix)
{
    EXPECT_EQ(std::string("999 b"), std::format("{}", HumanReadableBitAmount<int> {999}));
    EXPECT_EQ(std::string("1.00 kb"), std::format("{}", HumanReadableBitAmount<int> {1'001}));
    EXPECT_EQ(std::string("1.00 Mb"), std::format("{}", HumanReadableBitAmount<int> {1'000'001}));
}

TEST(AmonUtilHuman, HumanreadableamountFormatsBinaryUnits)
{
    EXPECT_EQ(
        std::string("2.00 ki"),
        std::format(
            "{}",
            HumanReadableAmount<int, "", HumanReadableAmountStyle::BINARY> {2 * 1024}));
    EXPECT_EQ(
        std::string("2.00 Mi"),
        std::format(
            "{}",
            HumanReadableAmount<int, "", HumanReadableAmountStyle::BINARY> {2 * 1024 * 1024}));
    EXPECT_EQ(
        std::string("2.00 Gi"),
        std::format(
            "{}",
            HumanReadableAmount<int64_t, "", HumanReadableAmountStyle::BINARY> {
                2LL * 1024 * 1024 * 1024}));
    EXPECT_EQ(
        std::string("2.00 Ti"),
        std::format(
            "{}",
            HumanReadableAmount<int64_t, "", HumanReadableAmountStyle::BINARY> {
                2LL * 1024 * 1024 * 1024 * 1024}));
}

TEST(AmonUtilHuman, Humanreadablebyteamount1024FormatsBinaryByteSuffix)
{
    EXPECT_EQ(
        std::string("2.00 kiB"),
        std::format("{}", HumanReadableByteAmount1024<int> {2 * 1024}));
    EXPECT_EQ(
        std::string("2.00 MiB"),
        std::format("{}", HumanReadableByteAmount1024<int> {2 * 1024 * 1024}));
}

TEST(AmonUtilHuman, Humanreadablebitamount1024FormatsBinaryBitSuffix)
{
    EXPECT_EQ(
        std::string("2.00 kib"),
        std::format("{}", HumanReadableBitAmount1024<int> {2 * 1024}));
    EXPECT_EQ(
        std::string("2.00 Mib"),
        std::format("{}", HumanReadableBitAmount1024<int> {2 * 1024 * 1024}));
}

#endif

} // namespace amon::util
