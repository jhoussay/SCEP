include(../SCEP.pri)

TEMPLATE = app
QT *= core gui widgets

INCLUDEPATH *= include
HEADERS += include/SCEP/SCEP.h
HEADERS += include/SCEP/ExplorerWidget.h
HEADERS += include/SCEP/Error.h

SOURCES += src/SCEP/main.cpp
SOURCES += src/SCEP/ExplorerWidget.cpp
