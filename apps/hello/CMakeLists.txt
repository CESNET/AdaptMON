add_executable(hello
    src/main.cpp
)

target_link_libraries(hello PRIVATE amon-lib)
