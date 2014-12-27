#-------------------------------------------------
#
# Project created by QtCreator 2014-12-25T18:11:08
#
#-------------------------------------------------

QT       += core gui xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = jisikstar
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    lzma/lzmadecoder.cpp \
    lzma/LzmaDec.c \
    lzma/LzmaLib.c \
    reader/dictdbindex1reader.cpp \
    reader/dictdbindex2reader.cpp \
    reader/mathdictreader.cpp \
    reader/powerbiblereader.cpp \
    reader/powertalkerreader.cpp \
    reader/util.cpp \
    converthandler.cpp

HEADERS  += mainwindow.h \
    lzma/include/LzmaDec.h \
    lzma/include/lzmadecoder.h \
    lzma/include/LzmaLib.h \
    lzma/include/Types.h \
    reader/include/defs.h \
    reader/include/dictdbindex1reader.h \
    reader/include/dictdbindex2reader.h \
    reader/include/mathdictreader.h \
    reader/include/powerbiblereader.h \
    reader/include/powertalkerreader.h \
    converthandler.h

INCLUDEPATH += lzma/include reader/include

FORMS    += mainwindow.ui
