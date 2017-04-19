cmake_minimum_required(VERSION 3.2)

set(executableName welle-io)

project(Welle.Io VERSION 0.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)

add_definitions(-Wall)

if(MINGW)
    add_definitions ( -municode)
endif()

if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")

enable_testing()

include(GNUInstallDirs)

set(CMAKE_AUTOMOC ON)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake/Modules)

find_package(Threads REQUIRED)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Qml REQUIRED)
find_package(Qt5Multimedia REQUIRED)
find_package(Qt5Charts REQUIRED)
find_package(Qt5Quick REQUIRED)

find_package(FFTW3f REQUIRED)
find_package(Faad REQUIRED)

include_directories(
  src
  src/ofdm
  src/backend
  src/backend/audio
  src/backend/data
  src/output
  src/various
  src/input
  src/gui
)

set(sources
  src/main.cpp
  src/ofdm/ofdm-processor.cpp
  src/ofdm/ofdm-decoder.cpp
  src/ofdm/phasereference.cpp
  src/ofdm/phasetable.cpp
  src/ofdm/freq-interleaver.cpp
  src/backend/viterbi.cpp
  src/backend/fic-handler.cpp
  src/backend/msc-handler.cpp
  src/backend/protection.cpp
  src/backend/eep-protection.cpp
  src/backend/uep-protection.cpp
  src/backend/fib-processor.cpp
  src/backend/galois.cpp
  src/backend/reed-solomon.cpp
  src/backend/charsets.cpp
  src/backend/firecode-checker.cpp
  src/backend/dab-virtual.cpp
  src/backend/dab-processor.cpp
  src/backend/protTables.cpp
  src/backend/audio/dab-audio.cpp
  src/backend/audio/mp2processor.cpp
  src/backend/audio/mp4processor.cpp
  src/backend/data/pad_decoder.cpp
  src/backend/data/mot_manager.cpp
  src/backend/data/pad_decoder_adapter.cpp
  src/backend/tools.cpp
  src/output/CAudio.cpp
  src/various/fft.cpp
  src/various/Xtan2.cpp
  src/input/CInputFactory.cpp
  src/input/CNullDevice.cpp
  src/input/CRAWFile.cpp
  src/input/CRTL_TCP_Client.cpp
  src/DabConstants.cpp
  src/gui/CMOTImageProvider.cpp
  src/gui/CStationList.cpp
  src/gui/CGUI.cpp
  src/CRadioController.cpp
  src/CChannels.cpp
)

add_executable (${executableName}
  ${sources}
)

target_link_libraries (${executableName}
  Qt5::Widgets
  Qt5::Qml
  Qt5::Multimedia
  Qt5::Charts
  Qt5::Quick
  ${FFTW3F_LIBRARIES}
  ${FAAD_LIBRARIES}
  Threads::Threads
)

INSTALL (TARGETS ${executableName} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
