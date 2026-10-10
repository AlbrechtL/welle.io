TEMPLATE = subdirs
SUBDIRS = src/welle-gui

#!android: {
#SUBDIRS += \
#    src/welle-cli
#}

DISTFILES += $$PWD/README.md

macx {
    QMAKE_INFO_PLIST = $$(PWD)/welle-io.plist
}
