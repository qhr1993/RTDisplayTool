#-------------------------------------------------
#
# Project created by QtCreator 2016-06-14T09:06:27
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

LIBS += -L"/usr/local/lib" -lfftw3 -lm

TARGET = RTDispProj
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    qcustomplot.cpp \
    rtreadingthread.cpp

HEADERS  += mainwindow.h \
    qcustomplot.h \
    rtreadingthread.h \
    sharedcontrol.h

FORMS    += mainwindow.ui
CONFIG += debug_and_release
