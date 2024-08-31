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
    QML/MainView.qml \
    QML/RadioView.qml \
    QML/ExpertView.qml \
    QML/texts/TextStyle.qml \
    QML/texts/TextTitle.qml \
    QML/texts/TextStandart.qml \
    QML/texts/TextStation.qml \
    QML/texts/TextRadioInfo.qml \
    QML/texts/TextRadioStation.qml \
    QML/texts/TextExpert.qml \
    QML/InfoPage.qml \
    QML/settingpages/ExpertSettings.qml \
    QML/settingpages/GlobalSettings.qml \
    QML/settingpages/ChannelSettings.qml \
    QML/settingpages/RTLSDRSettings.qml \
    QML/settingpages/RTLTCPSettings.qml \
    QML/settingpages/SoapySDRSettings.qml \
    QML/settingpages/ExpertSettings.qml \
    QML/settingpages/NullSettings.qml \
    QML/components/MessagePopup.qml \
    QML/components/SettingSection.qml \
    QML/components/StationDelegate.qml \
    QML/components/Units.qml \
    QML/expertviews/ImpulseResponseGraph.qml \
    QML/expertviews/ConstellationGraph.qml \
    QML/expertviews/NullSymbolGraph.qml \
    QML/expertviews/SpectrumGraph.qml \
    QML/expertviews/TextOutputView.qml \
    QML/expertviews/RawRecorder.qml \
    QML/components/StationListModel.qml \
    QML/MotView.qml \
    QML/components/ViewBaseFrame.qml \
    QML/GeneralView.qml \
    QML/settingpages/RawFileSettings.qml \
    QML/components/WComboBox.qml \
    QML/components/WComboBoxList.qml \
    QML/components/WButton.qml \
    QML/components/WSwitch.qml \
    QML/components/WTumbler.qml \
    QML/components/WSpectrum.qml \
    QML/components/WMenu.qml \
    QML/expertviews/ServiceDetails.qml \
    QML/components/WDialog.qml

TRANSLATIONS = i18n/de_DE.ts \
    i18n/it_IT.ts \
    i18n/hu_HU.ts \
    i18n/nb_NO.ts \
    i18n/fr_FR.ts \
    i18n/pl_PL.ts \
    i18n/ru_RU.ts \
    i18n/nl_NL.ts

lupdate_only{ # Workaround for lupdate to scan QML files
SOURCES += QML/*.qml \
    QML/texts/*.qml \
    QML/settingpages/*.qml \
    QML/components/*.qml \
    QML/expertviews/*.qml \
}

HEADERS += \
    audio_output.h \
    debug_output.h \
    gui_helper.h \
    mot_image_provider.h \
    radio_controller.h \
    mpris/mpris.h \
    mpris/mpris_mp2.h \
    mpris/mpris_mp2_player.h \
    waterfallitem.h \
    version.h

SOURCES += \
    main.cpp \
    audio_output.cpp \
    debug_output.cpp \
    gui_helper.cpp \
    mot_image_provider.cpp \
    radio_controller.cpp \
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
