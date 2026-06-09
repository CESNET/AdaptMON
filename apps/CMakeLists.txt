add_subdirectory(hello)

if (ENABLE_PCAP)
    add_subdirectory(amon-anonymizer)
    add_subdirectory(amon-stats)
else ()
    message(STATUS "Skipping amon-anonymizer and amon-stats because PCAP support is disabled")
endif ()
