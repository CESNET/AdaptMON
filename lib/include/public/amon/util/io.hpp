/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Utilities for simple file I/O operations.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <filesystem>
#include <string>

namespace amon::util::io {

/**
 * @brief Write the entire string data to the file at the given path, overwriting if it exists.
 *
 * @warning The aim of this function is to be simple, not efficient. If you need efficiency, you
 * might be better off using something else.
 *
 * @param path The path to the file to write.
 * @param data The string data to write to the file.
 */
void writeAll(const std::filesystem::path& path, const std::string& data);

/**
 * @brief Read the entire contents of the file at the given path into a string.
 *
 * @warning The aim of this function is to be simple, not efficient. If you need efficiency, you
 * might be better off using something else.
 *
 * @param path The path to the file to read.
 * @return The contents of the file as a string.
 */
std::string readAll(const std::filesystem::path& path);

} // namespace amon::util::io
