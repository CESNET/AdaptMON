include(FetchContent)

FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest
    GIT_TAG 52eb8108c5bdec04579160ae17225d66034bd723 # v1.17.0
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

foreach (target gtest gtest_main)
    if (TARGET ${target})
        target_compile_options(${target} PRIVATE -Wno-error -Wno-sign-conversion)
    endif()
endforeach()
