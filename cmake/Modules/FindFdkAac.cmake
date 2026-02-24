# Try to find FDKAAC library and include path.
# Once done this will define
#
# FDKAAC_INCLUDE_DIRS - where to find faad.h, etc.
# FDKAAC_LIBRARIES - List of libraries when using libfaad.
# FDKAAC_FOUND - True if libfaad found.

find_path(FDKAAC_INCLUDE_DIR fdk-aac/aacdecoder_lib.h DOC "The directory where fdk-aac/aacdecoder_lib.h resides")
find_library(FDKAAC_LIBRARY NAMES fdk-aac DOC "The libfdk-aac library")

if(FDKAAC_INCLUDE_DIR AND FDKAAC_LIBRARY)
  set(FDKAAC_FOUND 1)
  set(FDKAAC_LIBRARIES ${FDKAAC_LIBRARY})
  set(FDKAAC_INCLUDE_DIRS ${FDKAAC_INCLUDE_DIR})
else(FDKAAC_INCLUDE_DIR AND FDKAAC_LIBRARY)
  set(FDKAAC_FOUND 0)
  set(FDKAAC_LIBRARIES)
  set(FDKAAC_INCLUDE_DIRS)
endif(FDKAAC_INCLUDE_DIR AND FDKAAC_LIBRARY)

mark_as_advanced(FDKAAC_INCLUDE_DIR)
mark_as_advanced(FDKAAC_LIBRARY)
mark_as_advanced(FDKAAC_FOUND)

if(NOT FDKAAC_FOUND)
  message(FATAL_ERROR "fdk-aac was not found. Make sure FDKAAC_LIBRARY and FDKAAC_INCLUDE_DIR are set.")
else(NOT FDKAAC_FOUND)
  message(STATUS "Found fdk-aac: ${FDKAAC_INCLUDE_DIR}, ${FDKAAC_LIBRARY}")
endif(NOT FDKAAC_FOUND)
