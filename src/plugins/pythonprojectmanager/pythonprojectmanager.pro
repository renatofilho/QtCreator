TEMPLATE = lib
TARGET = PythonProjectManager

QT += declarative

include(../../qtcreatorplugin.pri)
include(pythonprojectmanager_dependencies.pri)

DEFINES += PYTHONPROJECTMANAGER_LIBRARY
HEADERS += pythonappwizard.h

SOURCES += pythonappwizard.cpp

RESOURCES += pythonproject.qrc
