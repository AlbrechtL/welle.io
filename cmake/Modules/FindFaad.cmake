# Try to find FAAD library and include path.
# Once done this will define
#
# FAAD_INCLUDE_DIRS - where to find faad.h, etc.
# FAAD_LIBRARIES - List of libraries when using libfaad.
# FAAD_FOUND - True if libfaad found.

find_path(FAAD_INCLUDE_DIR faad.h DOC "The directory where faad.h resides")
find_library(FAAD_LIBRARY NAMES faad DOC "The libfaad library")

if(FAAD_INCLUDE_DIR AND FAAD_LIBRARY)
  set(FAAD_FOUND 1)
  set(FAAD_LIBRARIES ${FAAD_LIBRARY})
  set(FAAD_INCLUDE_DIRS ${FAAD_INCLUDE_DIR})
else(FAAD_INCLUDE_DIR AND FAAD_LIBRARY)
  set(FAAD_FOUND 0)
  set(FAAD_LIBRARIES)
  set(FAAD_INCLUDE_DIRS)
endif(FAAD_INCLUDE_DIR AND FAAD_LIBRARY)

mark_as_advanced(FAAD_INCLUDE_DIR)
mark_as_advanced(FAAD_LIBRARY)
mark_as_advanced(FAAD_FOUND)

if(NOT FAAD_FOUND)
  message(FATAL_ERROR "libfaad was not found. Make sure FAAD_LIBRARY and FAAD_INCLUDE_DIR are set.")
else(NOT FAAD_FOUND)
  message(STATUS "Found libfaad: ${FAAD_INCLUDE_DIR}, ${FAAD_LIBRARY}")
endif(NOT FAAD_FOUND)
