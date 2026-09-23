add_executable(amon-anonymizer
    src/3rdparty/cryptopan/panonymizer.cpp
    src/3rdparty/cryptopan/rijndael.cpp
    src/main.cpp
    src/PacketAnonymizer.cpp
    src/Stats.cpp
    src/ValueAnonymizer.cpp
)

target_link_libraries(amon-anonymizer PRIVATE amon-lib)

install(TARGETS amon-anonymizer RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
