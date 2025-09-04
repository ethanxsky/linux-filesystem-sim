cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0076 NEW)
project(hw3 LANGUAGES C CXX)
option(BUILD_CODEGRADE_TESTS "Build test suites into separate executables" OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
include_directories(include)
link_directories(lib)

# vcpkg
# find_package(GTest)

# local tests
if (NOT BUILD_CODEGRADE_TESTS)
    # The 'hw3_main' target is removed entirely, as it requires the missing src/hw3.c file.

    # terminal program
    add_executable(terminal
        src/filesys.c
        src/utility.c
        src/inode_manip.c
        src/file_operations.c
        src/terminal.cpp
    )
    target_compile_options(terminal PUBLIC -g -D DEBUG -Wall -Wextra -Wshadow -Wdouble-promotion -Wformat=2 -Wundef -Werror -Wno-unused-parameter -Wno-shadow -D_POSIX_C_SOURCE=202503L)
    target_compile_definitions(terminal PUBLIC DEBUG)
    target_link_libraries(terminal PUBLIC m)

endif()

# The test executables (part0_tests, part1_tests, part2_tests, part3_tests)
# have been removed as they depend on the tests/ directory.