########################################################################
# Checks for features.
########################################################################

include(CheckIncludeFile)
check_include_file("stdio.h"            HAVE_STDIO_H)
check_include_file("stdlib.h"           HAVE_STDLIB_H)
check_include_file("memory.h"           HAVE_MEMORY_H)
check_include_file("time.h"           HAVE_TIME_H)

include(CheckFunctionExists)
check_function_exists("memset"           HAVE_MEMSET)


########################################################################
# Build the executables and set up the tests
########################################################################

include_directories(.. ${CMAKE_CURRENT_BINARY_DIR})

add_executable(rs_speedtest rs_speedtest.c)
target_link_libraries(rs_speedtest fec)
add_test(rs_speedtest rs_speedtest)

add_executable(rstest rstest.c)
target_link_libraries(rstest fec)
add_test(rstest rstest)
