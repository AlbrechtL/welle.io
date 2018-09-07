include(../backend.pri)

TEMPLATE = app
TARGET = welle-cli

HEADERS += \
    alsa-output.h  \
    webprogrammehandler.h \
    webradiointerface.h

SOURCES += \
    alsa-output.cpp \
    tests.cpp \
    webprogrammehandler.cpp \
    webradiointerface.cpp \
    welle-cli.cpp

unix:!macx:!android: {
    INCLUDEPATH	+= /usr/local/include
    LIBS    += -lfftw3f
    LIBS    += -lusb-1.0
    LIBS    += -ldl
    LIBS    += -lfaad
    LIBS    += -lmp3lame
    CONFIG  += airspy
    CONFIG  += rtl_sdr
}


win32 {
    INCLUDEPATH += ../../../welle.io-win-libs/include
    LIBS    += -L../../../welle.io-win-libs/x86
    LIBS    += -lfftw3f-3
    LIBS    += -lole32
    LIBS    += -lwinpthread
    LIBS    += -lwinmm
    LIBS    += -lstdc++
    LIBS    += -lws2_32
    LIBS    += -llibfaad
    LIBS    += -lmp3lame
    LIBS    += -lusb-1.0
    LIBS    += -lws2_32
}


macx { # not tested
    INCLUDEPATH	+= /opt/local/include
    INCLUDEPATH	+= /usr/local/include
    LIBS    += -L/opt/local/lib
    LIBS    += -L/usr/local/lib
    LIBS    += -lfftw3f
    LIBS    += -lusb-1.0
    LIBS    += -ldl
    LIBS    += -lfaad
}

# Include git hash into build
unix:!macx { # macOS is not tested, exclude it
    GITHASHSTRING = $$system(git rev-parse --short HEAD)
}

win32 {
    GITHASHSTRING = $$system(git.exe rev-parse --short HEAD)
}

!isEmpty(GITHASHSTRING) {
    message("Current git hash = $$GITHASHSTRING")
    DEFINES += GITDESCRIBE=\\\"$$GITHASHSTRING\\\"
}
else {
    warning("Can't get git hash.")
}
