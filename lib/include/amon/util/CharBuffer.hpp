/**
 * @file
 * @author Lukas Hutak <hutak@cesnet.cz>
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Character buffer
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <string_view>

namespace amon::util {

/**
 * @brief Simple and very fast stack-based character buffer with locale-independent,
 * non-allocating, and non-throwing operations.
 *
 * Buffer allows to insert any amount of content. If its capacity is exceeded,
 * the additional excess content is truncated. The result is NOT null-terminated.
 *
 * @tparam N Buffer capacity (number of characters). Default is 4096.
 */
template <size_t N = 4096>
class CharBuffer {
public:
    /**
     * @brief Default constructor. Initializes an empty buffer.
     */
    CharBuffer() = default;

    /**
     * @brief Construct buffer from a string view.
     * @param str String view to initialize buffer with.
     */
    CharBuffer(std::string_view str) noexcept { append(str); };

    /**
     * @brief Destructor.
     */
    ~CharBuffer() = default;

    /**
     * @brief Get content of the buffer as a string view (contains only valid part).
     * @return String view of buffer content.
     */
    std::string_view view() const noexcept { return {data(), size()}; };

    /**
     * @brief Test if the buffer is empty.
     * @return True if buffer is empty, false otherwise.
     */
    bool empty() const noexcept { return (m_offset == 0); };

    /**
     * @brief Test if the buffer is full.
     * @return True if buffer is full, false otherwise.
     */
    bool full() const noexcept { return (m_offset == m_buffer.size()); };

    /**
     * @brief Get size of the buffer i.e. number of currently valid characters.
     * @return Number of valid characters in buffer.
     */
    size_t size() const noexcept { return m_offset; };

    /**
     * @brief Get capacity of the buffer i.e. maximum number of characters.
     * @return Buffer capacity.
     */
    size_t capacity() const noexcept { return m_buffer.size(); };

    /**
     * @brief Get unused size of the buffer i.e. number of characters that can be appended.
     * @return Number of unused characters in buffer.
     */
    size_t unused() const noexcept { return capacity() - size(); };

    /**
     * @brief Tests if the buffer has failed. Examples of failures include a formatting
     * problem or insufficient data storage capacity.
     * @return True if buffer has failed, false otherwise.
     */
    bool hasFailed() const noexcept { return m_hasFailed; };

    /**
     * @brief Returns a reference to the first valid character in the container.
     * @note Calling front on an empty container is undefined.
     * @return Reference to first character.
     */
    char& front() noexcept { return m_buffer[0]; };

    /**
     * @brief Returns a reference to the first valid character in the container.
     * @note Calling front on an empty container is undefined.
     * @return Reference to first character.
     */
    const char& front() const noexcept { return m_buffer[0]; };

    /**
     * @brief Returns a reference to the last valid character in the container.
     * @note Calling back on an empty container is undefined.
     * @return Reference to last character.
     */
    char& back() noexcept { return m_buffer[m_offset - 1]; };

    /**
     * @brief Returns a reference to the last valid character in the container.
     * @note Calling back on an empty container is undefined.
     * @return Reference to last character.
     */
    const char& back() const noexcept { return m_buffer[m_offset - 1]; };

    /**
     * @brief Returns pointer to the underlying array serving as character storage.
     * @return Pointer to buffer data.
     */
    char* data() noexcept { return m_buffer.data(); };

    /**
     * @brief Returns pointer to the underlying array serving as character storage.
     * @return Pointer to buffer data.
     */
    const char* data() const noexcept { return m_buffer.data(); };

    /**
     * @brief Returns a reference to the character at specified location @p pos.
     * @note No bounds checking is performed.
     * @param pos Position of character.
     * @return Reference to character at position.
     */
    char& operator[](size_t pos) noexcept { return m_buffer[pos]; };

    /**
     * @brief Returns a reference to the character at specified location @p pos.
     * @note No bounds checking is performed.
     * @param pos Position of character.
     * @return Reference to character at position.
     */
    const char& operator[](size_t pos) const noexcept { return m_buffer[pos]; };

    /**
     * @brief Clear the buffer.
     */
    void clear() noexcept
    {
        m_offset = 0;
        m_hasFailed = false;
    };

    /**
     * @brief Append a string to the buffer.
     * @param str String view to append.
     */
    void append(std::string_view str) noexcept
    {
        const size_t length = std::min(unused(), str.size());
        auto bufferIter = m_buffer.begin();

        std::advance(bufferIter, m_offset);
        std::copy_n(str.begin(), length, bufferIter);
        m_offset += length;

        if (length < str.size()) {
            m_hasFailed = true;
        }
    }

    /**
     * @brief Append content from another buffer.
     * @tparam M Capacity of other buffer.
     * @param other Buffer to append from.
     */
    template <size_t M>
    void append(const CharBuffer<M>& other)
    {
        append(other.view());
    }

    /**
     * @brief Append a numeric @p value with the given @p base.
     * @tparam T Integral type.
     * @param value Value to append.
     * @param base Numeric base (default 10).
     * @param padWidth Minimum width, padded with padSymbol.
     * @param padSymbol Padding character (default '0').
     */
    template <typename T>
        requires std::integral<T>
    void append(T value, int base = 10, int padWidth = 0, char padSymbol = '0') noexcept
    {
        std::array<char, 128> buffer;
        char* begin = buffer.data();
        char* end = buffer.data() + buffer.size();

        auto [ptr, ec] {std::to_chars(begin, end, value, base)};
        if (ec != std::errc()) {
            append("<error>");
            m_hasFailed = true;
            return;
        }

        const auto length = size_t(ptr - begin);
        if (padWidth >= 0 && size_t(padWidth) > length) {
            const size_t padAmount = std::min(size_t(padWidth) - length, buffer.size() - length);
            std::move(begin, begin + length, begin + padAmount);
            std::fill_n(begin, padAmount, padSymbol);
            ptr += padAmount;
        }

        append(std::string_view {begin, ptr});
    }

    /**
     * @brief Append multiple values to the buffer.
     * @tparam T First value type.
     * @tparam Ts Remaining value types.
     * @param value First value.
     * @param values Remaining values.
     */
    template <typename T, typename... Ts>
        requires(sizeof...(Ts) > 0)
    void append(T value, Ts... values) noexcept
    {
        append(value);
        append(values...);
    }

    /**
     * @brief Append a @p character to the buffer.
     * @param character Character to append.
     */
    void appendChar(char character) noexcept
    {
        if (full()) {
            m_hasFailed = true;
            return;
        }

        m_buffer[m_offset++] = character;
    }

private:
    static_assert(N > 0, "buffer size cannot be zero");

    std::array<char, N> m_buffer;
    std::size_t m_offset = 0;
    bool m_hasFailed = false;
};

} // namespace amon::util
