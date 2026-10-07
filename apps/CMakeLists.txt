add_subdirectory(hello)

if (AMON_ENABLE_DPDK)
    add_subdirectory(amon-receiver)
else()
    message(STATUS "Skipping amon-receiver because DPDK support is disabled")
endif()

if (AMON_ENABLE_PCAP)
    add_subdirectory(amon-anonymizer)
    add_subdirectory(amon-stats)
else ()
    message(STATUS "Skipping amon-anonymizer and amon-stats because PCAP support is disabled")
endif ()

add_subdirectory(pktparser-benchmark)
