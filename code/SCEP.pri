CONFIG -= flat

exists($$_PRO_FILE_PWD_/$${TARGET}.pro): HEADERS += $$_PRO_FILE_PWD_/$${TARGET}.pro
exists($$_PRO_FILE_PWD_/$${TARGET}.prf): HEADERS += $$_PRO_FILE_PWD_/$${TARGET}.prf

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
