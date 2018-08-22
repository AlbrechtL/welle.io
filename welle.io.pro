TEMPLATE = app
TARGET = welle-io
QT += network qml quick charts multimedia
CONFIG += c++14

Release: QMAKE_CFLAGS	+=  -flto -ffast-math -O3
Release: QMAKE_CXXFLAGS	+=  -flto -ffast-math -O3
Release: QMAKE_LFLAGS	+=  -flto -O3

RC_ICONS   =    icon.ico
RESOURCES +=    resources.qrc
DISTFILES +=    README.md \
    android/AndroidManifest.xml \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradlew \
    android/res/values/libs.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew.bat \
    android/java/io/welle/welle/DabMediaService.java \
    android/java/io/welle/welle/DabService.java \
    android/java/io/welle/welle/DabDelegate.java \
    src/gui/QML/MainView.qml \
    src/gui/QML/RadioView.qml \
    src/gui/QML/ExpertView.qml \
    src/gui/QML/texts/TextStyle.qml \
    src/gui/QML/texts/TextTitle.qml \
    src/gui/QML/texts/TextStandart.qml \
    src/gui/QML/texts/TextStation.qml \
    src/gui/QML/texts/TextRadioInfo.qml \
    src/gui/QML/texts/TextRadioStation.qml \
    src/gui/QML/texts/TextExpert.qml \
    src/gui/QML/InfoPage.qml \
    src/gui/QML/components/SettingsPopup.qml \
    src/gui/QML/settingpages/ExpertSettings.qml \
    src/gui/QML/components/SettingSwitch.qml \
    src/gui/QML/settingpages/GlobalSettings.qml \
    src/gui/QML/settingpages/ChannelSettings.qml \
    src/gui/QML/settingpages/RTLSDRSettings.qml \
    src/gui/QML/settingpages/RTLTCPSettings.qml \
    src/gui/QML/settingpages/SoapySDRSettings.qml \
    src/gui/QML/settingpages/ExpertSettings.qml \
    src/gui/QML/components/MessagePopup.qml \
    src/gui/QML/components/SettingsPopup.qml \
    src/gui/QML/components/SettingSection.qml \
    src/gui/QML/components/SettingSwitch.qml \
    src/gui/QML/components/StationDelegate.qml \
    src/gui/QML/components/Units.qml \
    src/gui/QML/expertviews/StationInformation.qml \
    src/gui/QML/expertviews/ImpulseResponseGraph.qml \
    src/gui/QML/expertviews/ConstellationGraph.qml \
    src/gui/QML/expertviews/NullSymbolGraph.qml \
    src/gui/QML/expertviews/SpectrumGraph.qml \
    src/gui/QML/expertviews/TextOutputView.qml \
    src/gui/QML/components/StationListModel.qml

TRANSLATIONS = i18n/de_DE.ts i18n/it_IT.ts i18n/hu_HU.ts i18n/nb_NO.ts i18n/fr_FR.ts i18n/pl_PL.ts i18n/ru_RU.ts

lupdate_only{ # Workaround for lupdate to scan QML files
SOURCES += src/gui/QML/*.qml \
    src/gui/QML/texts/*.qml \
    src/gui/QML/settingpages/*.qml \
    src/gui/QML/components/*.qml \
    src/gui/QML/expertviews/*.qml \
}

DEPENDPATH += \
    src \
    src/backend \
    src/backend/audio \
    src/backend/data \
    src/backend/ofdm \
    src/input \
    src/output \
    src/various \
    src/gui

INCLUDEPATH += \
    src \
    src/backend \
    src/output \
    src/various \
    src/input \
    src/gui

HEADERS += \
    src/backend/audio/CFaadDecoder.h \
    src/backend/audio/dab-audio.h \
    src/backend/audio/mp2processor.h \
    src/backend/audio/mp4processor.h \
    src/backend/audio/neaacdec.h \
    src/backend/charsets.h \
    src/backend/dab-constants.h \
    src/backend/dab-processor.h \
    src/backend/dab-virtual.h \
    src/backend/data/mot_manager.h \
    src/backend/data/pad_decoder.h \
    src/backend/eep-protection.h \
    src/backend/energy_dispersal.h \
    src/backend/fib-processor.h \
    src/backend/fic-handler.h \
    src/backend/firecode-checker.h \
    src/backend/galois.h \
    src/backend/mm_malloc.h \
    src/backend/msc-handler.h \
    src/backend/ofdm/freq-interleaver.h \
    src/backend/ofdm/ofdm-decoder.h \
    src/backend/ofdm/ofdm-processor.h \
    src/backend/ofdm/phasereference.h \
    src/backend/ofdm/phasetable.h \
    src/backend/ofdm/tii-decoder.h \
    src/backend/parity.h \
    src/backend/protTables.h \
    src/backend/protection.h \
    src/backend/radio-controller.h \
    src/backend/radio-receiver.h \
    src/backend/reed-solomon.h \
    src/backend/tools.h \
    src/backend/uep-protection.h \
    src/backend/viterbi.h \
    src/output/CAudio.h \
    src/various/fft.h \
    src/various/ringbuffer.h \
    src/various/Xtan2.h \
    src/various/channels.h \
    src/various/wavfile.h \
    src/various/Socket.h \
    src/input/CVirtualInput.h \
    src/input/CInputFactory.h \
    src/input/CNullDevice.h \
    src/input/CRAWFile.h \
    src/input/CRTL_TCP_Client.h \
    src/MathHelper.h \
    src/gui/CMOTImageProvider.h \
    src/CRadioController.h \
    src/CSplashScreen.h \
    src/CDebugOutput.h \
    src/gui/CGUIHelper.h

SOURCES += \
    src/main.cpp \
    src/backend/audio/CFaadDecoder.cpp \
    src/backend/audio/dab-audio.cpp \
    src/backend/audio/mp2processor.cpp \
    src/backend/audio/mp4processor.cpp \
    src/backend/charsets.cpp \
    src/backend/dab-constants.cpp \
    src/backend/data/mot_manager.cpp \
    src/backend/data/pad_decoder.cpp \
    src/backend/eep-protection.cpp \
    src/backend/fib-processor.cpp \
    src/backend/fic-handler.cpp \
    src/backend/firecode-checker.cpp \
    src/backend/galois.cpp \
    src/backend/msc-handler.cpp \
    src/backend/ofdm/freq-interleaver.cpp \
    src/backend/ofdm/ofdm-decoder.cpp \
    src/backend/ofdm/ofdm-processor.cpp \
    src/backend/ofdm/phasereference.cpp \
    src/backend/ofdm/phasetable.cpp \
    src/backend/ofdm/tii-decoder.cpp \
    src/backend/protTables.cpp \
    src/backend/reed-solomon.cpp \
    src/backend/radio-receiver.cpp \
    src/backend/tools.cpp \
    src/backend/uep-protection.cpp \
    src/backend/viterbi.cpp \
    src/various/Xtan2.cpp \
    src/various/channels.cpp \
    src/various/fft.cpp \
    src/various/wavfile.c \
    src/various/Socket.cpp \
    src/output/CAudio.cpp \
    src/input/CInputFactory.cpp \
    src/input/CNullDevice.cpp \
    src/input/CRAWFile.cpp \
    src/input/CRTL_TCP_Client.cpp \
    src/gui/CMOTImageProvider.cpp \
    src/CRadioController.cpp \
    src/CSplashScreen.cpp \
    src/CDebugOutput.cpp \
    src/gui/CGUIHelper.cpp

unix:!macx:!android: {
    INCLUDEPATH	+= /usr/local/include
    LIBS    += -lfftw3f
    LIBS    += -lusb-1.0
    LIBS    += -ldl
    LIBS    += -lfaad
    CONFIG  += airspy
    CONFIG  += rtl_sdr
    #CONFIG  += soapysdr

    #CONFIG  += kiss_fft_builtin
    #CONFIG  += libfaad_builtin
}


win32 {
    INCLUDEPATH += ../welle.io-win-libs/include
    LIBS    += -L../welle.io-win-libs/x86
    LIBS    += -lfftw3f-3
    LIBS    += -lole32
    LIBS    += -lwinpthread
    LIBS    += -lwinmm
    LIBS    += -lstdc++
    LIBS    += -lws2_32
    LIBS    += -llibfaad
    LIBS    += -lusb-1.0
    LIBS    += -lws2_32
    CONFIG  += rtl_sdr
    CONFIG  += airspy
}


macx {
    INCLUDEPATH	+= /opt/local/include
    INCLUDEPATH	+= /usr/local/include
    LIBS    += -L/opt/local/lib
    LIBS    += -L/usr/local/lib
    LIBS    += -lfftw3f
    LIBS    += -lusb-1.0
    LIBS    += -ldl
    LIBS    += -lfaad
    CONFIG  += airspy
    CONFIG  += rtl_sdr
    #CONFIG  += soapysdr        # not tested
}

android {
    QT += androidextras
    QT += remoteobjects
    LIBS += -ljnigraphics

    CONFIG  += kiss_fft_builtin
    CONFIG  += libfaad_builtin

    HEADERS      += src/gui/CAndroidJNI.h
    SOURCES      += src/gui/CAndroidJNI.cpp
    REPC_SOURCE  += src/CRadioController.rep
    REPC_REPLICA += src/CRadioController.rep
}

#### Built-in libraries ####
kiss_fft_builtin {
    DEFINES   += KISSFFT

    INCLUDEPATH += src/libs/kiss_fft

    HEADERS    += \
    src/libs/kiss_fft/kiss_fft.h \
    src/libs/kiss_fft/_kiss_fft_guts.h

    SOURCES    += src/libs/kiss_fft/kiss_fft.c
}

libfaad_builtin {
    DEFINES += HAVE_CONFIG_H

    # Dangerous but libfaad produces a lot of warnings
    QMAKE_CFLAGS += -Wno-unused-parameter
    QMAKE_CFLAGS += -Wno-unused-function
    QMAKE_CFLAGS += -Wno-unused-variable
    QMAKE_CFLAGS += -Wno-unused-but-set-variable
    QMAKE_CFLAGS += -Wno-old-style-declaration
    QMAKE_CFLAGS += -Wno-missing-braces

    INCLUDEPATH += \
    src/libs/faad2 \
    src/libs/faad2/libfaad \
    src/libs/faad2/libfaad/codebook \
    src/libs/faad2/include

    HEADERS += \
    src/libs/faad2/config.h \
    src/libs/faad2/include/faad.h \
    src/libs/faad2/include/neaacdec.h \
    src/libs/faad2/libfaad/analysis.h \
    src/libs/faad2/libfaad/bits.h \
    src/libs/faad2/libfaad/cfft.h \
    src/libs/faad2/libfaad/cfft_tab.h \
    src/libs/faad2/libfaad/common.h \
    src/libs/faad2/libfaad/drc.h \
    src/libs/faad2/libfaad/drm_dec.h \
    src/libs/faad2/libfaad/error.h \
    src/libs/faad2/libfaad/filtbank.h \
    src/libs/faad2/libfaad/fixed.h \
    src/libs/faad2/libfaad/huffman.h \
    src/libs/faad2/libfaad/ic_predict.h \
    src/libs/faad2/libfaad/iq_table.h \
    src/libs/faad2/libfaad/is.h \
    src/libs/faad2/libfaad/kbd_win.h \
    src/libs/faad2/libfaad/lt_predict.h \
    src/libs/faad2/libfaad/mdct.h \
    src/libs/faad2/libfaad/mdct_tab.h \
    src/libs/faad2/libfaad/mp4.h \
    src/libs/faad2/libfaad/ms.h \
    src/libs/faad2/libfaad/output.h \
    src/libs/faad2/libfaad/pns.h \
    src/libs/faad2/libfaad/ps_dec.h \
    src/libs/faad2/libfaad/ps_tables.h \
    src/libs/faad2/libfaad/pulse.h \
    src/libs/faad2/libfaad/rvlc.h \
    src/libs/faad2/libfaad/sbr_dct.h \
    src/libs/faad2/libfaad/sbr_dec.h \
    src/libs/faad2/libfaad/sbr_e_nf.h \
    src/libs/faad2/libfaad/sbr_fbt.h \
    src/libs/faad2/libfaad/sbr_hfadj.h \
    src/libs/faad2/libfaad/sbr_hfgen.h \
    src/libs/faad2/libfaad/sbr_huff.h \
    src/libs/faad2/libfaad/sbr_noise.h \
    src/libs/faad2/libfaad/sbr_qmf_c.h \
    src/libs/faad2/libfaad/sbr_qmf.h \
    src/libs/faad2/libfaad/sbr_syntax.h \
    src/libs/faad2/libfaad/sbr_tf_grid.h \
    src/libs/faad2/libfaad/sine_win.h \
    src/libs/faad2/libfaad/specrec.h \
    src/libs/faad2/libfaad/ssr_fb.h \
    src/libs/faad2/libfaad/ssr.h \
    src/libs/faad2/libfaad/ssr_ipqf.h \
    src/libs/faad2/libfaad/ssr_win.h \
    src/libs/faad2/libfaad/structs.h \
    src/libs/faad2/libfaad/syntax.h \
    src/libs/faad2/libfaad/tns.h \
    src/libs/faad2/libfaad/codebook/hcb_10.h \
    src/libs/faad2/libfaad/codebook/hcb_11.h \
    src/libs/faad2/libfaad/codebook/hcb_1.h \
    src/libs/faad2/libfaad/codebook/hcb_2.h \
    src/libs/faad2/libfaad/codebook/hcb_3.h \
    src/libs/faad2/libfaad/codebook/hcb_4.h \
    src/libs/faad2/libfaad/codebook/hcb_5.h \
    src/libs/faad2/libfaad/codebook/hcb_6.h \
    src/libs/faad2/libfaad/codebook/hcb_7.h \
    src/libs/faad2/libfaad/codebook/hcb_8.h \
    src/libs/faad2/libfaad/codebook/hcb_9.h \
    src/libs/faad2/libfaad/codebook/hcb.h \
    src/libs/faad2/libfaad/codebook/hcb_sf.h

    SOURCES    += \
    src/libs/faad2/libfaad/bits.c \
    src/libs/faad2/libfaad/cfft.c \
    src/libs/faad2/libfaad/common.c \
    src/libs/faad2/libfaad/decoder.c \
    src/libs/faad2/libfaad/drc.c \
    src/libs/faad2/libfaad/drm_dec.c \
    src/libs/faad2/libfaad/error.c \
    src/libs/faad2/libfaad/filtbank.c \
    src/libs/faad2/libfaad/hcr.c \
    src/libs/faad2/libfaad/huffman.c \
    src/libs/faad2/libfaad/ic_predict.c \
    src/libs/faad2/libfaad/is.c \
    src/libs/faad2/libfaad/lt_predict.c \
    src/libs/faad2/libfaad/mdct.c \
    src/libs/faad2/libfaad/mp4.c \
    src/libs/faad2/libfaad/ms.c \
    src/libs/faad2/libfaad/output.c \
    src/libs/faad2/libfaad/pns.c \
    src/libs/faad2/libfaad/ps_dec.c \
    src/libs/faad2/libfaad/ps_syntax.c \
    src/libs/faad2/libfaad/pulse.c \
    src/libs/faad2/libfaad/rvlc.c \
    src/libs/faad2/libfaad/sbr_dct.c \
    src/libs/faad2/libfaad/sbr_dec.c \
    src/libs/faad2/libfaad/sbr_e_nf.c \
    src/libs/faad2/libfaad/sbr_fbt.c \
    src/libs/faad2/libfaad/sbr_hfadj.c \
    src/libs/faad2/libfaad/sbr_hfgen.c \
    src/libs/faad2/libfaad/sbr_huff.c \
    src/libs/faad2/libfaad/sbr_qmf.c \
    src/libs/faad2/libfaad/sbr_syntax.c \
    src/libs/faad2/libfaad/sbr_tf_grid.c \
    src/libs/faad2/libfaad/specrec.c \
    src/libs/faad2/libfaad/ssr.c \
    src/libs/faad2/libfaad/ssr_fb.c \
    src/libs/faad2/libfaad/ssr_ipqf.c \
    src/libs/faad2/libfaad/syntax.c \
    src/libs/faad2/libfaad/tns.c
}

#### Devices ####
airspy {
    DEFINES    += HAVE_AIRSPY
    HEADERS    += src/input/CAirspy.h
    SOURCES    += src/input/CAirspy.cpp

    # The same lib for unix and Windows
    LIBS       += -lairspy
}

rtl_sdr {
    DEFINES    += HAVE_RTLSDR
    HEADERS    += src/input/CRTL_SDR.h
    SOURCES    += src/input/CRTL_SDR.cpp

    # The same lib for unix and Windows
    LIBS       += -lrtlsdr
}

soapysdr {
    DEFINES    += HAVE_SOAPYSDR
    HEADERS    += src/input/CSoapySdr.h
    SOURCES    += src/input/CSoapySdr.cpp

    # The same lib for unix and Windows
    LIBS       += -lSoapySDR
}

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

# Include git hash into build
unix:!macx { # macOS is not tested, exclude it
    GITHASHSTRING = $$system(git rev-parse --short HEAD)
}

win32 {
    GITHASHSTRING = $$system(git.exe rev-parse --short HEAD)
} else:macx {
    ICON = icon.icns
    QMAKE_INFO_PLIST = $$(PWD)/welle-io.plist
}

!isEmpty(GITHASHSTRING) {
    message("Current git hash = $$GITHASHSTRING")
    DEFINES += GITHASH=\\\"$$GITHASHSTRING\\\"
}
else {
    warning("Can't get git hash.")
}
