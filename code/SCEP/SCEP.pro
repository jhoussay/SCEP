include(../SCEP.pri)

TEMPLATE = app

# Other libs
QT *= core gui widgets
LIBS *= -lUxTheme
include(../third_parties/gears/gears.prf)
include(../third_parties/linkollector-win/linkollector-win.prf)

# h
INCLUDEPATH *= include
HEADERS += include/SCEP/SCEP.h
HEADERS += include/SCEP/ExplorerWidget.h
HEADERS += include/SCEP/ExplorerWidget2.h
HEADERS += include/SCEP/ExplorerWrapper.h
HEADERS += include/SCEP/ExplorerWrapper2.h
HEADERS += include/SCEP/Error.h
HEADERS += include/SCEP/MainWindow.h
HEADERS += include/SCEP/Theme.h
HEADERS += include/SCEP/AboutDialog.h
HEADERS += include/SCEP/win32_utils.h
HEADERS += include/SCEP/BreadcrumbAdressBar/BreadcrumbAdressBar.h
HEADERS += include/SCEP/BreadcrumbAdressBar/Layouts.h
HEADERS += include/SCEP/BreadcrumbAdressBar/ModelViews.h
HEADERS += include/SCEP/BreadcrumbAdressBar/Stylesheets.h

# cpp
SOURCES += src/SCEP/main.cpp
SOURCES += src/SCEP/ExplorerWidget.cpp
SOURCES += src/SCEP/ExplorerWidget2.cpp
SOURCES += src/SCEP/ExplorerWrapper.cpp
SOURCES += src/SCEP/ExplorerWrapper2.cpp
SOURCES += src/SCEP/MainWindow.cpp
SOURCES += src/SCEP/Theme.cpp
SOURCES += src/SCEP/AboutDialog.cpp
SOURCES += src/SCEP/win32_utils.cpp
SOURCES += src/SCEP/BreadcrumbAdressBar/BreadcrumbAdressBar.cpp
SOURCES += src/SCEP/BreadcrumbAdressBar/Layouts.cpp
SOURCES += src/SCEP/BreadcrumbAdressBar/ModelViews.cpp

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



