INCLUDEPATH += \
    $$PWD/../../shared \
    $$PWD/../../shared/python \
    $$PWD/../../shared/python/parser \
    $$PWD/../../shared/python/python-src

LIBS *= -l$$qtLibraryName(Python)
DEFINES += QT_CREATOR
