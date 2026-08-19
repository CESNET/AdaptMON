/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Helpful ANSI code definitions for richer terminal output.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <cstdint>
#include <format>
#include <string>

/// Switch to alternate buffer
#define AMON_ANSI_ALTBUF_SWITCH "\033[?1049h"
/// Return to normal buffer
#define AMON_ANSI_ALTBUF_RETURN "\033[?1049l"
/// Clear screen
#define AMON_ANSI_CLEAR_SCREEN "\033[2J"
/// Clear line
#define AMON_ANSI_CLEAR_LINE "\033[2K"
/// Color provided FG, BG numbers
#define AMON_ANSI_COLOR(FG, BG) "\033[" #FG ";" #BG "m"
/// Black foreground color
#define AMON_ANSI_COLOR_BLACK "\033[0;30m"
/// Red foreground color
#define AMON_ANSI_COLOR_RED "\033[0;31m"
/// Green foreground color
#define AMON_ANSI_COLOR_GREEN "\033[0;32m"
/// Yellow foreground color
#define AMON_ANSI_COLOR_YELLOW "\033[0;33m"
/// Blue foreground color
#define AMON_ANSI_COLOR_BLUE "\033[0;34m"
/// Purple foreground color
#define AMON_ANSI_COLOR_PURPLE "\033[0;35m"
/// Cyan foreground color
#define AMON_ANSI_COLOR_CYAN "\033[0;36m"
/// White foreground color
#define AMON_ANSI_COLOR_WHITE "\033[0;37m"
/// Reset color and other text formatting
#define AMON_ANSI_RESET "\033[0m"
/**
 * Moves the cursor up by the number of lines.
 * @param linesCount Number of lines to move.
 * @return ANSI string to move.
 */
static inline std::string amonAnsiMoveBackLines(uint32_t linesCount)
{
    if (linesCount == 0) {
        return {};
    }
    return std::format("\033[{}A", linesCount);
}
