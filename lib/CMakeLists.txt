add_library(amon-lib
    src/hello.cpp
    src/Logger.cpp
    src/Stacktrace.cpp
    src/Error.cpp
    src/SignalHandler.cpp
    src/layers/LayerView.cpp
    src/layers/Ethernet.cpp
    src/layers/ICMP.cpp
    src/layers/ICMPv6.cpp
    src/layers/IPv4.cpp
    src/layers/IPv6.cpp
    src/layers/MPLS.cpp
    src/layers/UDP.cpp
    src/layers/TCP.cpp
    src/layers/VLAN.cpp
    src/layers/GRE.cpp
    src/types/ByteArray.cpp
    src/types/IPAddress.cpp
    src/types/MACAddress.cpp
    src/types/Timestamp.cpp
    src/types/UIntBE.cpp
    src/util/numeric.cpp
    src/Packet.cpp
)

target_include_directories(amon-lib
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/public
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/private)

target_link_libraries(amon-lib
    PRIVATE
        spdlog::spdlog
        libbacktrace
        doctest
)

if (ENABLE_TESTS)
    add_executable(amon-lib-tests)

    target_compile_definitions(amon-lib-tests PRIVATE AMON_ENABLE_TESTS)

    get_target_property(srcs amon-lib SOURCES)
    target_sources(amon-lib-tests PRIVATE ${srcs} src/TestWithMain.cpp)

    get_target_property(libs amon-lib LINK_LIBRARIES)
    target_link_libraries(amon-lib-tests PRIVATE ${libs})

    get_target_property(incs amon-lib INCLUDE_DIRECTORIES)
    target_include_directories(amon-lib-tests PRIVATE ${incs})

    doctest_discover_tests(amon-lib-tests)
endif()
