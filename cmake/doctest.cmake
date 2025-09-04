include(FetchContent)

FetchContent_Declare(doctest
    GIT_REPOSITORY https://github.com/doctest/doctest
    GIT_TAG 1da23a3e8119ec5cce4f9388e91b065e20bf06f5 # v2.4.12
)
FetchContent_MakeAvailable(doctest)

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
