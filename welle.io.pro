TEMPLATE = app
TARGET = welle-io
QT += widgets network qml quick charts
CONFIG  += console
RC_ICONS = ./gui/QML/images/icon.ico

Release: QMAKE_CFLAGS	+=  -flto -ffast-math -O3
Release: QMAKE_CXXFLAGS	+=  -flto -ffast-math -O3
Release: QMAKE_LFLAGS	+=  -flto -O3
#Debug: QMAKE_CFLAGS	+=  -g
#Debug: QMAKE_CXXFLAGS	+=  -g
#Debug: QMAKE_LFLAGS	+=  -g

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
	      ./src/backend/data/journaline \
	      ./src/output \
	      ./src/various \
	      ./src/input \
	      ./src/input/rawfiles \
	      ./src/input/wavfiles \
	      ./includes/ofdm \
	      ./includes/backend \
	      ./includes/backend/audio \
	      ./includes/backend/data \
	      ./includes/backend/data/journaline \
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
	      ./src/input/rawfiles \
              ./src/input/wavfiles \
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
            ./includes/backend/data/journaline-datahandler.h \
            ./includes/backend/data/journaline/dabdatagroupdecoder.h \
            ./includes/backend/data/journaline/crc_8_16.h \
            ./includes/backend/data/journaline/log.h \
            ./includes/backend/data/journaline/newssvcdec_impl.h \
            ./includes/backend/data/journaline/Splitter.h \
            ./includes/backend/data/journaline/dabdgdec_impl.h \
            ./includes/backend/data/journaline/newsobject.h \
            ./includes/backend/data/journaline/NML.h \
            ./includes/backend/protection.h \
            ./includes/backend/eep-protection.h \
            ./includes/backend/uep-protection.h \
            ./includes/output/audio-base.h \
            ./includes/output/newconverter.h \
            ./includes/output/audiosink.h \
            ./includes/output/fir-filters.h \
            ./includes/various/fft.h \
            ./includes/various/ringbuffer.h \
            ./includes/various/Xtan2.h \
            ./src/input/virtual-input.h \
            ./src/input/rawfiles/rawfiles.h \
            ./src/input/wavfiles/wavfiles.h \
            ./gui/gui.h \
            ./gui/motimageprovider.h \
            ./gui/stationlist.h \
            ./gui/tools.h \
            ./gui/fic_decoder.h \
            ./gui/pad_decoder.h \
            ./gui/mot_manager.h \
            ./gui/pad_decoder_adapter.h

FORMS +=	./src/input/filereader-widget.ui 

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
            ./src/backend/data/journaline-datahandler.cpp \
            ./src/backend/data/journaline/crc_8_16.c \
            ./src/backend/data/journaline/log.c \
            ./src/backend/data/journaline/newssvcdec_impl.cpp \
            ./src/backend/data/journaline/Splitter.cpp \
            ./src/backend/data/journaline/dabdgdec_impl.c \
            ./src/backend/data/journaline/newsobject.cpp \
            ./src/backend/data/journaline/NML.cpp \
            ./src/output/audio-base.cpp \
            ./src/output/newconverter.cpp \
            ./src/output/audiosink.cpp \
            ./src/output/fir-filters.cpp \
            ./src/various/fft.cpp \
            ./src/various/Xtan2.cpp \
            ./src/input/virtual-input.cpp \
            ./src/input/rawfiles/rawfiles.cpp \
            ./src/input/wavfiles/wavfiles.cpp \
            ./gui/gui.cpp \
            ./gui/motimageprovider.cpp \
            ./gui/stationlist.cpp \
            ./gui/tools.cpp \
            ./gui/fic_decoder.cpp \
            ./gui/pad_decoder.cpp \
            ./gui/mot_manager.cpp \
            ./gui/pad_decoder_adapter.cpp

unix {
CONFIG		+= dabstick
#CONFIG		+= sdrplay-exp
#CONFIG		+= sdrplay
CONFIG		+= rtl_tcp
#CONFIG		+= airspy
DESTDIR		= ./linux-bin
INCLUDEPATH	+= /usr/local/include
LIBS		+= -lfftw3f  -lusb-1.0 -ldl  #
LIBS		+= -lportaudio
LIBS		+= -lz
LIBS		+= -lsamplerate
LIBS		+= -lfaad
}


win32 {
INCLUDEPATH += ../welle.io-win-libs/include
LIBS		+= -L../welle.io-win-libs/x86
LIBS		+= -lfftw3f-3
LIBS		+= -lportaudio_x86
LIBS		+= -lole32
LIBS		+= -lwinpthread
LIBS		+= -lwinmm
LIBS 		+= -lstdc++
LIBS		+= -lws2_32
LIBS		+= -llibfaad
LIBS		+= -lusb-1.0
LIBS		+= -llibsamplerate
LIBS		+= -lzlib
CONFIG		+= extio
CONFIG		+= airspy
CONFIG		+= rtl_tcp
CONFIG		+= dabstick
CONFIG		+= sdrplay-exp
#CONFIG		+= tcp-streamer
}


#### Devices ####
dabstick {
	DEFINES		+= HAVE_DABSTICK
	INCLUDEPATH	+= /home/jan/rtl-sdr/include
	INCLUDEPATH	+= ./src/input/dabstick
	HEADERS		+= ./src/input/dabstick/dabstick.h \
	                   ./src/input/dabstick/dongleselect.h
	SOURCES		+= ./src/input/dabstick/dabstick.cpp \
	                   ./src/input/dabstick/dongleselect.cpp
	FORMS		+= ./src/input/dabstick/dabstick-widget.ui
}

sdrplay {
	DEFINES		+= HAVE_SDRPLAY
	INCLUDEPATH	+= ./src/input/sdrplay
	HEADERS		+= ./src/input/sdrplay/sdrplay.h \
	                   ./src/input/sdrplay/sdrplay-loader.h \
	                   ./src/input/sdrplay/sdrplay-worker.h 
	SOURCES		+= ./src/input/sdrplay/sdrplay.cpp \
	                   ./src/input/sdrplay/sdrplay-loader.cpp \
	                   ./src/input/sdrplay/sdrplay-worker.cpp 
	FORMS		+= ./src/input/sdrplay/sdrplay-widget.ui
}

sdrplay-exp {
	DEFINES		+= HAVE_SDRPLAY
	INCLUDEPATH	+= ./src/input/sdrplay-exp
	HEADERS		+= ./src/input/sdrplay-exp/sdrplay.h 
	SOURCES		+= ./src/input/sdrplay-exp/sdrplay.cpp 
	FORMS		+= ./src/input/sdrplay-exp/sdrplay-widget.ui
}

airspy {
	DEFINES		+= HAVE_AIRSPY
	INCLUDEPATH	+= ./src/input/airspy \
	                    /usr/local/include/libairspy
	HEADERS		+= ./src/input/airspy/airspy-handler.h \
	                    /usr/local/include/libairspy/airspy.h
	SOURCES		+= ./src/input/airspy/airspy-handler.cpp 
	FORMS		+= ./src/input/airspy/airspy-widget.ui
}

tcp-streamer	{
	DEFINES		+= TCP_STREAMER
	QT		+= network
	HEADERS		+= ./includes/output/tcp-streamer.h
	SOURCES		+= ./src/output/tcp-streamer.cpp
}

rtl_tcp {
	DEFINES		+= HAVE_RTL_TCP
	QT		+= network
	INCLUDEPATH	+= ./src/input/rtl_tcp
	HEADERS		+= ./src/input/rtl_tcp/rtl_tcp_client.h
	SOURCES		+= ./src/input/rtl_tcp/rtl_tcp_client.cpp
	FORMS		+= ./src/input/rtl_tcp/rtl_tcp-widget.ui
}
