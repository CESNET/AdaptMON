add_executable(pktparser-benchmark
    src/main.cpp
)

target_link_libraries(pktparser-benchmark PRIVATE amon-lib)
