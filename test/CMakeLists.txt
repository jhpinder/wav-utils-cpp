include(FetchContent)
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.12
)
FetchContent_MakeAvailable(doctest)

add_executable(tests test_hello.cpp)
target_link_libraries(tests PRIVATE doctest::doctest)
target_include_directories(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_test(NAME HelloWorldTest COMMAND tests)