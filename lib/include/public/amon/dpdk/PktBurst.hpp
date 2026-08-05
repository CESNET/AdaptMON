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

// @cond DO_NOT_DOCUMENT
namespace detail {
template <typename Fn, typename... Args>
constexpr bool IsNoexceptCallable = noexcept(std::declval<Fn&>()(std::declval<Args>()...));

template <typename>
struct PktBurstOps;

template <>
struct PktBurstOps<rte_mbuf> {
    static void refcntUpdate(rte_mbuf* mbuf, int16_t value)
    {
        rte_pktmbuf_refcnt_update(mbuf, value);
    }
    static void freeSingle(rte_mbuf* mbuf) { rte_pktmbuf_free(mbuf); }
    static void freeBulk(rte_mbuf** mbufs, unsigned count) { rte_pktmbuf_free_bulk(mbufs, count); }
};
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
template <typename ValueType, typename Ops = detail::PktBurstOps<ValueType>>
class PktBurst {
public:
    /**
     * @brief Const iterator type for traversing the burst.
     */
    using ConstIterator = ValueType* const*;

    /**
     * @brief Iterator type for traversing the burst.
     */
    using Iterator = ValueType**;

    /**
     * @brief Construct a burst with given capacity.
     * @param capacity Maximum number of packets in the burst.
     */
    PktBurst(std::size_t capacity)
        : m_data(capacity, nullptr)
    {
    }

    /**
     * @brief Disable copy constructor.
     */
    PktBurst(const PktBurst&) = delete;

    /**
     * @brief Disable copy assignment.
     */
    PktBurst& operator=(const PktBurst&) = delete;

    /**
     * @brief Move constructor.
     * @param other The moved from instance.
     */
    PktBurst(PktBurst&& other) noexcept
        : m_data(std::exchange(other.m_data, {}))
        , m_size(std::exchange(other.m_size, 0))
    {
    }

    /**
     * @brief Move assignment.
     * @param other The moved from instance.
     * @return The return value.
     */
    PktBurst& operator=(PktBurst&& other) noexcept
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
    ~PktBurst() { clear(); }

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
    ValueType** data() noexcept { return m_data.data(); }

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
    const ValueType*& operator[](size_t index) const noexcept { return m_data[index]; }

    /**
     * @brief Access packet at given index.
     * @param index Index of the packet.
     * @return Reference to pointer to mbuf.
     */
    ValueType*& operator[](size_t index) noexcept { return m_data[index]; }

    /**
     * @brief Remove all packets from the burst and free them.
     */
    void clear() noexcept
    {
        if (m_size > 0) {
            Ops::freeBulk(m_data.data(), static_cast<unsigned>(m_size));
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

        Ops::freeSingle(*pos);
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
        Ops::freeBulk(&*first, count);
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
    void eraseIf(Predicate pred) noexcept(detail::IsNoexceptCallable<Predicate, ValueType*>)
    {
        // Temporary storage for bulk freeing of elements. We assume that this is a size that should
        // be big enough for most cases to free everything in 1 bulk free call.
        std::array<ValueType*, 64> freeBuf;

        Iterator readHead = begin();
        Iterator localWriteHead = begin();
        auto moveWriteHead = freeBuf.begin();
        Iterator oldEnd = end();

        while (readHead != oldEnd) {
            if (pred(*readHead)) {
                if (moveWriteHead == freeBuf.end()) [[unlikely]] {
                    Ops::freeBulk(freeBuf.data(), freeBuf.size());
                    moveWriteHead = freeBuf.begin();
                }
                *(moveWriteHead++) = *(readHead++);
                --m_size;
            } else {
                *(localWriteHead++) = *(readHead++);
            }
        }

        Ops::freeBulk(
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
            Ops::refcntUpdate(*it, 1);
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
    void copy(Iterator first, Iterator last, PktBurst& output) noexcept
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
        detail::IsNoexceptCallable<Predicate, ValueType*>)
    {
        auto outIt = outputFirst;
        for (Iterator it = begin(); it != end(); ++it) {
            if (pred(*it)) {
                (*outIt++) = *it;
                Ops::refcntUpdate(*it, 1);
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
    void copyIf(Predicate pred, PktBurst& output) noexcept
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
    void move(Iterator first, Iterator last, PktBurst& output) noexcept
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
        detail::IsNoexceptCallable<Predicate, ValueType*>)
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
    void moveIf(Predicate pred, PktBurst& output) noexcept(
        detail::IsNoexceptCallable<Predicate, ValueType*>)
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
    void fill(FillFn fillFn) noexcept(detail::IsNoexceptCallable<FillFn, std::span<ValueType*>>)
    {
        const std::span<ValueType*> available {m_data.data() + size(), m_data.data() + capacity()};
        const size_t nFilled = fillFn(available);
        m_size += nFilled;
    }

private:
    std::vector<ValueType*> m_data; ///< Storage for mbuf pointers.
    size_t m_size = 0; ///< Number of packets in the burst.
};

} // namespace amon::dpdk
