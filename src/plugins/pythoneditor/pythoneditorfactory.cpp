/*
 * Copyright (c) 2011 Instituto Nokia de Tecnologia
 *   Author: Paulo Alcantara <paulo.alcantara@openbossa.org>
 *
 * This software may be redistributed and/or modified under the terms of
 * the GNU General Public License ("GPL") version 2 as published by the
 * Free Software Foundation.
 */

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <QtGui/QMainWindow>

#include "pythoneditorfactory.h"
#include "pythoneditorconstants.h"
#include "pythoneditoreditable.h"
#include "pythoneditorplugin.h"

using namespace PythonEditor;
using namespace PythonEditor::Internal;
using namespace PythonEditor::Constants;

PyEditorFactory::PyEditorFactory(QObject *parent)
  : Core::IEditorFactory(parent)
{
    qDebug() << "PyEditorFactory::PyEditorFactory()";
    m_mimeTypes
        << QLatin1String(PythonEditor::Constants::PYTHON_SOURCE_MIMETYPE_0)
        << QLatin1String(PythonEditor::Constants::PYTHON_SOURCE_MIMETYPE_1)
        ;
}

PyEditorFactory::~PyEditorFactory()
{
}

QString PyEditorFactory::id(void) const
{
    return QLatin1String(PYTHONEDITOR_ID);
}

QString PyEditorFactory::displayName(void) const
{
    return tr(PYTHONEDITOR_DISPLAY_NAME);
}

Core::IFile *PyEditorFactory::open(const QString &fileName)
{
    Core::IEditor *iface =
        Core::EditorManager::instance()->openEditor(fileName, id());
    if (!iface) {
        qWarning()
            << "PyEditorFactory::open: openEditor failed for " << fileName;
        return 0;
    }

    return iface->file();
}

Core::IEditor *PyEditorFactory::createEditor(QWidget *parent)
{
    qDebug() << "PyEditorFactory::createEditor()";
    PythonEditor::PyTextEditorWidget *rc =
                    new PythonEditor::PyTextEditorWidget(parent);
    PyEditorPlugin::instance()->initializeEditor(rc);

    return rc->editor();
}

QStringList PyEditorFactory::mimeTypes() const
{
    return m_mimeTypes;
}

void PyEditorFactory::updateEditorInfoBar(Core::IEditor *)
{
}
