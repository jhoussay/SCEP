include(../SCEP.pri)

TEMPLATE = vclib

# Other libs
QT *= core gui widgets
LIBS *= -lUxTheme
include(../third_parties/linkollector-win/linkollector-win.prf)

# h
INCLUDEPATH *= include
HEADERS += include/SCEP_CORE/BrowserListener.h
HEADERS += include/SCEP_CORE/ExplorerWrapper.h
HEADERS += include/SCEP_CORE/Error.h
HEADERS += include/SCEP_CORE/Navigation.h
HEADERS += include/SCEP_CORE/SCEP_CORE.h
HEADERS += include/SCEP_CORE/win32_utils.h

# cpp
SOURCES += src/SCEP_CORE/BrowserListener.cpp
SOURCES += src/SCEP_CORE/ExplorerWrapper.cpp
SOURCES += src/SCEP_CORE/Navigation.cpp
SOURCES += src/SCEP_CORE/win32_utils.cpp

# preprocessor
DEFINES += SCEP_CORE_EXPORT
