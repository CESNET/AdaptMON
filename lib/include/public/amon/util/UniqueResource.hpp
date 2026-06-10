/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Unique resource utility class implementation
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <cassert>
#include <optional>
#include <stdexcept>
#include <type_traits>

#include <unistd.h>

namespace amon::util {

namespace deleters {

/**
 * @brief Deleter for file descriptors using close().
 */
struct Close {
    /**
     * @brief Closes the file descriptor.
     * @param fd File descriptor to close.
     */
    void operator()(int fd) noexcept { ::close(fd); }
};

} // namespace deleters

/**
 * @brief Manages a unique resource with a custom deleter.
 *
 * This class provides ownership semantics for a resource (such as a file descriptor)
 * and ensures proper cleanup using the specified deleter when the resource is released
 * or the object is destroyed.
 *
 * @tparam ValueType Type of the resource.
 * @tparam Deleter Callable type used to release the resource.
 */
template <typename ValueType, typename Deleter>
class UniqueResource {
public:
    static_assert(
        std::is_nothrow_default_constructible_v<Deleter>
            && noexcept(std::declval<Deleter&>()(std::declval<ValueType>())),
        "Deleter must be noexcept construable and callable");

    /**
     * @brief Default constructor. No resource is owned.
     */
    UniqueResource() noexcept = default;

    /**
     * @brief Construct with a resource value.
     * @param value Resource to manage.
     */
    UniqueResource(ValueType value) noexcept(std::is_nothrow_move_constructible_v<ValueType>)
        : m_resource(std::move(value))
    {
    }

    UniqueResource(const UniqueResource&) = delete;
    UniqueResource& operator=(const UniqueResource&) = delete;

    /**
     * @brief Move constructor. Transfers ownership from another UniqueResource.
     * @param other Resource to move from.
     */
    UniqueResource(UniqueResource&& other) noexcept(std::is_nothrow_move_constructible_v<ValueType>)
    {
        if (other.hasValue()) {
            reset(other.unsafeRelease());
        }
    }

    /**
     * @brief Move assignment operator. Transfers ownership from another UniqueResource.
     * @param other Resource to move from.
     * @return Reference to this object.
     */
    UniqueResource&
    operator=(UniqueResource&& other) noexcept(std::is_nothrow_move_constructible_v<ValueType>)
    {
        if (&other != this) {
            if (other.hasValue()) {
                reset(other.unsafeRelease());
            } else {
                reset();
            }
        }
        return *this;
    }

    /**
     * @brief Assign a new resource value. Releases any currently held resource.
     * @param value New resource to manage.
     * @return Reference to this object.
     */
    UniqueResource&
    operator=(ValueType value) noexcept(std::is_nothrow_move_constructible_v<ValueType>)
    {
        reset(std::move(value));
        return *this;
    }

    /**
     * @brief Access the managed resource.
     * @return Pointer to the managed resource.
     */
    ValueType* operator->() { return &get(); }

    /**
     * @brief Access the managed resource.
     * @return Const pointer to the managed resource.
     */
    const ValueType* operator->() const { return &get(); }

    /**
     * @brief Replace the managed resource. Releases any currently held resource.
     * @param value New resource to manage.
     */
    void reset(ValueType value) noexcept( // NOLINT(bugprone-exception-escape)
        std::is_nothrow_move_constructible_v<ValueType>)
    {
        if (m_resource.has_value()) {
            Deleter {}(m_resource.value());
            if constexpr (!std::is_nothrow_move_constructible_v<ValueType>) {
                m_resource.reset(); // reset in case move assignment would throw later
            }
        }
        m_resource.emplace(std::move(value));
    }

    /**
     * @brief Release the managed resource, if any.
     */
    void reset() noexcept // NOLINT(bugprone-exception-escape)
    {
        if (m_resource.has_value()) {
            Deleter {}(m_resource.value());
        }
        m_resource.reset();
    }

    /**
     * @brief Check if a resource is currently managed.
     * @return True if a resource is held, false otherwise.
     */
    bool hasValue() const noexcept { return m_resource.has_value(); }

    /**
     * @brief Get a reference to the managed resource.
     * @return Reference to the resource.
     * @throw logic_error if no resource is held.
     */
    ValueType& get()
    {
        if (!m_resource.has_value()) {
            throw std::logic_error("get() called on UniqueResource that holds no value");
        }
        return m_resource.value();
    }

    /**
     * @brief Get a const reference to the managed resource.
     * @return Reference to the resource.
     * @throw logic_error if no resource is held.
     */
    const ValueType& get() const
    {
        if (!m_resource.has_value()) {
            throw std::logic_error("get() called on UniqueResource that holds no value");
        }
        return m_resource.value();
    }

    /**
     * @brief Release ownership of the resource.
     * @return The released resource.
     * @throw logic_error if no resource is held.
     */
    ValueType release()
    {
        if (!m_resource.has_value()) {
            throw std::logic_error("release() called on UniqueResource that holds no value");
        }
        return unsafeRelease();
    }

    /**
     * @brief Destructor. Releases the resource if held.
     */
    ~UniqueResource() noexcept { reset(); }

private:
    std::optional<ValueType> m_resource;

    ValueType unsafeRelease() noexcept(std::is_nothrow_move_constructible_v<ValueType>)
    {
        ValueType value {std::move(*m_resource)};
        m_resource.reset();
        return std::move(value);
    }
};

/**
 * @brief Type alias for UniqueResource managing a file descriptor.
 */
using UniqueFd = UniqueResource<int, deleters::Close>;
} // namespace amon::util
