TEMPLATE = app
TARGET = welle-io
QT += network qml quick charts multimedia
CONFIG  += console
RC_ICONS = ./gui/QML/images/icon.ico

Release: QMAKE_CFLAGS	+=  -flto -ffast-math -O3
Release: QMAKE_CXXFLAGS	+=  -flto -ffast-math -O3
Release: QMAKE_LFLAGS	+=  -flto -O3

DEFINES	 += MOT_BASICS__ # Not necessary after code clean up

RESOURCES += gui/touch_gui_resource.qrc

DEPENDPATH += . \
	      ./src \
	      ./src/ofdm \
	      ./src/backend \
	      ./src/backend/audio \
	      ./src/backend/data \
	      ./src/output \
	      ./src/various \
	      ./src/input \
              ./src/ofdm \
              ./src/backend \
              ./src/backend/audio \
              ./src/backend/data \
              ./src/output \
              ./src/various \
              ./gui

INCLUDEPATH += . \
	      ./ \
	      ./src \
              ./src/ofdm \
              ./src/backend \
              ./src/backend/audio \
              ./src/backend/data \
              ./src/backend/data/journaline \
              ./src/output \
              ./src/various \
	      ./src/input \
              ./gui

# Input
HEADERS += ./src/dab-constants.h \
            ./src/ofdm/ofdm-processor.h \
            ./src/ofdm/ofdm-decoder.h \
            ./src/ofdm/phasereference.h \
            ./src/ofdm/phasetable.h \
            ./src/ofdm/freq-interleaver.h \
            ./src/backend/viterbi.h \
            ./src/backend/fic-handler.h \
            ./src/backend/msc-handler.h \
            ./src/backend/fib-processor.h  \
            ./src/backend/galois.h \
            ./src/backend/reed-solomon.h \
            ./src/backend/charsets.h \
            ./src/backend/firecode-checker.h \
            ./src/backend/dab-processor.h \
            ./src/backend/dab-virtual.h \
            ./src/backend/audio/dab-audio.h \
            ./src/backend/audio/mp2processor.h \
            ./src/backend/audio/mp4processor.h \
            ./src/backend/audio/faad-decoder.h \
            ./src/backend/data/dab-data.h \
            ./src/backend/data/data-processor.h \
            ./src/backend/data/pad-handler.h \
            ./src/backend/data/virtual-datahandler.h \
            ./src/backend/data/ip-datahandler.h \
            ./src/backend/data/mot-databuilder.h \
            ./src/backend/data/mot-data.h \
            ./src/backend/protection.h \
            ./src/backend/eep-protection.h \
            ./src/backend/uep-protection.h \
            ./src/output/CAudio.h \
            ./src/various/fft.h \
            ./src/various/ringbuffer.h \
            ./src/various/Xtan2.h \
            ./src/input/CVirtualInput.h \
            ./gui/gui.h \
            ./gui/motimageprovider.h \
            ./gui/stationlist.h \
            ./gui/tools.h \
            ./gui/fic_decoder.h \
            ./gui/pad_decoder.h \
            ./gui/mot_manager.h \
            ./gui/pad_decoder_adapter.h

SOURCES += ./main.cpp \
            ./src/ofdm/ofdm-processor.cpp \
            ./src/ofdm/ofdm-decoder.cpp \
            ./src/ofdm/phasereference.cpp \
            ./src/ofdm/phasetable.cpp \
            ./src/ofdm/freq-interleaver.cpp \
            ./src/backend/viterbi.cpp \
            ./src/backend/fic-handler.cpp \
            ./src/backend/msc-handler.cpp \
            ./src/backend/protection.cpp \
            ./src/backend/eep-protection.cpp \
            ./src/backend/uep-protection.cpp \
            ./src/backend/fib-processor.cpp  \
            ./src/backend/galois.cpp \
            ./src/backend/reed-solomon.cpp \
            ./src/backend/charsets.cpp \
            ./src/backend/firecode-checker.cpp \
            ./src/backend/dab-virtual.cpp \
            ./src/backend/dab-processor.cpp \
            ./src/backend/protTables.cpp \
            ./src/backend/audio/dab-audio.cpp \
            ./src/backend/audio/mp2processor.cpp \
            ./src/backend/audio/mp4processor.cpp \
            ./src/backend/data/pad-handler.cpp \
            ./src/backend/data/dab-data.cpp \
            ./src/backend/data/data-processor.cpp \
            ./src/backend/data/virtual-datahandler.cpp \
            ./src/backend/data/ip-datahandler.cpp \
            ./src/backend/data/mot-databuilder.cpp \
            ./src/backend/data/mot-data.cpp \
            ./src/output/CAudio.cpp \
            ./src/various/fft.cpp \
            ./src/various/Xtan2.cpp \
            ./gui/gui.cpp \
            ./gui/motimageprovider.cpp \
            ./gui/stationlist.cpp \
            ./gui/tools.cpp \
            ./gui/fic_decoder.cpp \
            ./gui/pad_decoder.cpp \
            ./gui/mot_manager.cpp \
            ./gui/pad_decoder_adapter.cpp

unix {
INCLUDEPATH	+= /usr/local/include
LIBS		+= -lfftw3f
LIBS		+= -lusb-1.0
LIBS		+= -ldl
LIBS		+= -lfaad
CONFIG		+= rtl_sdr
CONFIG		+= rtl_tcp
CONFIG          += rawfile
}


win32 {
INCLUDEPATH += ../welle.io-win-libs/include
LIBS		+= -L../welle.io-win-libs/x86
LIBS		+= -lfftw3f-3
LIBS		+= -lole32
LIBS		+= -lwinpthread
LIBS		+= -lwinmm
LIBS 		+= -lstdc++
LIBS		+= -lws2_32
LIBS		+= -llibfaad
LIBS		+= -lusb-1.0
CONFIG		+= rtl_tcp
CONFIG		+= rtl_sdr
CONFIG          += rawfile
}


#### Devices ####
rtl_sdr {
        DEFINES		+= HAVE_RTLSDR
        HEADERS		+= ./src/input/CRTL_SDR.h
        SOURCES		+= ./src/input/CRTL_SDR.cpp

        unix {
        LIBS            += -lrtlsdr
        }
}

rtl_tcp {
        DEFINES		+= HAVE_RTL_TCP
        HEADERS		+= ./src/input/CRTL_TCP_Client.h
        SOURCES		+= ./src/input/CRTL_TCP_Client.cpp
}

rawfile {
        DEFINES		+= HAVE_RAWFILE
        HEADERS		+= ./src/input/CRAWFile.h
        SOURCES		+= ./src/input/CRAWFile.cpp
}
