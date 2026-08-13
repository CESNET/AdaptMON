/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief A DPDK packet wrapper that encapsulates an rte_mbuf and an amon::Packet.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/Packet.hpp>
#include <amon/amon.hpp>

#include <rte_mbuf.h>
#include <rte_mbuf_core.h>
#include <rte_version.h>

namespace amon::dpdk {

// This is because of rte_mbuf containing RTE_MARKER which does not compile with -Wpedantic.
// It needs to be placed here to wrap the entire class for the suppression to work.
#if RTE_VERSION < RTE_VERSION_NUM(24, 0, 0, 0)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

/**
 * @brief A DPDK packet wrapper that encapsulates an rte_mbuf along with an
 * amon::Packet structure stored in the mbuf's private area.
 */
struct DpdkPacket : private rte_mbuf {
    /**
     * @brief Get the underlying rte_mbuf.
     *
     * @return Pointer to the embedded rte_mbuf.
     */
    rte_mbuf* mbuf() { return static_cast<rte_mbuf*>(this); }

    /**
     * @brief Get the amon::Packet structure stored in the mbuf's private area.
     *
     * @pre The mbuf's priv_size must be at least sizeof(amon::Packet).
     *
     * @return Reference to the amon::Packet stored in the private area.
     */
    amon::Packet& packet()
    {
        AMON_ASSERT(mbuf()->priv_size >= sizeof(amon::Packet));
        return *reinterpret_cast<amon::Packet*>(rte_mbuf_to_priv(mbuf()));
    }
};

#if RTE_VERSION < RTE_VERSION_NUM(24, 0, 0, 0)
#pragma GCC diagnostic pop
#endif

static_assert(std::is_standard_layout_v<DpdkPacket>, "DpdkPacket is not standard layout");
static_assert(
    std::is_trivially_default_constructible_v<DpdkPacket>,
    "DpdkPacket is not trivially default constructible");
static_assert(
    std::is_trivially_destructible_v<DpdkPacket>,
    "DpdkPacket is not trivially destructible");
static_assert(std::is_trivial_v<DpdkPacket>, "DpdkPacket is not trivial");
static_assert(sizeof(DpdkPacket) == sizeof(rte_mbuf), "DpdkPacket is not same size as rte_mbuf");
static_assert(
    alignof(DpdkPacket) == alignof(rte_mbuf),
    "DpdkPacket does not have same alignment as rte_mbuf");
static_assert(
    std::is_trivially_destructible_v<amon::Packet>,
    "Packet structure is not trivially destructible");

} // namespace amon::dpdk
