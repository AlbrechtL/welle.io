TEMPLATE = app
TARGET = welle-io
QT += network qml quick charts multimedia
CONFIG  += console
RC_ICONS = ./icon.ico

Release: QMAKE_CFLAGS	+=  -flto -ffast-math -O3
Release: QMAKE_CXXFLAGS	+=  -flto -ffast-math -O3
Release: QMAKE_LFLAGS	+=  -flto -O3

RESOURCES += ./touch_gui_resource.qrc

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
              ./src/gui

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
              ./src/gui

# Input
HEADERS += ./src/DabConstants.h \
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
    ./src/backend/audio/neaacdec.h \
    ./src/backend/data/pad_decoder.h \
    ./src/backend/data/mot_manager.h \
    ./src/backend/data/pad_decoder_adapter.h \
    ./src/backend/tools.h \
    ./src/backend/protection.h \
    ./src/backend/eep-protection.h \
    ./src/backend/uep-protection.h \
    ./src/output/CAudio.h \
    ./src/various/fft.h \
    ./src/various/ringbuffer.h \
    ./src/various/Xtan2.h \
    ./src/input/CVirtualInput.h \
    ./src/gui/gui.h \
    ./src/gui/motimageprovider.h \
    ./src/gui/stationlist.h \
    ./src/input/CInputFactory.h \
    ./src/input/CNullDevice.h \
    ./src/input/CRAWFile.h \
    ./src/input/CRTL_TCP_Client.h \
    ./src/MathHelper.h

SOURCES += ./src/main.cpp \
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
    ./src/backend/data/pad_decoder.cpp \
    ./src/backend/data/mot_manager.cpp \
    ./src/backend/data/pad_decoder_adapter.cpp \
    ./src/backend/tools.cpp \
    ./src/output/CAudio.cpp \
    ./src/various/fft.cpp \
    ./src/various/Xtan2.cpp \
    ./src/gui/gui.cpp \
    ./src/gui/motimageprovider.cpp \
    ./src/gui/stationlist.cpp \
    ./src/input/CInputFactory.cpp \
    ./src/input/CNullDevice.cpp \
    ./src/input/CRAWFile.cpp \
    ./src/input/CRTL_TCP_Client.cpp \
    ./src/DabConstants.cpp

unix {
INCLUDEPATH	+= /opt/local/include
LIBS            += -L/opt/local/lib
LIBS		+= -lfftw3f
LIBS		+= -lusb-1.0
LIBS		+= -ldl
LIBS		+= -lfaad
CONFIG          += airspy
CONFIG		+= rtl_sdr
#CONFIG      += soapysdr
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
CONFIG		+= rtl_sdr
CONFIG          += airspy
}


#### Devices ####
airspy {
        DEFINES		+= HAVE_AIRSPY
        HEADERS		+= ./src/input/CAirspy.h
        SOURCES		+= ./src/input/CAirspy.cpp

        # The same lib for unix and Windows
        LIBS            += -lairspy
}

rtl_sdr {
        DEFINES		+= HAVE_RTLSDR
        HEADERS		+= ./src/input/CRTL_SDR.h
        SOURCES		+= ./src/input/CRTL_SDR.cpp

        # The same lib for unix and Windows
        LIBS            += -lrtlsdr       
}

#<<<<<<< HEAD
#DISTFILES += \
#    README.md
#=======
#soapysdr {
#        DEFINES		+= HAVE_SOAPYSDR
#        HEADERS		+= ./src/input/CSoapySdr.h
#        SOURCES		+= ./src/input/CSoapySdr.cpp

#        # The same lib for unix and Windows
#        LIBS            += -lSoapySDR
#}
#>>>>>>> 7cea1ad71faf5c86d587b40819b2ef7e003d40d0

DISTFILES += \
    README.md
