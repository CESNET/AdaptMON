/**
 * @file
 * @author Jan Sobol <sobol@cesnet.cz>
 * @brief Fixed-capacity vector with in-place storage.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <array>
#include <cstddef>
#include <iterator>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace amon::types {

/**
 * @brief Fixed-capacity vector with in-place storage.
 *
 * Stores up to @p Capacity elements of type @p T in a preallocated buffer.
 * No dynamic memory allocation is performed. Provides standard vector-like
 * interface for element access, iteration, and modification.
 *
 * @tparam T Element type.
 * @tparam Capacity Maximum number of elements.
 */
template <typename T, size_t Capacity>
class InplaceVector {
public:
    /// Type of stored value.
    using ValueType = T;
    /// Type for size and indexing.
    using SizeType = size_t;
    /// Reference to element.
    using Reference = T&;
    /// Const reference to element.
    using ConstReference = const T&;
    /// Pointer to element.
    using Pointer = T*;
    /// Const pointer to element.
    using ConstPointer = const T*;
    /// Iterator type.
    using Iterator = Pointer;
    /// Const iterator type.
    using ConstIterator = ConstPointer;
    /// Reverse iterator type.
    using ReverseIterator = std::reverse_iterator<Iterator>;
    /// Const reverse iterator type.
    using ConstReverseIterator = std::reverse_iterator<ConstIterator>;

    /**
     * @brief Default constructor. Constructs an empty vector.
     */
    InplaceVector() = default;

    /**
     * @brief Trivial destructor.
     */
    ~InplaceVector()
        requires std::is_trivially_destructible_v<T>
    = default;

    /**
     * @brief Destructor. Destroys all contained elements.
     */
    ~InplaceVector()
        requires(!std::is_trivially_destructible_v<T>)
    {
        clear();
    }

    /**
     * @brief Copy constructor.
     * @param other Vector to copy from.
     */
    InplaceVector(const InplaceVector& other)
    {
        for (SizeType i = 0; i < other.m_size; ++i) {
            emplaceBack(other[i]);
        }
    }

    /**
     * @brief Move constructor.
     * @param other Vector to move from.
     */
    InplaceVector(InplaceVector&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        for (SizeType i = 0; i < other.m_size; ++i) {
            emplaceBackUnsafe(std::move(other[i]));
        }
        other.clear();
    }

    /**
     * @brief Copy assignment.
     * @param other Vector to copy from.
     * @return Reference to this vector.
     */
    InplaceVector& operator=(const InplaceVector& other)
    {
        if (this != &other) {
            clear();
            for (SizeType i = 0; i < other.m_size; ++i) {
                emplaceBack(other[i]);
            }
        }
        return *this;
    }

    /**
     * @brief Move assignment.
     * @param other Vector to move from.
     * @return Reference to this vector.
     */
    InplaceVector&
    operator=(InplaceVector&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        if (this != &other) {
            clear();
            for (SizeType i = 0; i < other.m_size; ++i) {
                emplaceBackUnsafe(std::move(other[i]));
            }
            other.clear();
        }
        return *this;
    }

    /**
     * @brief Iterator to the beginning.
     * @return Iterator to first element.
     */
    Iterator begin() noexcept { return data(); }
    /**
     * @brief Const iterator to the beginning.
     * @return Const iterator to first element.
     */
    ConstIterator begin() const noexcept { return data(); }
    /**
     * @brief Const iterator to the beginning.
     * @return Const iterator to first element.
     */
    ConstIterator cbegin() const noexcept { return data(); }

    /**
     * @brief Iterator to the end.
     * @return Iterator past last element.
     */
    Iterator end() noexcept { return data() + m_size; }
    /**
     * @brief Const iterator to the end.
     * @return Const iterator past last element.
     */
    ConstIterator end() const noexcept { return data() + m_size; }
    /**
     * @brief Const iterator to the end.
     * @return Const iterator past last element.
     */
    ConstIterator cend() const noexcept { return end(); }

    /**
     * @brief Reverse iterator to the beginning.
     * @return Reverse iterator to last element.
     */
    ReverseIterator rbegin() noexcept { return ReverseIterator(end()); }
    /**
     * @brief Const reverse iterator to the beginning.
     * @return Const reverse iterator to last element.
     */
    ConstReverseIterator rbegin() const noexcept { return ConstReverseIterator(end()); }
    /**
     * @brief Const reverse iterator to the beginning.
     * @return Const reverse iterator to last element.
     */
    ConstReverseIterator crbegin() const noexcept { return rbegin(); }

    /**
     * @brief Reverse iterator to the end.
     * @return Reverse iterator past first element.
     */
    ReverseIterator rend() noexcept { return ReverseIterator(begin()); }
    /**
     * @brief Const reverse iterator to the end.
     * @return Const reverse iterator past first element.
     */
    ConstReverseIterator rend() const noexcept { return ConstReverseIterator(begin()); }
    /**
     * @brief Const reverse iterator to the end.
     * @return Const reverse iterator past first element.
     */
    ConstReverseIterator crend() const noexcept { return rend(); }

    /**
     * @brief Number of elements in the vector.
     * @return Current size.
     */
    [[nodiscard]] SizeType size() const noexcept { return m_size; }

    /**
     * @brief Maximum number of elements the vector can hold.
     * @return Capacity.
     */
    [[nodiscard]] SizeType capacity() const noexcept { return Capacity; }

    /**
     * @brief Check if the vector is empty.
     * @return True if empty, false otherwise.
     */
    [[nodiscard]] bool empty() const noexcept { return m_size == 0; }

    /**
     * @brief Check if the vector is full.
     * @return True if full, false otherwise.
     */
    [[nodiscard]] bool full() const noexcept { return m_size == Capacity; }

    /**
     * @brief Access element at index (unchecked).
     * @param index Index of element.
     * @return Reference to element.
     */
    Reference operator[](SizeType index) noexcept { return *ptrAt(index); }
    /**
     * @brief Access element at index (unchecked, const).
     * @param index Index of element.
     * @return Const reference to element.
     */
    ConstReference operator[](SizeType index) const noexcept { return *ptrAt(index); }

    /**
     * @brief Access element at index with bounds checking.
     * @param index Index of element.
     * @return Reference to element.
     * @throws std::out_of_range if index is out of bounds.
     */
    Reference at(SizeType index)
    {
        if (index >= m_size) {
            throw std::out_of_range("InplaceVector::at");
        }
        return (*this)[index];
    }

    /**
     * @brief Access element at index with bounds checking (const).
     * @param index Index of element.
     * @return Const reference to element.
     * @throws std::out_of_range if index is out of bounds.
     */
    ConstReference at(SizeType index) const
    {
        if (index >= m_size) {
            throw std::out_of_range("InplaceVector::at");
        }
        return (*this)[index];
    }

    /**
     * @brief Access first element.
     * @return Reference to first element.
     */
    Reference front() noexcept { return (*this)[0]; }
    /**
     * @brief Access first element (const).
     * @return Const reference to first element.
     */
    ConstReference front() const noexcept { return (*this)[0]; }

    /**
     * @brief Access last element.
     * @return Reference to last element.
     */
    Reference back() noexcept { return (*this)[m_size - 1]; }
    /**
     * @brief Access last element (const).
     * @return Const reference to last element.
     */
    ConstReference back() const noexcept { return (*this)[m_size - 1]; }

    /**
     * @brief Get pointer to underlying data.
     * @return Pointer to data.
     */
    Pointer data() noexcept { return ptrAt(0); }
    /**
     * @brief Get pointer to underlying data (const).
     * @return Const pointer to data.
     */
    ConstPointer data() const noexcept { return ptrAt(0); }

    /**
     * @brief Remove all elements from the vector.
     */
    void clear() noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (SizeType i = 0; i < m_size; ++i) {
                std::destroy_at(ptrAt(i));
            }
        }
        m_size = 0;
    }

    /**
     * @brief Construct element in-place at the end.
     * @tparam Args Argument types for element construction.
     * @param args Arguments to forward to element constructor.
     * @return Reference to the new element.
     * @throws std::length_error if vector is full.
     */
    template <typename... Args>
    Reference emplaceBack(Args&&... args)
    {
        if (m_size >= Capacity) {
            throw std::length_error("InplaceVector is full");
        }
        return emplaceBackUnsafe(std::forward<Args>(args)...);
    }

    /**
     * @brief Add a copy of an element to the end.
     * @param value Element to copy.
     */
    void pushBack(const T& value) { emplaceBack(value); }
    /**
     * @brief Add a moved element to the end.
     * @param value Element to move.
     */
    void pushBack(T&& value) { emplaceBack(std::move(value)); }

    /**
     * @brief Remove the last element.
     * @throws std::out_of_range if vector is empty.
     */
    void popBack()
    {
        if (empty()) {
            throw std::out_of_range("InplaceVector::popBack");
        }
        --m_size;
        if constexpr (!std::is_trivially_destructible_v<T>) {
            std::destroy_at(ptrAt(m_size));
        }
    }

private:
    /// Raw storage for elements.
    alignas(alignof(T)) std::array<std::byte, Capacity * sizeof(T)> m_storage;
    /// Number of elements currently stored.
    SizeType m_size = 0;

    /**
     * @brief Get pointer to element at index.
     * @param index Index of element.
     * @return Pointer to element.
     */
    T* ptrAt(SizeType index) noexcept
    {
        return std::launder(reinterpret_cast<T*>(m_storage.data() + (index * sizeof(T))));
    }

    /**
     * @brief Get pointer to element at index (const).
     * @param index Index of element.
     * @return Const pointer to element.
     */
    const T* ptrAt(SizeType index) const noexcept
    {
        return std::launder(reinterpret_cast<const T*>(m_storage.data() + (index * sizeof(T))));
    }

    /**
     * @brief Construct element in-place at the end without bounds checking.
     * @tparam Args Argument types for element construction.
     * @param args Arguments to forward to element constructor.
     * @return Reference to the new element.
     */
    template <typename... Args>
    Reference emplaceBackUnsafe(Args&&... args) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        T* ptr = ptrAt(m_size);
        std::construct_at(ptr, std::forward<Args>(args)...);
        ++m_size;
        return *ptr;
    }
};

} // namespace amon::types
