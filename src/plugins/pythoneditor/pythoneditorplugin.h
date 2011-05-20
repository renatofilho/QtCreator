/*
 * Copyright (c) 2011 Instituto Nokia de Tecnologia
 *   Author: Paulo Alcantara <paulo.alcantara@openbossa.org>
 *
 * This software may be redistributed and/or modified under the terms of
 * the GNU General Public License ("GPL") version 2 as published by the
 * Free Software Foundation.
 */

#ifndef _PYTHONEDITORPLUGIN_H_
#define _PYTHONEDITORPLUGIN_H_

#include <extensionsystem/iplugin.h>
#include <coreplugin/icontext.h>
#include <QtCore/QPointer>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QTimer)

namespace TextEditor {
class TextEditorActionHandler;
}

namespace Core {
class Command;
class ActionContainer;
class ActionManager;
}

namespace TextEditor {
class ITextEditor;
}

namespace PythonEditor {

class PyTextEditorWidget;

namespace Internal {

class PyEditorFactory;

class PyEditorPlugin : public ExtensionSystem::IPlugin {
    Q_OBJECT

public:
    PyEditorPlugin();
    virtual ~PyEditorPlugin();

    /* IPlugin */
    bool initialize(const QStringList &arguments, QString *error_message = 0);
    void extensionsInitialized(void);
    ShutdownFlag aboutToShutdown(void);

    static PyEditorPlugin *instance(void) { return m_instance; }

    void initializeEditor(PythonEditor::PyTextEditorWidget *editor);

private:
    QByteArray pyFile(const QString &fileName);
    Core::Command *addToolAction(QAction *a, Core::ActionManager *am,
                                    Core::Context &context, const QString &name,
                                    Core::ActionContainer *c1,
                                    const QString &keySequence);

    static PyEditorPlugin *m_instance;

    PyEditorFactory *m_editor;
    TextEditor::TextEditorActionHandler *m_actionHandler;

    QPointer<TextEditor::ITextEditor> m_currentTextEditable;
};

}
}

#endif /* _PYTHONEDITORPLUGIN_H_ */
