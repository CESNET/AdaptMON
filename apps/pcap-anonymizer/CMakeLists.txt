add_executable(pcap-anonymizer
    src/3rdparty/cryptopan/panonymizer.cpp
    src/3rdparty/cryptopan/rijndael.cpp
    src/main.cpp
    src/Stats.cpp
    src/ValueAnonymizer.cpp
)

target_link_libraries(pcap-anonymizer PRIVATE amon-lib)
