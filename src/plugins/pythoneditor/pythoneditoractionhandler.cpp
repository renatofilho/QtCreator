/*
 * Copyright (c) 2011 Instituto Nokia de Tecnologia
 *   Author: Paulo Alcantara <paulo.alcantara@openbossa.org>
 *
 * This software may be redistributed and/or modified under the terms of
 * the GNU General Public License ("GPL") version 2 as published by the
 * Free Software Foundation.
 */

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>

#include <QtCore/QDebug>
#include <QtGui/QAction>
#include <QtGui/QMainWindow>
#include <QtGui/QMessageBox>

#include "pythoneditoractionhandler.h"
#include "pythoneditorconstants.h"
#include "pythoneditor.h"

namespace PythonEditor {
namespace Internal {

PyEditorActionHandler::PyEditorActionHandler()
  : TextEditor::TextEditorActionHandler(
                    PythonEditor::Constants::PYTHONEDITOR_ID, Format)
{
}

void PyEditorActionHandler::createActions(void)
{
    TextEditor::TextEditorActionHandler::createActions();
}

}
}
