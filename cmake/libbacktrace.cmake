include(FetchContent)

FetchContent_Declare(
    libbacktrace
    GIT_REPOSITORY https://github.com/ianlancetaylor/libbacktrace.git
    GIT_TAG 793921876c981ce49759114d7bb89bb89b2d3a2d
)
FetchContent_MakeAvailable(libbacktrace)

if (CMAKE_POSITION_INDEPENDENT_CODE)
    set(LIBBACKTRACE_WITH_PIC "--with-pic")
else()
    set(LIBBACKTRACE_WITH_PIC "")
endif()

set(LIBBACKTRACE_BUILD_PARAMS "CC=${CMAKE_C_COMPILER};CXX=${CMAKE_CXX_COMPILER};LIBBACKTRACE_WITH_PIC=${LIBBACKTRACE_WITH_PIC}")
set(LIBBACKTRACE_BUILD_PARAMS_FILE "${libbacktrace_BINARY_DIR}/CMAKE_BUILD_PARAMS")
if (EXISTS "${LIBBACKTRACE_BUILD_PARAMS_FILE}")
    file(READ "${LIBBACKTRACE_BUILD_PARAMS_FILE}" OLD_BUILD_PARAMS)
else()
    set(OLD_BUILD_PARAMS "")
endif()

if (NOT "${OLD_BUILD_PARAMS}" STREQUAL "${LIBBACKTRACE_BUILD_PARAMS}")
    file(WRITE "${LIBBACKTRACE_BUILD_PARAMS_FILE}" "${LIBBACKTRACE_BUILD_PARAMS}")
    message("== Building libbacktrace")
    execute_process(
        OUTPUT_QUIET
        ERROR_QUIET
        COMMAND sh -c "make distclean || true"
        WORKING_DIRECTORY ${libbacktrace_SOURCE_DIR}
    )
    execute_process(
        OUTPUT_QUIET
        COMMAND
            ./configure
            --prefix=${libbacktrace_BINARY_DIR}
            ${LIBBACKTRACE_WITH_PIC}
            CC=${CMAKE_C_COMPILER}
            CXX=${CMAKE_CXX_COMPILER}
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

make_directory("${libbacktrace_BINARY_DIR}/include")
set_target_properties(libbacktrace PROPERTIES
    IMPORTED_LOCATION ${libbacktrace_BINARY_DIR}/lib/libbacktrace.a
    INTERFACE_INCLUDE_DIRECTORIES ${libbacktrace_BINARY_DIR}/include
)
