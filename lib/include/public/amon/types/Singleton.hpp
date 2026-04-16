/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief CRTP-based Singleton pattern implementation.
 *
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

namespace amon::types {

/**
 * @brief CRTP-based Singleton pattern implementation.
 *
 * This template class provides a thread-safe singleton instance of the derived class.
 *
 * Usage:
 *   Inherit from Singleton<YourClass> in your class definition. The constructor of the derived
 * class must be private or protected, and Singleton<YourClass> must be a friend to allow
 * instantiation.
 *
 * Example:
 *   @code{.cpp}
 *   class SingletonImpl : public Singleton<SingletonImpl> {
 *   private:
 *       friend class Singleton<SingletonImpl>;
 *       SingletonImpl() {}
 *   };
 *   @endcode
 *
 * @tparam DerivedClass The class that must be a singleton.
 */
template <typename DerivedClass>
class Singleton {
public:
    /**
     * @brief Returns the singleton instance of the derived class.
     *
     * @return Reference to the singleton instance.
     */
    static DerivedClass& instance()
    {
        static DerivedClass inst;
        return inst;
    }

    /**
     * @brief Deleted copy constructor.
     */
    Singleton(const Singleton&) = delete; // NOLINT(bugprone-crtp-constructor-accessibility)

    /**
     * @brief Deleted move constructor.
     */
    Singleton(Singleton&&) = delete; // NOLINT(bugprone-crtp-constructor-accessibility)

    /**
     * @brief Deleted copy assignment operator.
     */
    Singleton& operator=(const Singleton&)
        = delete; // NOLINT(bugprone-crtp-constructor-accessibility)

    /**
     * @brief Deleted move assignment operator.
     */
    Singleton& operator=(Singleton&&) = delete; // NOLINT(bugprone-crtp-constructor-accessibility)

private:
    friend DerivedClass;

    /**
     * @brief Default constructor.
     */
    Singleton() = default;
};

} // namespace amon::types
