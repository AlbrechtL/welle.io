include(../backend.pri)

TEMPLATE = app
TARGET = welle-io

CUR_VERSION = $$cat(_current_version, lines)
if(!contains(CUR_VERSION, .*[a-zA-Z\-\ ].*)) {
    # Set VERSION only if it respects the format x.y.z.t (otherwise, on windows build would fail
    VERSION = $$CUR_VERSION
}
DEFINES += CURRENT_VERSION=$$shell_quote(\"$$CUR_VERSION\")

QT += core gui quickcontrols2 qml quick charts multimedia dbus

RC_ICONS   =    icons/icon.ico
RESOURCES +=    resources.qrc
DISTFILES +=    \
    QML/*.qml \
    QML/app/*.qml \
    QML/controls/*.qml \
    QML/expert/*.qml \
    QML/home/*.qml \
    QML/models/*.qml \
    QML/pages/*.qml \
    QML/settings/*.qml \
    QML/settings/devices/*.qml

TRANSLATIONS = i18n/de_DE.ts \
    i18n/it_IT.ts \
    i18n/hu_HU.ts \
    i18n/nb_NO.ts \
    i18n/fr_FR.ts \
    i18n/pl_PL.ts \
    i18n/ru_RU.ts \
    i18n/nl_NL.ts \
    i18n/es_ES.ts

lupdate_only{ # Workaround for lupdate to scan QML files
SOURCES += QML/*.qml \
    QML/app/*.qml \
    QML/controls/*.qml \
    QML/expert/*.qml \
    QML/home/*.qml \
    QML/models/*.qml \
    QML/pages/*.qml \
    QML/settings/*.qml \
    QML/settings/devices/*.qml
}

HEADERS += \
    app_setup.h \
    audio_output.h \
    debug_output.h \
    gui_helper.h \
    mot_image_provider.h \
    station_logos.h \
    radio_controller.h \
    smoke_test.h \
    mpris/mpris.h \
    mpris/mpris_mp2.h \
    mpris/mpris_mp2_player.h \
    waterfallitem.h \
    version.h

SOURCES += \
    main.cpp \
    app_setup.cpp \
    audio_output.cpp \
    debug_output.cpp \
    gui_helper.cpp \
    mot_image_provider.cpp \
    station_logos.cpp \
    radio_controller.cpp \
    smoke_test.cpp \
    mpris/mpris.cpp \
    mpris/mpris_mp2.cpp \
    mpris/mpris_mp2_player.cpp \
    waterfallitem.cpp

android {
    # DEPRECATED. Since Qt6.3, android build is managed by cmake. See CMakeLists.txt
    # It is possible to produce MULTI ABI apks, only from Qt6.3 and only with cmake
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

    DISTFILES += \
        android/AndroidManifest.xml \
        android/build.gradle \
        android/res/values/libs.xml

    DISTFILES += \
        android/java/io/welle/welle/InstallRtlTcpAndro.java

    ANDROID_VERSION_NAME = "$$CUR_VERSION"
    ANDROID_VERSION_CODE = "24"

    QT += core-private # For QtAndroidPrivate
    QT += svg
    QT += multimediawidgets
    QT += 3dextras # To fix "Skipping "..." It has unmet dependencies: lib/libQt63DExtras_arm64-v8a.so"
    qtHaveModule(virtualkeyboard): QT += virtualkeyboard

    HEADERS += android_rtl_sdr.h
    SOURCES += android_rtl_sdr.cpp

    # Remove MPRIS related code since Qt for Android doesn't ship dbus
    QT -= dbus
    SOURCES -= \
        mpris/mpris.cpp \
        mpris/mpris_mp2.cpp \
        mpris/mpris_mp2_player.cpp
    HEADERS -= \
        mpris/mpris.h \
        mpris/mpris_mp2.h \
        mpris/mpris_mp2_player.h
}

# Include git hash & BUILD_DATE into build
unix: {
    GITHASHSTRING = $$system(git rev-parse --short HEAD)
    isEmpty(SOURCE_DATE_EPOCH) {
        SOURCE_DATE_EPOCH=$$(SOURCE_DATE_EPOCH)
    }
    isEmpty(SOURCE_DATE_EPOCH) {
        SOURCE_DATE_EPOCH=$$system(git log -1 --pretty=%ct)
    }
    isEmpty(SOURCE_DATE_EPOCH) {
        SOURCE_DATE_EPOCH=$$system(date +%s)
    }
}

win32 {
    GITHASHSTRING = $$system(git.exe rev-parse --short HEAD)
    isEmpty(SOURCE_DATE_EPOCH) {
        SOURCE_DATE_EPOCH=$$(SOURCE_DATE_EPOCH)
    }
    isEmpty(SOURCE_DATE_EPOCH) {
        SOURCE_DATE_EPOCH=$$system(git.exe log -1 --pretty=%ct)
    }
    isEmpty(SOURCE_DATE_EPOCH) {
        SOURCE_DATE_EPOCH=$$system(powershell -Command (Get-Date -UFormat %s -Millisecond 0))
    }
} else:macx {
    ICON = icons/icon.icns
}

!isEmpty(GITHASHSTRING) {
    message("Current git hash = $$GITHASHSTRING")
    DEFINES += GITHASH=\\\"$$GITHASHSTRING\\\"
}
else {
    warning("Can't get git hash.")
}

!isEmpty(SOURCE_DATE_EPOCH) {
    message("BUILD_DATE = $$SOURCE_DATE_EPOCH")
    DEFINES += BUILD_DATE=\\\"$$SOURCE_DATE_EPOCH\\\"
}
else {
    warning("BUILD_DATE could not be set.")
}
