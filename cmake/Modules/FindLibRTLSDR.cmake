if(NOT LIBRTLSDR_FOUND)

  include(FindPkgConfig)
  pkg_check_modules(LIBRTLSDR_PKG librtlsdr)

  find_path(LIBRTLSDR_INCLUDE_DIRS NAMES rtl-sdr.h
	PATHS
	${LIBRTLSDR_PKG_INCLUDE_DIRS}
	/usr/include
	/usr/local/include
  )

  find_library(LIBRTLSDR_LIBRARIES NAMES rtlsdr
	PATHS
	${LIBRTLSDR_PKG_LIBRARY_DIRS}
	/usr/lib
        /usr/lib64
	/usr/local/lib
  )

  if(LIBRTLSDR_INCLUDE_DIRS AND LIBRTLSDR_LIBRARIES)
	set(LIBRTLSDR_FOUND TRUE CACHE INTERNAL "librtlsdr found")
	message(STATUS "Found librtlsdr: ${LIBRTLSDR_INCLUDE_DIRS}, ${LIBRTLSDR_LIBRARIES}")
  else()
	set(LIBRTLSDR_FOUND FALSE CACHE INTERNAL "librtlsdr found")
	message(STATUS "librtlsdr not found.")
  endif()

  mark_as_advanced(LIBRTLSDR_INCLUDE_DIRS LIBRTLSDR_LIBRARIES)

endif()
