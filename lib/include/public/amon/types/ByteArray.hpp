/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief Fixed-size array of bytes with utility methods.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <algorithm>
#include <compare>
#include <cstddef>
#include <cstring>
#include <stdexcept>

namespace amon::types {

/**
 * @brief Fixed-size array of bytes with utility methods.
 *
 * Provides element access, iterators, capacity queries, and modifiers.
 * Throws std::out_of_range on invalid access via at().
 *
 * E.g. use as type like MAC address or IP addresses.
 *
 * @tparam N Number of bytes in the array.
 */
template <std::size_t N>
class ByteArray {
public:
    /**
     * @brief Access element at given index (unchecked).
     * @param index Index of the element.
     * @return Reference to the byte at the given index.
     */
    std::byte& operator[](std::size_t index) noexcept { return m_data[index]; }

    /**
     * @brief Access element at given index (unchecked, const).
     * @param index Index of the element.
     * @return Const reference to the byte at the given index.
     */
    const std::byte& operator[](std::size_t index) const noexcept { return m_data[index]; }

    /**
     * @brief Access element at given index with bounds checking.
     * @param index Index of the element.
     * @return Reference to the byte at the given index.
     * @throws std::out_of_range if index is out of bounds.
     */
    std::byte& at(std::size_t index)
    {
        if (index >= N) {
            throw std::out_of_range("ByteArray index out of range");
        }
        return m_data[index];
    }

    /**
     * @brief Access element at given index with bounds checking (const).
     * @param index Index of the element.
     * @return Const reference to the byte at the given index.
     * @throws std::out_of_range if index is out of bounds.
     */
    const std::byte& at(std::size_t index) const
    {
        if (index >= N) {
            throw std::out_of_range("ByteArray index out of range");
        }
        return m_data[index];
    }

    /**
     * @brief Access the first element.
     * @return Reference to the first byte.
     */
    std::byte& front() noexcept { return m_data[0]; }

    /**
     * @brief Access the first element (const).
     * @return Const reference to the first byte.
     */
    const std::byte& front() const noexcept { return m_data[0]; }

    /**
     * @brief Access the last element.
     * @return Reference to the last byte.
     */
    std::byte& back() noexcept { return m_data[N - 1]; }

    /**
     * @brief Access the last element (const).
     * @return Const reference to the last byte.
     */
    const std::byte& back() const noexcept { return m_data[N - 1]; }

    /**
     * @brief Get pointer to the underlying data.
     * @return Pointer to the byte array.
     */
    std::byte* data() noexcept { return m_data; }

    /**
     * @brief Get pointer to the underlying data (const).
     * @return Const pointer to the byte array.
     */
    const std::byte* data() const noexcept { return m_data; }

    /**
     * @brief Get iterator to the beginning.
     * @return Pointer to the first byte.
     */
    std::byte* begin() noexcept { return m_data; }

    /**
     * @brief Get iterator to the beginning (const).
     * @return Const pointer to the first byte.
     */
    const std::byte* begin() const noexcept { return m_data; }

    /**
     * @brief Get const iterator to the beginning.
     * @return Const pointer to the first byte.
     */
    const std::byte* cbegin() const noexcept { return m_data; }

    /**
     * @brief Get iterator to the end.
     * @return Pointer past the last byte.
     */
    std::byte* end() noexcept { return m_data + N; }

    /**
     * @brief Get iterator to the end (const).
     * @return Const pointer past the last byte.
     */
    const std::byte* end() const noexcept { return m_data + N; }

    /**
     * @brief Get const iterator to the end.
     * @return Const pointer past the last byte.
     */
    const std::byte* cend() const noexcept { return m_data + N; }

    /**
     * @brief Get the number of bytes in the array.
     * @return Size of the array.
     */
    constexpr std::size_t size() const noexcept { return N; }

    /**
     * @brief Fill the array with the given byte value.
     * @param value Byte value to fill.
     */
    void fill(std::byte value) { std::fill(begin(), end(), value); }

    /**
     * @brief Clear the array (set all bytes to zero).
     */
    void clear() { fill(std::byte {0}); }

    /**
     * @brief Compare this ByteArray with another for ordering.
     * @param other ByteArray to compare with.
     * @return Strong ordering result.
     */
    auto operator<=>(const ByteArray<N>& other) const
    {
        const int cmpRes = std::memcmp(m_data, other.m_data, N);
        if (cmpRes < 0) {
            return std::strong_ordering::less;
        }
        if (cmpRes > 0) {
            return std::strong_ordering::greater;
        }
        return std::strong_ordering::equal;
    }

    /**
     * @brief Compare this ByteArray with another for equality.
     * @param other ByteArray to compare with.
     * @return True if equal, false otherwise.
     */
    bool operator==(const ByteArray<N>& other) const = default;

private:
    std::byte m_data[N]; ///< Underlying byte storage.
};

} // namespace amon::types
