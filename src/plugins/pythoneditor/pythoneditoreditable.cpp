/*
 * Copyright (c) 2011 Instituto Nokia de Tecnologia
 *   Author: Paulo Alcantara <paulo.alcantara@openbossa.org>
 *
 * This software may be redistributed and/or modified under the terms of
 * the GNU General Public License ("GPL") version 2 as published by the
 * Free Software Foundation.
 */


#include <coreplugin/mimedatabase.h>
#include <coreplugin/icore.h>
#include <coreplugin/designmode.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/coreconstants.h>
#include <texteditor/texteditorconstants.h>
#include <projectexplorer/projectexplorerconstants.h>

#include "pythoneditoreditable.h"
#include "pythoneditor.h"
#include "pythoneditorconstants.h"

namespace PythonEditor {
namespace Internal {

PyEditorEditable::PyEditorEditable(PyTextEditorWidget *editor)
    : BaseTextEditor(editor)
{
    setContext(Core::Context(PythonEditor::Constants::PYTHONEDITOR_ID,
                             TextEditor::Constants::C_TEXTEDITOR));
}

QString PyEditorEditable::preferredModeType(void) const
{
    return QString();
}

}
}
