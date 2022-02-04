CONFIG -= flat

exists( $$(TARGET).pro ): HEADERS += $$(TARGET).pro
exists( $$(TARGET).prf ): HEADERS += $$(TARGET).prf

ROOT_DIR = ../..
build_pass:CONFIG(debug, debug|release) {
    TYPE=debug
    DESTDIR = $$ROOT_DIR/bin/$$TYPE
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
    MOC_DIR = $$ROOT_DIR/tmp/$$TARGET/$$TYPE/moc
    OBJECTS_DIR = $$ROOT_DIR/tmp/$$TARGET/$$TYPE/obj
    UI_DIR = $$ROOT_DIR/tmp/$$TARGET/$$TYPE/ui
    RCC_DIR  = $$ROOT_DIR/tmp/$$TARGET/$$TYPE/rcc
    PRECOMPILED_DIR = $$ROOT_DIR/tmp/$$TARGET/$$TYPE/pch
}
