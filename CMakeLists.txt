cmake_minimum_required(VERSION 3.22)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)
set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

project(adaptmon
    VERSION ${VERSION}
    LANGUAGES CXX C)

option(ENABLE_DOXYGEN  "Enable build of code documentation"    OFF)
option(ENABLE_TESTS    "Enable building of tests"              OFF)

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

# Add external dependencies
include(cmake/spdlog.cmake)
include(cmake/libbacktrace.cmake)
include(cmake/doctest.cmake)

add_subdirectory(apps)
add_subdirectory(doc)
add_subdirectory(lib)
