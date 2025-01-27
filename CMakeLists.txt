cmake_minimum_required(VERSION 3.15)
project(icypuff 
    VERSION 0.1.0 
    LANGUAGES CXX
    DESCRIPTION "A simple C++ library"
)

# Enforce C++20 and disable exceptions
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set UTF-8 encoding for source files
if(MSVC)
    add_compile_options(/utf-8)
else()
    add_compile_options(-finput-charset=UTF-8)
endif()

# Options
option(ICYPUFF_BUILD_EXAMPLES "Build example applications" ON)

# Global definitions
add_compile_definitions(SPDLOG_NO_EXCEPTIONS=ON)

# Dependencies
find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(lz4 CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# Collect source files
set(ICYPUFF_SOURCES
    src/blob.cpp
    src/icypuff.cpp
    src/blob_metadata.cpp
    src/file_metadata.cpp
    src/file_metadata_parser.cpp
    src/local_input_file.cpp
    src/local_output_file.cpp
    src/icypuff_reader.cpp
    src/icypuff_writer.cpp
)

set(ICYPUFF_HEADERS
    include/icypuff/blob.h
    include/icypuff/compression_codec.h
    include/icypuff/icypuff.h
    include/icypuff/macros.h
    include/icypuff/result.h
    include/icypuff/version.h
    include/icypuff/blob_metadata.h
    include/icypuff/file_metadata.h
    include/icypuff/file_metadata_parser.h
    include/icypuff/input_file.h
    include/icypuff/output_file.h
    include/icypuff/seekable_input_stream.h
    include/icypuff/position_output_stream.h
    include/icypuff/local_input_file.h
    include/icypuff/local_output_file.h
    include/icypuff/icypuff_writer.h
    include/icypuff/icypuff_reader.h
    include/icypuff/format_constants.h
)

# Library
add_library(icypuff
    ${ICYPUFF_SOURCES}
    ${ICYPUFF_HEADERS}
)

add_library(icypuff::icypuff ALIAS icypuff)

target_include_directories(icypuff
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Disable exceptions for our targets only
if(MSVC)
    target_compile_options(icypuff PRIVATE /EHs-c-)
    target_compile_definitions(icypuff PRIVATE _HAS_EXCEPTIONS=0)
else()
    target_compile_options(icypuff PRIVATE -fno-exceptions)
endif()

target_link_libraries(icypuff
    PUBLIC
        fmt::fmt
        nlohmann_json::nlohmann_json
        lz4::lz4
        $<IF:$<TARGET_EXISTS:zstd::libzstd_shared>,zstd::libzstd_shared,zstd::libzstd_static>
        spdlog::spdlog
)

# Set source groups for better IDE organization
source_group(
    TREE ${CMAKE_CURRENT_SOURCE_DIR}
    PREFIX "Source Files"
    FILES ${ICYPUFF_SOURCES}
)

source_group(
    TREE ${CMAKE_CURRENT_SOURCE_DIR}
    PREFIX "Header Files"
    FILES ${ICYPUFF_HEADERS}
)

# Testing
option(ICYPUFF_BUILD_TESTS "Build tests" ON)
if(ICYPUFF_BUILD_TESTS)
    enable_testing()
    find_package(GTest CONFIG REQUIRED)
    
    # Test sources
    set(ICYPUFF_TEST_SOURCES
        tests/file_metadata_parser_test.cpp
        tests/icypuff_reader_test.cpp
        tests/icypuff_writer_test.cpp
    )

    set(ICYPUFF_TEST_HEADERS
        tests/test_resources.h
    )
    
    # Add test executable
    add_executable(icypuff_tests
        ${ICYPUFF_TEST_SOURCES}
        ${ICYPUFF_TEST_HEADERS}
    )
    
    target_include_directories(icypuff_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    
    # Disable exceptions for tests
    if(MSVC)
        target_compile_options(icypuff_tests PRIVATE /EHs-c-)
        target_compile_definitions(icypuff_tests PRIVATE _HAS_EXCEPTIONS=0)
    else()
        target_compile_options(icypuff_tests PRIVATE -fno-exceptions)
    endif()
    
    target_link_libraries(icypuff_tests
        PRIVATE
            icypuff::icypuff
            GTest::gtest
            GTest::gtest_main
    )
    
    # Copy test resources to build directory
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tests/resources DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/tests)
    
    add_test(NAME icypuff_tests COMMAND icypuff_tests)
endif()

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