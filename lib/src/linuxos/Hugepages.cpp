/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Utilities for managing hugepages.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amon/Error.hpp>
#include <amon/Logger.hpp>
#include <amon/linuxos/Hugepages.hpp>
#include <amon/util/Human.hpp>
#include <amon/util/io.hpp>
#include <amon/util/string.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <optional>
#include <set>
#include <string>

#include <sys/mount.h>

namespace amon::linuxos {

// default directory for non-NUMA huge pages
static constexpr const char* NO_NUMA_HUGE_DIR = "/sys/kernel/mm/hugepages";
// default base directory for NUMA nodes
static constexpr const char* NUMA_NODE_BASE_DIR = "/sys/devices/system/node";

static amon::Logger g_logger = LoggerProvider::get("Hugepages");

static unsigned int parseBytes(std::string_view str)
{
    str = util::string::trimSuffix(str, "B");

    unsigned int multiplier = 1;
    for (auto [suffix, mult] : std::array {
             // Yes, this is ugly, but it is the simplest way to solve this and it is only a static
             // helper function anyway...
             std::pair<std::string_view, unsigned int> {"k", 1024},
             std::pair<std::string_view, unsigned int> {" k", 1024},
             std::pair<std::string_view, unsigned int> {"M", 1024 * 1024},
             std::pair<std::string_view, unsigned int> {" M", 1024 * 1024},
             std::pair<std::string_view, unsigned int> {"G", 1024 * 1024 * 1024},
             std::pair<std::string_view, unsigned int> {" G", 1024 * 1024 * 1024},
         }) {
        if (auto rest = util::string::stripSuffix(str, suffix)) {
            multiplier = mult;
            str = *rest;
            break;
        }
    }

    return util::string::parse<unsigned int>(str) * multiplier;
}

static unsigned int getHugepageDirBytes(std::string_view dirName)
{
    auto parts
        = util::string::splitOnce(dirName, "-"); // dirname is for example hugepages-1048576kB
    if (!parts) {
        throw amon::Error(
            "unexpected hugepage dir name '{}': expected format 'hugepages-XXX'",
            dirName);
    }
    const std::string_view sizeStr
        = util::string::trimSuffix(parts->second, "B"); // parts->second is for example 1048576kB
    return parseBytes(sizeStr);
}

static void doMount(
    const std::string& mountPath,
    unsigned int pageSizeBytes,
    std::optional<unsigned int> uid,
    std::optional<unsigned int> gid)
{
    g_logger.info(
        "Mounting {} hugepages at '{}' (uid: {}, gid: {})",
        util::HumanReadableByteAmount1024 {pageSizeBytes},
        mountPath,
        uid.value_or(0),
        gid.value_or(0));

    std::string opts;
    opts += "pagesize=" + std::to_string(pageSizeBytes);
    if (uid) {
        opts += ",uid=" + std::to_string(uid.value());
    }
    if (gid) {
        opts += ",gid=" + std::to_string(gid.value());
    }
    const int ret = ::mount("nodev", mountPath.c_str(), "hugetlbfs", 0, opts.c_str());
    if (ret != 0) {
        throw amon::ErrnoError(-ret, "mount", "nodev", mountPath, "hugetlbfs", 0, opts);
    }
}

static void doUnmount(const std::string& mountPath)
{
    g_logger.info("Unmounting hugepages at '{}'", mountPath);
    const int ret = ::umount(mountPath.c_str());
    if (ret != 0) {
        throw amon::ErrnoError("umount", mountPath);
    }
}

namespace {
struct HugepagesSysDir {
    std::filesystem::path path;
    unsigned int socketId;
    unsigned int pageSizeBytes;
};
} // namespace

static std::vector<HugepagesSysDir> scanHugepageSysDirs()
{
    std::vector<HugepagesSysDir> res;

    if (std::filesystem::exists(NUMA_NODE_BASE_DIR)) {
        for (unsigned int socketId = 0;; ++socketId) {
            const std::filesystem::path nodeDir
                = std::filesystem::path(NUMA_NODE_BASE_DIR) / ("node" + std::to_string(socketId));
            if (!std::filesystem::exists(nodeDir)) {
                break;
            }

            for (const auto& entry : std::filesystem::directory_iterator {nodeDir / "hugepages"}) {
                res.push_back(
                    HugepagesSysDir {
                        .path = entry.path(),
                        .socketId = socketId,
                        .pageSizeBytes = getHugepageDirBytes(std::string {entry.path().filename()}),
                    });
            }
        }

    } else if (std::filesystem::exists(NO_NUMA_HUGE_DIR)) {
        for (const auto& entry : std::filesystem::directory_iterator {NO_NUMA_HUGE_DIR}) {
            res.push_back(
                HugepagesSysDir {
                    .path = entry.path(),
                    .socketId = 0,
                    .pageSizeBytes = getHugepageDirBytes(std::string {entry.path().filename()}),
                });
        }
    }

    return res;
}

static std::set<unsigned int> getSocketIds(const std::vector<HugepagesSysDir>& hugeDirs)
{
    std::set<unsigned int> res;
    for (const auto& dir : hugeDirs) {
        res.emplace(dir.socketId);
    }
    return res;
}

static std::filesystem::path getHugepageSysDir(
    const std::vector<HugepagesSysDir>& dirs,
    unsigned int pageSizeBytes,
    unsigned int socketId)
{
    auto it = std::ranges::find_if(dirs, [&](const HugepagesSysDir& dir) {
        return dir.socketId == socketId && dir.pageSizeBytes == pageSizeBytes;
    });
    if (it == dirs.end()) {
        throw amon::Error(
            "could not find hugepages dir for pageSize={} socketId={}",
            util::HumanReadableByteAmount1024 {pageSizeBytes},
            socketId);
    }
    return it->path;
}

static unsigned int doGetDefaultHugepageSize()
{
    std::ifstream ifs {"/proc/meminfo"};
    if (!ifs) {
        throw amon::Error("could not open '/proc/meminfo' for reading");
    }

    std::string line;
    while (std::getline(ifs, line)) {
        if (auto rest = util::string::stripPrefix(line, "Hugepagesize:")) {
            return parseBytes(util::string::trimWhitespace(*rest));
        }
    }

    throw amon::Error(
        "could not determine default hugepage size: key 'Hugepagesize:' not found in "
        "/proc/meminfo");
}

static void doReserve(
    const std::vector<HugepagesSysDir>& hugepageDirs,
    unsigned int pageSizeBytes,
    unsigned int nPages,
    unsigned int socketId)
{
    g_logger.info(
        "Reserving {}x {} hugepages on socket ID {}",
        nPages,
        util::HumanReadableByteAmount1024 {pageSizeBytes},
        socketId);

    const std::filesystem::path hugeDir = getHugepageSysDir(hugepageDirs, pageSizeBytes, socketId);

    const std::string nPagesStr = std::to_string(nPages);
    util::io::writeAll(hugeDir / "nr_hugepages", nPagesStr);

    std::string contents {
        util::string::trimWhitespace(util::io::readAll(hugeDir / "nr_hugepages"))};
    if (contents != nPagesStr) {
        throw amon::Error {
            "could not reserve hugepages ({}): wanted {}, got {}",
            util::HumanReadableByteAmount1024 {pageSizeBytes},
            nPages,
            contents};
    }
}

static std::pair<bool, unsigned> checkMounted(const std::string& mountPath)
{
    for (auto line : util::string::split(util::io::readAll("/proc/mounts"), '\n')) {
        auto parts = util::string::splitWhitespace(line);
        if (parts.size() >= 3 && parts[2] == "hugetlbfs" && parts[1] == mountPath) {
            unsigned pageSize = 0;
            if (parts.size() >= 4) {
                auto optParts = util::string::split(parts[3], "pagesize=");
                if (optParts.size() >= 2) {
                    pageSize = parseBytes(util::string::split(optParts[1], ",")[0]);
                }
            }
            return {true, pageSize};
        }
    }
    return {false, 0};
}

void Hugepages::clear()
{
    g_logger.info("Clearing all hugepage reservations");
    for (const auto& hugeDir : scanHugepageSysDirs()) {
        g_logger.debug(
            "Clearing hugepages of size {} for socket ID {} (at '{}')",
            util::HumanReadableByteAmount1024 {hugeDir.pageSizeBytes},
            hugeDir.socketId,
            hugeDir.path.string());
        util::io::writeAll(hugeDir.path / "nr_hugepages", "0");
    }
}

void Hugepages::reserve(unsigned int pageSizeBytes, unsigned int nPages, unsigned int socketId)
{
    const std::vector<HugepagesSysDir> hugepageDirs = scanHugepageSysDirs();
    doReserve(hugepageDirs, pageSizeBytes, nPages, socketId);
}

void Hugepages::mount(
    unsigned int pageSizeBytes,
    const std::string& mountPath,
    std::optional<unsigned int> uid,
    std::optional<unsigned int> gid)
{
    doMount(mountPath, pageSizeBytes, uid, gid);
}

void Hugepages::unmount(const std::string& mountPath)
{
    doUnmount(mountPath);
}

void Hugepages::setup(
    unsigned int pageSizeBytes,
    unsigned int nPages,
    const std::string& mountPath,
    std::optional<unsigned int> socketId)
{
    const std::vector<HugepagesSysDir> hugepageDirs = scanHugepageSysDirs();

    if (socketId.has_value()) {
        doReserve(hugepageDirs, pageSizeBytes, nPages, socketId.value());

    } else {
        for (const unsigned int socketId : getSocketIds(hugepageDirs)) {
            doReserve(hugepageDirs, pageSizeBytes, nPages, socketId);
        }
    }

    auto [isMounted, mountedPageSize] = checkMounted(mountPath);
    if (isMounted && mountedPageSize != pageSizeBytes) {
        throw amon::Error {
            "hugepages are mounted at '{}' but of size {}, not {}",
            mountPath,
            util::HumanReadableByteAmount1024 {mountedPageSize},
            util::HumanReadableByteAmount1024 {pageSizeBytes}};
    }

    if (!isMounted) {
        g_logger.info("Existing hugepage mount for '{}' not found...", mountPath);
        doMount(mountPath, pageSizeBytes, std::nullopt, std::nullopt);
    } else {
        g_logger.info(
            "Hugepage mount for '{}' with page size {} already exists, skipping mount step",
            mountPath,
            util::HumanReadableByteAmount1024 {pageSizeBytes});
    }
}

unsigned int Hugepages::getDefaultHugepageSize()
{
    return doGetDefaultHugepageSize();
}

} // namespace amon::linuxos
