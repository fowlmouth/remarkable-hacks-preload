######################################################################
# rmhacks — LD_PRELOAD based hacks for reMarkable xochitl (rm2)
#
# Pure interposition + runtime-address calls; no binary patching.
# Build with the official reMarkable SDK (see README.md).
######################################################################

TEMPLATE = lib
TARGET = rmhacks
VERSION = 0.1.0
INCLUDEPATH += . src

CONFIG += c++17
QT += core gui

# interposed symbols must stay exported
DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    src/main.cpp \
    src/xochitl.cpp \
    src/gestures.cpp \
    src/actions.cpp

HEADERS += \
    src/xochitl.h \
    src/gestures.h \
    src/actions.h

QMAKE_CXXFLAGS += -fvisibility=hidden
LIBS += -ldl
