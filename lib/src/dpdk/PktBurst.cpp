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

struct MockRte {
    using mbuf = MockMbuf;

    static void pktmbuf_refcnt_update(mbuf* m, int v) { m->refcnt += v; }

    static void pktmbuf_free(mbuf* mbuf)
    {
        if (mbuf != nullptr) {
            mbuf->refcnt -= 1;
        }
    }

    static void pktmbuf_free_bulk(mbuf** mbufs, unsigned count)
    {
        for (unsigned i = 0; i < count; ++i) {
            pktmbuf_free(mbufs[i]);
        }
    }
};

} // namespace

using TestBurst = amon::dpdk::PktBurstImpl<MockRte>;

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

TEST_CASE("PktBurst test erase single")
{
    TestCtx c(4);

    c.burst.erase(c.burst.begin() + 1);

    CHECK(c.mbufs[0]->refcnt == 1);
    CHECK(c.mbufs[1]->refcnt == 0);
    CHECK(c.mbufs[2]->refcnt == 1);
    CHECK(c.mbufs[3]->refcnt == 1);

    REQUIRE(c.burst.size() == 3);
    CHECK(c.burst[0]->id == 0);
    CHECK(c.burst[1]->id == 2);
    CHECK(c.burst[2]->id == 3);
}

TEST_CASE("PktBurst test erase range")
{
    TestCtx c(4);

    c.burst.erase(c.burst.begin(), c.burst.begin() + 2);

    CHECK(c.mbufs[0]->refcnt == 0);
    CHECK(c.mbufs[1]->refcnt == 0);
    CHECK(c.mbufs[2]->refcnt == 1);
    CHECK(c.mbufs[3]->refcnt == 1);

    REQUIRE(c.burst.size() == 2);
    CHECK(c.burst[0]->id == 2);
    CHECK(c.burst[1]->id == 3);
}

TEST_CASE("PktBurst test erase invalid range")
{
    TestCtx c(4);

    CHECK_THROWS_AS(c.burst.erase(c.burst.begin() + 4), std::invalid_argument);
    CHECK_THROWS_AS(c.burst.erase(c.burst.begin() + 1, c.burst.end() + 1), std::invalid_argument);
}

TEST_CASE("PktBurst test release")
{
    TestCtx c(3);

    c.burst.release(c.burst.begin(), c.burst.begin() + 2);

    CHECK(c.mbufs[0]->refcnt == 1);
    CHECK(c.mbufs[1]->refcnt == 1);
    CHECK(c.mbufs[2]->refcnt == 1);

    REQUIRE(c.burst.size() == 1);
    CHECK(c.burst[0]->id == 2);
}

TEST_CASE("PktBurst test release invalid range")
{
    TestCtx c(4);

    CHECK_THROWS_AS(
        c.burst.release(c.burst.begin() + 2, c.burst.begin() + 1),
        std::invalid_argument);
}

TEST_CASE("PktBurst test copy")
{
    TestCtx c(3);

    std::vector<MockMbuf*> out(2);
    c.burst.copy(c.burst.begin(), c.burst.begin() + 2, out.begin());

    REQUIRE(out[0] != nullptr);
    CHECK(out[0]->id == 0);
    REQUIRE(out[1] != nullptr);
    CHECK(out[1]->id == 1);

    CHECK(c.mbufs[0]->refcnt == 2);
    CHECK(c.mbufs[1]->refcnt == 2);
    CHECK(c.mbufs[2]->refcnt == 1);

    REQUIRE(c.burst.size() == 3);
    CHECK(c.burst[0]->id == 0);
    CHECK(c.burst[1]->id == 1);
    CHECK(c.burst[2]->id == 2);
}

TEST_CASE("PktBurst test copy to burst")
{
    TestCtx c(3);

    PktBurstImpl<MockRte> out(2);
    c.burst.copy(c.burst.begin(), c.burst.begin() + 2, out);

    REQUIRE(out.size() == 2);
    REQUIRE(out[0] != nullptr);
    CHECK(out[0]->id == 0);
    REQUIRE(out[1] != nullptr);
    CHECK(out[1]->id == 1);

    CHECK(c.mbufs[0]->refcnt == 2);
    CHECK(c.mbufs[1]->refcnt == 2);
    CHECK(c.mbufs[2]->refcnt == 1);

    REQUIRE(c.burst.size() == 3);
    CHECK(c.burst[0]->id == 0);
    CHECK(c.burst[1]->id == 1);
    CHECK(c.burst[2]->id == 2);
}

TEST_CASE("PktBurst test copy to burst that is too small")
{
    TestCtx c(3);

    PktBurstImpl<MockRte> out(1);
    CHECK_THROWS_AS(c.burst.copy(c.burst.begin(), c.burst.begin() + 2, out), std::length_error);
}

TEST_CASE("PktBurst test copyIf")
{
    TestCtx c(3);

    std::vector<MockMbuf*> out(3);
    auto pred = [](MockMbuf* m) { return m->id % 2 == 0; };
    auto it = c.burst.copyIf(pred, out.begin());

    CHECK(it == out.begin() + 2);
    REQUIRE(out[0] != nullptr);
    CHECK(out[0]->id == 0);
    REQUIRE(out[1] != nullptr);
    CHECK(out[1]->id == 2);

    CHECK(c.mbufs[0]->refcnt == 2);
    CHECK(c.mbufs[1]->refcnt == 1);
    CHECK(c.mbufs[2]->refcnt == 2);

    REQUIRE(c.burst.size() == 3);
    CHECK(c.burst[0]->id == 0);
    CHECK(c.burst[1]->id == 1);
    CHECK(c.burst[2]->id == 2);
}

TEST_CASE("PktBurst test copyIf to burst")
{
    TestCtx c(3);

    PktBurstImpl<MockRte> out(3);
    auto pred = [](MockMbuf* m) { return m->id % 2 == 0; };
    c.burst.copyIf(pred, out);

    REQUIRE(out.size() == 2);
    REQUIRE(out[0] != nullptr);
    CHECK(out[0]->id == 0);
    REQUIRE(out[1] != nullptr);
    CHECK(out[1]->id == 2);

    CHECK(c.mbufs[0]->refcnt == 2);
    CHECK(c.mbufs[1]->refcnt == 1);
    CHECK(c.mbufs[2]->refcnt == 2);

    REQUIRE(c.burst.size() == 3);
    CHECK(c.burst[0]->id == 0);
    CHECK(c.burst[1]->id == 1);
    CHECK(c.burst[2]->id == 2);
}

TEST_CASE("PktBurst test copyIf to burst that is too small")
{
    TestCtx c(3);

    PktBurstImpl<MockRte> out(2);
    auto pred = [](MockMbuf* m) { return m->id % 2 == 0; };
    CHECK_THROWS_AS(c.burst.copyIf(pred, out), std::length_error);
}

TEST_CASE("PktBurst test move")
{
    TestCtx c(3);

    std::vector<MockMbuf*> out(2);
    c.burst.move(c.burst.begin(), c.burst.begin() + 2, out.begin());

    REQUIRE(out[0] != nullptr);
    CHECK(out[0]->id == 0);
    REQUIRE(out[1] != nullptr);
    CHECK(out[1]->id == 1);

    CHECK(c.mbufs[0]->refcnt == 1);
    CHECK(c.mbufs[1]->refcnt == 1);
    CHECK(c.mbufs[2]->refcnt == 1);

    REQUIRE(c.burst.size() == 1);
    CHECK(c.burst[0]->id == 2);
}

TEST_CASE("PktBurst test move to burst")
{
    TestCtx c(3);

    PktBurstImpl<MockRte> out(2);
    c.burst.move(c.burst.begin(), c.burst.begin() + 2, out);

    REQUIRE(out.size() == 2);
    REQUIRE(out[0] != nullptr);
    CHECK(out[0]->id == 0);
    REQUIRE(out[1] != nullptr);
    CHECK(out[1]->id == 1);

    CHECK(c.mbufs[0]->refcnt == 1);
    CHECK(c.mbufs[1]->refcnt == 1);
    CHECK(c.mbufs[2]->refcnt == 1);

    REQUIRE(c.burst.size() == 1);
    CHECK(c.burst[0]->id == 2);
}

TEST_CASE("PktBurst test move to burst that is too small")
{
    TestCtx c(3);

    PktBurstImpl<MockRte> out(1);
    CHECK_THROWS_AS(c.burst.move(c.burst.begin(), c.burst.begin() + 2, out), std::length_error);
}

TEST_CASE("PktBurst test moveIf")
{
    TestCtx c(3);

    std::vector<MockMbuf*> out(3);
    auto pred = [](MockMbuf* m) { return m->id % 2 == 0; };
    auto it = c.burst.moveIf(pred, out.begin());

    CHECK(it == out.begin() + 2);
    CHECK(out[0]->id == 0);
    CHECK(out[1]->id == 2);

    CHECK(c.mbufs[0]->refcnt == 1);
    CHECK(c.mbufs[1]->refcnt == 1);
    CHECK(c.mbufs[2]->refcnt == 1);

    REQUIRE(c.burst.size() == 1);
    CHECK(c.burst[0]->id == 1);
}

TEST_CASE("PktBurst test moveIf to burst")
{
    TestCtx c(3);

    PktBurstImpl<MockRte> out(3);
    auto pred = [](MockMbuf* m) { return m->id % 2 == 0; };
    c.burst.moveIf(pred, out);

    REQUIRE(out.size() == 2);
    CHECK(out[0]->id == 0);
    CHECK(out[1]->id == 2);

    CHECK(c.mbufs[0]->refcnt == 1);
    CHECK(c.mbufs[1]->refcnt == 1);
    CHECK(c.mbufs[2]->refcnt == 1);

    REQUIRE(c.burst.size() == 1);
    CHECK(c.burst[0]->id == 1);
}

TEST_CASE("PktBurst test moveIf to burst that is too small")
{
    TestCtx c(3);

    PktBurstImpl<MockRte> out(2);
    auto pred = [](MockMbuf* m) { return m->id % 2 == 0; };
    CHECK_THROWS_AS(c.burst.moveIf(pred, out), std::length_error);
}

TEST_CASE("PktBurst test eraseIf")
{
    TestCtx c(3);

    auto pred = [](MockMbuf* m) { return m->id % 2 == 0; };
    c.burst.eraseIf(pred);

    CHECK(c.mbufs[0]->refcnt == 0);
    CHECK(c.mbufs[1]->refcnt == 1);
    CHECK(c.mbufs[2]->refcnt == 0);

    REQUIRE(c.burst.size() == 1);
    CHECK(c.burst[0]->id == 1);
}

TEST_CASE("PktBurst test resize smaller")
{
    TestCtx c(3);

    c.burst.resize(2);

    CHECK(c.mbufs[0]->refcnt == 1);
    CHECK(c.mbufs[1]->refcnt == 1);
    CHECK(c.mbufs[2]->refcnt == 0);

    REQUIRE(c.burst.size() == 2);
    CHECK(c.burst[0]->id == 0);
    CHECK(c.burst[1]->id == 1);
}

TEST_CASE("PktBurst test resize larger")
{
    TestCtx c(4);

    c.burst.resize(2);
    c.burst.resize(4);

    CHECK(c.mbufs[0]->refcnt == 1);
    CHECK(c.mbufs[1]->refcnt == 1);
    CHECK(c.mbufs[2]->refcnt == 0);
    CHECK(c.mbufs[3]->refcnt == 0);

    REQUIRE(c.burst.size() == 4);
    CHECK(c.burst[0]->id == 0);
    CHECK(c.burst[1]->id == 1);
    CHECK(c.burst[2] == nullptr);
    CHECK(c.burst[3] == nullptr);
}

TEST_CASE("PktBurst test resize beyond capacity")
{
    TestCtx c(4);

    CHECK_THROWS_AS(c.burst.resize(5), std::length_error);

    CHECK(c.mbufs[0]->refcnt == 1);
    CHECK(c.mbufs[1]->refcnt == 1);
    CHECK(c.mbufs[2]->refcnt == 1);
    CHECK(c.mbufs[3]->refcnt == 1);

    REQUIRE(c.burst.size() == 4);
    CHECK(c.burst[0]->id == 0);
    CHECK(c.burst[1]->id == 1);
    CHECK(c.burst[2]->id == 2);
    CHECK(c.burst[3]->id == 3);
}

TEST_CASE("PktBurst test clear")
{
    TestCtx c(3);

    c.burst.clear();

    CHECK(c.mbufs[0]->refcnt == 0);
    CHECK(c.mbufs[1]->refcnt == 0);
    CHECK(c.mbufs[2]->refcnt == 0);

    CHECK(c.burst.size() == 0);
}

TEST_CASE("PktBurst test operator[]")
{
    TestCtx c(2);

    CHECK(c.burst[0] == c.mbufs[0].get()); // NOLINT
    CHECK(c.burst[1] == c.mbufs[1].get());

    c.burst[1] = nullptr;
    CHECK(c.burst[1] == nullptr);
    CHECK(c.mbufs[1]->refcnt == 1);
}

TEST_CASE("PktBurst test available/capacity")
{
    TestCtx c(4);

    c.burst.resize(2);
    CHECK(c.burst.available() == 2);
    CHECK(c.burst.capacity() == 4);

    CHECK(c.mbufs[0]->refcnt == 1);
    CHECK(c.mbufs[1]->refcnt == 1);
    CHECK(c.mbufs[2]->refcnt == 0);
    CHECK(c.mbufs[3]->refcnt == 0);
}

TEST_CASE("PktBurst test resize to zero")
{
    TestCtx c(2);

    c.burst.resize(0);

    REQUIRE(c.burst.size() == 0);
    CHECK(c.mbufs[0]->refcnt == 0);
    CHECK(c.mbufs[1]->refcnt == 0);
}

TEST_CASE("PktBurst test erase single-item range")
{
    TestCtx c(2);

    c.burst.erase(c.burst.begin(), c.burst.begin() + 1);

    CHECK(c.mbufs[0]->refcnt == 0);
    CHECK(c.mbufs[1]->refcnt == 1);

    REQUIRE(c.burst.size() == 1);
    CHECK(c.burst[0]->id == 1);
}

TEST_CASE("PktBurst test erase all one by one")
{
    TestCtx c(2);

    c.burst.erase(c.burst.begin());

    CHECK(c.mbufs[0]->refcnt == 0);
    CHECK(c.mbufs[1]->refcnt == 1);

    REQUIRE(c.burst.size() == 1);
    CHECK(c.burst[0]->id == 1);

    c.burst.erase(c.burst.begin());

    CHECK(c.mbufs[0]->refcnt == 0);
    CHECK(c.mbufs[1]->refcnt == 0);

    REQUIRE(c.burst.size() == 0);
}

TEST_CASE("PktBurst erase on empty burst")
{
    TestCtx c(4);

    c.burst.resize(0);
    REQUIRE(c.burst.size() == 0);
    REQUIRE(c.burst.begin() == c.burst.end());

    CHECK_THROWS_AS(c.burst.erase(c.burst.begin()), std::invalid_argument);
}

TEST_CASE("PktBurst release on empty burst")
{
    TestCtx c(4);

    c.burst.resize(0);
    REQUIRE(c.burst.size() == 0);
    REQUIRE(c.burst.begin() == c.burst.end());

    c.burst.release(c.burst.begin(), c.burst.begin());
    CHECK(c.burst.size() == 0);
    CHECK(c.burst.capacity() == 4);
}

TEST_CASE("PktBurst clear on empty burst")
{
    TestCtx c(4);

    c.burst.resize(0);
    REQUIRE(c.burst.size() == 0);
    REQUIRE(c.burst.capacity() == 4);
    REQUIRE(c.mbufs[0]->refcnt == 0);
    REQUIRE(c.mbufs[1]->refcnt == 0);
    REQUIRE(c.mbufs[2]->refcnt == 0);
    REQUIRE(c.mbufs[3]->refcnt == 0);

    c.burst.clear();
    CHECK(c.burst.size() == 0);
    CHECK(c.burst.capacity() == 4);
    CHECK(c.mbufs[0]->refcnt == 0);
    CHECK(c.mbufs[1]->refcnt == 0);
    CHECK(c.mbufs[2]->refcnt == 0);
    CHECK(c.mbufs[3]->refcnt == 0);
}

TEST_CASE("PktBurst resize to same size")
{
    TestCtx c(2);

    c.burst.resize(2);

    CHECK(c.mbufs[0]->refcnt == 1);
    CHECK(c.mbufs[1]->refcnt == 1);
    CHECK(c.burst.size() == 2);
    CHECK(c.burst[0]->id == 0);
    CHECK(c.burst[1]->id == 1);
}

TEST_CASE("PktBurst erase with out of bounds iterator throws")
{
    TestCtx c(2);

    CHECK_THROWS_AS(c.burst.erase(c.burst.end()), std::invalid_argument);

    CHECK(c.mbufs[0]->refcnt == 1);
    CHECK(c.mbufs[1]->refcnt == 1);
    CHECK(c.burst.size() == 2);
    CHECK(c.burst[0]->id == 0);
    CHECK(c.burst[1]->id == 1);
}

TEST_CASE("PktBurst release with invalid range throws")
{
    TestCtx c(2);

    CHECK_THROWS_AS(c.burst.release(c.burst.begin() + 2, c.burst.begin() + 1), std::exception);

    CHECK(c.mbufs[0]->refcnt == 1);
    CHECK(c.mbufs[1]->refcnt == 1);
    CHECK(c.burst.size() == 2);
    CHECK(c.burst[0]->id == 0);
    CHECK(c.burst[1]->id == 1);
}

TEST_CASE("PktBurst fill")
{
    TestCtx c(4);

    c.burst.resize(0);
    REQUIRE(c.burst.size() == 0);
    REQUIRE(c.burst.begin() == c.burst.end());

    c.burst.fill([&](std::span<MockRte::mbuf*> mbufs) -> size_t {
        for (unsigned i = 0; i < 2; ++i) {
            mbufs[i] = c.mbufs[i].get();
            ++c.mbufs[i]->refcnt;
        }
        return 2;
    });
    REQUIRE(c.burst.size() == 2);
    CHECK(c.burst[0]->id == 0);
    CHECK(c.burst[1]->id == 1);
}

TEST_CASE("PktBurst test destructor")
{
    TestCtx c(4);

    {
        PktBurstImpl<MockRte> burst2(c.burst.size());
        c.burst.move(c.burst.begin(), c.burst.end(), burst2);

        REQUIRE(c.burst.size() == 0);
        REQUIRE(burst2.size() == 4);
        REQUIRE(c.mbufs[0]->refcnt == 1);
        REQUIRE(c.mbufs[1]->refcnt == 1);
        REQUIRE(c.mbufs[2]->refcnt == 1);
        REQUIRE(c.mbufs[3]->refcnt == 1);
    }

    CHECK(c.mbufs[0]->refcnt == 0);
    CHECK(c.mbufs[1]->refcnt == 0);
    CHECK(c.mbufs[2]->refcnt == 0);
    CHECK(c.mbufs[3]->refcnt == 0);
}

TEST_CASE("PktBurst test move constructor")
{
    TestCtx c(4);

    {
        const PktBurstImpl<MockRte> burst2 {std::move(c.burst)};

        // c.burst.size() is undefined at this point (moved from object)
        CHECK(burst2.size() == 4);
        CHECK(c.mbufs[0]->refcnt == 1);
        CHECK(c.mbufs[1]->refcnt == 1);
        CHECK(c.mbufs[2]->refcnt == 1);
        CHECK(c.mbufs[3]->refcnt == 1);
    }

    CHECK(c.mbufs[0]->refcnt == 0);
    CHECK(c.mbufs[1]->refcnt == 0);
    CHECK(c.mbufs[2]->refcnt == 0);
    CHECK(c.mbufs[3]->refcnt == 0);
}

TEST_CASE("PktBurst test move assignment operator")
{
    std::vector<std::unique_ptr<MockMbuf>> mbufs;

    {
        TestCtx c(4);

        {
            PktBurstImpl<MockRte> burst2(4);

            burst2 = std::move(c.burst);
            // c.burst size() is undefined at this point (moved from object)
            CHECK(burst2.size() == 4);
            CHECK(c.mbufs[0]->refcnt == 1);
            CHECK(c.mbufs[1]->refcnt == 1);
            CHECK(c.mbufs[2]->refcnt == 1);
            CHECK(c.mbufs[3]->refcnt == 1);
        }

        mbufs = std::move(c.mbufs);
    }

    CHECK(mbufs[0]->refcnt == 0);
    CHECK(mbufs[1]->refcnt == 0);
    CHECK(mbufs[2]->refcnt == 0);
    CHECK(mbufs[3]->refcnt == 0);
}

#endif

} // namespace amon::dpdk
