include(FetchContent)
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.12
)
FetchContent_MakeAvailable(doctest)

add_executable(test_basics test_basics.cpp)
target_link_libraries(test_basics PRIVATE wav doctest::doctest)

add_test(NAME test_basics COMMAND test_basics)