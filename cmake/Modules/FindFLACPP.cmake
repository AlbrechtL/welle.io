# - Find libFLAC++
#
#  This module defines
#  FLACPP_FOUND         - True if libFLAC++ has been found.
#  FLACPP_LIBRARIES     - List of libraries when using libFLAC++.
#  FLACPP_INCLUDE_DIRS  - libFLAC++ include directories.

# Look for the header file.
find_path(FLACPP_INCLUDE_DIRS
		NAMES FLAC++/all.h FLAC++/decoder.h FLAC++/encoder.h FLAC++/export.h FLAC++/metadata.h)

# Find the library.
find_library(FLACPP_LIBRARIES
		NAMES FLAC++)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FLACPP DEFAULT_MSG FLACPP_LIBRARIES FLACPP_INCLUDE_DIRS)
