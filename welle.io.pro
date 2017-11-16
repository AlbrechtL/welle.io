TEMPLATE = app
TARGET = welle-io
QT += network qml quick charts multimedia
CONFIG += console c++14

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
    android/java/io/welle/welle/DabDelegate.java

TRANSLATIONS = i18n/de_DE.ts i18n/it_IT.ts

lupdate_only{ # Workaround for lupdate to scan QML files
SOURCES += src/gui/QML/*.qml \
    src/gui/QML/style/*.qml \
}


DEPENDPATH += \
    src \
    src/output \
#    src/input \
    src/gui


INCLUDEPATH += \
    src \
    src/output \
#    src/input \
    src/gui

HEADERS += \
    src/MathHelper.h \
    src/input/CVirtualInput.h \
    src/input/CInputFactory.h \
    src/input/CNullDevice.h \
    src/input/CRAWFile.h \
    src/input/CRTL_TCP_Client.h \
    src/gui/CMOTImageProvider.h \
    src/gui/CStationList.h \
    src/gui/CGUI.h \
    src/CRadioController.h \
    src/CChannels.h \
    src/output/CAudioDecoder.h \
    src/various/CRingBuffer.h \
    src/output/CFaadDecoder.h \
    src/various/Tools.h \
    src/output/CAudioOutput.h \
    src/CSdrDabInterface.h \
    src/CFicData.h \
    src/input/CSdrDabInputAdapter.h

SOURCES += \
    src/main.cpp \
    src/input/CInputFactory.cpp \
    src/input/CNullDevice.cpp \
    src/input/CRAWFile.cpp \
    src/input/CRTL_TCP_Client.cpp \
    src/gui/CMOTImageProvider.cpp \
    src/gui/CStationList.cpp \
    src/gui/CGUI.cpp \
    src/CRadioController.cpp \
    src/CChannels.cpp \
    src/output/CAudioDecoder.cpp \
    src/various/Tools.cpp \
    src/output/CAudioOutput.cpp \
    src/output/CFaadDecoder.cpp \
    src/CSdrDabInterface.cpp \
    src/CFicData.cpp \
    src/input/CSdrDabInputAdapter.cpp

unix:!macx:!android: {
    INCLUDEPATH	+= /usr/local/include

    LIBS    += -lsamplerate -lrtlsdr

    LIBS    += -lfftw3f
    LIBS    += -lusb-1.0
    LIBS    += -ldl
    LIBS    += -lfaad
    CONFIG  += airspy
    CONFIG  += rtl_sdr
    #CONFIG  += soapysdr

    #CONFIG  += kiss_fft_builtin
    #CONFIG  += libfaad_builtin

    DEFINES += SSE_AVAILABLE

    CONFIG  += sdrdab_builtin
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
    CONFIG  += rtl_sdr
    CONFIG  += airspy

    # SSE under Windows not working. welle.io crashes
    #QMAKE_CFLAGS += -msse2
    #DEFINES += SSE_AVAILABLE
}

macx {
    INCLUDEPATH	+= /opt/local/include
    LIBS    += -L/opt/local/lib
    LIBS    += -lfftw3f
    LIBS    += -lusb-1.0
    LIBS    += -ldl
    LIBS    += -lfaad
    CONFIG  += airspy
    CONFIG  += rtl_sdr
    #CONFIG  += soapysdr        # not tested
    DEFINES += SSE_AVAILABLE
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

    equals(ANDROID_TARGET_ARCH, x86)  {
        message("Enable SSE")
        DEFINES += SSE_AVAILABLE
    }
}

#### Built-in libraries ####
sdrdab_builtin {
    INCLUDEPATH	+= src/libs \
    src/libs/sdrdab \
    src/libs/sdrdab/AudioDecoder \
    src/libs/sdrdab/DataDecoder \
    src/libs/sdrdab/DataFeeder \
    src/libs/sdrdab/Resampler \
    src/libs/sdrdab/RingBuffer \
    src/libs/sdrdab/threading \
    src/libs/rscode

    DEPENDPATH += \
    src/libs/sdrdab \
    src/libs/sdrdab/AudioDecoder \
    src/libs/sdrdab/DataDecoder \
    src/libs/sdrdab/DataFeeder \
    src/libs/sdrdab/Resampler \
    src/libs/sdrdab/RingBuffer \
    src/libs/sdrdab/threading \
    src/libs/rscode

    HEADERS    += \
    src/libs/sdrdab/synchronizer_data.h \
    src/libs/sdrdab/threading/wrapper_functions.h \
    src/libs/sdrdab/threading/blocking_queue.h \
    src/libs/sdrdab/threading/scoped_lock.h \
    src/libs/sdrdab/threading/signaled_worker_thread.h \
    src/libs/sdrdab/data_format.h \
    src/libs/sdrdab/fft_engine.h \
    src/libs/sdrdab/DataDecoder/energy_dispersal.h \
    src/libs/sdrdab/DataDecoder/data_decoder_data.h \
    src/libs/sdrdab/DataDecoder/superframe.h \
    src/libs/sdrdab/DataDecoder/depuncturer.h \
    src/libs/sdrdab/DataDecoder/reed_solomon.h \
    src/libs/sdrdab/DataDecoder/deviterbi.h \
    src/libs/sdrdab/DataDecoder/extract_from_bitstream.h \
    src/libs/sdrdab/Resampler/resampler.h \
    src/libs/sdrdab/osx_compat.h \
    src/libs/sdrdab/RingBuffer/resampling_ring_buffer.h \
    src/libs/sdrdab/RingBuffer/ring_buffer.h \
    src/libs/sdrdab/synchronizer.h \
    src/libs/sdrdab/audio_decoder.h \
    src/libs/sdrdab/DataFeeder/abstract_data_feeder.h \
    src/libs/sdrdab/DataFeeder/rtl_data_feeder.h \
    src/libs/sdrdab/DataFeeder/file_data_feeder.h \
    src/libs/sdrdab/demodulator.h \
    src/libs/sdrdab/AudioDecoder/fake_sink.h \
    src/libs/sdrdab/AudioDecoder/file_sink.h \
    src/libs/sdrdab/AudioDecoder/null_sink.h \
    src/libs/sdrdab/AudioDecoder/file_src.h \
    src/libs/sdrdab/AudioDecoder/ring_src.h \
    src/libs/sdrdab/AudioDecoder/pulse_sink.h \
    src/libs/sdrdab/AudioDecoder/ogg_sink.h \
    src/libs/sdrdab/AudioDecoder/abstract_src.h \
    src/libs/sdrdab/AudioDecoder/player.h \
    src/libs/sdrdab/AudioDecoder/abstract_sink.h \
    src/libs/sdrdab/AudioDecoder/blocking_ring_buffer.h \
    src/libs/sdrdab/scheduler.h \
    src/libs/sdrdab/data_decoder.h \
    src/libs/rscode/ecc.h

    SOURCES    += \
    src/libs/sdrdab/threading/signaled_worker_thread.cc \
    src/libs/sdrdab/threading/scoped_lock.cc \
    src/libs/sdrdab/demodulator.cc \
    src/libs/sdrdab/DataDecoder/extract_from_bitstream.cc \
    src/libs/sdrdab/DataDecoder/deviterbi.cc \
    src/libs/sdrdab/DataDecoder/superframe.cc \
    src/libs/sdrdab/DataDecoder/depuncturer.cc \
    src/libs/sdrdab/DataDecoder/reed_solomon.cc \
    src/libs/sdrdab/DataDecoder/energy_dispersal.cc \
    src/libs/sdrdab/DataDecoder/galois_field_arithmetic.cc \
    src/libs/sdrdab/data_decoder.cc \
    src/libs/sdrdab/Resampler/resampler.cc \
    src/libs/sdrdab/fft_engine.cc \
    src/libs/sdrdab/RingBuffer/resampling_ring_buffer.cc \
    src/libs/sdrdab/DataFeeder/abstract_data_feeder.cc \
    src/libs/sdrdab/DataFeeder/rtl_data_feeder.cc \
    src/libs/sdrdab/DataFeeder/file_data_feeder.cc \
    src/libs/sdrdab/osx_compat.cc \
    src/libs/sdrdab/AudioDecoder/abstract_sink.cc \
    src/libs/sdrdab/AudioDecoder/blocking_ring_buffer.cc \
    src/libs/sdrdab/AudioDecoder/abstract_src.cc \
    src/libs/sdrdab/scheduler.cc \
    src/libs/sdrdab/synchronizer.cc \
    src/libs/rscode/galois.c \
    src/libs/rscode/rs.c \
    src/libs/rscode/berlekamp.c \
    src/libs/rscode/crcgen.c
}

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
}

!isEmpty(GITHASHSTRING) {
    message("Current git hash = $$GITHASHSTRING")
    DEFINES += GITHASH=\\\"$$GITHASHSTRING\\\"
}
else {
    warning("Can't get git hash.")
}
