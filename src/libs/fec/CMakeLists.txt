########################################################################
# Project setup
########################################################################

project(libfec ASM C)


########################################################################
# Compiler specific setup
########################################################################

if((CMAKE_SYSTEM_PROCESSOR MATCHES "i386|i686|x86|AMD64") AND (CMAKE_SIZEOF_VOID_P EQUAL 4))
    set(TARGET_ARCH "x86")
elseif((CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64") AND (CMAKE_SIZEOF_VOID_P EQUAL 8))
    set(TARGET_ARCH "x64")
elseif((CMAKE_SYSTEM_PROCESSOR MATCHES "i386") AND (CMAKE_SIZEOF_VOID_P EQUAL 8) AND (APPLE))
    # Mac is weird like that.
    set(TARGET_ARCH "x64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm*")
    set(TARGET_ARCH "ARM")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)64le")
    set(TARGET_ARCH "ppc64" "ppc64le")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)64")
    set(TARGET_ARCH "ppc64" "ppc")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)")
    set(TARGET_ARCH "ppc")
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANGCC)
    add_definitions(-Wall)
    add_definitions(-Wno-unused)

    if(TARGET_ARCH MATCHES "x64")
         add_definitions(-fPIC)
         add_definitions(-msse2)
    elseif(TARGET_ARCH MATCHES "x86")
         add_definitions(-mmmx)
         add_definitions(-msse)
         add_definitions(-msse2)
    elseif(TARGET_ARCH MATCHES "ppc|ppc64")
         add_definitions(-fno-common)
         add_definitions(-faltivec)
    endif()

endif()


########################################################################
# Find build dependencies
########################################################################

find_library(M_LIB m REQUIRED)


########################################################################
# Checks for features.
########################################################################

include(CheckIncludeFile)
check_include_file("stdio.h"            HAVE_STDIO_H)
check_include_file("stdlib.h"           HAVE_STDLIB_H)
check_include_file("string.h"           HAVE_STRING_H)

include(CheckFunctionExists)
check_function_exists("memset"           HAVE_MEMSET)
check_function_exists("memmove"          HAVE_MEMMOVE)


########################################################################
# Setup apps
########################################################################

set(libfec_sources
    encode_rs_char.c
    decode_rs_char.c
    init_rs_char.c
)


########################################################################
# Setup libraries
########################################################################

add_library(fec STATIC ${libfec_sources})
set_target_properties(fec PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(fec ${M_LIB})


########################################################################
# Build the tests
########################################################################

add_subdirectory(test)
