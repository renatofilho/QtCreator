TEMPLATE = lib
CONFIG += dll
TARGET = Python
DEFINES += PYTHON_BUILD_LIB QT_CREATOR

include(../../qtcreatorlibrary.pri)
include(python-lib.pri)
include(../utils/utils.pri)
