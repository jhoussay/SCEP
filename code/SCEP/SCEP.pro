include(../SCEP.pri)

TEMPLATE = app
QT *= core gui widgets

INCLUDEPATH *= include
HEADERS += include/SCEP/SCEP.h
HEADERS += include/SCEP/ExplorerWidget.h
HEADERS += include/SCEP/ExplorerWrapper.h
HEADERS += include/SCEP/Error.h
HEADERS += include/SCEP/MainWindow.h
HEADERS += include/SCEP/Theme.h
HEADERS += include/SCEP/About.h

SOURCES += src/SCEP/main.cpp
SOURCES += src/SCEP/ExplorerWidget.cpp
SOURCES += src/SCEP/ExplorerWrapper.cpp
SOURCES += src/SCEP/MainWindow.cpp
SOURCES += src/SCEP/Theme.cpp
SOURCES += src/SCEP/About.cpp

FORMS += forms/SCEP/MainWindow.ui

RESOURCES += resources/SCEP.qrc

LIBS *= -lUxTheme

include(../third_parties/gears/gears.prf)