cmake_minimum_required(VERSION 3.2)

project(Welle.Io LANGUAGES C CXX)

file(READ "src/welle-gui/_current_version" VERSION_FILE)
string(STRIP ${VERSION_FILE} CURRENT_VERSION)
set(PROJECT_VERSION ${CURRENT_VERSION})

message ("Building project '${PROJECT_NAME}' version '${PROJECT_VERSION}'...")

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
option(FLAC              "Compile with flac support for streaming" OFF )

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
    find_package(Qt6 COMPONENTS Widgets Quick QuickControls2 Multimedia Charts Qml REQUIRED)
    set(CMAKE_AUTOMOC ON)
    if(ANDROID AND Qt6Core_VERSION VERSION_LESS 6.8.0)
        # Woraround for QTBUG-106466
        set(CMAKE_AUTORCC ON)
    endif()
endif()

if(PROFILING)
    add_definitions(-DWITH_PROFILING)
endif()

find_package(ALSA)
if(ALSA_FOUND)
    add_definitions(-DHAVE_ALSA)
endif()


if(BUILD_WELLE_CLI AND NOT ANDROID)
    find_package(Lame REQUIRED)
    if (FLAC)
    find_package(FLACPP REQUIRED) # test if FLAC is installed on the system
    add_definitions(-DHAVE_FLAC)
    endif()
endif()

find_package(Threads REQUIRED)

if(NOT ANDROID)
    if(KISS_FFT)
        add_definitions(-DKISSFFT)
        set(fft_sources src/libs/kiss_fft/kiss_fft.c)
        set(KISS_INCLUDE_DIRS src/libs/kiss_fft)
    else()
        find_package(FFTW3f REQUIRED)
        set(fft_sources "")
        set(KISS_INCLUDE_DIRS "")
    endif()
    find_package(Faad REQUIRED)
    find_package(MPG123 REQUIRED)
else()
    # For KISSFFT
    add_definitions(-DKISSFFT)
    include_directories(
        src/libs/kiss_fft
    )
    set(fft_sources
        src/libs/kiss_fft/kiss_fft.c
    )

    # For MPG123
    add_definitions(-DOPT_GENERIC)
    include_directories(
        src/libs/mpg123
    )
    set(mpg123_sources
        src/libs/mpg123/compat.c
        src/libs/mpg123/compat_str.c
        src/libs/mpg123/parse.c
        src/libs/mpg123/frame.c
        src/libs/mpg123/format.c
        src/libs/mpg123/dct64.c
        src/libs/mpg123/equalizer.c
        src/libs/mpg123/id3.c
        src/libs/mpg123/icy.c
        src/libs/mpg123/icy2utf8.c
        src/libs/mpg123/optimize.c
        src/libs/mpg123/readers.c
        src/libs/mpg123/tabinit.c
        src/libs/mpg123/libmpg123.c
        src/libs/mpg123/index.c
        src/libs/mpg123/layer1.c
        src/libs/mpg123/layer2.c
        src/libs/mpg123/layer3.c
        src/libs/mpg123/dither.c
        src/libs/mpg123/feature.c
        src/libs/mpg123/synth.c
        src/libs/mpg123/synth_real.c
        src/libs/mpg123/ntom.c
        src/libs/mpg123/synth_8bit.c
        src/libs/mpg123/synth_s32.c
        src/libs/mpg123/stringbuf.c
    )

    # For FAAD
    add_definitions(-DHAVE_CONFIG_H)
    include_directories(
        src/libs/faad2
        src/libs/faad2/include
        src/libs/faad2/libfaad
        src/libs/faad2/libfaad/codebook
    )
    set(faad_sources
        src/libs/faad2/libfaad/bits.c
        src/libs/faad2/libfaad/cfft.c
        src/libs/faad2/libfaad/common.c
        src/libs/faad2/libfaad/decoder.c
        src/libs/faad2/libfaad/drc.c
        src/libs/faad2/libfaad/drm_dec.c
        src/libs/faad2/libfaad/error.c
        src/libs/faad2/libfaad/filtbank.c
        src/libs/faad2/libfaad/hcr.c
        src/libs/faad2/libfaad/huffman.c
        src/libs/faad2/libfaad/ic_predict.c
        src/libs/faad2/libfaad/is.c
        src/libs/faad2/libfaad/lt_predict.c
        src/libs/faad2/libfaad/mdct.c
        src/libs/faad2/libfaad/mp4.c
        src/libs/faad2/libfaad/ms.c
        src/libs/faad2/libfaad/output.c
        src/libs/faad2/libfaad/pns.c
        src/libs/faad2/libfaad/ps_dec.c
        src/libs/faad2/libfaad/ps_syntax.c
        src/libs/faad2/libfaad/pulse.c
        src/libs/faad2/libfaad/rvlc.c
        src/libs/faad2/libfaad/sbr_dct.c
        src/libs/faad2/libfaad/sbr_dec.c
        src/libs/faad2/libfaad/sbr_e_nf.c
        src/libs/faad2/libfaad/sbr_fbt.c
        src/libs/faad2/libfaad/sbr_hfadj.c
        src/libs/faad2/libfaad/sbr_hfgen.c
        src/libs/faad2/libfaad/sbr_huff.c
        src/libs/faad2/libfaad/sbr_qmf.c
        src/libs/faad2/libfaad/sbr_syntax.c
        src/libs/faad2/libfaad/sbr_tf_grid.c
        src/libs/faad2/libfaad/specrec.c
        src/libs/faad2/libfaad/ssr.c
        src/libs/faad2/libfaad/ssr_fb.c
        src/libs/faad2/libfaad/ssr_ipqf.c
        src/libs/faad2/libfaad/syntax.c
        src/libs/faad2/libfaad/tns.c
    )
endif()

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
    ${CMAKE_CURRENT_BINARY_DIR}
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
    ${FLACPP_INCLUDE_DIRS}
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

if(Qt6DBus_FOUND)
    list(APPEND welle_io_sources
        src/welle-gui/mpris/mpris.cpp
        src/welle-gui/mpris/mpris_mp2.cpp
        src/welle-gui/mpris/mpris_mp2_player.cpp
    )
endif()

if(ANDROID)
    include_directories(
        src/welle-gui
    )
    list(APPEND welle_io_sources
        src/welle-gui/android_rtl_sdr.cpp
    )
endif()

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

add_definitions("-DCURRENT_VERSION=\"${CURRENT_VERSION}\"")

STRING(TIMESTAMP BUILD_DATE "%s" UTC)
add_definitions("-DBUILD_DATE=\"${BUILD_DATE}\"")

if(BUILD_WELLE_IO)
    set(executableName welle-io)

    if(CMAKE_AUTORCC)
        qt_add_executable (${executableName} MANUAL_FINALIZATION ${welle_io_sources} ${backend_sources} ${input_sources} ${fft_sources} ${mpg123_sources} ${faad_sources} ${EXTRA_MOCS} src/welle-gui/resources.qrc)
    else()
        qt_add_resources(welle_io_sources src/welle-gui/resources.qrc)
        qt_add_executable (${executableName} MANUAL_FINALIZATION ${welle_io_sources} ${backend_sources} ${input_sources} ${fft_sources} ${mpg123_sources} ${faad_sources} ${EXTRA_MOCS})
    endif()

    if(ANDROID)
        # Compute version code from CURRENT_VERSION value
        string(REGEX MATCH
          "^([0-9]+)\.([0-9]+)\.?([0-9]*)([^0-9]*)"
          CVC_MATCH
          "${CURRENT_VERSION}"
          )

        if(NOT CMAKE_MATCH_3 STREQUAL "")
          math(EXPR CURRENT_VERSION_CODE "${CMAKE_MATCH_1} * 100 + ${CMAKE_MATCH_2} * 10 + ${CMAKE_MATCH_3}")
        else()
          math(EXPR CURRENT_VERSION_CODE "${CMAKE_MATCH_1} * 100 + ${CMAKE_MATCH_2} * 10")
        endif()

        if(NOT CMAKE_MATCH_4 STREQUAL "")
          math(EXPR CURRENT_VERSION_CODE "${CURRENT_VERSION_CODE} - 1")
        endif()

        message (VERBOSE "CURRENT_VERSION_CODE: '${CURRENT_VERSION_CODE}'")

        if(CMAKE_AUTORCC)
            set_property(TARGET ${executableName} PROPERTY AUTORCC_OPTIONS "--no-zstd")
        endif()

        set_property(TARGET ${executableName} PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/welle-gui/android)
        set_property(TARGET ${executableName} PROPERTY QT_ANDROID_VERSION_NAME ${CURRENT_VERSION})
        set_property(TARGET ${executableName} PROPERTY QT_ANDROID_VERSION_CODE ${CURRENT_VERSION_CODE})

        # Additional requirements when using Android
        target_link_libraries (${executableName} PRIVATE
            Qt6::CorePrivate
        )

        # Additional components to link to, because otherwise, the builder reports errors like this one:
        #   Skipping /usr/lib/qt-android-6.3-armeabi-v7a/plugins/platforminputcontexts/libplugins_platforminputcontexts_qtvirtualkeyboardplugin_armeabi-v7a.so. It has unmet dependencies: lib/libQt6VirtualKeyboard_armeabi-v7a.so.
        find_package(Qt6 COMPONENTS Svg REQUIRED)
        target_link_libraries (${executableName} PRIVATE
            Qt6::Svg
        )
        find_package(Qt6 COMPONENTS VirtualKeyboard)
        if(Qt6VirtualKeyboard_FOUND)
            target_link_libraries (${executableName} PRIVATE
                Qt6::VirtualKeyboard
            )
        endif()
        if(Qt6Core_VERSION_MAJOR EQUAL 6 AND Qt6Core_VERSION_MINOR GREATER_EQUAL 3)
            find_package(Qt6 COMPONENTS Quick3DUtils REQUIRED)
            target_link_libraries (${executableName} PRIVATE
                Qt6::Quick3DUtils
            )
        endif()
    else()
        # Qt6::DBus should not be used for Android
        find_package(Qt6 COMPONENTS DBus REQUIRED)
        target_link_libraries (${executableName} PRIVATE Qt6::DBus)
    endif()

    target_link_libraries (${executableName} PRIVATE
      ${LIBRTLSDR_LIBRARIES}
      ${LIBAIRSPY_LIBRARIES}
      ${FFTW3F_LIBRARIES}
      ${FAAD_LIBRARIES}
      ${SoapySDR_LIBRARIES}
      ${MPG123_LIBRARIES}
      Threads::Threads
      Qt6::Core Qt6::Widgets Qt6::Multimedia Qt6::Charts Qt6::Qml Qt6::Quick Qt6::QuickControls2
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
    elseif(NOT ANDROID AND NOT WIN32)
        INSTALL (TARGETS ${executableName} RUNTIME DESTINATION ${GUI_INSTALL_DIR})
        INSTALL (FILES src/welle-gui/doc/man/welle-io.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1/)

        if(UNIX AND NOT APPLE)
            INSTALL (FILES ${PROJECT_SOURCE_DIR}/io.welle.welle-gui.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
            INSTALL (FILES ${PROJECT_SOURCE_DIR}/io.welle.welle-gui.appdata.xml DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo)

            INSTALL (FILES ${PROJECT_SOURCE_DIR}/src/welle-gui/icons/16x16/welle-io.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/16x16/apps RENAME io.welle.welle-gui.png)
            INSTALL (FILES ${PROJECT_SOURCE_DIR}/src/welle-gui/icons/24x24/welle-io.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/24x24/apps RENAME io.welle.welle-gui.png)
            INSTALL (FILES ${PROJECT_SOURCE_DIR}/src/welle-gui/icons/32x32/welle-io.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/32x32/apps RENAME io.welle.welle-gui.png)
            INSTALL (FILES ${PROJECT_SOURCE_DIR}/src/welle-gui/icons/48x48/welle-io.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/48x48/apps RENAME io.welle.welle-gui.png)
            INSTALL (FILES ${PROJECT_SOURCE_DIR}/src/welle-gui/icons/128x128/welle-io.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps RENAME io.welle.welle-gui.png)
            INSTALL (FILES ${PROJECT_SOURCE_DIR}/src/welle-gui/icons/256x256/welle-io.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/256x256/apps RENAME io.welle.welle-gui.png)
        endif()
    endif()
endif()

if(BUILD_WELLE_CLI AND NOT ANDROID)
    set(cliExecutableName welle-cli)
    add_executable (${cliExecutableName}
        ${welle_cli_sources}
        ${backend_sources}
        ${input_sources}
        ${fft_sources}
        index.html.h
        index.js.h
        favicon.ico.h)

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
      ${FLACPP_LIBRARIES}
      Threads::Threads
    )

    find_program(RESOURCE_COMPILER xxd)
    add_custom_command(
        OUTPUT index.html.h
        COMMAND ${RESOURCE_COMPILER} -i -n index_html
                ${CMAKE_SOURCE_DIR}/src/welle-cli/index.html
                index.html.h
        DEPENDS ${CMAKE_SOURCE_DIR}/src/welle-cli/index.html)
    set_property(SOURCE index.html.h PROPERTY SKIP_AUTOMOC ON)

    add_custom_command(
        OUTPUT index.js.h
        COMMAND ${RESOURCE_COMPILER} -i -n index_js
                ${CMAKE_SOURCE_DIR}/src/welle-cli/index.js
                index.js.h
        DEPENDS ${CMAKE_SOURCE_DIR}/src/welle-cli/index.js)
    set_property(SOURCE index.js.h PROPERTY SKIP_AUTOMOC ON)

    add_custom_command(
        OUTPUT favicon.ico.h
        COMMAND ${RESOURCE_COMPILER} -i -n favicon_ico
                ${CMAKE_SOURCE_DIR}/src/welle-cli/favicon.ico
                favicon.ico.h
        DEPENDS ${CMAKE_SOURCE_DIR}/src/welle-cli/favicon.ico)
    set_property(SOURCE favicon.ico.h PROPERTY SKIP_AUTOMOC ON)

    if(UNIX AND NOT APPLE OR NOT WITH_APP_BUNDLE)
        INSTALL (TARGETS ${cliExecutableName} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
        INSTALL (FILES src/welle-cli/doc/man/welle-cli.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1/)
    endif()
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

if(Qt6Core_FOUND)
  if(BUILD_WELLE_IO)
    qt_finalize_target(welle-io)
  endif()
  if(CMAKE_VERSION VERSION_LESS "3.19")
    message("since CMAKE_VERSION=${CMAKE_VERSION}, add qt6_finalize_project")
    qt6_finalize_project()
  endif()
endif()
