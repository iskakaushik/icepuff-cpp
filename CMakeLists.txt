cmake_minimum_required(VERSION 3.15)
project(icypuff 
    VERSION 0.1.0 
    LANGUAGES CXX
    DESCRIPTION "A simple C++ library"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(ICYPUFF_BUILD_EXAMPLES "Build example applications" ON)

# Dependencies
find_package(fmt CONFIG REQUIRED)

# Library
add_library(icypuff
    src/icypuff.cpp
)

add_library(icypuff::icypuff ALIAS icypuff)

target_include_directories(icypuff
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(icypuff
    PUBLIC
        fmt::fmt
)

# Installation
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(
    TARGETS icypuff
    EXPORT icypuff-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    DIRECTORY include/icypuff
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    EXPORT icypuff-targets
    FILE icypuff-targets.cmake
    NAMESPACE icypuff::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/icypuff
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/icypuff-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/icypuff-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/icypuff
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/icypuff-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/icypuff-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/icypuff-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/icypuff
)

# Examples
if(ICYPUFF_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Generate version.h
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/include/icypuff/version.h
    @ONLY
) 