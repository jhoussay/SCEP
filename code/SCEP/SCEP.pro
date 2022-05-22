include(../SCEP.pri)

TEMPLATE = app

# Other libs
QT *= core gui widgets
equals(QT_MAJOR_VERSION, 5) {
   QT *= winextras
}
LIBS *= -lShlwapi -lUxTheme
include(../third_parties/linkollector-win/linkollector-win.prf)

# h
INCLUDEPATH *= include
HEADERS += include/SCEP/SCEP.h
HEADERS += include/SCEP/Version.h
HEADERS += include/SCEP/Date.h
HEADERS += include/SCEP/ExplorerWidget.h
HEADERS += include/SCEP/ExplorerWrapper.h
HEADERS += include/SCEP/Error.h
HEADERS += include/SCEP/MainWindow.h
HEADERS += include/SCEP/Theme.h
HEADERS += include/SCEP/AboutDialog.h
HEADERS += include/SCEP/win32_utils.h
HEADERS += include/SCEP/Navigation.h
HEADERS += include/SCEP/HotKey.h
HEADERS += include/SCEP/BreadcrumbsAddressBar/BreadcrumbsAddressBar.h
HEADERS += include/SCEP/BreadcrumbsAddressBar/Layouts.h
HEADERS += include/SCEP/BreadcrumbsAddressBar/ModelViews.h
HEADERS += include/SCEP/BreadcrumbsAddressBar/Stylesheets.h

# cpp
SOURCES += src/SCEP/main.cpp
SOURCES += src/SCEP/ExplorerWidget.cpp
SOURCES += src/SCEP/ExplorerWrapper.cpp
SOURCES += src/SCEP/MainWindow.cpp
SOURCES += src/SCEP/Theme.cpp
SOURCES += src/SCEP/AboutDialog.cpp
SOURCES += src/SCEP/win32_utils.cpp
SOURCES += src/SCEP/Navigation.cpp
SOURCES += src/SCEP/HotKey.cpp
SOURCES += src/SCEP/BreadcrumbsAddressBar/BreadcrumbsAddressBar.cpp
SOURCES += src/SCEP/BreadcrumbsAddressBar/Layouts.cpp
SOURCES += src/SCEP/BreadcrumbsAddressBar/ModelViews.cpp

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
