cmake_minimum_required(VERSION 3.24)
project(wav-utils-cpp VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define the header-only library
add_library(wav INTERFACE)
target_include_directories(wav INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Format target
file(GLOB_RECURSE ALL_SOURCE_FILES
    include/*.hpp
)

file(GLOB_RECURSE ALL_TEST_FILES
    test/*.cpp
)

add_custom_target(
    format
    COMMAND clang-format -i ${ALL_SOURCE_FILES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running clang-format on source files"
)

# cppcheck target
add_custom_target(
    cppcheck
    COMMAND cppcheck
            --enable=all
            --inconclusive
            --std=c++17
            --quiet
            --suppress-xml=suppressions.xml
            -ibuild
            ${CMAKE_SOURCE_DIR}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running cppcheck on include files"
)

# Optional: Build examples
option(BUILD_EXAMPLES "Build example programs" ON)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
    # Copy example data files (wavs/) to build output after examples are built
    add_custom_target(copy_example_data ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/wavs
        ${CMAKE_CURRENT_BINARY_DIR}/examples/wavs
        COMMENT "Copying example WAV files to build output"
    )
    add_dependencies(copy_example_data basic_usage)
endif()

# Enable testing globally
enable_testing()

# Create a minimal DartConfiguration.tcl in the build tree to avoid
# spurious ctest warnings when running dashboard-style tests (e.g. `-T test`).
# CTest will try to read ${CMAKE_BINARY_DIR}/DartConfiguration.tcl for
# dashboard configuration; if it's missing it prints a harmless warning.
# Writing a tiny file here suppresses that message.
file(WRITE "${CMAKE_BINARY_DIR}/DartConfiguration.tcl" "set(CTEST_SITE \"\")\n")

# Tests
add_subdirectory(test)