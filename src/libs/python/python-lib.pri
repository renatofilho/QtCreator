INCLUDEPATH += \
    $$PWD/python-src \
    $$PWD/python-src/Include

HEADERS += \
    $$PWD/parser/pythondriver.h \
    $$PWD/parser/memorypool.h \
    $$PWD/parser/locationtable.h \
    $$PWD/parser/ast.h \
    $$PWD/parser/astdefaultvisitor.h \
    $$PWD/parser/astvisitor.h \
    $$PWD/parser/astbuilder.h \
    $$PWD/parser/parsesession.h

SOURCES += \
    $$PWD/parser/pythondriver.cpp \
    $$PWD/parser/ast.cpp \
    $$PWD/parser/astdefaultvisitor.cpp \
    $$PWD/parser/astvisitor.cpp \
    $$PWD/parser/astbuilder.cpp \
    $$PWD/parser/parsesession.cpp
