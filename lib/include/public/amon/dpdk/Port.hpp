/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief DPDK Port handling.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/Logger.hpp>
#include <amon/dpdk/DpdkPacket.hpp>
#include <amon/dpdk/PktBurst.hpp>
#include <amon/dpdk/PktMempool.hpp>
#include <amon/util/Formatter.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace amon::dpdk {

/**
 * @brief Capabilities and limits of a DPDK port device.
 *
 * rte_eth_dev_info abstraction.
 */
struct PortDevInfo {
    /**
     * @brief RX info structure.
     */
    struct Rx {
        /// Maximum number of Rx queues
        uint16_t nQueuesMax;
        /// Current number of Rx queues
        uint16_t nQueuesCurrent;
        /// Minimum number of Rx descriptors
        uint16_t nDescriptorsMin;
        /// Maximum number of Rx descriptors
        uint16_t nDescriptorsMax;
        /// RX offload capabilities (bitmask of DPDK constants for offloads)
        uint64_t offloadsCapa;
    };
    PortDevInfo::Rx rx; ///< RX limits.

    /**
     * @brief TX info structure.
     */
    struct Tx {
        /// Maximum number of Tx queues
        uint16_t nQueuesMax;
        /// Current number of Tx queues
        uint16_t nQueuesCurrent;
        /// Minimum number of Tx descriptors
        uint16_t nDescriptorsMin;
        /// Maximum number of Tx descriptors
        uint16_t nDescriptorsMax;
        /// TX offload capabilities (bitmask of DPDK constants for offloads)
        uint64_t offloadsCapa;
    };
    PortDevInfo::Tx tx; ///< TX limits.

    /// Minimum MTU allowed
    uint16_t mtuMin;
    /// Maximum MTU allowed
    uint16_t mtuMax;
};

/**
 * @brief Configuration for a DPDK port.
 */
struct PortConfig {
    /**
     * @brief RX queue configuration.
     */
    struct Rx {
        /// Number of RX queues.
        uint16_t nQueues = 0;
        /// Number of RX descriptors per queue.
        uint16_t nDescriptors = 0;
        /// Enable promiscuous mode.
        bool promiscuous = true;
        /// Mempool provider for RX.
        std::shared_ptr<PktMempoolProvider> mempoolProvider;
        /// RX offload flags (use DPDK constants for offloads).
        uint64_t offloads = 0;
    };
    /// RX configuration (optional).
    std::optional<PortConfig::Rx> rx = std::nullopt;

    /**
     * @brief TX queue configuration.
     */
    struct Tx {
        /// Number of TX queues.
        uint16_t nQueues = 0;
        /// Number of TX descriptors per queue.
        uint16_t nDescriptors = 0;
        /// TX offload flags (use DPDK constants for offloads).
        uint64_t offloads = 0;
    };
    /// TX configuration (optional).
    std::optional<PortConfig::Tx> tx = std::nullopt;

    /// MTU size.
    unsigned int mtu = 1500;
};

// Necessary forward declarations
class Port;
class PortHooks;
class PortRxQueue;
class PortTxQueue;
class XStats;

/**
 * @brief Handle for a DPDK port.
 *
 * Provides methods to query port information, configure the port, bring it up or down,
 * and obtain RX/TX queue handles. Instances of this class are typically obtained via
 * PortHandle::allPorts() or PortHandle::fromDevName().
 */
class PortHandle {
public:
    /**
     * @brief Construct a PortHandle for the given port ID.
     * @param portId The DPDK port ID.
     */
    PortHandle(uint16_t portId);

    /**
     * @brief Get handles for all available DPDK ports.
     * @return Vector of PortHandle objects for all ports.
     */
    static std::vector<PortHandle> allPorts();

    /**
     * @brief Get a PortHandle from a device name.
     * @param devName The device name (e.g., "0000:01:00.0").
     * @return PortHandle for the given device name.
     */
    static PortHandle fromDevName(std::string_view devName);

    /**
     * @brief Get the port ID.
     * @return The DPDK port ID.
     */
    uint16_t id() const;

    /**
     * @brief Get the device name of the port.
     * @return Device name as a string.
     */
    std::string devName() const;

    /**
     * @brief Get the driver name of the port.
     * @return Driver name as a string.
     */
    std::string driverName() const;

    /**
     * @brief Get the device information of the port.
     * @return Port device information.
     */
    PortDevInfo devInfo() const;

    /**
     * @brief Get the NUMA socket ID of the port. If the socket ID cannot be determined (e.g. not
     * supported by the driver, happens for vdevs), `SocketID::ANY` is returned.
     * @return Socket ID.
     */
    SocketID socketId() const;

    /**
     * @brief Configure the port with the given configuration.
     *
     * After configuration, the link is in the down state. To start receiving/sending data, you must
     * call @ref PortHandle::up() first. This is to that you can configure the port beforehand and
     * only bring it up when everything is set-up and ready.
     *
     * @param config Port configuration parameters.
     */
    void configure(const PortConfig& config);

    /**
     * @brief Configure the port with the given configuration and hooks.
     *
     * After configuration, the link is in the down state. To start receiving/sending data, you must
     * call @ref PortHandle::up() first. This is to that you can configure the port beforehand and
     * only bring it up when everything is set-up and ready.
     *
     * @param config Port configuration parameters.
     * @param hooks Vector of unique pointers to PortHooks to be called.
     */
    void configure(const PortConfig& config, std::vector<std::unique_ptr<PortHooks>> hooks);

    /**
     * @brief Bring the port down (disable it).
     */
    void down();

    /**
     * @brief Bring the port up (enable it).
     */
    void up();

    /**
     * @brief Get a handle for a TX queue of the port.
     * @param id Queue ID.
     * @return PortTxQueue handle for the given queue.
     */
    PortTxQueue txQueue(uint16_t id);

    /**
     * @brief Get a handle for an RX queue of the port.
     * @param id Queue ID.
     * @return PortRxQueue handle for the given queue.
     */
    PortRxQueue rxQueue(uint16_t id);

    /**
     * @brief Get an instance of the portHandle's port's extended statistics.
     * @throws DpdkError if the port is unconfigured.
     * @return XStats instance of the desired port.
     */
    XStats xstats();

private:
    uint16_t m_portId;
    std::shared_ptr<Port> m_port;
};

/**
 * @brief Represents a transmit queue for a DPDK port.
 *
 * Provides methods to transmit packets through a specific TX queue of a DPDK port.
 * Instances of this class are typically obtained via PortHandle::txQueue().
 */
class PortTxQueue {
public:
    /**
     * @brief Transmit as many packets as possible from the burst.
     *
     * Attempts to transmit the packets in the given burst, returning a span of the packets that
     * were not transmitted (if any).
     *
     * Ownership of transmitted packets is transferred to the TX queue, caller does not free them.
     *
     * @param burst Span of packet pointers to transmit.
     * @return Span of packet pointers that were not transmitted.
     */
    std::span<rte_mbuf*> transmit(std::span<rte_mbuf*> burst) noexcept;

    /**
     * @brief Transmit as many packets as possible from the burst.
     *
     * Attempts to transmit the packets in the given burst. Transmitted packets are released from
     * the burst. Packets that were unable to be transmitted are left in the burst and moved to the
     * beginning.
     *
     * @param burst Reference to a PktBurst containing packets to transmit.
     */
    void transmit(PktBurst<rte_mbuf>& burst) noexcept;

    /// @copydoc amon::dpdk::PortTxQueue::transmit(PktBurst<rte_mbuf>&)
    void transmit(PktBurst<DpdkPacket>& burst) noexcept;

    /**
     * @brief Transmit all packets in the burst, blocking until all are sent.
     *
     * Ownership of transmitted packets is transferred to the TX queue, caller does not free them.
     *
     * @param burst Span of packet pointers to transmit.
     */
    void transmitAllBlocking(std::span<rte_mbuf*> burst) noexcept;

    /**
     * @brief Transmit all packets in the given PktBurst, blocking until all are sent. Transmitted
     * packets are released from the burst.
     *
     * @param burst Reference to a PktBurst containing packets to transmit.
     */
    void transmitAllBlocking(PktBurst<rte_mbuf>& burst) noexcept;

    /// @copydoc amon::dpdk::PortTxQueue::transmitAllBlocking(PktBurst<rte_mbuf>&)
    void transmitAllBlocking(PktBurst<DpdkPacket>& burst) noexcept;

private:
    friend class Port;

    PortTxQueue(PortHandle port, uint16_t queueId);
    uint16_t m_queueId;
    PortHandle m_port;
};

/**
 * @brief Represents a receive queue for a DPDK port.
 *
 * Provides methods to receive packets from a specific RX queue of a DPDK port.
 * Instances of this class are typically obtained via PortHandle::rxQueue().
 */
class PortRxQueue {
public:
    /**
     * @brief Receive packets into the provided burst span.
     *
     * Attempts to receive packets from the RX queue into the given span,
     * returning a span of the received packet pointers.
     *
     * @warning Entries in `burst` that were not filled might still have been overwritten by
     * garbage. They do not stay untouched. This is a weird oddity of the underlying
     * rte_eth_rx_burst function.
     *
     * @param burst Span of packet pointers to fill with received packets.
     * @return Span of packet pointers that were filled with received packets.
     */
    std::span<rte_mbuf*> receive(std::span<rte_mbuf*> burst) noexcept;

    /**
     * @brief Receive packets into the given PktBurst.
     *
     * Attempts to receive packets from the RX queue and append them to the provided PktBurst.
     *
     * @param burst Reference to a PktBurst to append received packets to.
     */
    void receive(PktBurst<rte_mbuf>& burst) noexcept;

    /// @copydoc amon::dpdk::PortRxQueue::receive(PktBurst<rte_mbuf>&)
    void receive(PktBurst<DpdkPacket>& burst) noexcept;

    /**
     * @brief Receive at most `maxPkts` packets into the given PktBurst.
     *
     * Attempts to receive at most `maxPkts` from the RX queue and append them to the provided
     * PktBurst.
     *
     * @param burst Reference to a PktBurst to append received packets to.
     * @param maxPkts Maximum number of packets to receive.
     */
    void receive(PktBurst<rte_mbuf>& burst, size_t maxPkts) noexcept;

    /// @copydoc amon::dpdk::PortRxQueue::receive(PktBurst<rte_mbuf>&, size_t)
    void receive(PktBurst<DpdkPacket>& burst, size_t maxPkts) noexcept;

private:
    friend class Port;

    PortRxQueue(PortHandle port, uint16_t queueId);
    uint16_t m_queueId;
    PortHandle m_port;
};

/**
 * @brief Format PortHandle for output.
 * @param port PortHandle to format.
 * @param fmt Formatter to write to.
 */
inline void format(const PortHandle& port, amon::util::Formatter& fmt)
{
    fmt.format("Port(id={}, dev={})", port.id(), port.devName());
}

} // namespace amon::dpdk
