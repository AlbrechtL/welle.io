# This file was generated with an AI assistant (Claude Code).

find_package(Qt6 COMPONENTS Core Test REQUIRED)

set(CMAKE_AUTOMOC ON)

set(WELLE_TEST_DATA_DIR "" CACHE PATH
    "Checkout of the welle.io-test-data repository (overridden by the WELLE_TEST_DATA_DIR environment variable)")

# Shared harness: plays a recording through libwelle and collects the results
add_library(welle_test_common STATIC
    common/recording_runner.cpp
    common/expectations.cpp)
target_include_directories(welle_test_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/common)
target_compile_definitions(welle_test_common PUBLIC
    WELLE_TEST_DATA_DIR_DEFAULT="${WELLE_TEST_DATA_DIR}")
target_link_libraries(welle_test_common PUBLIC welle Qt6::Core Threads::Threads)

add_executable(welle-test-probe common/welle_test_probe.cpp)
target_link_libraries(welle-test-probe PRIVATE welle_test_common)

# CTest names the tests <label>.<name>, e.g. unit.rawfile, so that
# "ctest -L unit" and "ctest -R rawfile" both work.

# Unit tests, no test data needed
add_executable(tst_rawfile unit/tst_rawfile.cpp)
target_link_libraries(tst_rawfile PRIVATE welle_test_common Qt6::Test)
add_test(NAME unit.rawfile COMMAND tst_rawfile)

add_executable(tst_dabconstants unit/tst_dabconstants.cpp)
target_link_libraries(tst_dabconstants PRIVATE welle_test_common Qt6::Test)
add_test(NAME unit.dabconstants COMMAND tst_dabconstants)

add_executable(rstest ${CMAKE_SOURCE_DIR}/src/libs/fec/test/rstest.c)
target_link_libraries(rstest PRIVATE welle)
add_test(NAME unit.reed_solomon COMMAND rstest)

set_tests_properties(unit.rawfile unit.dabconstants unit.reed_solomon PROPERTIES LABELS unit)

# Recordings of the welle.io-test-data repository. Every clip of its
# manifest becomes one test, backend.recording.<clip id>, which runs
# "tst_recordings decodeRecording:<clip id>". The list of clips is read when
# CMake runs, so CMake has to know the test data directory.
add_executable(tst_recordings backend/tst_recordings.cpp)
target_link_libraries(tst_recordings PRIVATE welle_test_common Qt6::Test)

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
    endforeach()
    message(STATUS "Test data: ${clip_count} recordings in ${test_data_dir}")
else()
    # Skips with a message that explains how to get the test data
    add_test(NAME backend.recordings COMMAND tst_recordings)
    set_tests_properties(backend.recordings PROPERTIES LABELS backend)
    message(STATUS "Test data: not found, set WELLE_TEST_DATA_DIR to a checkout of welle.io-test-data")
endif()

# GUI tests: the C++ part of welle.io and its QML user interface, without
# a display (offscreen platform). They are built from the same sources as
# the welle-io executable, without main.cpp.
if(TARGET welle-io)
    find_package(Qt6 COMPONENTS Widgets Quick QuickControls2 Multimedia Charts Qml REQUIRED)

    set(gui_test_sources)
    foreach(source ${welle_io_sources})
        if(NOT source MATCHES "(main\\.cpp|qrc_.*\\.cpp)$")
            get_filename_component(source ${source} ABSOLUTE BASE_DIR ${CMAKE_SOURCE_DIR})
            list(APPEND gui_test_sources ${source})
        endif()
    endforeach()
    qt_add_resources(gui_test_sources ${CMAKE_SOURCE_DIR}/src/welle-gui/resources.qrc)

    foreach(name radiocontroller qml_smoke)
        add_executable(tst_${name} gui/tst_${name}.cpp gui/gui_test_main.h ${gui_test_sources})
        target_include_directories(tst_${name} PRIVATE ${CMAKE_SOURCE_DIR}/src/welle-gui)
        target_link_libraries(tst_${name} PRIVATE welle_test_common Qt6::Test
            Qt6::Widgets Qt6::Multimedia Qt6::Charts Qt6::Qml Qt6::Quick Qt6::QuickControls2)
        if(TARGET Qt6::DBus)
            target_link_libraries(tst_${name} PRIVATE Qt6::DBus)
        endif()
        add_test(NAME gui.${name} COMMAND tst_${name})
        set_tests_properties(gui.${name} PROPERTIES
            LABELS gui
            TIMEOUT 300
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen;WELLE_TEST_DATA_DIR=${test_data_dir}")
    endforeach()
endif()
