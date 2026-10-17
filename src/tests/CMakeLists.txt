# This file was generated with an AI assistant (Claude Code).

find_package(Qt6 COMPONENTS Core Test REQUIRED)

set(CMAKE_AUTOMOC ON)

set(WELLE_TEST_DATA_DIR "" CACHE PATH
    "Checkout of the welle.io-test-data repository (overridden by the WELLE_TEST_DATA_DIR environment variable)")

# Shared harness: plays a recording through libwelle and collects the results
add_library(welle_test_common STATIC
    common/recording_runner.cpp
    common/expectations.cpp
    common/spi_replay.cpp
    common/channel_model.cpp)
target_include_directories(welle_test_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/common)
target_compile_definitions(welle_test_common PUBLIC
    WELLE_TEST_DATA_DIR_DEFAULT="${WELLE_TEST_DATA_DIR}")
target_link_libraries(welle_test_common PUBLIC welle Qt6::Core Threads::Threads)

add_executable(welle-test-probe common/welle_test_probe.cpp)
target_link_libraries(welle-test-probe PRIVATE welle_test_common)

# Captures the SPI subchannels and logos from an rtl_tcp server (test data)
add_executable(spi_capture tools/spi_capture.cpp)
target_include_directories(spi_capture PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(spi_capture PRIVATE welle_test_common)

# CTest names the tests <label>.<name>, e.g. unit.rawfile, so that
# "ctest -L unit" and "ctest -R rawfile" both work.

# Unit tests, no test data needed
add_executable(tst_rawfile unit/tst_rawfile.cpp)
target_link_libraries(tst_rawfile PRIVATE welle_test_common Qt6::Test)
add_test(NAME unit.rawfile COMMAND tst_rawfile)

add_executable(tst_dabconstants unit/tst_dabconstants.cpp)
target_link_libraries(tst_dabconstants PRIVATE welle_test_common Qt6::Test)
add_test(NAME unit.dabconstants COMMAND tst_dabconstants)

add_executable(tst_fib unit/tst_fib.cpp)
target_link_libraries(tst_fib PRIVATE welle_test_common Qt6::Test)
add_test(NAME unit.fib COMMAND tst_fib)

add_executable(tst_mp2_scf_crc unit/tst_mp2_scf_crc.cpp)
target_link_libraries(tst_mp2_scf_crc PRIVATE welle_test_common Qt6::Test)
add_test(NAME unit.mp2_scf_crc COMMAND tst_mp2_scf_crc)

add_executable(tst_spi unit/tst_spi.cpp)
target_link_libraries(tst_spi PRIVATE welle_test_common Qt6::Test)
add_test(NAME unit.spi COMMAND tst_spi)

add_executable(tst_software_agc unit/tst_software_agc.cpp)
target_link_libraries(tst_software_agc PRIVATE welle_test_common Qt6::Test)
add_test(NAME unit.software_agc COMMAND tst_software_agc)

add_executable(tst_rational_resampler unit/tst_rational_resampler.cpp)
target_link_libraries(tst_rational_resampler PRIVATE welle_test_common Qt6::Test)
add_test(NAME unit.rational_resampler COMMAND tst_rational_resampler)

add_executable(tst_coarse_frequency unit/tst_coarse_frequency.cpp)
target_link_libraries(tst_coarse_frequency PRIVATE welle_test_common Qt6::Test)
add_test(NAME unit.coarse_frequency COMMAND tst_coarse_frequency)

add_executable(rstest ${CMAKE_SOURCE_DIR}/src/libs/fec/test/rstest.c)
target_link_libraries(rstest PRIVATE welle)
add_test(NAME unit.reed_solomon COMMAND rstest)

set_tests_properties(unit.rawfile unit.dabconstants unit.fib unit.mp2_scf_crc unit.spi unit.software_agc unit.rational_resampler unit.coarse_frequency unit.reed_solomon PROPERTIES LABELS unit)

# Recordings of the welle.io-test-data repository. Every clip of its
# manifest becomes one test, backend.recording.<clip id>, which runs
# "tst_recordings decodeRecording:<clip id>". The list of clips is read when
# CMake runs, so CMake has to know the test data directory.
add_executable(tst_recordings backend/tst_recordings.cpp)
target_link_libraries(tst_recordings PRIVATE welle_test_common Qt6::Test)

# backend.sensitivity: odr_dabmod through noise and an echo
add_executable(tst_sensitivity backend/tst_sensitivity.cpp)
target_link_libraries(tst_sensitivity PRIVATE welle_test_common Qt6::Test)
add_test(NAME backend.sensitivity COMMAND tst_sensitivity)
set_tests_properties(backend.sensitivity PROPERTIES LABELS backend)

find_package(Qt6 COMPONENTS Gui REQUIRED)
add_executable(tst_spi_recordings backend/tst_spi_recordings.cpp)
target_link_libraries(tst_spi_recordings PRIVATE welle_test_common Qt6::Test Qt6::Gui)

# Black box tests of the welle-cli binary (cli/). welle-cli is started with
# its command line options, the tests only look at its output, the files it
# writes and its web server.
if(TARGET welle-cli)
    find_package(Qt6 COMPONENTS Network REQUIRED)
    add_library(welle_cli_process STATIC cli/welle_cli_process.cpp)
    target_include_directories(welle_cli_process PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/cli)
    target_link_libraries(welle_cli_process PUBLIC welle_test_common Qt6::Test Qt6::Network)

    add_executable(tst_cli cli/tst_cli.cpp)
    add_executable(tst_cli_recordings cli/tst_cli_recordings.cpp)
    foreach(target tst_cli tst_cli_recordings)
        target_link_libraries(${target} PRIVATE welle_cli_process)
        add_dependencies(${target} welle-cli)
    endforeach()
endif()

set(test_data_dir "${WELLE_TEST_DATA_DIR}")
if(NOT test_data_dir)
    set(test_data_dir "$ENV{WELLE_TEST_DATA_DIR}")
endif()
set(test_manifest "${test_data_dir}/manifest.json")

if(test_data_dir AND EXISTS "${test_manifest}" AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
    # Run CMake again when the manifest changes
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${test_manifest}")

    file(READ "${test_manifest}" manifest_json)
    string(JSON clip_count LENGTH "${manifest_json}" clips)
    math(EXPR last_clip "${clip_count} - 1")
    foreach(i RANGE ${last_clip})
        string(JSON clip_id GET "${manifest_json}" clips ${i} id)
        add_test(NAME backend.recording.${clip_id}
            COMMAND tst_recordings decodeRecording:${clip_id})
        set_tests_properties(backend.recording.${clip_id} PROPERTIES
            LABELS backend
            TIMEOUT 300
            ENVIRONMENT "WELLE_TEST_DATA_DIR=${test_data_dir}")
        if(TARGET welle-cli)
            add_test(NAME cli.recording.${clip_id}
                COMMAND tst_cli_recordings checkRecording:${clip_id})
            set_tests_properties(cli.recording.${clip_id} PROPERTIES
                LABELS cli
                TIMEOUT 120
                ENVIRONMENT "WELLE_TEST_DATA_DIR=${test_data_dir};WELLE_CLI=$<TARGET_FILE:welle-cli>")
        endif()
    endforeach()
    message(STATUS "Test data: ${clip_count} recordings in ${test_data_dir}")

    # Recorded SPI subchannels: backend.spi.<id>
    string(JSON spi_count ERROR_VARIABLE spi_error LENGTH "${manifest_json}" spi)
    if(NOT spi_error AND spi_count GREATER 0)
        math(EXPR last_spi "${spi_count} - 1")
        foreach(i RANGE ${last_spi})
            string(JSON spi_id GET "${manifest_json}" spi ${i} id)
            add_test(NAME backend.spi.${spi_id}
                COMMAND tst_spi_recordings decodeSpi:${spi_id})
            set_tests_properties(backend.spi.${spi_id} PROPERTIES
                LABELS backend
                ENVIRONMENT "WELLE_TEST_DATA_DIR=${test_data_dir}")
        endforeach()
    endif()
else()
    # Skips with a message that explains how to get the test data
    add_test(NAME backend.recordings COMMAND tst_recordings)
    set_tests_properties(backend.recordings PROPERTIES LABELS backend)
    message(STATUS "Test data: not found, set WELLE_TEST_DATA_DIR to a checkout of welle.io-test-data")
endif()

# cli.<function>: options (no recording needed) and the features of
# welle-cli with the recording odr_dabmod, see cli/tst_cli.cpp
if(TARGET welle-cli)
    set(cli_test_env "WELLE_CLI=$<TARGET_FILE:welle-cli>")
    # Otherwise $WELLE_TEST_DATA_DIR of the caller is used
    if(test_data_dir)
        list(APPEND cli_test_env "WELLE_TEST_DATA_DIR=${test_data_dir}")
    endif()
    foreach(function options webServer webServerFlac carousel dumpAll testMode alsaPlayback)
        add_test(NAME cli.${function} COMMAND tst_cli ${function})
        set_tests_properties(cli.${function} PROPERTIES
            LABELS cli
            TIMEOUT 300
            ENVIRONMENT "${cli_test_env}")
    endforeach()
endif()

# GUI tests: the C++ part of welle.io and its QML user interface, without
# a display (offscreen platform). They are built from the same sources as
# the welle-io executable, without main.cpp. These sources are compiled
# once into an object library that every GUI test links.
#
# gui.ui.<area>: automated tests of the user interface, which click
# through MainView.qml while a recording plays (gui/ui/tst_ui_<area>.cpp).
if(TARGET welle-io)
    find_package(Qt6 COMPONENTS Widgets Quick QuickControls2 QuickTest Multimedia Charts Qml Svg REQUIRED)

    set(gui_test_sources)
    foreach(source ${welle_io_sources})
        if(NOT source MATCHES "(main\\.cpp|qrc_.*\\.cpp)$")
            get_filename_component(source ${source} ABSOLUTE BASE_DIR ${CMAKE_SOURCE_DIR})
            list(APPEND gui_test_sources ${source})
        endif()
    endforeach()
    qt_add_resources(gui_test_sources ${CMAKE_SOURCE_DIR}/src/welle-gui/resources.qrc)

    add_library(welle_gui_objects OBJECT ${gui_test_sources}
        gui/gui_test_main.h
        gui/ui/ui_app.cpp
        gui/ui/ui_app.h
        gui/ui/ui_test_case.h)
    target_include_directories(welle_gui_objects PUBLIC
        ${CMAKE_SOURCE_DIR}/src/welle-gui
        ${CMAKE_CURRENT_SOURCE_DIR}/gui
        ${CMAKE_CURRENT_SOURCE_DIR}/gui/ui)
    target_link_libraries(welle_gui_objects PUBLIC welle_test_common Qt6::Test Qt6::QuickTest
        Qt6::Widgets Qt6::Multimedia Qt6::Charts Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::Svg)
    if(TARGET Qt6::DBus)
        target_link_libraries(welle_gui_objects PUBLIC Qt6::DBus)
    endif()

    function(add_gui_test name source)
        add_executable(tst_${name} ${source})
        target_link_libraries(tst_${name} PRIVATE welle_gui_objects)
        string(REPLACE "ui_" "ui." test_name "${name}")
        add_test(NAME gui.${test_name} COMMAND tst_${name})
        set_tests_properties(gui.${test_name} PROPERTIES
            LABELS gui
            TIMEOUT 600
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen;WELLE_TEST_DATA_DIR=${test_data_dir}")
    endfunction()

    add_gui_test(radiocontroller gui/tst_radiocontroller.cpp)
    add_gui_test(qml_smoke gui/tst_qml_smoke.cpp)

    # gui.smoke_option: welle-io --smoke-test with the script that checks
    # the installed packages in the package workflows (smoke/smoke_test.py)
    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        add_test(NAME gui.smoke_option
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/smoke/smoke_test.py
                --welle-io $<TARGET_FILE:welle-io>
                --work-dir ${CMAKE_CURRENT_BINARY_DIR}/smoke_option)
        set_tests_properties(gui.smoke_option PROPERTIES
            LABELS gui
            TIMEOUT 300
            SKIP_RETURN_CODE 77
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen;WELLE_TEST_DATA_DIR=${test_data_dir}")
    endif()

    file(GLOB ui_tests RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS gui/ui/tst_ui_*.cpp)
    foreach(source ${ui_tests})
        get_filename_component(name ${source} NAME_WE)
        string(REPLACE "tst_" "" name "${name}")
        add_gui_test(${name} ${source})
    endforeach()
endif()
