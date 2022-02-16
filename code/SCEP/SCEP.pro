include(../SCEP.pri)

TEMPLATE = app

# Other libs
QT *= core gui widgets
LIBS *= -lUxTheme
include(../third_parties/gears/gears.prf)

# h
INCLUDEPATH *= include
HEADERS += include/SCEP/SCEP.h
HEADERS += include/SCEP/ExplorerWidget.h
HEADERS += include/SCEP/ExplorerWrapper.h
HEADERS += include/SCEP/Error.h
HEADERS += include/SCEP/MainWindow.h
HEADERS += include/SCEP/Theme.h
HEADERS += include/SCEP/AboutDialog.h

# cpp
SOURCES += src/SCEP/main.cpp
SOURCES += src/SCEP/ExplorerWidget.cpp
SOURCES += src/SCEP/ExplorerWrapper.cpp
SOURCES += src/SCEP/MainWindow.cpp
SOURCES += src/SCEP/Theme.cpp
SOURCES += src/SCEP/AboutDialog.cpp

# ui
FORMS += forms/SCEP/MainWindow.ui
FORMS += forms/SCEP/AboutDialog.ui

# qrc
RESOURCES += resources/SCEP.qrc

# Windows resources files
include(../../VERSION)
VERSION = $${SCEP_MAJ_VERSION}.$${SCEP_MIN_VERSION}.$${SCEP_PATCH_VERSION}
RC_ICONS = resources/SCEP.ico
QMAKE_TARGET_DESCRIPTION = "SCEP is a multi tab file explorer for Windows."
QMAKE_TARGET_PRODUCT = SCEP
#QMAKE_TARGET_COPYRIGHT =



