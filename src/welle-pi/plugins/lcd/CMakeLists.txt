# Character LCD through liblcd (https://github.com/fridrich/lcd), skipped without it
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(WELLE_PI_LIBLCD QUIET IMPORTED_TARGET liblcd-0.0)
endif()
if(NOT WELLE_PI_LIBLCD_FOUND)
    message(STATUS "welle-pi LCD plugin: liblcd-0.0 not found, skipped")
    return()
endif()

welle_pi_plugin(lcd lcd_display.cpp)
target_link_libraries(welle-pi-lcd PRIVATE PkgConfig::WELLE_PI_LIBLCD)
