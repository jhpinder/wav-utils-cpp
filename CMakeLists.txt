cmake_minimum_required(VERSION 3.10)
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

# Optional: Build tests
option(BUILD_TESTS "Build unit tests" OFF)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
