include(FetchContent)

set(YAML_CPP_BUILD_TOOLS OFF)
set(YAML_CPP_BUILD_CONTRIB OFF)

FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG a83cd31548b19d50f3f983b069dceb4f4d50756d
)
FetchContent_MakeAvailable(yaml-cpp)

target_compile_options(yaml-cpp PRIVATE -Wno-sign-conversion -Wno-conversion -Wno-extra)
