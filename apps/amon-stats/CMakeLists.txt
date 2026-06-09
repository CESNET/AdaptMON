add_executable(amon-stats
    src/LayerNode.cpp
    src/main.cpp
)

target_link_libraries(amon-stats PRIVATE amon-lib)
