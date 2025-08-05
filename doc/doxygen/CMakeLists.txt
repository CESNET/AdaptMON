find_package(Doxygen REQUIRED)

set(DOXYGEN_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/")

if (NOT DOXYGEN_WARN_AS_ERROR)
    set(DOXYGEN_WARN_AS_ERROR "NO")
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in"
    "${DOXYGEN_DIRECTORY}/Doxyfile"
    @ONLY
)

add_custom_target(
    doxygen
    COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
    WORKING_DIRECTORY "${DOXYGEN_DIRECTORY}"
    COMMENT "Generating API documentation with Doxygen to ${DOXYGEN_DIRECTORY}"
    VERBATIM
)

