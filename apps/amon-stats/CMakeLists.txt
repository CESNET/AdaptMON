add_executable(amon-stats
    src/LayerCounter.cpp
    src/LayerNode.cpp
    src/main.cpp
)

target_link_libraries(amon-stats PRIVATE amon-lib Threads::Threads)

if (AMON_ENABLE_INSTALL)
    install(TARGETS amon-stats RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
