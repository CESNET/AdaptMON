/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Utilities for simple file I/O operations.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon/Error.hpp>
#include <amon/util/UniqueResource.hpp>
#include <amon/util/io.hpp>

#include <algorithm>

#include <fcntl.h>

namespace amon::util::io {

void writeAll(const std::filesystem::path& path, const std::string& data)
{
    const int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0666);
    if (fd < 0) {
        throw amon::ErrnoError {"open", path};
    }
    const UniqueFd _ {fd};

    size_t offset = 0;
    const size_t len = data.size();
    while (offset < len) {
        const ssize_t nWritten = ::write(fd, &data[offset], len - offset);
        if (nWritten < 0 && (errno == EINTR || errno == EAGAIN)) {
            continue;
        }
        if (nWritten < 0) {
            throw amon::ErrnoError {"write", path};
        }
        offset += static_cast<size_t>(nWritten);
    }
}

std::string readAll(const std::filesystem::path& path)
{
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        throw amon::ErrnoError {"open", path};
    }
    const UniqueFd _ {fd};

    std::string result;
    static constexpr size_t bufsize = 4096;
    char buffer[bufsize];

    while (true) {
        const ssize_t nRead = ::read(fd, buffer, bufsize);
        if (nRead == 0) {
            break;
        }
        if (nRead < 0 && (errno == EINTR || errno == EAGAIN)) {
            continue;
        }
        if (nRead < 0) {
            throw amon::ErrnoError {"read", path};
        }
        result.append(buffer, static_cast<size_t>(nRead));
    }

    return result;
}

} // namespace amon::util::io
