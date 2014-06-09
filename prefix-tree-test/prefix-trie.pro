TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.c \
    prefix-tree.c \
    mm-block.c \
    mm-array.c

HEADERS += \
    prefix-tree.h \
    mm-block.h \
    mm-array.h

OTHER_FILES +=

