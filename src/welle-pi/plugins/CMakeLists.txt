# welle-pi plugins, see docs/development/welle-pi-input.md#plugins. Single
# file plugins here, larger ones (own dependencies) in a subdirectory.

set(WELLE_PI_PLUGIN_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

# welle_pi_plugin(<name> [NO_INSTALL] <sources>...): builds welle-pi-<name>.so
# into welle-pi-plugins/ next to welle-pi, where it looks first, and installs
# it into WELLE_PI_PLUGIN_DIR
function(welle_pi_plugin name)
    cmake_parse_arguments(PLUGIN "NO_INSTALL" "" "" ${ARGN})
    set(target welle-pi-${name})
    add_library(${target} MODULE ${PLUGIN_UNPARSED_ARGUMENTS})
    target_include_directories(${target} PRIVATE "${WELLE_PI_PLUGIN_INCLUDE_DIR}")
    set_target_properties(${target} PROPERTIES PREFIX ""
        LIBRARY_OUTPUT_DIRECTORY "${WELLE_PI_PLUGIN_OUTPUT_DIR}")
    if(NOT PLUGIN_NO_INSTALL)
        install(TARGETS ${target} LIBRARY DESTINATION "${WELLE_PI_PLUGIN_DIR}")
    endif()
endfunction()

add_subdirectory(fb)
add_subdirectory(lcd)

# loaded by name in unit.input_plugin and unit.display_plugin
if(BUILD_TESTS)
    welle_pi_plugin(example NO_INSTALL example.c)
endif()
