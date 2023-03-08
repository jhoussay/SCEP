CONFIG -= flat
CONFIG += c++17

exists($$_PRO_FILE_PWD_/$${TARGET}.pro): HEADERS += $$_PRO_FILE_PWD_/$${TARGET}.pro
exists($$_PRO_FILE_PWD_/$${TARGET}.prf): HEADERS += $$_PRO_FILE_PWD_/$${TARGET}.prf

DEFINES *= NOMINMAX

ROOT_DIR = $$PWD/..
build_pass:CONFIG(debug, debug|release) {
    TYPE=debug
    DESTDIR = $$ROOT_DIR/bin/$$TYPE
    LIBS *= -L$$DESTDIR
    MOC_DIR = $$ROOT_DIR/tmp/$$TARGET/$$TYPE/moc
    OBJECTS_DIR = $$ROOT_DIR/tmp/$$TARGET/$$TYPE/obj
    UI_DIR = $$ROOT_DIR/tmp/$$TARGET/$$TYPE/ui
    RCC_DIR  = $$ROOT_DIR/tmp/$$TARGET/$$TYPE/rcc
    PRECOMPILED_DIR = $$ROOT_DIR/tmp/$$TARGET/$$TYPE/pch
    # Console
    CONFIG *= console
    # Last !
    TARGET = $$join(TARGET,,,_d)
} else {
    TYPE=release
    DESTDIR = $$ROOT_DIR/bin/$$TYPE
    LIBS *= -L$$DESTDIR
    MOC_DIR = $$ROOT_DIR/tmp/$$TARGET/$$TYPE/moc
    OBJECTS_DIR = $$ROOT_DIR/tmp/$$TARGET/$$TYPE/obj
    UI_DIR = $$ROOT_DIR/tmp/$$TARGET/$$TYPE/ui
    RCC_DIR  = $$ROOT_DIR/tmp/$$TARGET/$$TYPE/rcc
    PRECOMPILED_DIR = $$ROOT_DIR/tmp/$$TARGET/$$TYPE/pch
}

# Windows resources files
include(../VERSION)
VERSION = $${SCEP_MAJ_VERSION}.$${SCEP_MIN_VERSION}.$${SCEP_PATCH_VERSION}

QMAKE_TARGET_DESCRIPTION = "SCEP is a multi tab file explorer for Windows."
QMAKE_TARGET_PRODUCT = SCEP
#QMAKE_TARGET_COPYRIGHT =
