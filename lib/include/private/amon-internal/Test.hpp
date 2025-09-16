/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief File to be included when writing tests. Wraps around doctest.
 * @copyright Copyright (c) 2025 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#ifndef AMON_ENABLE_TESTS
#define DOCTEST_CONFIG_DISABLE
#endif

#include <doctest/doctest.h>
