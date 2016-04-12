#-------------------------------------------------
#
# Project created by QtCreator 2016-04-05T23:01:45
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = MassSpecReader
TEMPLATE = app


SOURCES += main.cpp\
        readerwindow.cpp \
    qcustomplot.cpp

HEADERS  += readerwindow.h \
    qcustomplot.h

FORMS    += readerwindow.ui


RESOURCES += \
    mainresources.qrc
