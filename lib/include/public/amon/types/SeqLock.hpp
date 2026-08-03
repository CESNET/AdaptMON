/**
 * @file
 * @author Samuel Luptak <luptak@cesnet.cz>
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @author Lukas Hutak <hutak@cesnet.cz>
 * @brief Sequential lock
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 */

#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace amon::types {

/*
 * Cheat sheet for the acquire and release semantics in different use cases:
 *
 * -----------------------------------------------------------------------------------------------
 * store(release)
 *  - All loads and stores before this store cannot be reordered after It.
 *  - This does not stop loads and stores after this store to be reordered before It.
 *  - Thus the release store serves as a one-way barrier for loads and stores.
 *
 * load(acquire)
 *  - All loads and stores after this load cannot be reordered before It.
 *  - This does not stop loads and stores before this load to be reordered after It.
 *  - Thus the acquire load serves as a one-way barrier for loads and stores.
 *
 * fence(release)
 *  - All loads and stores before this fence cannot be reordered with ANY stores
 *    that are after this fence.
 *  - This means that loads and stores before the fence cannot be reordered after the fence
 *    and stores after the fence cannot be reordered before the fence. ANY loads after the fence
 *    can still be reordered before it.
 *  - Thus the release fence is a one-way barrier for loads and a two-way barrier for stores.
 *
 * fence(acquire)
 *  - All loads and stores after this fence cannot be reordered with ANY loads
 *    that are before this fence.
 *  - This means that loads and stores after the fence cannot be reordered before the fence
 *    and loads before the fence cannot be reordered after the fence. ANY stores before the fence
 *    can still be reordered after it.
 *  - Thus the acquire fence is a one-way barrier for stores and a two-way barrier for loads.
 * -----------------------------------------------------------------------------------------------
 *
 * If you are still lost, refer to:
 *   https://preshing.com/20120913/acquire-and-release-semantics/
 *   https://preshing.com/20130922/acquire-and-release-fences/
 *   https://preshing.com/20131125/acquire-and-release-fences-dont-work-the-way-youd-expect/
 */

/**
 * @brief Lock-free single-writer/multi-reader holder of a trivially copyable value.
 *
 * Lets a single writer thread store a value of type @p T while any number of
 * reader threads can load it concurrently without blocking the writer or other readers. The writer
 * sets an odd sequence number before storing the value and an even one afterwards; a reader
 * retries until a stable even sequence number is observed. The value is stored as an array of
 * atomic 64-bit words. Aligned to a cache line to avoid false sharing.
 *
 * @tparam T Value type. Must be trivially copyable (its bytes fully represent
 *   its value; no owned resources, virtuals, or custom copy semantics). Raw
 *   pointer members are copied shallowly.
 *
 * @warning Exactly one writer thread is assumed. The sequence number is updated
 *   with plain stores instead of read-modify-write operations, so concurrent
 *   calls to store() from multiple threads would corrupt the protocol.
 *   Readers require no coordination among themselves and scale to N threads.
 */
template <typename T>
class alignas(64) SeqLock {
public:
    /** @brief Constructs a seqlock with a default constructed value. */
    SeqLock() noexcept(std::is_nothrow_default_constructible_v<T>)
        requires std::is_default_constructible_v<T>
        : SeqLock(T {})
    {
    }

    /** @brief Constructs with a user provided initial value.
     *  @param value User provided initial value.
     */
    explicit SeqLock(const T& value) noexcept { storeWords(toWords(value)); }

    SeqLock(const SeqLock&) = delete;
    SeqLock& operator=(const SeqLock&) = delete;

    SeqLock(SeqLock&&) noexcept = delete;
    SeqLock& operator=(SeqLock&&) noexcept = delete;

    /**
     * @brief Atomically store a new value.
     * @param value Value to store for the readers.
     */
    void store(const T& value) noexcept
    {
        const Words words = toWords(value);

        const std::uint32_t seq = m_seq.load(std::memory_order_relaxed);

        m_seq.store(seq + 1, std::memory_order_relaxed);

        /* Release fence prevents the seq update to be reordered
         * after the fence and the data stores to be reordered before
         * the fence thus guaranteeing that seq will always be updated before
         * the new data is stored.
         */
        std::atomic_thread_fence(std::memory_order_release);

        storeWords(words);

        /* A release store prevents the data stores to be reordered after the seq update thus
         * making sure that seq is updated only after all the data has been successfully stored.
         */
        m_seq.store(seq + 2, std::memory_order_release);
    }

    /**
     * @brief Atomically load a consistent snapshot of the stored value.
     * @return Copy of the most recently stored value.
     */
    [[nodiscard]] T load() const noexcept
    {
        Words words;
        std::uint32_t seq;

        do {
            do {
                /* An acquire load prevents the data loads to be reordered before the seq parity
                 * check thus making sure that the data is loaded only after seq is validated.
                 */
                seq = m_seq.load(std::memory_order_acquire);
            } while (static_cast<bool>(seq & 1));

            loadWords(words);

            /* Acquire fence prevents the seq re-validation to be reordered
             * before the fence and the data loads to be reordered after
             * the fence thus guaranteeing that seq will always be checked after
             * all the data has been loaded.
             */
            std::atomic_thread_fence(std::memory_order_acquire);

        } while (seq != m_seq.load(std::memory_order_relaxed));

        return fromWords(words);
    }

private:
    static_assert(
        std::is_trivially_copyable_v<T>,
        "SeqLock<T>: T must be trivially copyable (word-wise byte copy must be a valid copy)");

    using Word = std::uint64_t;
    static_assert(
        std::atomic<Word>::is_always_lock_free,
        "SeqLock<T>: requires lock-free 64-bit atomics");

    static constexpr std::size_t WORD_COUNT = (sizeof(T) + sizeof(Word) - 1) / sizeof(Word);
    ///< Number of 64-bit words needed to hold the value (rounded up).

    using Words = std::array<Word, WORD_COUNT>;

    ///< Helper struct used to cast the Words representation of value into the T representation
    struct Representation {
        alignas(T) std::byte bytes[sizeof(T)];
    };
    static_assert(
        sizeof(Representation) == sizeof(T),
        "SeqLock<T>: Failed to represent T as bytes");

    std::atomic<std::uint32_t> m_seq {0}; ///< Sequential lock synchronization variable.
    std::array<std::atomic<Word>, WORD_COUNT> m_data {}; ///< Protected data.

    [[nodiscard]] static Words toWords(const T& value) noexcept
    {
        Words words {};
        std::memcpy(words.data(), &value, sizeof(T));
        return words;
    }

    [[nodiscard]] static T fromWords(const Words& words) noexcept
    {
        Representation repr;
        std::memcpy(repr.bytes, words.data(), sizeof(T));
        return std::bit_cast<T>(repr);
    }

    void storeWords(const Words& words) noexcept
    {
        for (std::size_t i = 0; i < WORD_COUNT; ++i) {
            m_data[i].store(words[i], std::memory_order_relaxed);
        }
    }

    void loadWords(Words& words) const noexcept
    {
        for (std::size_t i = 0; i < WORD_COUNT; ++i) {
            words[i] = m_data[i].load(std::memory_order_relaxed);
        }
    }
};

} // namespace amon::types
