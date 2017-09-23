ADD_DEFINITIONS(-std=c++98)
ADD_DEFINITIONS(-Werror)

IF( NOT CMAKE_BUILD_TYPE )
   SET( CMAKE_BUILD_TYPE Debug ... FORCE )
ENDIF()

file( GLOB_RECURSE SDRDAB_SOURCES "*.cc" )

add_library("sdrdab" ${SDRDAB_SOURCES})
include_directories(src)

#include(../cmake/Modules/FindGStreamer.cmake)
#include_directories( ${GSTREAMER_INCLUDE_DIR})
#add_definitions(${GSTREAMER_DEFINITIONS})
#target_link_libraries("sdrdab" ${GSTREAMER_LIBRARIES})

target_include_directories("sdrdab" "PUBLIC" "/usr/include/gstreamer-1.0/") #TODO
target_include_directories("sdrdab" "PUBLIC" "/usr/lib/gstreamer-1.0/include") #TODO ARCH_COMPAT
target_include_directories("sdrdab" "PUBLIC" "/usr/local/lib/gstreamer-1.0/include/") #TODO MAC_COMPAT
target_include_directories("sdrdab" "PUBLIC" "/usr/lib/x86_64-linux-gnu/gstreamer-1.0/include/") #TODO UBUNTU_COMPAT
target_include_directories("sdrdab" "PUBLIC" "/usr/local/include/gstreamer-1.0") #TODO MAC_COMPAT
target_include_directories("sdrdab" "PUBLIC" "/usr/lib/x86_64-linux-gnu/gstreamer-1.0/include/") #TODO UBUNTU_COMPAT
target_include_directories("sdrdab" "PUBLIC" "../rtlsdr/include/")#TODO
target_link_libraries("sdrdab" "gstreamer-1.0")
target_link_libraries("sdrdab" "gstapp-1.0")
target_link_libraries("sdrdab" "gobject-2.0")
target_link_libraries("sdrdab" "pthread")
target_link_libraries("sdrdab" "fftw3f")
target_link_libraries("sdrdab" "ecc")
target_link_libraries("sdrdab" "rtlsdr_static_custom")
target_link_libraries("sdrdab" "samplerate")

target_link_libraries("sdrdab" "glib-2.0")

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB_PKG glib-2.0)
if (GLIB_PKG_FOUND)
    message(Found glib-2.0)
include_directories(${GLIB_PKG_INCLUDE_DIRS})
ENDIF()
