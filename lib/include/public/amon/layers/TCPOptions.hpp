/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief TCP option layer definitions.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/layers/Layer.hpp>
#include <amon/layers/LayerView.hpp>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <utility>

namespace amon::layers {

/**
 * @brief Minimal TCP option header.
 *
 * Represents only the first byte of a TCP option, the "kind" field.
 * This structure intentionally contains just the kind byte, because:
 *
 * - Special TCP options END_OF_OPTION_LIST (0) and NO_OPERATION (1)
 *   consist of a single byte and do not have a length field.
 * - All other TCP options include a second byte ("length"), which specifies
 *   the total size of the option in bytes (including kind and length),
 *   but that byte is not part of this minimal header.
 *
 * Used as the base for parsing or creating TCPOptionView instances.
 */
struct [[gnu::packed]] TCPOptionHeader {
    uint8_t kind; ///< TCP option kind
};

/**
 * @brief View for a single TCP option.
 *
 * Provides access to the option kind, size, and value.
 */
class TCPOptionView : public LayerView<TCPOptionHeader> {
public:
    /**
     * @brief Create and validate TCPOptionView from a raw byte data.
     *
     * @param data Span of bytes containing the TCP option.
     * @return std::expected containing TCPOptionView or LayerError.
     */
    static std::expected<TCPOptionView, LayerError>
    create(std::span<const std::byte> data) noexcept;

    /**
     * @brief Get the kind of the TCP option.
     *
     * @return Option kind as uint8_t.
     */
    uint8_t kind() const noexcept { return header().kind; }

    /**
     * @brief Get the length of the TCP option in bytes.
     *
     * @return Length of the TCP option.
     */
    uint8_t optionLength() const noexcept;

    /**
     * @brief Get the value portion of the TCP option.
     *
     * @return Span of bytes representing the option value.
     */
    std::span<const std::byte> getValueSpan() const noexcept;
};

/**
 * @brief Enumeration of commonly used TCP option kinds.
 */
enum TCPOptionKind : uint8_t {
    /// End of Option List – marks the end of TCP options
    EOL = 0,
    /// No-Operation – padding byte for alignment
    NOP = 1,
    /// Maximum Segment Size – limits max TCP segment size
    MSS = 2,
    /// Window Scale – extends TCP window for high-speed networks
    SCALE = 3,
    /// SACK Permitted – indicates support for Selective Acknowledgment
    SACK_PERMITTED = 4,
    /// SACK – contains blocks of selectively acknowledged data
    SACK = 5,
    /// Timestamp – used for RTT measurement and protection against wrapped sequence numbers
    TIMESTAMP = 8,
    /// Multipath TCP – for splitting TCP flow across multiple paths
    MPTCP = 30,
    /// TCP Fast Open – allows sending data in SYN for faster connection setup
    TFO = 34,
};

// Forward declaration
class TCPOptionIterator;

/**
 * @brief Range for iterating over TCP options.
 *
 * Provides begin() and end() methods for iteration.
 */
class TCPOptionRange {
public:
    /**
     * @brief Construct a TCPOptionRange from raw TCP options data.
     *
     * @param data Span of bytes containing the TCP options.
     */
    explicit TCPOptionRange(std::span<const std::byte> data) noexcept
        : m_data(data)
    {
    }

    /**
     * @brief Get iterator to the beginning of the TCP options.
     *
     * @return TCPOptionIterator at the beginning.
     */
    TCPOptionIterator begin() const noexcept;

    /**
     * @brief Get iterator to the end of the TCP options.
     *
     * @return TCPOptionIterator at the end.
     */
    TCPOptionIterator end() const noexcept;

private:
    std::span<const std::byte> m_data;
};

/**
 * @brief Input iterator over TCP options.
 *
 * Dereferencing the iterator yields a std::expected containing either
 * a valid TCPOptionView or a LayerError if parsing failed.
 *
 * The iterator advances sequentially through the TCP options buffer and
 * stops when:
 * - END_OF_OPTION_LIST (EOL) option is encountered
 * - End of buffer is reached
 * - A parsing error occurs
 */
class TCPOptionIterator {
public:
    /// @name Standard iterator traits
    /// @{

    using iterator_category = std::input_iterator_tag; ///< Input iterator category
    using value_type = std::expected<TCPOptionView, LayerError>; ///< Iterator value type
    using difference_type = std::ptrdiff_t; ///< Difference type
    using pointer = const value_type*; ///< Pointer type
    using reference = const value_type&; ///< Reference type

    /// @}

    /**
     * @brief Construct an iterator at a given byte offset.
     *
     * @param data Span containing raw TCP options.
     * @param pos  Byte offset into the span.
     */
    TCPOptionIterator(std::span<const std::byte> data, std::size_t pos) noexcept
        : m_data(data)
        , m_pos(pos)
        , m_currentOption(std::unexpected(LayerError::HEADER_SHORT))
    {
        loadNextOption();
    }

    /**
     * @brief Dereference the iterator.
     *
     * @return Reference to the current TCP option or an error.
     */
    reference operator*() const noexcept { return m_currentOption; }

    /**
     * @brief Access members of the current iterator value.
     *
     * Equivalent to `&operator*()`.
     *
     * @return Pointer to the current iterator value.
     */
    pointer operator->() const noexcept { return &m_currentOption; }

    /**
     * @brief Advance to the next TCP option (prefix).
     *
     * @return Reference to this iterator.
     */
    TCPOptionIterator& operator++() noexcept
    {
        if (m_finished) {
            return *this;
        }

        /**
         * If the current option was terminal (EOL or error),
         * the next increment moves to end().
         */
        if (m_terminal) {
            m_finished = true;
            m_pos = m_data.size();
            return *this;
        }

        m_pos += m_currentOption->optionLength();
        loadNextOption();
        return *this;
    }

    /**
     * @brief Advance to the next TCP option (postfix).
     *
     * @return Iterator state before the increment.
     */
    TCPOptionIterator operator++(int) noexcept
    {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    /**
     * @brief Compare two iterators for equality.
     *
     * @param other Iterator to compare with.
     * @return true if the iterators are equal, false otherwise.
     */
    bool operator==(const TCPOptionIterator& other) const noexcept
    {
        return m_pos == other.m_pos && m_data.data() == other.m_data.data();
    }

    /**
     * @brief Compare two iterators for inequality.
     *
     * @param other Iterator to compare with.
     * @return true if the iterators are not equal, false otherwise.
     */
    bool operator!=(const TCPOptionIterator& other) const noexcept { return !(*this == other); }

private:
    void loadNextOption() noexcept
    {
        m_terminal = false;

        if (m_pos >= m_data.size()) {
            m_finished = true;
            return;
        }

        m_currentOption = TCPOptionView::create(m_data.subspan(m_pos));
        if (!m_currentOption) {
            // Error is returned as a valid dereference, but marks this iterator as terminal.
            m_terminal = true;
            return;
        }

        if (m_currentOption->kind() == std::to_underlying(TCPOptionKind::EOL)) {
            // EOL is returned as a valid dereference
            m_terminal = true;
        }
    }

    std::span<const std::byte> m_data;
    std::size_t m_pos = 0;
    value_type m_currentOption;
    bool m_terminal = false; ///< current option is last (EOL or error)
    bool m_finished = false; ///< end() reached
};

} // namespace amon::layers
