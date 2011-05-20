/*
 * Copyright (c) 2011 Instituto Nokia de Tecnologia
 *   Author: Paulo Alcantara <paulo.alcantara@openbossa.org>
 *
 * This software may be redistributed and/or modified under the terms of
 * the GNU General Public License ("GPL") version 2 as published by the
 * Free Software Foundation.
 */

#ifndef _PYTHONEDITORACTIONHANDLER_H_
#define _PYTHONEDITORACTIONHANDLER_H_

#include <texteditor/texteditoractionhandler.h>

namespace PythonEditor {
namespace Internal {

class PyEditorActionHandler : public TextEditor::TextEditorActionHandler {
    Q_OBJECT

public:
    PyEditorActionHandler();

    void createActions(void);
};

}
}

#endif /* _PYTHONEDITORACTIONHANDLER_H_ */
