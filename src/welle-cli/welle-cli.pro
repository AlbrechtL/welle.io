include(../backend.pri)

TEMPLATE = app
TARGET = welle-cli
CONFIG += console

HEADERS += \
    alsa-output.h  \
    webprogrammehandler.h \
    webradiointerface.h \
    jsonconvert.h

SOURCES += \
    alsa-output.cpp \
    tests.cpp \
    webprogrammehandler.cpp \
    webradiointerface.cpp \
    jsonconvert.cpp \
    welle-cli.cpp

# Include git hash into build
unix: {
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
