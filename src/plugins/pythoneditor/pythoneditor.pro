TEMPLATE = lib
TARGET = PythonEditor

include(../../qtcreatorplugin.pri)
include(pythoneditor_dependencies.pri)

DEFINES += QT_CREATOR

DESTDIR = $$IDE_PLUGIN_PATH/Nokia
PROVIDER = INdT
include(../../plugins/coreplugin/coreplugin.pri)

INCLUDEPATH += \
    $$PWD/../texteditor \
    $$PWD/../texteditor/generichighlighter

HEADERS += pythoneditor.h \
        pythoneditoreditable.h \
        pythoneditorconstants.h \
        pythoneditorplugin.h \
        pythoneditorfactory.h \
        pythoneditoractionhandler.h \
        pythonfilewizard.h \
        pythonhoverhandler.h

SOURCES += pythoneditor.cpp \
        pythoneditoreditable.cpp \
        pythoneditorplugin.cpp \
        pythoneditorfactory.cpp \
        pythoneditoractionhandler.cpp \
        pythonfilewizard.cpp \
        pythonhoverhandler.cpp

OTHER_FILES += PythonEditor.mimetypes.xml
RESOURCES += pythoneditor.qrc
