cmake_minimum_required(VERSION 3.22)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)
set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

project(adaptmon
    VERSION ${VERSION}
    LANGUAGES CXX C)

option(ENABLE_DOXYGEN    "Enable build of code documentation"            OFF)
option(ENABLE_TESTS      "Enable building of tests"                      OFF)
option(ENABLE_SANITIZERS "Enable building with sanitizers (ASAN, UBSAN)" OFF)
option(BUILD_SHARED_LIBS "Enable building as shared library"             OFF)

set(ENABLE_ASSERTS "AUTO" CACHE STRING "Enable assertions (AUTO, ON, OFF)")
set_property(CACHE ENABLE_ASSERTS PROPERTY STRINGS AUTO ON OFF)
if (NOT ENABLE_ASSERTS MATCHES "^(AUTO|ON|OFF)$")
    message(FATAL_ERROR "ENABLE_ASSERTS must be one of: AUTO, ON, OFF")
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

if (ENABLE_TESTS)
    include(CTest)
    enable_testing()
endif()

if (BUILD_SHARED_LIBS)
    set(AMON_BUILD_SHARED ON)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(BUILD_SHARED_LIBS OFF)
endif()

if (ENABLE_SANITIZERS)
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

# Add external dependencies
include(cmake/spdlog.cmake)
include(cmake/libbacktrace.cmake)
include(cmake/gtest.cmake)
include(cmake/libpcap.cmake)
include(cmake/yaml-cpp.cmake)
include(cmake/libdpdk.cmake)

if (AMON_BUILD_SHARED)
    set(BUILD_SHARED_LIBS ON)
endif()

add_subdirectory(apps)
add_subdirectory(doc)
add_subdirectory(lib)
