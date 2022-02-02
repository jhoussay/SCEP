include(../SCEP.pri)

TEMPLATE = app
QT *= core gui widgets

INCLUDEPATH *= include
HEADERS += include/SCEP/SCEP.h

SOURCES += src/SCEP/main.cpp
