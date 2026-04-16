/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Utilities for managing hugepages.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <optional>
#include <string>

namespace amon::linuxos {

/**
 * @brief Static class for hugepages management
 */
class Hugepages {
public:
    /// Default systemd mount point for huge pages
    static constexpr const char* DEFAULT_MOUNT_PATH = "/dev/hugepages";
    /// 1 GiB hugepage size in bytes.
    static constexpr unsigned int PAGESIZE_1GB = 1 * 1024 * 1024 * 1024;
    /// 2 MiB hugepage size in bytes.
    static constexpr unsigned int PAGESIZE_2MB = 2 * 1024 * 1024;

    /**
     * @brief Set up hugepages by reserving and mounting the specified number and size on a given
     * socket.
     *
     * This is the function you should use if you want to just make things work and don't need
     * anything special. It is the equivalent of calling reserve() followed by mount().
     *
     * If the requested amount of hugepages is already reserved, this will not reserve any more.
     * Likewise if the hugepages are already mounted, it will not try to mount them again.
     * It is therefore safe to call this function on every start of the application, even without
     * any prior clean-up.
     *
     * After the application exits, the hugepages still remain reserved/mounted. If you want to
     * clean-up the hugepages, you have to do it yourself using the clean() and unmount() functions.
     * Although it is usually not recommended as described in the clean() function docs. The
     * application will simply re-use them on next start.
     *
     * @warning If you mount hugepages at a different mount point than the default one, you have to
     * specify the hugepages path in DPDK EAL parameters as well!
     *
     * @param pageSizeBytes The size of each hugepage in bytes (see helpful constants such as
     * hugepage::PAGESIZE_1GB).
     * @param nPages The number of hugepages to reserve.
     * @param mountPath The mount path (default: DEFAULT_MOUNT_PATH)
     * @param socketId The NUMA socket ID on which to reserve the pages (default: none = all
     * sockets).
     * @throws amon::Error If the desired amount of hugepages could not be reserved or the mount
     * point is already occupied (most likely by hugepages of different size).
     */
    static void setup(
        unsigned int pageSizeBytes,
        unsigned int nPages,
        const std::string& mountPath = DEFAULT_MOUNT_PATH,
        std::optional<unsigned int> socketId = std::nullopt);

    /**
     * @brief Reserve a number of hugepages of a given size on a specific NUMA socket.
     *
     * If the requested amount of hugepages of the specific type on the specific socket is already
     * reserved, it does not reserve any more. Therefore you can call this repeatedly and the number
     * of reserved hugepages will not increase.
     *
     * @param pageSizeBytes The size of each hugepage in bytes.
     * @param nPages The number of hugepages to reserve.
     * @param socketId The NUMA socket ID on which to reserve the pages.
     * @throws amon::Error If the reservation was not possible to fulfill.
     */
    static void reserve(unsigned int pageSizeBytes, unsigned int nPages, unsigned int socketId);

    /**
     * @brief Clear all reserved hugepages on all sockets.
     *
     * Clear does NOT unmount the hugepages, it simply "unreserves" them.
     *
     * If you want to do full clean-up, you should call unmount() followed by clear().
     *
     * Although it is recommended to not do this and leave the hugepages allocated,
     * as repeated deallocations/allocations can lead to the hugepages not being able to be
     * allocated again due to fragmentation.
     *
     * @throws amon::Error on file write failure.
     */
    static void clear();

    /**
     * @brief Mount the hugepage filesystem with the specified page size and optional user/group
     * ownership.
     *
     * If the mount path is already occupied (even if it is because the correct hugepages are
     * already mounted there), this function will fail and throw an error.
     *
     * You do not provide a socket ID here. All sockets access the hugepages through the same
     * mountpoint.
     *
     * @param pageSizeBytes The size of each hugepage in bytes.
     * @param mountPath The mount path (default: DEFAULT_MOUNT_PATH)
     * @param uid Optional user ID to own the mount (default: none = root).
     * @param gid Optional group ID to own the mount (default: none = root).
     * @throws amon::Error If the mount operation fails.
     */
    static void mount(
        unsigned int pageSizeBytes,
        const std::string& mountPath = DEFAULT_MOUNT_PATH,
        std::optional<unsigned int> uid = std::nullopt,
        std::optional<unsigned int> gid = std::nullopt);

    /**
     * @brief Unmount the hugepage filesystem.
     *
     * @param mountPath The mount path (default: DEFAULT_MOUNT_PATH)
     * @throws amon::ErrnoError If the unmount operation fails.
     */
    static void unmount(const std::string& mountPath = DEFAULT_MOUNT_PATH);

    /**
     * @brief Get the default system hugepage size in bytes.
     *
     * This is a system-level setting. You cannot modify it by any of the functions provided here.
     *
     * @return The size in bytes.
     * @throws amon::Error If the size cannot be determined.
     */
    static unsigned int getDefaultHugepageSize();
};

} // namespace amon::linuxos
