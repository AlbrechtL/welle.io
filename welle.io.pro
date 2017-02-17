TEMPLATE = app
TARGET = welle-io
QT += network qml quick charts multimedia
CONFIG  += console
RC_ICONS = ./gui/QML/images/icon.ico

Release: QMAKE_CFLAGS	+=  -flto -ffast-math -O3
Release: QMAKE_CXXFLAGS	+=  -flto -ffast-math -O3
Release: QMAKE_LFLAGS	+=  -flto -O3

DEFINES	 += GUI_3 # Not necessary after code clean up
DEFINES	 += MOT_BASICS__ # Not necessary after code clean up

RESOURCES += gui/touch_gui_resource.qrc

DEPENDPATH += . \
	      ./src \
	      ./includes \
	      ./src/ofdm \
	      ./src/backend \
	      ./src/backend/audio \
	      ./src/backend/data \
	      ./src/output \
	      ./src/various \
	      ./src/input \
	      ./includes/ofdm \
	      ./includes/backend \
	      ./includes/backend/audio \
	      ./includes/backend/data \
	      ./includes/output \
              ./includes/various \
              ./gui

INCLUDEPATH += . \
	      ./ \
	      ./src \
	      ./includes \
	      ./includes/ofdm \
	      ./includes/backend \
	      ./includes/backend/audio \
	      ./includes/backend/data \
	      ./includes/backend/data/journaline \
	      ./includes/output \
	      ./includes/various \
	      ./src/input \
              ./gui

# Input
HEADERS += ./includes/dab-constants.h \
            ./includes/ofdm/ofdm-processor.h \
            ./includes/ofdm/ofdm-decoder.h \
            ./includes/ofdm/phasereference.h \
            ./includes/ofdm/phasetable.h \
            ./includes/ofdm/freq-interleaver.h \
            ./includes/backend/viterbi.h \
            ./includes/backend/fic-handler.h \
            ./includes/backend/msc-handler.h \
            ./includes/backend/fib-processor.h  \
            ./includes/backend/galois.h \
            ./includes/backend/reed-solomon.h \
            ./includes/backend/charsets.h \
            ./includes/backend/firecode-checker.h \
            ./includes/backend/dab-processor.h \
            ./includes/backend/dab-virtual.h \
            ./includes/backend/audio/dab-audio.h \
            ./includes/backend/audio/mp2processor.h \
            ./includes/backend/audio/mp4processor.h \
            ./includes/backend/audio/faad-decoder.h \
            ./includes/backend/data/dab-data.h \
            ./includes/backend/data/data-processor.h \
            ./includes/backend/data/pad-handler.h \
            ./includes/backend/data/virtual-datahandler.h \
            ./includes/backend/data/ip-datahandler.h \
            ./includes/backend/data/mot-databuilder.h \
            ./includes/backend/data/mot-data.h \
            ./includes/backend/protection.h \
            ./includes/backend/eep-protection.h \
            ./includes/backend/uep-protection.h \
            ./includes/output/CAudio.h \
            ./includes/various/fft.h \
            ./includes/various/ringbuffer.h \
            ./includes/various/Xtan2.h \
            ./src/input/virtual-input.h \
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
            ./src/input/virtual-input.cpp \
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
CONFIG		+= dabstick
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
CONFIG		+= dabstick
CONFIG          += rawfile
}


#### Devices ####
dabstick {
	DEFINES		+= HAVE_DABSTICK
	INCLUDEPATH	+= ./src/input/dabstick
	HEADERS		+= ./src/input/dabstick/dabstick.h \
	                   ./src/input/dabstick/dongleselect.h
	SOURCES		+= ./src/input/dabstick/dabstick.cpp \
	                   ./src/input/dabstick/dongleselect.cpp
	FORMS		+= ./src/input/dabstick/dabstick-widget.ui
}

rtl_tcp {
	DEFINES		+= HAVE_RTL_TCP
	INCLUDEPATH	+= ./src/input/rtl_tcp
	HEADERS		+= ./src/input/rtl_tcp/rtl_tcp_client.h
	SOURCES		+= ./src/input/rtl_tcp/rtl_tcp_client.cpp
}

rawfile {
        DEFINES		+= HAVE_RAWFILE
        INCLUDEPATH	+= ./src/input/rawfile
        HEADERS		+= ./src/input/rawfile/rawfile.h
        SOURCES		+= ./src/input/rawfile/rawfile.cpp
}
