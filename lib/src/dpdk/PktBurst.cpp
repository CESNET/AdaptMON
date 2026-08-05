/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Packet burst representation.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon-internal/Test.hpp>
#include <amon/amon.hpp>
#include <amon/dpdk/PktBurst.hpp>

#include <memory>

namespace amon::dpdk {

#ifdef AMON_ENABLE_TESTS

// NOLINTBEGIN
namespace {

struct MockMbuf {
    size_t id = 0;
    int refcnt = 1;
};

struct MockPktBurstOps {
    static void refcntUpdate(MockMbuf* m, int v) { m->refcnt += v; }

    static void freeSingle(MockMbuf* mbuf)
    {
        if (mbuf != nullptr) {
            mbuf->refcnt -= 1;
        }
    }

    static void freeBulk(MockMbuf** mbufs, unsigned count)
    {
        for (unsigned i = 0; i < count; ++i) {
            freeSingle(mbufs[i]);
        }
    }
};

} // namespace

using TestBurst = amon::dpdk::PktBurst<MockMbuf, MockPktBurstOps>;

namespace {

struct TestCtx {
    std::vector<std::unique_ptr<MockMbuf>> mbufs;
    TestBurst burst;

    TestCtx(size_t burstSize)
        : mbufs(burstSize)
        , burst(burstSize)
    {
        burst.resize(burstSize);
        for (size_t i = 0; i < burstSize; ++i) {
            mbufs[i] = std::make_unique<MockMbuf>(i, 1);
            burst[i] = mbufs[i].get();
        }
    }
};

} // namespace
// NOLINTEND

TEST(AmonDpdkPktBurst, PktburstTestEraseSingle)
{
    TestCtx c(4);

    c.burst.erase(c.burst.begin() + 1);

    EXPECT_TRUE(c.mbufs[0]->refcnt == 1);
    EXPECT_TRUE(c.mbufs[1]->refcnt == 0);
    EXPECT_TRUE(c.mbufs[2]->refcnt == 1);
    EXPECT_TRUE(c.mbufs[3]->refcnt == 1);

    ASSERT_TRUE(c.burst.size() == 3);
    EXPECT_TRUE(c.burst[0]->id == 0);
    EXPECT_TRUE(c.burst[1]->id == 2);
    EXPECT_TRUE(c.burst[2]->id == 3);
}

TEST(AmonDpdkPktBurst, PktburstTestEraseRange)
{
    TestCtx c(4);

    c.burst.erase(c.burst.begin(), c.burst.begin() + 2);

    EXPECT_TRUE(c.mbufs[0]->refcnt == 0);
    EXPECT_TRUE(c.mbufs[1]->refcnt == 0);
    EXPECT_TRUE(c.mbufs[2]->refcnt == 1);
    EXPECT_TRUE(c.mbufs[3]->refcnt == 1);

    ASSERT_TRUE(c.burst.size() == 2);
    EXPECT_TRUE(c.burst[0]->id == 2);
    EXPECT_TRUE(c.burst[1]->id == 3);
}

TEST(AmonDpdkPktBurst, PktburstTestEraseInvalidRange)
{
    TestCtx c(4);

    EXPECT_DEATH(c.burst.erase(c.burst.begin() + 4), "ASSERTION FAILED");
    EXPECT_DEATH(c.burst.erase(c.burst.begin() + 1, c.burst.end() + 1), "ASSERTION FAILED");
}

TEST(AmonDpdkPktBurst, PktburstTestRelease)
{
    TestCtx c(3);

    c.burst.release(c.burst.begin(), c.burst.begin() + 2);

    EXPECT_TRUE(c.mbufs[0]->refcnt == 1);
    EXPECT_TRUE(c.mbufs[1]->refcnt == 1);
    EXPECT_TRUE(c.mbufs[2]->refcnt == 1);

    ASSERT_TRUE(c.burst.size() == 1);
    EXPECT_TRUE(c.burst[0]->id == 2);
}

TEST(AmonDpdkPktBurst, PktburstTestReleaseInvalidRange)
{
    TestCtx c(4);

    EXPECT_DEATH(c.burst.release(c.burst.begin() + 2, c.burst.begin() + 1), "ASSERTION FAILED");
}

TEST(AmonDpdkPktBurst, PktburstTestCopy)
{
    TestCtx c(3);

    std::vector<MockMbuf*> out(2);
    c.burst.copy(c.burst.begin(), c.burst.begin() + 2, out.begin());

    ASSERT_TRUE(out[0] != nullptr);
    EXPECT_TRUE(out[0]->id == 0);
    ASSERT_TRUE(out[1] != nullptr);
    EXPECT_TRUE(out[1]->id == 1);

    EXPECT_TRUE(c.mbufs[0]->refcnt == 2);
    EXPECT_TRUE(c.mbufs[1]->refcnt == 2);
    EXPECT_TRUE(c.mbufs[2]->refcnt == 1);

    ASSERT_TRUE(c.burst.size() == 3);
    EXPECT_TRUE(c.burst[0]->id == 0);
    EXPECT_TRUE(c.burst[1]->id == 1);
    EXPECT_TRUE(c.burst[2]->id == 2);
}

TEST(AmonDpdkPktBurst, PktburstTestCopyToBurst)
{
    TestCtx c(3);

    TestBurst out(2);
    c.burst.copy(c.burst.begin(), c.burst.begin() + 2, out);

    ASSERT_TRUE(out.size() == 2);
    ASSERT_TRUE(out[0] != nullptr);
    EXPECT_TRUE(out[0]->id == 0);
    ASSERT_TRUE(out[1] != nullptr);
    EXPECT_TRUE(out[1]->id == 1);

    EXPECT_TRUE(c.mbufs[0]->refcnt == 2);
    EXPECT_TRUE(c.mbufs[1]->refcnt == 2);
    EXPECT_TRUE(c.mbufs[2]->refcnt == 1);

    ASSERT_TRUE(c.burst.size() == 3);
    EXPECT_TRUE(c.burst[0]->id == 0);
    EXPECT_TRUE(c.burst[1]->id == 1);
    EXPECT_TRUE(c.burst[2]->id == 2);
}

TEST(AmonDpdkPktBurst, PktburstTestCopyToBurstThatIsTooSmall)
{
    TestCtx c(3);

    TestBurst out(1);
    EXPECT_DEATH(c.burst.copy(c.burst.begin(), c.burst.begin() + 2, out), "ASSERTION FAILED");
}

TEST(AmonDpdkPktBurst, PktburstTestCopyif)
{
    TestCtx c(3);

    std::vector<MockMbuf*> out(3);
    auto pred = [](MockMbuf* m) { return m->id % 2 == 0; };
    auto it = c.burst.copyIf(pred, out.begin());

    EXPECT_TRUE(it == out.begin() + 2);
    ASSERT_TRUE(out[0] != nullptr);
    EXPECT_TRUE(out[0]->id == 0);
    ASSERT_TRUE(out[1] != nullptr);
    EXPECT_TRUE(out[1]->id == 2);

    EXPECT_TRUE(c.mbufs[0]->refcnt == 2);
    EXPECT_TRUE(c.mbufs[1]->refcnt == 1);
    EXPECT_TRUE(c.mbufs[2]->refcnt == 2);

    ASSERT_TRUE(c.burst.size() == 3);
    EXPECT_TRUE(c.burst[0]->id == 0);
    EXPECT_TRUE(c.burst[1]->id == 1);
    EXPECT_TRUE(c.burst[2]->id == 2);
}

TEST(AmonDpdkPktBurst, PktburstTestCopyifToBurst)
{
    TestCtx c(3);

    TestBurst out(3);
    auto pred = [](MockMbuf* m) { return m->id % 2 == 0; };
    c.burst.copyIf(pred, out);

    ASSERT_TRUE(out.size() == 2);
    ASSERT_TRUE(out[0] != nullptr);
    EXPECT_TRUE(out[0]->id == 0);
    ASSERT_TRUE(out[1] != nullptr);
    EXPECT_TRUE(out[1]->id == 2);

    EXPECT_TRUE(c.mbufs[0]->refcnt == 2);
    EXPECT_TRUE(c.mbufs[1]->refcnt == 1);
    EXPECT_TRUE(c.mbufs[2]->refcnt == 2);

    ASSERT_TRUE(c.burst.size() == 3);
    EXPECT_TRUE(c.burst[0]->id == 0);
    EXPECT_TRUE(c.burst[1]->id == 1);
    EXPECT_TRUE(c.burst[2]->id == 2);
}

TEST(AmonDpdkPktBurst, PktburstTestCopyifToBurstThatIsTooSmall)
{
    TestCtx c(3);

    TestBurst out(2);
    auto pred = [](MockMbuf* m) { return m->id % 2 == 0; };
    EXPECT_DEATH(c.burst.copyIf(pred, out), "ASSERTION FAILED");
}

TEST(AmonDpdkPktBurst, PktburstTestMove)
{
    TestCtx c(3);

    std::vector<MockMbuf*> out(2);
    c.burst.move(c.burst.begin(), c.burst.begin() + 2, out.begin());

    ASSERT_TRUE(out[0] != nullptr);
    EXPECT_TRUE(out[0]->id == 0);
    ASSERT_TRUE(out[1] != nullptr);
    EXPECT_TRUE(out[1]->id == 1);

    EXPECT_TRUE(c.mbufs[0]->refcnt == 1);
    EXPECT_TRUE(c.mbufs[1]->refcnt == 1);
    EXPECT_TRUE(c.mbufs[2]->refcnt == 1);

    ASSERT_TRUE(c.burst.size() == 1);
    EXPECT_TRUE(c.burst[0]->id == 2);
}

TEST(AmonDpdkPktBurst, PktburstTestMoveToBurst)
{
    TestCtx c(3);

    TestBurst out(2);
    c.burst.move(c.burst.begin(), c.burst.begin() + 2, out);

    ASSERT_TRUE(out.size() == 2);
    ASSERT_TRUE(out[0] != nullptr);
    EXPECT_TRUE(out[0]->id == 0);
    ASSERT_TRUE(out[1] != nullptr);
    EXPECT_TRUE(out[1]->id == 1);

    EXPECT_TRUE(c.mbufs[0]->refcnt == 1);
    EXPECT_TRUE(c.mbufs[1]->refcnt == 1);
    EXPECT_TRUE(c.mbufs[2]->refcnt == 1);

    ASSERT_TRUE(c.burst.size() == 1);
    EXPECT_TRUE(c.burst[0]->id == 2);
}

TEST(AmonDpdkPktBurst, PktburstTestMoveToBurstThatIsTooSmall)
{
    TestCtx c(3);

    TestBurst out(1);
    EXPECT_DEATH(c.burst.move(c.burst.begin(), c.burst.begin() + 2, out), "ASSERTION FAILED");
}

TEST(AmonDpdkPktBurst, PktburstTestMoveif)
{
    TestCtx c(3);

    std::vector<MockMbuf*> out(3);
    auto pred = [](MockMbuf* m) { return m->id % 2 == 0; };
    auto it = c.burst.moveIf(pred, out.begin());

    EXPECT_TRUE(it == out.begin() + 2);
    EXPECT_TRUE(out[0]->id == 0);
    EXPECT_TRUE(out[1]->id == 2);

    EXPECT_TRUE(c.mbufs[0]->refcnt == 1);
    EXPECT_TRUE(c.mbufs[1]->refcnt == 1);
    EXPECT_TRUE(c.mbufs[2]->refcnt == 1);

    ASSERT_TRUE(c.burst.size() == 1);
    EXPECT_TRUE(c.burst[0]->id == 1);
}

TEST(AmonDpdkPktBurst, PktburstTestMoveifToBurst)
{
    TestCtx c(3);

    TestBurst out(3);
    auto pred = [](MockMbuf* m) { return m->id % 2 == 0; };
    c.burst.moveIf(pred, out);

    ASSERT_TRUE(out.size() == 2);
    EXPECT_TRUE(out[0]->id == 0);
    EXPECT_TRUE(out[1]->id == 2);

    EXPECT_TRUE(c.mbufs[0]->refcnt == 1);
    EXPECT_TRUE(c.mbufs[1]->refcnt == 1);
    EXPECT_TRUE(c.mbufs[2]->refcnt == 1);

    ASSERT_TRUE(c.burst.size() == 1);
    EXPECT_TRUE(c.burst[0]->id == 1);
}

TEST(AmonDpdkPktBurst, PktburstTestMoveifToBurstThatIsTooSmall)
{
    TestCtx c(3);

    TestBurst out(2);
    auto pred = [](MockMbuf* m) { return m->id % 2 == 0; };
    EXPECT_DEATH(c.burst.moveIf(pred, out), "ASSERTION FAILED");
}

TEST(AmonDpdkPktBurst, PktburstTestEraseif)
{
    TestCtx c(3);

    auto pred = [](MockMbuf* m) { return m->id % 2 == 0; };
    c.burst.eraseIf(pred);

    EXPECT_TRUE(c.mbufs[0]->refcnt == 0);
    EXPECT_TRUE(c.mbufs[1]->refcnt == 1);
    EXPECT_TRUE(c.mbufs[2]->refcnt == 0);

    ASSERT_TRUE(c.burst.size() == 1);
    EXPECT_TRUE(c.burst[0]->id == 1);
}

TEST(AmonDpdkPktBurst, PktburstTestResizeSmaller)
{
    TestCtx c(3);

    c.burst.resize(2);

    EXPECT_TRUE(c.mbufs[0]->refcnt == 1);
    EXPECT_TRUE(c.mbufs[1]->refcnt == 1);
    EXPECT_TRUE(c.mbufs[2]->refcnt == 0);

    ASSERT_TRUE(c.burst.size() == 2);
    EXPECT_TRUE(c.burst[0]->id == 0);
    EXPECT_TRUE(c.burst[1]->id == 1);
}

TEST(AmonDpdkPktBurst, PktburstTestResizeLarger)
{
    TestCtx c(4);

    c.burst.resize(2);
    c.burst.resize(4);

    EXPECT_TRUE(c.mbufs[0]->refcnt == 1);
    EXPECT_TRUE(c.mbufs[1]->refcnt == 1);
    EXPECT_TRUE(c.mbufs[2]->refcnt == 0);
    EXPECT_TRUE(c.mbufs[3]->refcnt == 0);

    ASSERT_TRUE(c.burst.size() == 4);
    EXPECT_TRUE(c.burst[0]->id == 0);
    EXPECT_TRUE(c.burst[1]->id == 1);
    EXPECT_TRUE(c.burst[2] == nullptr);
    EXPECT_TRUE(c.burst[3] == nullptr);
}

TEST(AmonDpdkPktBurst, PktburstTestResizeBeyondCapacity)
{
    TestCtx c(4);

    EXPECT_DEATH(c.burst.resize(5), "ASSERTION FAILED");
}

TEST(AmonDpdkPktBurst, PktburstTestClear)
{
    TestCtx c(3);

    c.burst.clear();

    EXPECT_TRUE(c.mbufs[0]->refcnt == 0);
    EXPECT_TRUE(c.mbufs[1]->refcnt == 0);
    EXPECT_TRUE(c.mbufs[2]->refcnt == 0);

    EXPECT_TRUE(c.burst.size() == 0);
}

TEST(AmonDpdkPktBurst, PktburstTestOperator)
{
    TestCtx c(2);

    EXPECT_TRUE(c.burst[0] == c.mbufs[0].get()); // NOLINT
    EXPECT_TRUE(c.burst[1] == c.mbufs[1].get());

    c.burst[1] = nullptr;
    EXPECT_TRUE(c.burst[1] == nullptr);
    EXPECT_TRUE(c.mbufs[1]->refcnt == 1);
}

TEST(AmonDpdkPktBurst, PktburstTestAvailableCapacity)
{
    TestCtx c(4);

    c.burst.resize(2);
    EXPECT_TRUE(c.burst.available() == 2);
    EXPECT_TRUE(c.burst.capacity() == 4);

    EXPECT_TRUE(c.mbufs[0]->refcnt == 1);
    EXPECT_TRUE(c.mbufs[1]->refcnt == 1);
    EXPECT_TRUE(c.mbufs[2]->refcnt == 0);
    EXPECT_TRUE(c.mbufs[3]->refcnt == 0);
}

TEST(AmonDpdkPktBurst, PktburstTestResizeToZero)
{
    TestCtx c(2);

    c.burst.resize(0);

    ASSERT_TRUE(c.burst.size() == 0);
    EXPECT_TRUE(c.mbufs[0]->refcnt == 0);
    EXPECT_TRUE(c.mbufs[1]->refcnt == 0);
}

TEST(AmonDpdkPktBurst, PktburstTestEraseSingleItemRange)
{
    TestCtx c(2);

    c.burst.erase(c.burst.begin(), c.burst.begin() + 1);

    EXPECT_TRUE(c.mbufs[0]->refcnt == 0);
    EXPECT_TRUE(c.mbufs[1]->refcnt == 1);

    ASSERT_TRUE(c.burst.size() == 1);
    EXPECT_TRUE(c.burst[0]->id == 1);
}

TEST(AmonDpdkPktBurst, PktburstTestEraseAllOneByOne)
{
    TestCtx c(2);

    c.burst.erase(c.burst.begin());

    EXPECT_TRUE(c.mbufs[0]->refcnt == 0);
    EXPECT_TRUE(c.mbufs[1]->refcnt == 1);

    ASSERT_TRUE(c.burst.size() == 1);
    EXPECT_TRUE(c.burst[0]->id == 1);

    c.burst.erase(c.burst.begin());

    EXPECT_TRUE(c.mbufs[0]->refcnt == 0);
    EXPECT_TRUE(c.mbufs[1]->refcnt == 0);

    ASSERT_TRUE(c.burst.size() == 0);
}

TEST(AmonDpdkPktBurst, PktburstEraseOnEmptyBurst)
{
    TestCtx c(4);

    c.burst.resize(0);
    ASSERT_TRUE(c.burst.size() == 0);
    ASSERT_TRUE(c.burst.begin() == c.burst.end());

    EXPECT_DEATH(c.burst.erase(c.burst.begin()), "ASSERTION FAILED");
}

TEST(AmonDpdkPktBurst, PktburstReleaseOnEmptyBurst)
{
    TestCtx c(4);

    c.burst.resize(0);
    ASSERT_TRUE(c.burst.size() == 0);
    ASSERT_TRUE(c.burst.begin() == c.burst.end());

    c.burst.release(c.burst.begin(), c.burst.begin());
    EXPECT_TRUE(c.burst.size() == 0);
    EXPECT_TRUE(c.burst.capacity() == 4);
}

TEST(AmonDpdkPktBurst, PktburstClearOnEmptyBurst)
{
    TestCtx c(4);

    c.burst.resize(0);
    ASSERT_TRUE(c.burst.size() == 0);
    ASSERT_TRUE(c.burst.capacity() == 4);
    ASSERT_TRUE(c.mbufs[0]->refcnt == 0);
    ASSERT_TRUE(c.mbufs[1]->refcnt == 0);
    ASSERT_TRUE(c.mbufs[2]->refcnt == 0);
    ASSERT_TRUE(c.mbufs[3]->refcnt == 0);

    c.burst.clear();
    EXPECT_TRUE(c.burst.size() == 0);
    EXPECT_TRUE(c.burst.capacity() == 4);
    EXPECT_TRUE(c.mbufs[0]->refcnt == 0);
    EXPECT_TRUE(c.mbufs[1]->refcnt == 0);
    EXPECT_TRUE(c.mbufs[2]->refcnt == 0);
    EXPECT_TRUE(c.mbufs[3]->refcnt == 0);
}

TEST(AmonDpdkPktBurst, PktburstResizeToSameSize)
{
    TestCtx c(2);

    c.burst.resize(2);

    EXPECT_TRUE(c.mbufs[0]->refcnt == 1);
    EXPECT_TRUE(c.mbufs[1]->refcnt == 1);
    EXPECT_TRUE(c.burst.size() == 2);
    EXPECT_TRUE(c.burst[0]->id == 0);
    EXPECT_TRUE(c.burst[1]->id == 1);
}

TEST(AmonDpdkPktBurst, PktburstEraseWithOutOfBoundsIterator)
{
    TestCtx c(2);

    EXPECT_DEATH(c.burst.erase(c.burst.end()), "ASSERTION FAILED");
}

TEST(AmonDpdkPktBurst, PktburstReleaseWithInvalidRange)
{
    TestCtx c(2);

    EXPECT_DEATH(c.burst.release(c.burst.begin() + 2, c.burst.begin() + 1), "ASSERTION FAILED");
}

TEST(AmonDpdkPktBurst, PktburstFill)
{
    TestCtx c(4);

    c.burst.resize(0);
    ASSERT_TRUE(c.burst.size() == 0);
    ASSERT_TRUE(c.burst.begin() == c.burst.end());

    c.burst.fill([&](std::span<MockMbuf*> mbufs) -> size_t {
        for (unsigned i = 0; i < 2; ++i) {
            mbufs[i] = c.mbufs[i].get();
            ++c.mbufs[i]->refcnt;
        }
        return 2;
    });
    ASSERT_TRUE(c.burst.size() == 2);
    EXPECT_TRUE(c.burst[0]->id == 0);
    EXPECT_TRUE(c.burst[1]->id == 1);
}

TEST(AmonDpdkPktBurst, PktburstTestDestructor)
{
    TestCtx c(4);

    {
        TestBurst burst2(c.burst.size());
        c.burst.move(c.burst.begin(), c.burst.end(), burst2);

        ASSERT_TRUE(c.burst.size() == 0);
        ASSERT_TRUE(burst2.size() == 4);
        ASSERT_TRUE(c.mbufs[0]->refcnt == 1);
        ASSERT_TRUE(c.mbufs[1]->refcnt == 1);
        ASSERT_TRUE(c.mbufs[2]->refcnt == 1);
        ASSERT_TRUE(c.mbufs[3]->refcnt == 1);
    }

    EXPECT_TRUE(c.mbufs[0]->refcnt == 0);
    EXPECT_TRUE(c.mbufs[1]->refcnt == 0);
    EXPECT_TRUE(c.mbufs[2]->refcnt == 0);
    EXPECT_TRUE(c.mbufs[3]->refcnt == 0);
}

TEST(AmonDpdkPktBurst, PktburstTestMoveConstructor)
{
    TestCtx c(4);

    {
        const TestBurst burst2 {std::move(c.burst)};

        // c.burst.size() is undefined at this point (moved from object)
        EXPECT_TRUE(burst2.size() == 4);
        EXPECT_TRUE(c.mbufs[0]->refcnt == 1);
        EXPECT_TRUE(c.mbufs[1]->refcnt == 1);
        EXPECT_TRUE(c.mbufs[2]->refcnt == 1);
        EXPECT_TRUE(c.mbufs[3]->refcnt == 1);
    }

    EXPECT_TRUE(c.mbufs[0]->refcnt == 0);
    EXPECT_TRUE(c.mbufs[1]->refcnt == 0);
    EXPECT_TRUE(c.mbufs[2]->refcnt == 0);
    EXPECT_TRUE(c.mbufs[3]->refcnt == 0);
}

TEST(AmonDpdkPktBurst, PktburstTestMoveAssignmentOperator)
{
    std::vector<std::unique_ptr<MockMbuf>> mbufs;

    {
        TestCtx c(4);

        {
            TestBurst burst2(4);

            burst2 = std::move(c.burst);
            // c.burst size() is undefined at this point (moved from object)
            EXPECT_TRUE(burst2.size() == 4);
            EXPECT_TRUE(c.mbufs[0]->refcnt == 1);
            EXPECT_TRUE(c.mbufs[1]->refcnt == 1);
            EXPECT_TRUE(c.mbufs[2]->refcnt == 1);
            EXPECT_TRUE(c.mbufs[3]->refcnt == 1);
        }

        mbufs = std::move(c.mbufs);
    }

    EXPECT_TRUE(mbufs[0]->refcnt == 0);
    EXPECT_TRUE(mbufs[1]->refcnt == 0);
    EXPECT_TRUE(mbufs[2]->refcnt == 0);
    EXPECT_TRUE(mbufs[3]->refcnt == 0);
}

#endif

} // namespace amon::dpdk
