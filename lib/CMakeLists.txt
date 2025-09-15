add_library(amon-lib
    src/hello.cpp
    src/Logger.cpp
    src/Stacktrace.cpp
    src/Error.cpp
    src/SignalHandler.cpp
)

target_include_directories(amon-lib
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/public
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/private)

target_link_libraries(amon-lib
    PRIVATE
        spdlog::spdlog
        libbacktrace
)
