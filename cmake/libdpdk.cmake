find_package(PkgConfig REQUIRED)

if (AMON_USE_SYSTEM_DPDK)
    pkg_check_modules(libdpdk REQUIRED IMPORTED_TARGET libdpdk>=21.11)
    add_library(libdpdk ALIAS PkgConfig::libdpdk)

else()
    include(FetchContent)

    find_program(MESON_EXECUTABLE meson REQUIRED)
    find_program(NINJA_EXECUTABLE ninja REQUIRED)

    FetchContent_Declare(
        dpdk
        GIT_REPOSITORY https://github.com/DPDK/dpdk.git
        GIT_TAG 6953b6da139a83b62c92093aab8e88a5c1e0c042 # v25.11.4
    )
    FetchContent_MakeAvailable(dpdk)

    set(dpdk_prefix "${dpdk_BINARY_DIR}/install")
    set(dpdk_build_dir "${dpdk_BINARY_DIR}/build")
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(dpdk_build_type debug)
    else()
        set(dpdk_build_type release)
    endif()

    set(DPDK_BUILD_PARAMS "CC=${CMAKE_C_COMPILER};CXX=${CMAKE_CXX_COMPILER};TAG=${AMON_DPDK_TAG};BUILD_TYPE=${dpdk_build_type};LIBS=*")
    set(DPDK_BUILD_PARAMS_FILE "${dpdk_BINARY_DIR}/CMAKE_BUILD_PARAMS")
    set(dpdk_old_build_params "")
    if (EXISTS "${DPDK_BUILD_PARAMS_FILE}")
        file(READ "${DPDK_BUILD_PARAMS_FILE}" dpdk_old_build_params)
    endif()

    if (NOT "${dpdk_old_build_params}" STREQUAL "${DPDK_BUILD_PARAMS}"
            OR NOT EXISTS "${dpdk_prefix}/lib/pkgconfig/libdpdk.pc"
            OR NOT EXISTS "${dpdk_prefix}/lib/librte_eal.a"
            OR NOT EXISTS "${dpdk_prefix}/lib/librte_acl.a")
        message(STATUS "Building DPDK")
        file(REMOVE "${DPDK_BUILD_PARAMS_FILE}")
        set(dpdk_setup_options "")
        if (EXISTS "${dpdk_build_dir}/meson-private/coredata.dat")
            list(APPEND dpdk_setup_options --wipe)
        endif()
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E env
                "CC=${CMAKE_C_COMPILER}" "CXX=${CMAKE_CXX_COMPILER}"
                ${MESON_EXECUTABLE} setup ${dpdk_setup_options}
                "${dpdk_build_dir}" "${dpdk_SOURCE_DIR}"
                --prefix=${dpdk_prefix} --libdir=lib
                --buildtype=${dpdk_build_type}
                -Ddefault_library=static -Db_staticpic=true -Dibverbs_link=shared
                -Dplatform=generic -Dtests=false -Denable_docs=false -Dexamples=
                -Denable_libs=*
                ${AMON_DPDK_MESON_OPTIONS}
            COMMAND_ERROR_IS_FATAL ANY
        )
        execute_process(
            COMMAND ${NINJA_EXECUTABLE} -C "${dpdk_build_dir}"
            COMMAND_ERROR_IS_FATAL ANY
        )
        execute_process(
            COMMAND ${MESON_EXECUTABLE} install -C "${dpdk_build_dir}"
            COMMAND_ERROR_IS_FATAL ANY
        )
        file(WRITE "${DPDK_BUILD_PARAMS_FILE}" "${DPDK_BUILD_PARAMS}")
        message(STATUS "DPDK build done")
    endif()

    # Use the local metadata and retain static library ordering and whole-archive
    # flags, which keep the drivers' registration code in the final executable.
    set(dpdk_old_pkg_config_path "$ENV{PKG_CONFIG_PATH}")
    set(ENV{PKG_CONFIG_PATH} "${dpdk_prefix}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    unset(__pkg_config_checked_amon_dpdk CACHE)
    pkg_check_modules(amon_dpdk REQUIRED "${dpdk_prefix}/lib/pkgconfig/libdpdk.pc")
    set(ENV{PKG_CONFIG_PATH} "${dpdk_old_pkg_config_path}")

    add_library(libdpdk INTERFACE)
    target_include_directories(libdpdk SYSTEM INTERFACE ${amon_dpdk_STATIC_INCLUDE_DIRS})
    target_compile_options(libdpdk INTERFACE ${amon_dpdk_STATIC_CFLAGS_OTHER})
    target_link_libraries(libdpdk INTERFACE ${amon_dpdk_STATIC_LDFLAGS})
endif()
