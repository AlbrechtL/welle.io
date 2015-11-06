cmake_minimum_required( VERSION 2.6 )
#set ( CMAKE_BUILD_TYPE Release )
set (objectName device_remote)
ADD_DEFINITIONS (${QT_DEFINITIONS})
ADD_DEFINITIONS (-DQT_PLUGIN)
ADD_DEFINITIONS (-DQT_SHARED)
ADD_DEFINITIONS (-DQT_NO_DEBUG)
ADD_DEFINITIONS (-DQR_NETWORKING)
set (CMAKE_INSTALL_PREFIX ./../../../../../../../)
add_definitions ( -Wall )
find_package (Qt4 COMPONENTS QtCore QtNetwork REQUIRED)
find_library (DYNAMIC_LOAD dl)
if(NOT(DYNAMIC_LOAD))
message(FATAL_ERROR "please install -ldl")
else(NOT(DYNAMIC_LOAD))
set(extraLibs ${DYNAMIC_LOAD})
endif(NOT(DYNAMIC_LOAD))

if (QWT_INCLUDE_DIRS)
  # Already in cache, be silent
  set (QWT_FIND_QUIETLY TRUE)
endif (QWT_INCLUDE_DIRS)

find_path (QWT_INCLUDE_DIR
	NAMES qwt_plot.h
	PATHS
	/usr/local/include/qwt-qt4
	/usr/local/include/qwt
	/usr/local/include/qwt6
	/usr/include/qwt-qt4
	/usr/include/qwt6
	/usr/include/qwt
)
list( APPEND QWT_INCLUDE_DIRS ${QWT_INCLUDE_DIR} )
find_library (QWT_LIBRARIES
	NAMES qwt-qt4 qwt qwt6
	PATHS /usr/local/lib /usr/lib
)
message (STATUS ${QWT_LIBRARIES} ${QWT_INCLUDE_DIRS})
# handle the QUIETLY and REQUIRED arguments and set QWT_FOUND to TRUE if 
# all listed variables are TRUE
include ( FindPackageHandleStandardArgs )
find_package_handle_standard_args (Qwt DEFAULT_MSG QWT_LIBRARIES QWT_INCLUDE_DIRS )
MARK_AS_ADVANCED (QWT_LIBRARIES QWT_INCLUDE_DIRS)
#
#  FFTW_INCLUDES    - where to find fftw3f.h
#  FFTW_LIBRARIES   - List of libraries when using FFTW.
#  FFTW_FOUND       - True if FFTW found.

if (FFTW_INCLUDE_DIRS)
  # Already in cache, be silent
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
list( APPEND FFTW_INCLUDE_DIRS ${FFTW_INCLUDE_DIR} )
find_library (FFTW_LIBRARIES NAMES fftw3f
              HINTS ENV FFTW_ROOT
              PATHS
              PATH_SUFFIXES lib Lib)

message (STATUS ${FFTW_LIBRARIES} ${FFTW_INCLUDE_DIRS})
# handle the QUIETLY and REQUIRED arguments and set FFTW_FOUND to TRUE if
# all listed variables are TRUE

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (FFTW DEFAULT_MSG FFTW_LIBRARIES FFTW_INCLUDE_DIRS)
mark_as_advanced (FFTW_LIBRARIES FFTW_INCLUDE_DIRS)

find_library (PTHREADS pthread)
if(NOT(PTHREADS))
message (FATAL_ERROR "please install libpthread")
else(NOT(PTHREADS))
set (extraLibs ${extraLibs} ${PTHREADS})
endif(NOT(PTHREADS))

include ( ${QT_USE_FILE} )
include_directories (
	${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
	${QT_QTCORE_INCLUDE_DIR} ${QT_QTGUI_INCLUDE_DIR}
	.
	../
	../.. 
	../decoder-utils 
	../../radio-utils 
	../../../includes 
	../../../includes/filters 
	../../../includes/various 
	../../../includes/output
)

set ( ${objectName}_HDRS
	../rig-interface.h 
	./remote.h 
	../../../includes/filters/fir-filters.h 
	../../../includes/filters/iir-filters.h 
	../../../includes/various/sincos.h 
	../../../includes/various/utilities.h 
	../../../includes/swradio-constants.h
	)

set ( ${objectName}_SRCS
	./remote.cpp 
	../../../src/filters/fir-filters.cpp 
	../../../src/filters/iir-filters.cpp 
	../../../src/various/sincos.cpp 
	../../../src/various/utilities.cpp 
	)

set (${objectName}_UIS ./widget.ui)
QT4_WRAP_UI(UIS ${${objectName}_UIS})

set ( ${objectName}_MOCS
	../rig-interface.h 
	./remote.h
	)
include_directories (${QWT_INCLUDE_DIRS} ${FFTW_INCLUDE_DIRS} ${SAMPLERATE_INCLUDE_DIRS} ${PORTAUDIO_INCLUDE_DIRS} ${SNDFILES_INCLUDE_DIRS})
QT4_WRAP_CPP(MOCS ${${objectName}_MOCS})
add_library ( ${objectName} SHARED ${${objectName}_SRCS} ${UIS} ${RSCS} ${TRS} ${MOCS} )
target_link_libraries (${objectName}  ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${FFTW_LIBRARIES} ${extraLibs} ${SAMPLERATE_LIBRARIES} ${QWT_LIBRARIES})

INSTALL(TARGETS ${objectName} LIBRARY DESTINATION ./linux-bin/input-plugins-sw)
