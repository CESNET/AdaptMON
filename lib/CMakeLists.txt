add_library(amon-lib
    src/hello.cpp
)

target_include_directories(amon-lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)

target_link_libraries(amon-lib
    PRIVATE
        spdlog::spdlog
)
