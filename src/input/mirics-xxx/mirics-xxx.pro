TEMPLATE    = lib
CONFIG      += plugin
QT          += core gui widgets

INCLUDEPATH += . \
	       ../ ../.. \
	       ../../../includes \
	       ../../../includes/filters \
	       ../../../includes/various \
	       ./mirics-lib ./mirics-lib/convert
HEADERS     = ../rig-interface.h \
	      ./mirics-xxx.h \
	      ../../../includes/filters/fir-filters.h \
	      ../../../includes/swradio-constants.h \
	      ./mirisdr.h
SOURCES     =  ./mirics-xxx.cpp \
	       ./libmirisdr.c \
	       ../../../src/filters/fir-filters.cpp
TARGET      = $$qtLibraryTarget(device_mirics)
FORMS	+= ./widget.ui

win32 {
DESTDIR     = ../../../../../../windows32-bin/input-plugins-sw
# includes in mingw differ from the includes in fedora linux
INCLUDEPATH += /usr/i686-w64-mingw32/sys-root/mingw/include
#INCLUDEPATH += /usr/i686-w64-mingw32/sys-root/mingw/include/qwt
LIBS	+= -lusb-1.0
LIBS	+= -lole32
LIBS	+= -lwinmm
}

unix{
DESTDIR     = ../../../../../../linux-bin/input-plugins-sw
}

