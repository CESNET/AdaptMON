add_executable(amon-receiver
    src/Config.cpp
    src/main.cpp
    src/printers/TerminalPrinter.cpp
    src/stats/StatsManager.cpp
    src/stats/StatsQueue.cpp
)

target_link_libraries(amon-receiver PRIVATE amon-lib amon-dpdk)

if (AMON_ENABLE_INSTALL)
    install(TARGETS amon-receiver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
