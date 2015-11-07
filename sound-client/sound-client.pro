#
TEMPLATE    = app
CONFIG      += console
QT          += core gui network  widgets

INCLUDEPATH += . 

HEADERS     = ./sound-client.h \
	      ./audiosink.h \
	      ./sound-constants.h \
	      ./ringbuffer.h 

SOURCES     =  ./sound-client.cpp ./audiosink.cpp main.cpp
TARGET      = soundClient
FORMS		+= ./soundwidget.ui

#for 32 bits windows we use
win32 {
DESTDIR     = ../../../windows-bin-dab
# includes in mingw differ from the includes in fedora linux
INCLUDEPATH += /usr/i686-mingw32/sys-root/mingw/include
LIBS	+= -lportaudio
LIBS	+= -lole32
LIBS	+= -lwinmm
}

unix{
DESTDIR     = ../linux-bin
LIBS	+= -lportaudio
}

