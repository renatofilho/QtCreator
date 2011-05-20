/*
 * Copyright (c) 2011 Instituto Nokia de Tecnologia
 *   Author: Paulo Alcantara <paulo.alcantara@openbossa.org>
 *
 * This software may be redistributed and/or modified under the terms of
 * the GNU General Public License ("GPL") version 2 as published by the
 * Free Software Foundation.
 */

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/helpmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>

#include <QtGui/QTextCursor>
#include <QtCore/QUrl>
#include <QtCore/QDebug>

#include "pythonhoverhandler.h"
#include "pythoneditoreditable.h"
#include "pythoneditor.h"

using namespace PythonEditor;
using namespace PythonEditor::Internal;
using namespace Core;

PyHoverHandler::PyHoverHandler(QObject *parent)
    : BaseHoverHandler(parent)
{
}

PyHoverHandler::~PyHoverHandler()
{
}

bool PyHoverHandler::acceptEditor(IEditor *editor)
{
    if (qobject_cast<PyEditorEditable *>(editor) != 0)
        return true;

    return false;
}

void PyHoverHandler::identifyMatch(TextEditor::ITextEditor *editor, int pos)
{
    qDebug() << "PyHoverHandler::identifyMatch()";

    if (PyTextEditorWidget *glslEditor = qobject_cast<PyTextEditorWidget *>(editor->widget())) {
        if (! glslEditor->extraSelectionTooltip(pos).isEmpty()) {
            setToolTip(glslEditor->extraSelectionTooltip(pos));
        }
    }
}

void PyHoverHandler::decorateToolTip(void)
{
    if (Qt::mightBeRichText(toolTip()))
        setToolTip(Qt::escape(toolTip()));
}
