find_package(PkgConfig REQUIRED)

pkg_check_modules(libdpdk REQUIRED IMPORTED_TARGET libdpdk>=21.11)

add_library(libdpdk ALIAS PkgConfig::libdpdk)
