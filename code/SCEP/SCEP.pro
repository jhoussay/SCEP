include(../SCEP.pri)

TEMPLATE = app

# Other libs
QT *= core gui widgets

include(../SCEP_CORE/SCEP_CORE.prf)
include(../third_parties/linkollector-win/linkollector-win.prf)

# h
INCLUDEPATH *= include
HEADERS += include/SCEP/SCEP.h
HEADERS += include/SCEP/ExplorerWidget.h
HEADERS += include/SCEP/ExplorerWidget2.h
HEADERS += include/SCEP/ExplorerWrapper2.h
HEADERS += include/SCEP/MainWindow.h
HEADERS += include/SCEP/Theme.h
HEADERS += include/SCEP/AboutDialog.h
HEADERS += include/SCEP/BreadcrumbsAddressBar/BreadcrumbsAddressBar.h
HEADERS += include/SCEP/BreadcrumbsAddressBar/Layouts.h
HEADERS += include/SCEP/BreadcrumbsAddressBar/ModelViews.h
HEADERS += include/SCEP/BreadcrumbsAddressBar/Stylesheets.h

# cpp
SOURCES += src/SCEP/main.cpp
SOURCES += src/SCEP/ExplorerWidget.cpp
SOURCES += src/SCEP/ExplorerWidget2.cpp
SOURCES += src/SCEP/ExplorerWrapper2.cpp
SOURCES += src/SCEP/MainWindow.cpp
SOURCES += src/SCEP/Theme.cpp
SOURCES += src/SCEP/AboutDialog.cpp
SOURCES += src/SCEP/BreadcrumbsAddressBar/BreadcrumbsAddressBar.cpp
SOURCES += src/SCEP/BreadcrumbsAddressBar/Layouts.cpp
SOURCES += src/SCEP/BreadcrumbsAddressBar/ModelViews.cpp

# ui
FORMS += forms/SCEP/MainWindow.ui
FORMS += forms/SCEP/AboutDialog.ui

# qrc & rc
RESOURCES += resources/SCEP.qrc
RC_ICONS = resources/SCEP.ico
