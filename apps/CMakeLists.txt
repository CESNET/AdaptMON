add_subdirectory(hello)

if (ENABLE_PCAP)
    add_subdirectory(amon-anonymizer)
else()
    message(STATUS "Skipping amon-anonymizer because PCAP support is disabled")
endif()
