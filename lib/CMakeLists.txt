add_library(amon-lib
    src/conf/ConfigError.cpp
    src/conf/Node.cpp
    src/conf/Parser.cpp
    src/conf/Scalar.cpp
    src/conf/Struct.cpp
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
    src/Logger.cpp
    src/Packet.cpp
    src/PacketParser.cpp
    src/SignalHandler.cpp
    src/Stacktrace.cpp
    src/types/ByteArray.cpp
    src/types/IPAddress.cpp
    src/types/MACAddress.cpp
    src/types/Timestamp.cpp
    src/types/UIntBE.cpp
    src/util/ArgParser.cpp
    src/util/ArgvList.cpp
    src/util/getopt.cpp
    src/util/numeric.cpp
    src/util/Pcap.cpp
    src/util/string.cpp
)

target_include_directories(amon-lib
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/public
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/private)

target_link_libraries(amon-lib
    PRIVATE
        doctest
        libbacktrace
        spdlog::spdlog
        yaml-cpp
    PUBLIC
        PkgConfig::libpcap
)

add_custom_command(TARGET amon-lib PRE_LINK
    COMMAND sh -c '${PROJECT_SOURCE_DIR}/tools/generate-export-map > ${CMAKE_BINARY_DIR}/export.map'
    COMMENT "Generating export map for namespace amon"
)
target_link_options(amon-lib PRIVATE
    -Wl,--version-script=${CMAKE_BINARY_DIR}/export.map
)

if (ENABLE_SANITIZERS)
    target_link_libraries(amon-lib PRIVATE asan ubsan)
    target_compile_options(amon-lib PRIVATE -fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all)
endif()

if (ENABLE_TESTS)
    add_executable(amon-lib-tests)

    target_compile_definitions(amon-lib-tests PRIVATE AMON_ENABLE_TESTS)

    get_target_property(opts amon-lib COMPILE_OPTIONS)
    target_compile_options(amon-lib-tests PRIVATE ${opts})

    get_target_property(srcs amon-lib SOURCES)
    target_sources(amon-lib-tests PRIVATE ${srcs} src/TestWithMain.cpp)

    get_target_property(libs amon-lib LINK_LIBRARIES)
    target_link_libraries(amon-lib-tests PRIVATE ${libs})

    get_target_property(incs amon-lib INCLUDE_DIRECTORIES)
    target_include_directories(amon-lib-tests PRIVATE ${incs})

    doctest_discover_tests(amon-lib-tests)
endif()
