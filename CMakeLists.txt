cmake_minimum_required(VERSION 3.25)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)
set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

project(adaptmon
    VERSION ${VERSION}
    LANGUAGES CXX C)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "14.0")
    message(FATAL_ERROR "GCC 14 or newer is required")
endif()

option(AMON_ENABLE_DOXYGEN    "Enable build of code documentation"            OFF)
option(AMON_ENABLE_TESTS      "Enable building of tests"                      OFF)
option(AMON_ENABLE_SANITIZERS "Enable building with sanitizers (ASAN, UBSAN)" OFF)
option(AMON_ENABLE_COVERAGE   "Enable building with coverage instrumentation" OFF)
option(AMON_ENABLE_DPDK       "Enable DPDK support"                           ON)
option(AMON_ENABLE_PCAP       "Enable PCAP support"                           ON)
option(AMON_ENABLE_APPS       "Enable building of apps"                       ON)
option(BUILD_SHARED_LIBS      "Enable building as shared library"             OFF)

set(AMON_ENABLE_ASSERTS "AUTO" CACHE STRING "Enable assertions (AUTO, ON, OFF)")
set_property(CACHE AMON_ENABLE_ASSERTS PROPERTY STRINGS AUTO ON OFF)
if (NOT AMON_ENABLE_ASSERTS MATCHES "^(AUTO|ON|OFF)$")
    message(FATAL_ERROR "AMON_ENABLE_ASSERTS must be one of: AUTO, ON, OFF")
endif()

set(DEFAULT_BUILD_TYPE "Release")
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS
        "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
    set(CMAKE_BUILD_TYPE ${DEFAULT_BUILD_TYPE}
        CACHE STRING "build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE
        PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(is_debug "$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>")
set(is_release "$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>")

# Link time optimization, enabled only for the optimized build types.
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL ON)

add_compile_options(
    -Wpedantic
    -Wall
    -Wextra
    -Wunused
    -Wconversion
    -Wsign-conversion
    $<${is_debug}:-g>
    $<${is_debug}:-ggdb3>
    -fdebug-prefix-map=${PROJECT_SOURCE_DIR}=.
    $<${is_release}:-O3>
    $<${is_release}:-Werror>
)

if (AMON_ENABLE_TESTS)
    set(CMAKE_CTEST_ARGUMENTS "--output-on-failure")
    include(CTest)
    enable_testing()
endif()

if (BUILD_SHARED_LIBS)
    set(AMON_BUILD_SHARED ON)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(BUILD_SHARED_LIBS OFF)
endif()

if (AMON_ENABLE_SANITIZERS)
    add_compile_options(
        -fsanitize=address
        -static-libasan
        -fsanitize=undefined
        -static-libubsan
        -fno-sanitize-recover=all)
    add_link_options(
        -fsanitize=address
        -static-libasan
        -fsanitize=undefined
        -static-libubsan
        -fno-sanitize-recover=all)
endif()

if (AMON_ENABLE_COVERAGE)
    add_compile_options(--coverage -fprofile-update=atomic)
    add_link_options(--coverage)
endif()

# Add external dependencies
find_package(Threads REQUIRED)
include(cmake/spdlog.cmake)
include(cmake/libbacktrace.cmake)
include(cmake/libpcap.cmake)
if (AMON_ENABLE_TESTS)
    include(cmake/gtest.cmake)
endif()
if (AMON_ENABLE_PCAP)
    include(cmake/libpcap.cmake)
endif()
include(cmake/yaml-cpp.cmake)
if (AMON_ENABLE_DPDK)
    include(cmake/libdpdk.cmake)
endif()

if (AMON_BUILD_SHARED)
    set(BUILD_SHARED_LIBS ON)
endif()

if (AMON_ENABLE_APPS)
    add_subdirectory(apps)
endif()
add_subdirectory(doc)
add_subdirectory(lib)
