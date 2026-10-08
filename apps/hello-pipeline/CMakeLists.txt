add_executable(hello-pipeline
    src/DummyStage.cpp
    src/main.cpp
)

target_link_libraries(hello-pipeline PRIVATE amon-lib amon-dpdk)
