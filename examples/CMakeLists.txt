cmake_minimum_required(VERSION 3.10)

# Basic example showing how to use the wav::Reader class
add_executable(basic_usage basic_usage.cpp)
target_link_libraries(basic_usage PRIVATE wav)

# Copy example data files (wavs/) to build output directory
add_custom_target(copy_example_wavs ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/wavs
    ${CMAKE_CURRENT_BINARY_DIR}/wavs
    COMMENT "Copying example WAV files to build output"
)
add_dependencies(basic_usage copy_example_wavs)

# Set working directory so the example can find wavs/ relative to build/examples
set_target_properties(basic_usage PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
