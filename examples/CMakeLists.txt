cmake_minimum_required(VERSION 3.10)

# Basic example showing how to use the wav::Reader class
add_executable(basic_usage basic_usage.cpp)
target_link_libraries(basic_usage PRIVATE wav)
