/**
 * @file
 * @author Samuel Luptak <luptak@cesnet.cz>
 * @brief The DPDK test runner. Initializes DPDK EAL once before running the tests.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/Test.hpp>
#include <amon/dpdk/EAL.hpp>

#include <memory>
#include <string>
#include <vector>

namespace {

/**
 * @brief Initializes EAL before the first test, keeps it alive until the test binary exits.
 */
class DpdkEnvironment : public testing::Environment {
public:
    void SetUp() override
    {
        std::vector<std::string> args
            = {"amon-dpdk-tests",
               "--in-memory",
               "--no-pci",
               "--no-huge",
               "--no-telemetry",
               "-l", // Not optimal for parallel execution using ctest
               "0",
               "--log-level",
               "error"};
        m_eal = std::make_unique<amon::dpdk::EAL>(args);
    }

private:
    std::unique_ptr<amon::dpdk::EAL> m_eal;
};

} // namespace

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);

    testing::AddGlobalTestEnvironment(
        new DpdkEnvironment); // NOLINT(cppcoreguidelines-owning-memory)

    return RUN_ALL_TESTS();
}
