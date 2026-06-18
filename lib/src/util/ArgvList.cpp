/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Utility class to manage argc/argv style argument lists.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/Test.hpp>
#include <amon/util/ArgvList.hpp>

#include <cstring>

namespace amon::util {

ArgvList::ArgvList(const std::vector<std::string>& args)
{
    m_storage.reserve(args.size());
    for (std::string arg : args) {
        arg.push_back('\0');
        m_storage.emplace_back(std::move(arg));
    }

    m_argv.reserve(m_storage.size() + 1);
    for (std::string& arg : m_storage) {
        m_argv.emplace_back(arg.data());
    }
    m_argv.emplace_back(nullptr);
}

#ifdef AMON_ENABLE_TESTS

TEST(AmonUtilArgvList, ArgvlistBasicArgumentHandling)
{
    const std::vector<std::string> args = {"program", "--test", "value"};
    amon::util::ArgvList argvList(args);

    EXPECT_TRUE(argvList.argc() == 3);
    EXPECT_TRUE(std::strcmp(argvList.argv()[0], "program") == 0);
    EXPECT_TRUE(std::strcmp(argvList.argv()[1], "--test") == 0);
    EXPECT_TRUE(std::strcmp(argvList.argv()[2], "value") == 0);
    EXPECT_TRUE(argvList.argv()[3] == nullptr);
}

TEST(AmonUtilArgvList, ArgvlistNullTermination)
{
    const std::vector<std::string> args = {"one", "two"};
    amon::util::ArgvList argvList(args);

    // Verify proper null termination
    EXPECT_TRUE(argvList.argv()[0][3] == '\0');
    EXPECT_TRUE(argvList.argv()[1][3] == '\0');
    EXPECT_TRUE(argvList.argv()[2] == nullptr);
}

TEST(AmonUtilArgvList, ArgvlistEmptyArgumentList)
{
    const std::vector<std::string> args;
    amon::util::ArgvList argvList(args);

    EXPECT_TRUE(argvList.argc() == 0);
    EXPECT_TRUE(argvList.argv()[0] == nullptr);
}

TEST(AmonUtilArgvList, ArgvlistSpecialCharacters)
{
    const std::vector<std::string> args = {"arg with spaces", "!@#$%^&*()", ""};
    amon::util::ArgvList argvList(args);

    EXPECT_TRUE(argvList.argc() == 3);
    EXPECT_TRUE(std::strcmp(argvList.argv()[0], "arg with spaces") == 0);
    EXPECT_TRUE(std::strcmp(argvList.argv()[1], "!@#$%^&*()") == 0);
    EXPECT_TRUE(std::strcmp(argvList.argv()[2], "") == 0);
}

#endif

} // namespace amon::util
