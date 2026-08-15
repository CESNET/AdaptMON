include(GNUInstallDirs)

add_library(amon-lib
    src/amon.cpp
    src/conf/ConfigError.cpp
    src/conf/Node.cpp
    src/conf/Parser.cpp
    src/conf/Scalar.cpp
    src/conf/Struct.cpp
    src/conf/yaml.cpp
    src/Error.cpp
    src/hello.cpp
    src/layers/AuthenticationHeader.cpp
    src/layers/ESP.cpp
    src/layers/Ethernet.cpp
    src/layers/GRE.cpp
    src/layers/ICMP.cpp
    src/layers/ICMPv6.cpp
    src/layers/IPv4.cpp
    src/layers/IPv6.cpp
    src/layers/LayerView.cpp
    src/layers/MPLS.cpp
    src/layers/TCP.cpp
    src/layers/TCPOptions.cpp
    src/layers/UDP.cpp
    src/layers/VLAN.cpp
    src/linuxos/Hugepages.cpp
    src/Logger.cpp
    src/Packet.cpp
    src/PacketParser.cpp
    src/SignalHandler.cpp
    src/Stacktrace.cpp
    src/Strerror.cpp
    src/types/BigEndian.cpp
    src/types/ByteArray.cpp
    src/types/IPAddress.cpp
    src/types/MACAddress.cpp
    src/types/Singleton.cpp
    src/types/Timestamp.cpp
    src/util/ArgParser.cpp
    src/util/ArgvList.cpp
    src/util/getopt.cpp
    src/util/Human.cpp
    src/util/io.cpp
    src/util/NameGenerator.cpp
    src/util/numeric.cpp
    src/util/string.cpp
)

if (AMON_ENABLE_PCAP)
    target_sources(amon-lib PRIVATE
        src/util/Pcap.cpp
    )
endif()

set_target_properties(amon-lib PROPERTIES
    OUTPUT_NAME amon
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

if (AMON_ENABLE_DPDK)
    add_library(amon-dpdk
        src/dpdk/Allocator.cpp
        src/dpdk/dpdk.cpp
        src/dpdk/DpdkPacket.cpp
        src/dpdk/EAL.cpp
        src/dpdk/Error.cpp
        src/dpdk/GlobalLCorePool.cpp
        src/dpdk/LCore.cpp
        src/dpdk/pipeline/Stage.cpp
        src/dpdk/pipeline/Stream.cpp
        src/dpdk/PktBurst.cpp
        src/dpdk/PktMempool.cpp
        src/dpdk/Port.cpp
        src/dpdk/PortHooks.cpp
        src/dpdk/porthooks/BNXT.cpp
        src/dpdk/porthooks/I40E.cpp
        src/dpdk/porthooks/MLX5.cpp
        src/dpdk/SocketID.cpp
        src/dpdk/XStats.cpp
    )

    set_target_properties(amon-dpdk PROPERTIES
        OUTPUT_NAME amon-dpdk
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )

    target_include_directories(amon-dpdk
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/public>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/private
    )
    target_link_libraries(amon-dpdk PUBLIC amon-lib PkgConfig::libdpdk)
endif()

target_include_directories(amon-lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/public>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/private)

target_link_libraries(amon-lib
    PRIVATE
        libbacktrace
        spdlog::spdlog
        Threads::Threads
        yaml-cpp
)

if (AMON_ENABLE_PCAP)
    target_link_libraries(amon-lib PUBLIC PkgConfig::libpcap)
endif()
add_custom_command(TARGET amon-lib PRE_LINK
    COMMAND sh -c '${PROJECT_SOURCE_DIR}/tools/generate-export-map > ${CMAKE_BINARY_DIR}/export.map'
    COMMENT "Generating export map for namespace amon"
)
target_link_options(amon-lib PRIVATE
    -Wl,--version-script=${CMAKE_BINARY_DIR}/export.map
)

if (AMON_ENABLE_DPDK)
    add_custom_command(TARGET amon-dpdk PRE_LINK
        COMMAND sh -c '${PROJECT_SOURCE_DIR}/tools/generate-export-map > ${CMAKE_BINARY_DIR}/export-dpdk.map'
        COMMENT "Generating export map for the amon DPDK library"
    )
    target_link_options(amon-dpdk PRIVATE
        -Wl,--version-script=${CMAKE_BINARY_DIR}/export-dpdk.map
    )
endif()

if (AMON_ENABLE_ASSERTS STREQUAL "ON" OR
        (AMON_ENABLE_ASSERTS STREQUAL "AUTO" AND CMAKE_BUILD_TYPE STREQUAL "Debug"))
    message(STATUS "AMON assertions will be enabled")
    target_compile_definitions(amon-lib PUBLIC AMON_ENABLE_ASSERTS=1)
else()
    message(STATUS "AMON assertions will be disabled")
endif()

if (AMON_ENABLE_INSTALL)
    install(TARGETS amon-lib
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

    if (AMON_ENABLE_DPDK)
        install(TARGETS amon-dpdk
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
    endif()

    install(DIRECTORY include/public/amon
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        PATTERN dpdk EXCLUDE
    )

    if (AMON_ENABLE_DPDK)
        install(DIRECTORY include/public/amon/dpdk
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/amon
        )
    endif()
endif()

if (AMON_ENABLE_TESTS)
    add_executable(amon-lib-tests)

    target_compile_definitions(amon-lib-tests PRIVATE AMON_ENABLE_TESTS)

    get_target_property(opts amon-lib COMPILE_OPTIONS)
    target_compile_options(amon-lib-tests PRIVATE ${opts})
    target_compile_definitions(amon-lib-tests PUBLIC AMON_ENABLE_ASSERTS=1)

    get_target_property(srcs amon-lib SOURCES)
    target_sources(amon-lib-tests PRIVATE ${srcs} src/TestWithMain.cpp)

    get_target_property(libs amon-lib LINK_LIBRARIES)
    target_link_libraries(amon-lib-tests PRIVATE ${libs} GTest::gtest_main)

    get_target_property(incs amon-lib INCLUDE_DIRECTORIES)
    target_include_directories(amon-lib-tests PRIVATE ${incs})

    gtest_add_tests(TARGET amon-lib-tests TEST_LIST amon_lib_test_list)
    set_tests_properties(${amon_lib_test_list}
        PROPERTIES ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0")
endif()
