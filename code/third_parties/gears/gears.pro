include(../../SCEP.pri)

TEMPLATE = lib
CONFIG += static
CONFIG -= QT

INCLUDEPATH *= .
HEADERS += gears/base/common/atl_headers_win32.h
HEADERS += gears/base/ie/atl_browser_headers.h
HEADERS += gears/base/ie/browser_listener.h

SOURCES += gears/base/ie/browser_listener.cc
