add_executable(amon-stats
    src/LayerCounter.cpp
    src/LayerNode.cpp
    src/main.cpp
)

target_link_libraries(amon-stats PRIVATE amon-lib)

install(TARGETS amon-stats RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
