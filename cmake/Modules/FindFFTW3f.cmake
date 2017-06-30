# http://tim.klingt.org/code/projects/supernova/repository/revisions/d336dd6f400e381bcfd720e96139656de0c53b6a/entry/cmake_modules/FindFFTW3f.cmake
# Modified to use pkg config and use standard var names

# Find single-precision (float) version of FFTW3

if(NOT FFTW3F_FOUND)

  include(FindPkgConfig)
  pkg_check_modules(PC_FFTW3F "fftw3f >= 3.0")

  find_path(
    FFTW3F_INCLUDE_DIRS
    NAMES fftw3.h
    HINTS $ENV{FFTW3_DIR}/include
          ${PC_FFTW3F_INCLUDE_DIR}
    PATHS /usr/local/include
          /usr/include
  )

  find_library(
    FFTW3F_LIBRARIES
    NAMES fftw3f libfftw3f
    HINTS $ENV{FFTW3_DIR}/lib
        ${PC_FFTW3F_LIBDIR}
    PATHS /usr/local/lib
          /usr/lib
          /usr/lib64
  )

  find_library(
    FFTW3F_THREADS_LIBRARIES
    NAMES fftw3f_threads libfftw3f_threads
    HINTS $ENV{FFTW3_DIR}/lib
        ${PC_FFTW3F_LIBDIR}
    PATHS /usr/local/lib
          /usr/lib
          /usr/lib64
  )

  if(FFTW3F_INCLUDE_DIRS AND FFTW3F_LIBRARIES)
    set(FFTW3F_FOUND TRUE CACHE INTERNAL "FFTW3f found")
    message(STATUS "Found FFTW3f: ${FFTW3F_INCLUDE_DIRS}, ${FFTW3F_LIBRARIES}")
  else()
    set(FFTW3F_FOUND FALSE CACHE INTERNAL "FFTW3f found")
    message(STATUS "FFTW3f not found.")
  endif()

  mark_as_advanced(FFTW3F_INCLUDE_DIRS FFTW3F_LIBRARIES FFTW3F_THREADS_LIBRARIES)

 endif()
