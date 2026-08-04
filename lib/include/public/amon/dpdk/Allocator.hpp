/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief DPDK memory allocation utilities: low-level alloc/dealloc and an STL-compatible allocator
 * using rte_malloc.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/dpdk/SocketID.hpp>

#include <algorithm>
#include <limits>

#include <rte_malloc.h>

namespace amon::dpdk {

/**
 * @brief Alignment for type @p T according to rte_malloc rules.
 */
template <typename T>
static constexpr size_t alignOf = std::max<size_t>(alignof(T), RTE_CACHE_LINE_SIZE);

/**
 * @brief Allocate memory for @p count objects of type @p T.
 *
 * Uses rte_malloc_socket to allocate memory on a specific socket or any.
 *
 * @tparam T Type of objects to allocate.
 * @param count Number of objects to allocate.
 * @param socketId Socket on which to allocate, defaults to SocketID::ANY.
 * @return Pointer to the first object, aligned appropriately.
 * @throws std::bad_array_new_length if the requested size overflows.
 * @throws std::bad_alloc if allocation fails.
 *
 * @warning The returned memory is uninitialized and the object must be constructed by the caller
 * using placement new or std::construct_at.
 */
template <typename T>
[[nodiscard]] static inline T* alloc(size_t count = 1, SocketID socketId = SocketID::ANY)
{
    static_assert(
        alignOf<T> <= std::numeric_limits<unsigned>::max(),
        "T requires an alignment that rte_malloc cannot represent");
    static_assert(std::has_single_bit(alignOf<T>), "rte_malloc requires a power-of-two alignment");

    if (count > std::numeric_limits<size_t>::max() / sizeof(T)) {
        throw std::bad_array_new_length {};
    }

    const size_t nBytes = sizeof(T) * count;
    void* raw = rte_malloc_socket(typeid(T).name(), nBytes, alignOf<T>, socketId);
    if (raw == nullptr) {
        throw std::bad_alloc {};
    }

    auto* bytes = ::new (raw) std::byte[nBytes];
    return std::launder(reinterpret_cast<T*>(bytes));
}

/**
 * @brief Deallocate memory previously allocated with alloc.
 *
 * @tparam T Type of the allocated objects.
 * @param ptr Pointer to the memory to deallocate.
 *
 * @warning Does not destruct the object. Destruction has to be done by the caller using placement
 * delete or std::destruct_at prior to calling this function.
 */
template <typename T>
static inline void dealloc(T* ptr)
{
    rte_free(static_cast<void*>(ptr));
}

/**
 * @brief STL-compatible allocator that uses DPDK's rte_malloc.
 *
 * @tparam T Type of objects allocated by this allocator.
 */
template <typename T>
class Allocator {
public:
    /// Required definition to satisfy std::allocator_traits
    using value_type = T;

    /**
     * @brief Construct an allocator for a specific socket.
     * @param socketId Socket on which allocations should be made.
     */
    explicit Allocator(SocketID socketId) noexcept
        : m_socketId(socketId)
    {
    }

    /**
     * @brief Converting constructor from another allocator.
     * @tparam U Type of the other allocator.
     * @param other Allocator to copy the socket ID from.
     */
    template <typename U>
    Allocator(const Allocator<U>& other) noexcept
        : Allocator(other.socketId())
    {
    }

    /**
     * @brief Allocate memory for @p count objects.
     * @param count Number of objects to allocate.
     * @return Pointer to the first object.
     */
    [[nodiscard]] T* allocate(std::size_t count) const { return alloc<T>(count, m_socketId); }

    /**
     * @brief Deallocate memory previously allocated with allocate.
     * @param ptr Pointer to the memory to deallocate.
     * @param count Size of the allocation (unused).
     */
    void deallocate(T* ptr, std::size_t count) const noexcept
    {
        (void) count;
        dealloc(ptr);
    }

    /**
     * @brief Compare allocators for equality.
     *
     * Two allocators are equal if they target the same socket.
     *
     * @tparam U Type of the other allocator.
     * @param other Other allocator to compare with.
     * @return true if the allocators are equal, false otherwise.
     */
    template <typename U>
    bool operator==(const Allocator<U>& other) const noexcept
    {
        return m_socketId == other.socketId();
    }

    /**
     * @brief Get the socket ID used by this allocator.
     * @return The socket ID.
     */
    SocketID socketId() const noexcept { return m_socketId; }

private:
    SocketID m_socketId;
};

} // namespace amon::dpdk
