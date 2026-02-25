add_executable(pcap-anonymizer
    src/main.cpp
)

target_link_libraries(pcap-anonymizer PRIVATE amon-lib)
