cmake_minimum_required( VERSION 2.8.11 )
set (CMAKE_BUILD_TYPE Release )
set (objectName dab-rpi-0.995)
set ( CMAKE_INSTALL_PREFIX ../ )
add_definitions ( -Wall)
######################################################################
#       R E A D T H I S
#####################################################################
#	adjust to your likings
#	if you want to use the SSE support on your machine, uncomment
#	set(NO_SSE_SUPPORT true)
#
#	if you want support for any of these devices, uncomment the line
	set(SDRPLAY true)
	set(AIRSPY true)
	set(DABSTICK_OSMO true)
#	set(DABSTICK_NEW true)
#	set(RTLTCP true)

########################################################################
	find_package (Qt5Widgets REQUIRED)
	find_package (Qt5Declarative REQUIRED)

	find_library (DYNAMIC_LOAD dl)
	if(NOT(DYNAMIC_LOAD))
	   message(FATAL_ERROR "please install -ldl")
	else(NOT(DYNAMIC_LOAD))
	   set(extraLibs ${DYNAMIC_LOAD})
	endif(NOT(DYNAMIC_LOAD))
#
#	all listed variables are TRUE
#	Find_Package(PkgConfig)
#	pkg_check_modules(FFTW fftw3f REQUIRED)
#	link_directories(${FFTW_INCLUDE_DIRS})
#	Find the native FFTW includes and library
#
#	FFTW_INCLUDES    - where to find fftw3f.h
#	FFTW_LIBRARIES   - List of libraries when using FFTW.
#	FFTW_FOUND       - True if FFTW found.

	if (FFTW_INCLUDE_DIRS)
#	Already in cache, be silent
	   set (FFTW_FIND_QUIETLY TRUE)
	endif (FFTW_INCLUDE_DIRS)

	find_path (FFTW_INCLUDE_DIR fftw3.h
	           HINTS
                   ENV FFTW_ROOT
                   PATHS 
	           PATH_SUFFIXES
	           include
	           Include
	)

	list (APPEND FFTW_INCLUDE_DIRS ${FFTW_INCLUDE_DIR})
	find_library (FFTW_LIBRARIES NAMES fftw3f
                      HINTS ENV FFTW_ROOT
                      PATHS
                      PATH_SUFFIXES lib64 Lib64 lib Lib)

	message (STATUS ${FFTW_LIBRARIES} ${FFTW_INCLUDE_DIRS})
#	handle the QUIETLY and REQUIRED arguments and set FFTW_FOUND to TRUE if
#	all listed variables are TRUE

	include (FindPackageHandleStandardArgs)
	find_package_handle_standard_args (
	                  FFTW DEFAULT_MSG FFTW_LIBRARIES FFTW_INCLUDE_DIRS)
	mark_as_advanced (FFTW_LIBRARIES FFTW_INCLUDE_DIRS)

	Find_Package (PkgConfig)
	find_library (FAAD faad)
	if (NOT(FAAD))
	   message(FATAL_ERROR "please install libfaad")
	else (NOT(FAAD))
	set (extraLibs  ${extraLibs} ${FAAD})
	endif(NOT(FAAD))

	find_library (PORTAUDIO portaudio)
	if (NOT(PORTAUDIO))
	   message(FATAL_ERROR "please install portaudio V19")
	else(NOT(PORTAUDIO))
	   set (extraLibs  ${extraLibs} ${PORTAUDIO})
	endif(NOT(PORTAUDIO))

	SET (CMAKE_FIND_LIBRARY_SUFFIXES .so .a)

	find_library (USB1 usb-1.0 HINTS /usr/lib)
	if (NOT(USB1))
	   message (FATAL_ERROR "please install libusb-1.0")
	else(NOT(USB1))
	   set(extraLibs ${extraLibs} ${USB1})
	endif(NOT(USB1))

	find_library (SNDFILE sndfile)
	if (NOT(SNDFILE))
	   message (FATAL_ERROR "please install libsndfile")
	else (NOT(SNDFILE))
	   set (extraLibs ${extraLibs} ${SNDFILE})
	endif (NOT(SNDFILE))

	find_library (PTHREADS pthread)
	if (NOT(PTHREADS))
	   message (FATAL_ERROR "please install libpthread")
	else (NOT(PTHREADS))
	   set (extraLibs ${extraLibs} ${PTHREADS})
	endif (NOT(PTHREADS))
#######################################################################
#
#	Here we really start
	set ( ${objectName}_UIS ./sdrgui.ui)

	if (NO_SSE_SUPPORT) 
	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./src/backend/spiral-code/spiral-no-sse.c )
	else ()
	   add_definitions (-DSSE_AVAILABLE)
	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./src/backend/spiral-code/spiral-sse.c )
	endif()

	include_directories (
	           ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
	           ${QT5Widgets_INCLUDES}
	           ${QT_QTCORE_INCLUDE_DIR}
	           ${QT_QTGUI_INCLUDE_DIR}
	           .
	           ./
	           ./includes
	           ./includes/ofdm
	           ./includes/backend
	           ./includes/various
	           ./includes/output
	           ./src/input
	           ./src/input/rawfiles
	           ./src/input/wavfiles
	           /usr/include/
	)

	set (${objectName}_HDRS
	     ./gui.h
	     ./includes/dab-constants.h
	     ./includes/ofdm/ofdm-processor.h
	     ./includes/ofdm/phasereference.h
	     ./includes/ofdm/phasetable.h
	     ./includes/ofdm/freq-interleaver.h
	     ./includes/backend/viterbi.h
	     ./includes/backend/msc-handler.h
	     ./includes/backend/rscodec.h
	     ./includes/backend/deconvolve.h
	     ./includes/backend/firecode-checker.h
	     ./includes/backend/dab-concurrent.h
	     ./includes/backend/dab-processor.h
	     ./includes/backend/dab-virtual.h
	     ./includes/backend/charsets.h
	     ./includes/backend/audio/mp2processor.h
	     ./includes/backend/audio/mp4processor.h
	     ./includes/backend/audio/faad_decoder.h
	     ./includes/backend/data/pad-handler.h
	     ./includes/backend/data/msc-datagroup.h
	     ./includes/backend/data/mot-data.h
	     ./src/input/virtual-input.h
	     ./src/input/rawfiles/rawfiles.h
	     ./src/input/wavfiles/wavfiles.h
	     ./includes/output/fir-filters.h
	     ./includes/output/audiosink.h
	     ./includes/various/fft.h
	     ./includes/various/ringbuffer.h
	     ./includes/various/Xtan2.h
	)

	set (${objectName}_SRCS
	     ${${objectName}_SRCS}
	     ./main.cpp
	     ./gui.cpp
	     ./src/ofdm/ofdm-processor.cpp
	     ./src/ofdm/ofdm-decoder.cpp
	     ./src/ofdm/phasereference.cpp
	     ./src/ofdm/phasetable.cpp
	     ./src/ofdm/freq-interleaver.cpp
	     ./src/backend/viterbi.cpp
	     ./src/backend/fic-handler.cpp
	     ./src/backend/msc-handler.cpp
	     ./src/backend/deconvolve.cpp
	     ./src/backend/fib-processor.cpp
	     ./src/backend/rscodec.cpp
	     ./src/backend/firecode-checker.cpp
	     ./src/backend/dab-concurrent.cpp
	     ./src/backend/dab-virtual.cpp
	     ./src/backend/dab-processor.cpp
	     ./src/backend/protTables.cpp
	     ./src/backend/charsets.cpp
	     ./src/backend/audio/mp2processor.cpp
	     ./src/backend/audio/mp4processor.cpp
	     ./src/backend/data/pad-handler.cpp
	     ./src/backend/data/msc-datagroup.cpp
	     ./src/backend/data/mot-data.cpp
	     ./src/input/virtual-input.cpp
	     ./src/input/rawfiles/rawfiles.cpp
	     ./src/input/wavfiles/wavfiles.cpp
	     ./src/output/audiosink.cpp
	     ./src/output/fir-filters.cpp
	     ./src/various/fft.cpp
	     ./src/various/Xtan2.cpp
	)

	set (${objectName}_MOCS
	     ./gui.h
	     ./includes/ofdm/ofdm-processor.h
	     ./includes/ofdm/ofdm-decoder.h
	     ./includes/backend/fic-handler.h
	     ./includes/backend/fib-processor.h
	     ./includes/backend/mp2processor.h
	     ./includes/backend/mp4processor.h
	     ./includes/backend/pad-handler.h
	     ./src/input/rawfiles/rawfiles.h
	     ./src/input/wavfiles/wavfiles.h
	)

	if (SDRPLAY)
	   set (${objectName}_UIS
	        ${${objectName}_UIS}
	        ./src/input/sdrplay/sdrplay-widget.ui 
	   )

	   set (${objectName}_MOCS
	        ${${objectName}_MOCS}
	        ./src/input/sdrplay/sdrplay.h
	   )

	   include_directories (
	     ./src/input/sdrplay
	   )

	   set ($(objectName)_HDRS
	        ${${objectName}_HDRS}
	        ./src/input/sdrplay/sdrplay.h
	        ./src/input/sdrplay/sdrplay-worker.h
	        ./src/input/sdrplay/sdrplay-loader.h
	   )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./src/input/sdrplay/sdrplay.cpp
	        ./src/input/sdrplay/sdrplay-worker.cpp
	        ./src/input/sdrplay/sdrplay-loader.cpp
	   )

	   add_definitions (-DHAVE_SDRPLAY)
	endif (SDRPLAY)
 

	if (AIRSPY)
           find_library (AIRSPY_LIB airspy)
           if (NOT(AIRSPY_LIB))
              message (FATAL_ERROR "please install airspy library")
           else (NOT(AIRSPY_LIB))
              set (extraLibs ${extraLibs} ${AIRSPY_LIB})
           endif()
           find_path (AIRSPYLIB_INCLUDE_DIR
                      NAMES airspy.h
                      PATHS
                      /usr/local/include/
                      /usr/local/include/libairspy
           )
           include_directories (${AIRSPYLIB_INCLUDE_DIR})

           set (${objectName}_UIS
                ${${objectName}_UIS}
                ./src/input/airspy/airspy-widget.ui
           )
	   set (${objectName}_MOCS
                ${${objectName}_MOCS}
                ./src/input/airspy/airspy-handler.h
           )

           include_directories (
             ./src/input/airspy
           )

           set ($(objectName)_HDRS
                ${${objectName}_HDRS}
                ./src/input/airspy/airspy-handler.h
           )

           set (${objectName}_SRCS
                ${${objectName}_SRCS}
                ./src/input/airspy/airspy-handler.cpp
           )

           add_definitions (-DHAVE_AIRSPY)
        endif (AIRSPY)

	if (DABSTICK_OSMO)
	   find_library (RTLSDR rtlsdr)
	   if (NOT(RTLSDR))
	      message (FATAL_ERROR "please install librtlsdr")
	   else (NOT(RTLSDR))
	      set (extraLibs ${extraLibs} ${RTLSDR})
	   endif()
#we know now that rtl-sdr.so is available
	   set (${objectName}_UIS
	        ${${objectName}_UIS}
	        ./src/input/dabstick-osmo/dabstick-widget.ui
	   )

	   set (${objectName}_MOCS
	        ${${objectName}_MOCS}
	        ./src/input/dabstick-osmo/dongleselect.h
	        ./src/input/dabstick-osmo/dabstick.h
	   )

	   include_directories (
	        ./src/input/dabstick-osmo/
	   )

	   set (${objectName}_HDRS
	        ${${objectName}_HDRS}
	        ./src/input/dabstick-osmo/dabstick.h 
	        ./src/input/dabstick-osmo/dongleselect.h
	   )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./src/input/dabstick-osmo/dabstick.cpp
	        ./src/input/dabstick-osmo/dongleselect.cpp
	   )

	   add_definitions (-DHAVE_DABSTICK)
	endif()

	if (DABSTICK_NEW)
	   find_library (RTLSDR rtlsdr)
	   if (NOT(RTLSDR))
	      message (FATAL_ERROR "please install librtlsdr")
	   else (NOT(RTLSDR))
	      set (extraLibs ${extraLibs} ${RTLSDR})
	   endif()
#we know now that rtl-sdr.so is available
	   set (${objectName}_UIS
	        ${${objectName}_UIS}
	        ./src/input/dabstick-new/dabstick-widget.ui
	   )

	   set (${objectName}_MOCS
	        ${${objectName}_MOCS}
	        ./src/input/dabstick-new/dongleselect.h
	        ./src/input/dabstick-new/dabstick.h
	   )

	   include_directories (
	        ./src/input/dabstick-new/
	   )

	   set (${objectName}_HDRS
	        ${${objectName}_HDRS}
	        ./src/input/dabstick-new/dabstick.h 
	        ./src/input/dabstick-new/dongleselect.h
	   )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./src/input/dabstick-new/dabstick.cpp
	        ./src/input/dabstick-new/dongleselect.cpp
	   )

	   add_definitions (-DHAVE_DABSTICK)
	endif()

	if (DONGLE)
	   set (${objectName}_UIS
	        ${${objectName}_UIS}
	        ./src/input/mirics-dongle/dongle-widget.ui 
	   )

	   set (${objectName}_MOCS
	        ${${objectName}_MOCS}
	        ./src/input/mirics-dongle/mirics-dongle.h
	   )

	   include_directories (
	      ./src/input/mirics-dongle
	   )

	   set ($(objectName)_HDRS
	      ${${objectName}_HDRS}
	      ./src/input/mirics-dongle/mirics-dongle.h
	      ./src/input/mirics-dongle/mirics-worker.h
	      ./src/input/mirics-dongle/mirics-loader.h
	   )

	   set (${objectName}_SRCS
	      ${${objectName}_SRCS}
	      ./src/input/mirics-dongle/mirics-dongle.cpp
	      ./src/input/mirics-dongle/mirics-worker.cpp
	      ./src/input/mirics-dongle/mirics-loader.cpp
	   )

	   add_definitions (-DHAVE_DONGLE)
	endif (DONGLE)
#
#
	if (RTLTCP)
	   find_package (Qt5Network REQUIRED)
	   set (${objectName}_UIS
	        ${${objectName}_UIS}
	        ./src/input/rtl_tcp/rtl_tcp-widget.ui 
	   )

	   set (${objectName}_MOCS
	        ${${objectName}_MOCS}
	        ./src/input/rtl_tcp/rtl_tcp_client.h
	   )

	   include_directories (
	      ./src/input/rtl_tcp
	      ${Qt5Network_INCLUDE_DIRS}
	   )

	   set ($(objectName)_HDRS
	        ${${objectName}_HDRS}
	        ./src/input/rtl_tcp/rtl_tcp_client.h
	   )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./src/input/rtl_tcp/rtl_tcp_client.cpp
	   )

	   set (RTLTCP_lib Qt5::Network)
	   add_definitions (-DHAVE_RTL_TCP)
	endif (RTLTCP)

	QT5_WRAP_UI (UIS ${${objectName}_UIS}
	             ./src/input/filereader-widget.ui) 

	include_directories (
	          ${SDRPLAY_INCLUDES}
	          ${QT5Widgets_INCLUDES}
	          ${QT_QTCORE_INCLUDE_DIR}
	          ${QT_QTGUI_INCLUDE_DIR}
	          ${QWT_INCLUDE_DIRS}
	          ${FFTW_INCLUDE_DIRS}
	          ${PORTAUDIO_INCLUDE_DIRS}
	          ${FAAD_INCLUDE_DIRS}
	          ${SNDFILES_INCLUDE_DIRS}
	)

	QT5_WRAP_CPP (MOCS ${${objectName}_MOCS})

	add_executable (${objectName}
	                ${${objectName}_SRCS}
	                ${UIS}
	                ${RSCS}
	                ${TRS}
	                ${MOCS}
	)

	target_link_libraries (${objectName}
	                       Qt5::Widgets
	                       ${RTLTCP_lib}
	                       ${FFTW_LIBRARIES}
	                       ${extraLibs}
	                       ${FAAD_LIBRARIES}
	                       ${QWT_LIBRARIES}
	)

	INSTALL (TARGETS ${objectName} DESTINATION ./linux-bin)
