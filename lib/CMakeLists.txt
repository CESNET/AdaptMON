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
    src/types/ByteArray.cpp
    src/types/IPAddress.cpp
    src/types/MACAddress.cpp
    src/types/Singleton.cpp
    src/types/Timestamp.cpp
    src/types/UIntBE.cpp
    src/util/ArgParser.cpp
    src/util/ArgvList.cpp
    src/util/getopt.cpp
    src/util/Human.cpp
    src/util/io.cpp
    src/util/NameGenerator.cpp
    src/util/numeric.cpp
    src/util/string.cpp
)

if (ENABLE_DPDK)
    target_sources(amon-lib PRIVATE
        src/dpdk/dpdk.cpp
        src/dpdk/Error.cpp
        src/dpdk/PktBurst.cpp
        src/dpdk/PktMempool.cpp
        src/dpdk/SocketID.cpp
    )
endif()

if (ENABLE_PCAP)
    target_sources(amon-lib PRIVATE
        src/util/Pcap.cpp
    )
endif()

target_include_directories(amon-lib
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/public
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/private)

target_link_libraries(amon-lib
    PRIVATE
        libbacktrace
        spdlog::spdlog
        yaml-cpp
)

if (ENABLE_DPDK)
    target_link_libraries(amon-lib PUBLIC PkgConfig::libdpdk)
endif()

if (ENABLE_PCAP)
    target_link_libraries(amon-lib PUBLIC PkgConfig::libpcap)
endif()

add_custom_command(TARGET amon-lib PRE_LINK
    COMMAND sh -c '${PROJECT_SOURCE_DIR}/tools/generate-export-map > ${CMAKE_BINARY_DIR}/export.map'
    COMMENT "Generating export map for namespace amon"
)
target_link_options(amon-lib PRIVATE
    -Wl,--version-script=${CMAKE_BINARY_DIR}/export.map
)

if (ENABLE_ASSERTS STREQUAL "ON" OR
        (ENABLE_ASSERTS STREQUAL "AUTO" AND CMAKE_BUILD_TYPE STREQUAL "Debug"))
    message(STATUS "AMON assertions will be enabled")
    target_compile_definitions(amon-lib PUBLIC AMON_ENABLE_ASSERTS=1)
else()
    message(STATUS "AMON assertions will be disabled")
endif()

if (ENABLE_TESTS)
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
