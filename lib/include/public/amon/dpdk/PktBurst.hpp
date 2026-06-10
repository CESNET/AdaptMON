/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Packet burst representation.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/amon.hpp>

#include <algorithm>
#include <cstddef>
#include <vector>

#include <rte_mbuf.h>

namespace amon::dpdk {

/**
 * @brief DPDK RTE mbuf wrapper for PktBurst.
 */
struct PktBurstDpdkRte {
    // NOLINTBEGIN
    // @cond DO_NOT_DOCUMENT
    using mbuf = ::rte_mbuf;
    static constexpr auto pktmbuf_refcnt_update = ::rte_pktmbuf_refcnt_update;
    static constexpr auto pktmbuf_free = ::rte_pktmbuf_free;
    static constexpr auto pktmbuf_free_bulk = ::rte_pktmbuf_free_bulk;
    // @endcond
    // NOLINTEND
};

// @cond DO_NOT_DOCUMENT
namespace detail {
template <typename Fn, typename... Args>
constexpr bool IsNoexceptCallable = noexcept(std::declval<Fn&>()(std::declval<Args>()...));
} // namespace detail
// @endcond

/**
 * @brief Generic packet burst container for DPDK mbufs.
 *
 * This class expects to claim ownership of the mbufs it holds. All the methods unless explicitly
 * specified otherwise perform pktmbuf_free operations if necessary.
 *
 * @tparam rte Traits class providing DPDK mbuf operations (default: PktBurstDpdkRte)
 */
template <typename Rte = PktBurstDpdkRte>
class PktBurstImpl {
public:
    /**
     * @brief Const iterator type for traversing the burst.
     */
    using ConstIterator = Rte::mbuf* const*;

    /**
     * @brief Iterator type for traversing the burst.
     */
    using Iterator = Rte::mbuf**;

    /**
     * @brief Construct a burst with given capacity.
     * @param capacity Maximum number of packets in the burst.
     */
    PktBurstImpl(std::size_t capacity)
        : m_data(capacity, nullptr)
    {
    }

    /**
     * @brief Disable copy constructor.
     */
    PktBurstImpl(const PktBurstImpl&) = delete;

    /**
     * @brief Disable copy assignment.
     */
    PktBurstImpl& operator=(const PktBurstImpl&) = delete;

    /**
     * @brief Move constructor.
     * @param other The moved from instance.
     */
    PktBurstImpl(PktBurstImpl&& other) noexcept
        : m_data(std::exchange(other.m_data, {}))
        , m_size(std::exchange(other.m_size, 0))
    {
    }

    /**
     * @brief Move assignment.
     * @param other The moved from instance.
     * @return The return value.
     */
    PktBurstImpl& operator=(PktBurstImpl&& other) noexcept
    {
        if (&other != this) {
            clear();
            std::swap(m_data, other.m_data);
            std::swap(m_size, other.m_size);
        }
        return *this;
    }

    /**
     * @brief Destructor.
     */
    ~PktBurstImpl() { clear(); }

    /**
     * @brief Get iterator to the beginning of the burst.
     * @return The iterator.
     */
    Iterator begin() noexcept { return m_data.data(); }

    /**
     * @brief Get iterator to the beginning of the burst (const).
     * @return The iterator.
     */
    ConstIterator begin() const noexcept { return cbegin(); }

    /**
     * @brief Get iterator to the beginning of the burst (const).
     * @return The iterator.
     */
    ConstIterator cbegin() const noexcept { return m_data.data(); }

    /**
     * @brief Get iterator to the end of the burst.
     * @return The iterator.
     */
    Iterator end() noexcept { return m_data.data() + m_size; }

    /**
     * @brief Get iterator to the end of the burst.
     * @return The iterator.
     */
    ConstIterator end() const noexcept { return cend(); }

    /**
     * @brief Get iterator to the end of the burst.
     * @return The iterator.
     */
    ConstIterator cend() const noexcept { return m_data.data() + m_size; }

    /**
     * @brief Get pointer to the underlying mbuf pointer array.
     * @return The data.
     */
    Rte::mbuf** data() noexcept { return m_data.data(); }

    /**
     * @brief Get the number of packets in the burst.
     * @return The value.
     */
    size_t size() const noexcept { return m_size; }

    /**
     * @brief Get the maximum capacity of the burst.
     * @return The value.
     */
    size_t capacity() const noexcept { return m_data.size(); }

    /**
     * @brief Get the number of available slots in the burst.
     * @return The value.
     */
    size_t available() const noexcept { return capacity() - size(); }

    /**
     * @brief Access packet at given index (const).
     * @param index Index of the packet.
     * @return Pointer to mbuf.
     */
    const Rte::mbuf*& operator[](size_t index) const noexcept { return m_data[index]; }

    /**
     * @brief Access packet at given index.
     * @param index Index of the packet.
     * @return Reference to pointer to mbuf.
     */
    Rte::mbuf*& operator[](size_t index) noexcept { return m_data[index]; }

    /**
     * @brief Remove all packets from the burst and free them.
     */
    void clear() noexcept
    {
        if (m_size > 0) {
            Rte::pktmbuf_free_bulk(m_data.data(), static_cast<unsigned>(m_size));
            m_size = 0;
        }
    }

    /**
     * @brief Erase a single packet at the given position and free it.
     * @param pos Iterator to the packet to erase.
     * @warning Behavior is undefined if pos is out of range.
     */
    void erase(Iterator pos) noexcept
    {
        AMON_ASSERT(pos >= begin() && pos < end(), "invalid erase pos");

        Rte::pktmbuf_free(*pos);
        std::copy(pos + 1, end(), pos);
        m_size -= 1;
    }

    /**
     * @brief Erase a range of packets and free them.
     * @param first Iterator to first packet to erase.
     * @param last Iterator to one past the last packet to erase.
     * @warning Behavior is undefined if range is invalid.
     */
    void erase(Iterator first, Iterator last) noexcept
    {
        AMON_ASSERT(first <= last && last <= end(), "invalid erase range");

        if (first == last) {
            return;
        }

        auto count = static_cast<unsigned int>(std::distance(first, last));
        Rte::pktmbuf_free_bulk(&*first, count);
        std::copy(last, end(), first); // NOLINT(readability-suspicious-call-argument)
        m_size -= count;
    }

    /**
     * @brief Erase packets matching a predicate and free them.
     *
     * Iterates over the burst, removes all packets for which the predicate returns true,
     * and frees their memory using bulk free operations for efficiency. The order of the
     * remaining packets is preserved.
     *
     * @tparam Predicate Predicate type. Callable with (Rte::mbuf*) and returns bool.
     * @param pred Predicate to select packets for erasure.
     * @note This method is noexcept if the predicate is noexcept.
     */
    template <typename Predicate>
    void
    eraseIf(Predicate pred) noexcept(detail::IsNoexceptCallable<Predicate, typename Rte::mbuf*>)
    {
        // Temporary storage for bulk freeing of elements. We assume that this is a size that should
        // be big enough for most cases to free everything in 1 bulk free call.
        std::array<typename Rte::mbuf*, 64> freeBuf;

        Iterator readHead = begin();
        Iterator localWriteHead = begin();
        auto moveWriteHead = freeBuf.begin();
        Iterator oldEnd = end();

        while (readHead != oldEnd) {
            if (pred(*readHead)) {
                if (moveWriteHead == freeBuf.end()) [[unlikely]] {
                    Rte::pktmbuf_free_bulk(freeBuf.data(), freeBuf.size());
                    moveWriteHead = freeBuf.begin();
                }
                *(moveWriteHead++) = *(readHead++);
                --m_size;
            } else {
                *(localWriteHead++) = *(readHead++);
            }
        }

        Rte::pktmbuf_free_bulk(
            freeBuf.data(),
            static_cast<unsigned>(std::distance(freeBuf.begin(), moveWriteHead)));
    }

    /**
     * @brief Release a range of packets without freeing them (the caller takes over their
     * ownership).
     * @param first Iterator to first packet to release.
     * @param last Iterator to one past the last packet to release.
     * @warning Behavior is undefined if range is invalid.
     */
    void release(Iterator first, Iterator last) noexcept
    {
        AMON_ASSERT(first <= last && last <= end(), "invalid release range");

        if (first == last) {
            return;
        }

        auto count = static_cast<unsigned int>(std::distance(first, last));
        std::copy(last, end(), first); // NOLINT(readability-suspicious-call-argument)
        m_size -= count;
    }

    /**
     * @brief Copy packets to output iterator and increment their refcount.
     * @param first Iterator to first packet to copy.
     * @param last Iterator to one past the last packet to copy.
     * @param outputFirst Output iterator to destination.
     * @warning Behavior is undefined if range is invalid or the destination does not have enough
     * capacity.
     */
    template <typename OutputIt>
    void copy(Iterator first, Iterator last, OutputIt outputFirst) noexcept
    {
        AMON_ASSERT(first <= last && last <= end(), "invalid copy range");

        std::copy(first, last, outputFirst);
        for (Iterator it = first; it != last; ++it) {
            Rte::pktmbuf_refcnt_update(*it, 1);
        }
    }

    /**
     * @brief Copy packets to output and increment their refcount.
     * @param first Iterator to first packet to copy.
     * @param last Iterator to one past the last packet to copy.
     * @param output Destination burst.
     * @warning Behavior is undefined if range is invalid or the destination does not have enough
     * capacity.
     */
    void copy(Iterator first, Iterator last, PktBurstImpl& output) noexcept
    {
        AMON_ASSERT(
            output.available() >= static_cast<size_t>(std::distance(first, last)),
            "output burst does not have enough capacity");

        copy(first, last, output.data() + output.size());
        output.m_size += static_cast<size_t>(std::distance(first, last));
    }

    /**
     * @brief Copy packets matching predicate to output and increment their refcount.
     * @tparam Predicate Predicate type.
     * @tparam OutputIt Output iterator type.
     * @param pred Predicate to select packets.
     * @param outputFirst Output iterator to destination.
     * @return Output iterator to end of copied range.
     * @warning Behavior is undefined if the destination does not have enough capacity.
     */
    template <typename Predicate, typename OutputIt>
    OutputIt copyIf(Predicate pred, OutputIt outputFirst) noexcept(
        detail::IsNoexceptCallable<Predicate, typename Rte::mbuf*>)
    {
        auto outIt = outputFirst;
        for (Iterator it = begin(); it != end(); ++it) {
            if (pred(*it)) {
                (*outIt++) = *it;
                Rte::pktmbuf_refcnt_update(*it, 1);
            }
        }
        return outIt;
    }

    /**
     * @brief Copy packets matching predicate to output and increment their refcount.
     * @tparam Predicate Predicate type.
     * @param pred Predicate to select packets.
     * @param output Destination burst.
     * @warning Behavior is undefined if range is invalid or the destination does not have enough
     * capacity.
     */
    template <typename Predicate>
    void copyIf(Predicate pred, PktBurstImpl& output) noexcept
    {
        AMON_ASSERT(output.available() >= size(), "output burst does not have enough capacity");

        auto outputFirst = output.data() + output.size();
        auto outputLast = copyIf(pred, outputFirst);
        output.m_size += static_cast<size_t>(std::distance(outputFirst, outputLast));
    }

    /**
     * @brief Move packets to output iterator and remove from burst.
     * @param first Iterator to first packet to move.
     * @param last Iterator to one past the last packet to move.
     * @param outputFirst Output iterator to destination.
     * @warning Behavior is undefined if the destination does not have enough capacity.
     */
    template <typename OutputIt>
    void move(Iterator first, Iterator last, OutputIt outputFirst) noexcept
    {
        AMON_ASSERT(first <= last && last <= end(), "invalid move range");

        std::copy(first, last, outputFirst);
        release(first, last);
    }

    /**
     * @brief Move packets to output and remove from burst.
     * @param first Iterator to first packet to move.
     * @param last Iterator to one past the last packet to move.
     * @param output Destination burst.
     * @warning Undefined behavior if range is invalid or the destination does not have enough
     * capacity.
     */
    void move(Iterator first, Iterator last, PktBurstImpl& output) noexcept
    {
        AMON_ASSERT(
            output.available() >= static_cast<size_t>(std::distance(first, last)),
            "output burst does not have enough capacity");

        move(first, last, output.data() + output.size());
        output.m_size += static_cast<size_t>(std::distance(first, last));
    }

    /**
     * @brief Move packets matching predicate to output and remove from burst.
     * @tparam Predicate Predicate type.
     * @tparam OutputIt Output iterator type.
     * @param pred Predicate to select packets.
     * @param outputFirst Output iterator to destination.
     * @return Output iterator to end of moved range.
     * @warning It's up to the caller to ensure that the destination has enough capacity!
     */
    template <typename Predicate, typename OutputIt>
    OutputIt moveIf(Predicate pred, OutputIt outputFirst) noexcept(
        detail::IsNoexceptCallable<Predicate, typename Rte::mbuf*>)
    {
        Iterator readHead = begin();
        Iterator localWriteHead = begin();
        OutputIt moveWriteHead = outputFirst;
        Iterator oldEnd = end();

        while (readHead != oldEnd) {
            if (pred(*readHead)) {
                *(moveWriteHead++) = *(readHead++);
                --m_size;
            } else {
                *(localWriteHead++) = *(readHead++);
            }
        }

        return moveWriteHead;
    }

    /**
     * @brief Move packets matching predicate to output and remove from burst.
     * @tparam Predicate Predicate type.
     * @param pred Predicate to select packets.
     * @param output Destination burst.
     * @warning Behavior is undefined if the output burst does not have the necessary capacity.
     */
    template <typename Predicate>
    void moveIf(Predicate pred, PktBurstImpl& output) noexcept(
        detail::IsNoexceptCallable<Predicate, typename Rte::mbuf*>)
    {
        AMON_ASSERT(output.available() >= size(), "output burst does not have enough capacity");

        auto outputFirst = output.data() + output.size();
        auto outputLast = moveIf(pred, outputFirst);
        output.m_size += static_cast<size_t>(std::distance(outputFirst, outputLast));
    }

    /**
     * @brief Resize the burst to new size, freeing or zeroing as needed.
     * If resizing introduces new elements, their value is set to zero. If resizing gets rid of
     * elements, they are properly freed.
     * @param newSize New size of the burst.
     * @warning Behavior is undefined if newSize > capacity().
     */
    void resize(size_t newSize) noexcept
    {
        if (newSize <= m_size) {
            erase(begin() + newSize, end());

        } else {
            AMON_ASSERT(newSize <= capacity(), "not enough space");

            Iterator oldEnd = end();
            m_size = newSize;
            std::fill(oldEnd, end(), nullptr);
        }
    }

    /**
     * @brief Fill the burst with packets using a user-provided filler functor.
     *
     * The fillFn functor is called with a std::span of available slots (as pointers to mbuf
     * pointers). It should fill as many slots as possible and return the number of packets filled.
     *
     * Expected signature is: `size_t fillFn(std::span<Rte::mbuf*> mbufs)`
     *
     * @tparam FillFn Type of the filler functor.
     * @param fillFn Functor that fills the available slots and returns the number of packets
     * filled.
     */
    template <typename FillFn>
    void
    fill(FillFn fillFn) noexcept(detail::IsNoexceptCallable<FillFn, std::span<typename Rte::mbuf*>>)
    {
        const std::span<typename Rte::mbuf*> available {
            m_data.data() + size(),
            m_data.data() + capacity()};
        const size_t nFilled = fillFn(available);
        m_size += nFilled;
    }

private:
    std::vector<typename Rte::mbuf*> m_data; ///< Storage for mbuf pointers.
    size_t m_size = 0; ///< Number of packets in the burst.
};

/**
 * @brief Default packet burst type using DPDK mbuf traits.
 */
using PktBurst = PktBurstImpl<PktBurstDpdkRte>;

} // namespace amon::dpdk
