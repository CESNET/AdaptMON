include(FetchContent)

FetchContent_Declare(
    libbacktrace
    GIT_REPOSITORY https://github.com/ianlancetaylor/libbacktrace.git
    GIT_TAG 793921876c981ce49759114d7bb89bb89b2d3a2d
)
FetchContent_MakeAvailable(libbacktrace)

if (NOT EXISTS ${libbacktrace_BINARY_DIR}/lib/libbacktrace.a)
    set(ENV{CC} ${CMAKE_C_COMPILER})
    set(ENV{CXX} ${CMAKE_CXX_COMPILER})
    message("== Building libbacktrace")
    execute_process(
        OUTPUT_QUIET
        COMMAND ./configure --prefix=${libbacktrace_BINARY_DIR}
        WORKING_DIRECTORY ${libbacktrace_SOURCE_DIR}
    )
    execute_process(
        OUTPUT_QUIET
        COMMAND make -j
        WORKING_DIRECTORY ${libbacktrace_SOURCE_DIR}
    )
    execute_process(
        OUTPUT_QUIET
        COMMAND make install
        WORKING_DIRECTORY ${libbacktrace_SOURCE_DIR}
    )
    message("== libbacktrace build done")
endif()

add_library(libbacktrace STATIC IMPORTED)
set_target_properties(libbacktrace PROPERTIES
    IMPORTED_LOCATION ${libbacktrace_BINARY_DIR}/lib/libbacktrace.a
    INTERFACE_INCLUDE_DIRECTORIES ${libbacktrace_BINARY_DIR}/include
)
