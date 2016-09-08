cmake_minimum_required( VERSION 2.8.11 )
set (objectName dab-rpi-0.997)
add_definitions ( -Wall)
#	modify if you want
set (CMAKE_INSTALL_PREFIX ../)

if(MINGW)
    add_definitions ( -municode)
endif()

########################################################################
# select the release build type by default to get optimization flags
########################################################################
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")

### make sure our local CMake Modules path comes first
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake/Modules)

######################################################################
#       R E A D T H I S
#####################################################################
#	adjust to your likings
#	if you want to use the SSE support on your machine, uncomment
#	but recall that MINGW does not support it (yet)
	set(NO_SSE_SUPPORT true)

#	select the GUI you want
	set(GUI_1 true)
#	set(GUI_2 true)

#	if you want support for any of these devices, uncomment the line
	set(SDRPLAY true)
	set(AIRSPY true)
	set(DABSTICK_OSMO true)
#	set(RTLTCP true)

#	add_definitions (-DMOT_BASICS__) 	# use at your own risk
#	add_definitions (-DMSC_DATA__)		# use at your own risk
#
#	If you want a tcp stream of the audio rather than
#	audio being sent to the soundcard, uncomment
#	set(TCP_STREAMER true)
#	set(RTP_STREAMER true)
########################################################################
	find_package (PkgConfig)

	find_package (Qt5Core REQUIRED)
	find_package (Qt5Widgets REQUIRED)
	find_package (Qt5Network REQUIRED)
	find_package (Qt5Declarative REQUIRED)

	include_directories (
	      ${Qt5Network_INCLUDE_DIRS}
	)

        find_package(FFTW3f)
        if (NOT FFTW3F_FOUND)
            message(FATAL_ERROR "please install FFTW3")
        endif ()

        find_package(Portaudio)
        if (NOT PORTAUDIO_FOUND)
            message(FATAL_ERROR "please install portaudio V19")
        endif ()
        list(APPEND extraLibs ${PORTAUDIO_LIBRARIES})

        find_package(Faad)
        if (NOT FAAD_FOUND )
            message(FATAL_ERROR "please install libfaad")
        endif ()

        find_package(zlib)
	if (NOT ZLIB_FOUND)
            message(FATAL_ERROR "please install libz")
        endif ()
	list(APPEND extraLibs ${ZLIB_LIBRARY})

        find_package(LibSndFile)
        if (NOT LIBSNDFILE_FOUND)
            message(FATAL_ERROR "please install libsndfile")
        endif ()
        list(APPEND extraLibs ${LIBSNDFILE_LIBRARY})

	find_library (PTHREADS pthread)
	if (NOT(PTHREADS))
	   message (FATAL_ERROR "please install libpthread")
	else (NOT(PTHREADS))
	   set (extraLibs ${extraLibs} ${PTHREADS})
	endif (NOT(PTHREADS))
#######################################################################
#
#	Here we really start

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
	           ./includes/backend/audio
	           ./includes/backend/data
	           ./includes/backend/data/journaline
	           ./includes/various
	           ./includes/output
	           ./src/input
	           ./src/input/rawfiles
	           ./src/input/wavfiles
	           /usr/include/
	)

	set (${objectName}_HDRS
	     ./includes/dab-constants.h
	     ./includes/ofdm/ofdm-processor.h
	     ./includes/ofdm/phasereference.h
	     ./includes/ofdm/phasetable.h
	     ./includes/ofdm/freq-interleaver.h
	     ./includes/backend/viterbi.h
	     ./includes/backend/msc-handler.h
	     ./includes/backend/deconvolve.h
	     ./includes/backend/firecode-checker.h
	     ./includes/backend/dab-processor.h
	     ./includes/backend/dab-virtual.h
	     ./includes/backend/charsets.h
	     ./includes/backend/galois.h
	     ./incluces/backend/reed-solomon.h
	     ./includes/backend/audio/dab-audio.h
	     ./includes/backend/audio/faad-decoder.h
	     ./includes/backend/audio/mp4processor.h 
	     ./includes/backend/audio/mp2processor.h 
	     ./includes/backend/data/ip-datahandler.h 
	     ./includes/backend/data/journaline-datahandler.h 
	     ./includes/backend/data/journaline/dabdatagroupdecoder.h
	     ./includes/backend/data/journaline/crc_8_16.h 
	     ./includes/backend/data/journaline/log.h 
	     ./includes/backend/data/journaline/newssvcdec_impl.h 
	     ./includes/backend/data/journaline/Splitter.h 
	     ./includes/backend/data/journaline/dabdgdec_impl.h 
	     ./includes/backend/data/journaline/newsobject.h 
	     ./includes/backend/data/journaline/NML.h 
	     ./includes/backend/data/mot-databuilder.h 
	     ./includes/backend/data/virtual-datahandler.h 
	     ./includes/backend/data/pad-handler.h 
	     ./includes/backend/data/mot-data.h 
	     ./includes/backend/data/msc-datagroup.h 
	     ./src/input/virtual-input.h
	     ./src/input/rawfiles/rawfiles.h
	     ./src/input/wavfiles/wavfiles.h
	     ./includes/output/fir-filters.h
	     ./includes/output/audio-base.h
	     ./includes/output/audiosink.h
	     ./includes/various/fft.h
	     ./includes/various/ringbuffer.h
	     ./includes/various/Xtan2.h
	)

	set (${objectName}_SRCS
	     ${${objectName}_SRCS}
	     ./main.cpp
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
	     ./src/backend/firecode-checker.cpp
	     ./src/backend/dab-virtual.cpp
	     ./src/backend/dab-processor.cpp
	     ./src/backend/protTables.cpp
	     ./src/backend/charsets.cpp
	     ./src/backend/dab-virtual.cpp 
	     ./src/backend/galois.cpp
	     ./src/backend/reed-solomon.cpp
	     ./src/backend/audio/dab-audio.cpp
	     ./src/backend/audio/mp4processor.cpp 
	     ./src/backend/audio/mp2processor.cpp 
	     ./src/backend/data/ip-datahandler.cpp 
	     ./src/backend/data/journaline-datahandler.cpp 
	     ./src/backend/data/journaline/crc_8_16.c 
	     ./src/backend/data/journaline/log.c 
	     ./src/backend/data/journaline/newssvcdec_impl.cpp 
	     ./src/backend/data/journaline/Splitter.cpp 
	     ./src/backend/data/journaline/dabdgdec_impl.c 
	     ./src/backend/data/journaline/newsobject.cpp 
	     ./src/backend/data/journaline/NML.cpp 
	     ./src/backend/data/mot-databuilder.cpp 
	     ./src/backend/data/virtual-datahandler.cpp 
	     ./src/backend/data/pad-handler.cpp 
	     ./src/backend/data/mot-data.cpp 
	     ./src/backend/data/msc-datagroup.cpp 
	     ./src/input/virtual-input.cpp
	     ./src/input/rawfiles/rawfiles.cpp
	     ./src/input/wavfiles/wavfiles.cpp
	     ./src/output/audio-base.cpp
	     ./src/output/audiosink.cpp
	     ./src/output/fir-filters.cpp
	     ./src/various/fft.cpp
	     ./src/various/Xtan2.cpp
	)

	set (${objectName}_MOCS
	     ./includes/output/audio-base.h
	     ./includes/output/audiosink.h
	     ./includes/ofdm/ofdm-processor.h
	     ./includes/ofdm/ofdm-decoder.h
	     ./includes/backend/fic-handler.h
	     ./includes/backend/fib-processor.h
	     ./includes/backend/audio/faad-decoder.h
	     ./includes/backend/audio/mp2processor.h
	     ./includes/backend/audio/mp4processor.h
	     ./includes/backend/data/virtual-datahandler.h
	     ./includes/backend/data/pad-handler.h
	     ./includes/backend/data/mot-data.h
	     ./includes/backend/data/ip-datahandler.h
	     ./includes/backend/data/journaline-datahandler.h
	     ./includes/backend/data/mot-databuilder.h
	     ./includes/backend/data/msc-datagroup.h
	     ./src/input/rawfiles/rawfiles.h
	     ./src/input/wavfiles/wavfiles.h
	)

	if (GUI_1)
	   include_directories ( ./gui_1)

	   set (${objectName}_UIS
	        ${${objectName}_UIS} ./gui_1/gui_1.ui)

	   set (${objectName}_MOCS
	        ${${objectName}_MOCS} ./gui_1/gui.h)

	   set ($(objectName)_HDRS
	        ${${objectName}_HDRS} ./gui_1/gui.h)

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS} ./gui_1/gui.cpp)
	
	   add_definitions (-DGUI_1)
	endif (GUI_1)

	if (GUI_2)
	   include_directories ( ./gui_2)

	   set (${objectName}_UIS
	        ${${objectName}_UIS} ./gui_2/gui_2.ui)

	   set (${objectName}_MOCS
	        ${${objectName}_MOCS} ./gui_2/gui.h)

	   set ($(objectName)_HDRS
	        ${${objectName}_HDRS} ./gui_2/gui.h)

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS} ./gui_2/gui.cpp)
	
	   add_definitions (-DGUI_2)
	endif (GUI_2)

	if (SDRPLAY)
	   find_path (SDRPLAYLIB_INCLUDE_DIR
                      NAMES mirsdrapi-rsp.h
                      PATHS
                      /usr/local/include/
	   )
           include_directories (${SDRPLAYLIB_INCLUDE_DIR})
	   set (${objectName}_UIS
	        ${${objectName}_UIS}
	        ./src/input/sdrplay/sdrplay-widget.ui 
	   )

	   set (${objectName}_MOCS
	        ${${objectName}_MOCS}
	        ./src/input/sdrplay/sdrplay.h
	        ./src/input/sdrplay/sdrplay-worker.h
	   )

	   include_directories (
	     ./src/input/sdrplay
	   )

	   set ($(objectName)_HDRS
	        ${${objectName}_HDRS}
	        ./src/input/sdrplay/sdrplay.h
	        ./src/input/sdrplay/sdrplay-loader.h
                ./src/input/sdrplay/sdrplay-worker.h
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
           find_package(LibAIRSPY)
           if (NOT LIBAIRSPY_FOUND)
               message(FATAL_ERROR "please install airspy library")
           endif ()
	   ### include_directories (${AIRSPYLIB_INCLUDE_DIR})

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

#
	if (DABSTICK_OSMO)
           find_package(LibRTLSDR)
           if (NOT LIBRTLSDR_FOUND)
               message(FATAL_ERROR "please install librtlsdr")
           endif ()
	   ###include_directories (${RTLSDR_INCLUDE_DIR})

	   set (${objectName}_UIS
	        ${${objectName}_UIS}
	        ./src/input/dabstick-osmo/dabstick-widget-osmo.ui
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
#
	if (TCP_STREAMER)
	   add_definitions (-DTCP_STREAMER)
	   find_package (Qt5Network REQUIRED)
	   set (${objectName}_MOCS
	        ${${objectName}_MOCS}
	        ./includes/output/tcp-streamer.h
	   )
	   set (${objectName}_HDRS
	        ${${objectName}_HDRS}
	        ./includes/output/tcp-streamer.h
	   )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./src/output/tcp-streamer.cpp
	   )
	endif (TCP_STREAMER)

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
	                       Qt5::Network
	                       ${RTLTCP_lib}
	                       ${FFTW3F_LIBRARIES}
	                       ${extraLibs}
	                       ${FAAD_LIBRARIES}
	                       ${QWT_LIBRARIES}
	                       ${CMAKE_DL_LIBS}
	)

	INSTALL (TARGETS ${objectName} DESTINATION ./linux-bin)

########################################################################
# Create uninstall target
########################################################################

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

