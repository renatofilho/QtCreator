/*
 * Copyright (c) 2011 Instituto Nokia de Tecnologia
 *   Author: Paulo Alcantara <paulo.alcantara@openbossa.org>
 *
 * This software may be redistributed and/or modified under the terms of
 * the GNU General Public License ("GPL") version 2 as published by the
 * Free Software Foundation.
 */

#ifndef _PYTHONEDITORCONSTANTS_H_
#define _PYTHONEDITORCONSTANTS_H_

namespace PythonEditor {
namespace Constants {

const char * const M_CONTEXT        = "PythonEditor.ContextMenu";
const char * const C_PYTHONEDITOR   = "PythonPlugin.PythonEditor";
const char * const PYTHONEDITOR_ID  = "PythonPlugin.PythonEditor";
const char * const PYTHONEDITOR_DISPLAY_NAME =
    QT_TRANSLATE_NOOP("OpenWith::Editors", "Python Editor");
const char * const M_REFACTORING_MENU_INSERTION_POINT =
    "PythonEditor.RefactorGroup";
const char * const SEPARATOR1 = "PythonEditor.Separator1";
const char * const SEPARATOR2 = "PythonEditor.Separator2";
const char * const RENAME_SYMBOL_UNDER_CURSOR =
    "PythonEditor.RenameSymbolUnderCursor";
const char * const M_TOOLS_PYTHON = "PythonEditor.Tools.Menu";

const char * const PYTHON_SOURCE_MIMETYPE_0 = "application/x-python";
const char * const PYTHON_SOURCE_MIMETYPE_1 = "text/x-python";
const char * const WIZARD_CATEGORY_PYTHON = "C.PYTHON";
const char * const WIZARD_TR_CATEGORY_PYTHON =
    QT_TRANSLATE_NOOP("PythonEditor", "Python");

}
}

#endif /* _PYTHONEDITORCONSTANTS_H_ */
