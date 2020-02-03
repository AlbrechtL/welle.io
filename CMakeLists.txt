cmake_minimum_required(VERSION 3.2)

project(Welle.Io VERSION 0.0.0 LANGUAGES C CXX)

if(NOT WELLE-IO_VERSION)
  set(WELLE-IO_VERSION ${PROJECT_VERSION})
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)

option(BUILD_WELLE_IO    "Build Welle.io"                        ON  )
option(BUILD_WELLE_CLI   "Build welle-cli"                       ON  )
option(WITH_APP_BUNDLE   "Enable Application Bundle for macOS"   ON  )
option(KISS_FFT          "KISS FFT instead of FFTW"              OFF )
option(PROFILING         "Enable profiling (see README.md)"      OFF )
option(AIRSPY            "Compile with Airspy support"           OFF )
option(RTLSDR            "Compile with RTL-SDR support"          OFF )
option(SOAPYSDR          "Compile with SoapySDR support"         OFF )

add_definitions(-Wall)
add_definitions(-g)
add_definitions(-DDABLIN_AAC_FAAD2)

if(MINGW)
    add_definitions(-municode)
endif()

if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")

#enable_testing()

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake/Modules)

if(APPLE AND WITH_APP_BUNDLE)
    if(NOT DEFINED BUNDLE_INSTALL_DIR)
        set(BUNDLE_INSTALL_DIR "/Applications")
    endif()

    set(GUI_INSTALL_DIR "${BUNDLE_INSTALL_DIR}")
else()
    include(GNUInstallDirs)

    set(GUI_INSTALL_DIR "${CMAKE_INSTALL_BINDIR}")
endif()

if(BUILD_WELLE_IO)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)

    find_package(Qt5 COMPONENTS Widgets Quick QuickControls2 Multimedia Charts Qml REQUIRED)
endif()

if(PROFILING)
    add_definitions(-DWITH_PROFILING)
endif()

find_package(ALSA)
if(ALSA_FOUND)
    add_definitions(-DHAVE_ALSA)
endif()

if(KISS_FFT)
    add_definitions(-DKISSFFT)
    set(fft_sources src/libs/kiss_fft/kiss_fft.c)
    set(KISS_INCLUDE_DIRS src/libs/kiss_fft)
else()
    find_package(FFTW3f REQUIRED)
    set(fft_sources "")
    set(KISS_INCLUDE_DIRS "")
endif()

if(BUILD_WELLE_CLI)
    find_package(Lame REQUIRED)
endif()

find_package(Threads REQUIRED)
find_package(Faad REQUIRED)
find_package(MPG123 REQUIRED)

if (RTLSDR)
    find_package(LibRTLSDR REQUIRED)
endif()

if (AIRSPY)
    find_package(LibAIRSPY REQUIRED)
endif()

if (SOAPYSDR)
  find_package(SoapySDR NO_MODULE REQUIRED)
  # Note: SoapySDRConfig.cmake sets C++11 standard so it needs to be reset to C++14
  set(CMAKE_CXX_STANDARD 14)
endif()

include_directories(
    src
    src/backend
    src/output
    src/various
    src/input
    src/gui
    src/libs/fec
    ${FFTW3F_INCLUDE_DIRS}
    ${KISS_INCLUDE_DIRS}
    ${FAAD_INCLUDE_DIRS}
    ${LIBRTLSDR_INCLUDE_DIRS}
    ${SoapySDR_INCLUDE_DIRS}
)

set(backend_sources
    src/backend/dab-audio.cpp
    src/backend/decoder_adapter.cpp
    src/backend/dab_decoder.cpp
    src/backend/dabplus_decoder.cpp
    src/backend/charsets.cpp
    src/backend/dab-constants.cpp
    src/backend/mot_manager.cpp
    src/backend/pad_decoder.cpp
    src/backend/eep-protection.cpp
    src/backend/fib-processor.cpp
    src/backend/fic-handler.cpp
    src/backend/msc-handler.cpp
    src/backend/freq-interleaver.cpp
    src/backend/ofdm-decoder.cpp
    src/backend/ofdm-processor.cpp
    src/backend/phasereference.cpp
    src/backend/phasetable.cpp
    src/backend/tii-decoder.cpp
    src/backend/protTables.cpp
    src/backend/radio-receiver.cpp
    src/backend/tools.cpp
    src/backend/uep-protection.cpp
    src/backend/viterbi.cpp
    src/various/Socket.cpp
    src/various/Xtan2.cpp
    src/various/channels.cpp
    src/various/fft.cpp
    src/various/profiling.cpp
    src/various/wavfile.c
    src/libs/fec/decode_rs_char.c
    src/libs/fec/encode_rs_char.c
    src/libs/fec/init_rs_char.c
)

set(welle_io_sources
    src/welle-gui/main.cpp
    src/welle-gui/audio_output.cpp
    src/welle-gui/mot_image_provider.cpp
    src/welle-gui/gui_helper.cpp
    src/welle-gui/radio_controller.cpp
    src/welle-gui/debug_output.cpp
    src/welle-gui/waterfallitem.cpp
)

set(welle_cli_sources
    src/welle-cli/welle-cli.cpp
    src/welle-cli/alsa-output.cpp
    src/welle-cli/webradiointerface.cpp
    src/welle-cli/jsonconvert.cpp
    src/welle-cli/webprogrammehandler.cpp
    src/welle-cli/tests.cpp
)

set(input_sources
    src/input/input_factory.cpp
    src/input/null_device.cpp
    src/input/raw_file.cpp
    src/input/rtl_tcp.cpp
)

if(LIBRTLSDR_FOUND)
    add_definitions (-DHAVE_RTLSDR)
    set(input_sources  ${input_sources} src/input/rtl_sdr.cpp)
endif()

if(LIBAIRSPY_FOUND)
    add_definitions (-DHAVE_AIRSPY)
    set(input_sources  ${input_sources} src/input/airspy_sdr.cpp)
endif()

if(SoapySDR_FOUND)
    add_definitions (-DHAVE_SOAPYSDR)
    set(input_sources  ${input_sources} src/input/soapy_sdr.cpp)
endif()

if(NOT GIT_COMMIT_HASH)
  execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()

if(GIT_COMMIT_HASH)
  add_definitions("-DGITHASH=\"${GIT_COMMIT_HASH}\"")
endif()

if(NOT GIT_DESCRIBE)
  execute_process(
    COMMAND git describe --tags --long
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_DESCRIBE
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()

if(GIT_DESCRIBE)
  add_definitions("-DGITDESCRIBE=\"${GIT_DESCRIBE}\"")
endif()

if(BUILD_WELLE_IO)
    set(executableName welle-io)
    add_executable (${executableName} ${welle_io_sources} ${backend_sources} ${input_sources} ${fft_sources} ${EXTRA_MOCS} src/welle-gui/resources.qrc)

    target_link_libraries (${executableName}
      ${LIBRTLSDR_LIBRARIES}
      ${LIBAIRSPY_LIBRARIES}
      ${FFTW3F_LIBRARIES}
      ${FAAD_LIBRARIES}
      ${SoapySDR_LIBRARIES}
      ${MPG123_LIBRARIES}
      Threads::Threads
      Qt5::Core Qt5::Widgets Qt5::Multimedia Qt5::Charts Qt5::Qml Qt5::Quick Qt5::QuickControls2
    )

    if(APPLE AND WITH_APP_BUNDLE)
        set(macOsIcon ${CMAKE_CURRENT_SOURCE_DIR}/src/welle-gui/icons/icon.icns)
        set_source_files_properties(${macOsIcon} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
        target_sources(${executableName} PRIVATE ${macOsIcon})

        set(SHORT_VERSION "${WELLE-IO_VERSION}")
        set(EXECUTABLE "${executableName}")
        set(TYPEINFO "io.welle.welle")
        set(PRODUCT_BUNDLE_IDENTIFIER "io.welle.welle")

        set_target_properties("${executableName}" PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/welle-io.plist
        )

        INSTALL (TARGETS ${executableName} BUNDLE DESTINATION ${GUI_INSTALL_DIR})
    else()
        INSTALL (TARGETS ${executableName} RUNTIME DESTINATION ${GUI_INSTALL_DIR})

        if(UNIX AND NOT APPLE)
            INSTALL (FILES ${PROJECT_SOURCE_DIR}/welle-io.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)

            INSTALL (FILES ${PROJECT_SOURCE_DIR}/src/welle-gui/icons/16x16/welle-io.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/16x16/apps)
            INSTALL (FILES ${PROJECT_SOURCE_DIR}/src/welle-gui/icons/24x24/welle-io.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/24x24/apps)
            INSTALL (FILES ${PROJECT_SOURCE_DIR}/src/welle-gui/icons/32x32/welle-io.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/32x32/apps)
            INSTALL (FILES ${PROJECT_SOURCE_DIR}/src/welle-gui/icons/48x48/welle-io.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/48x48/apps)
            INSTALL (FILES ${PROJECT_SOURCE_DIR}/src/welle-gui/icons/128x128/welle-io.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps)
            INSTALL (FILES ${PROJECT_SOURCE_DIR}/src/welle-gui/icons/256x256/welle-io.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/256x256/apps)
        endif()
    endif()
endif()

if(BUILD_WELLE_CLI)
    set(cliExecutableName welle-cli)
    add_executable (${cliExecutableName} ${welle_cli_sources} ${backend_sources} ${input_sources} ${fft_sources})

    if(CMAKE_BUILD_TYPE MATCHES Debug)
      SET_TARGET_PROPERTIES(${cliExecutableName} PROPERTIES COMPILE_FLAGS "-O2 -fno-omit-frame-pointer -fsanitize=address")
      SET_TARGET_PROPERTIES(${cliExecutableName} PROPERTIES LINK_FLAGS "-fno-omit-frame-pointer -fsanitize=address")
    endif(CMAKE_BUILD_TYPE MATCHES Debug)

    target_link_libraries (${cliExecutableName}
      ${LIBRTLSDR_LIBRARIES}
      ${LIBAIRSPY_LIBRARIES}
      ${FFTW3F_LIBRARIES}
      ${FAAD_LIBRARIES}
      ${ALSA_LIBRARIES}
      ${LAME_LIBRARIES}
      ${SoapySDR_LIBRARIES}
      ${MPG123_LIBRARIES}
      Threads::Threads
    )

    add_custom_command(
            TARGET ${cliExecutableName} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_SOURCE_DIR}/src/welle-cli/index.html
                    ${CMAKE_CURRENT_BINARY_DIR}/index.html)
    add_custom_command(
            TARGET ${cliExecutableName} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_SOURCE_DIR}/src/welle-cli/index.js
                    ${CMAKE_CURRENT_BINARY_DIR}/index.js)

    if(APPLE AND WITH_APP_BUNDLE)
        INSTALL (TARGETS ${cliExecutableName} RUNTIME DESTINATION ${GUI_INSTALL_DIR}/${executableName}.app/Contents/MacOS)
        INSTALL (
            FILES
                ${PROJECT_SOURCE_DIR}/src/welle-cli/index.html
                ${PROJECT_SOURCE_DIR}/src/welle-cli/index.js
            DESTINATION ${GUI_INSTALL_DIR}/${executableName}.app/Contents/Resources/welle-cli
        )
    elseif(UNIX)
        INSTALL (TARGETS ${cliExecutableName} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
        INSTALL (
            FILES
                ${PROJECT_SOURCE_DIR}/src/welle-cli/index.html
                ${PROJECT_SOURCE_DIR}/src/welle-cli/index.js
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/welle-io/html/
        )
    endif()
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
