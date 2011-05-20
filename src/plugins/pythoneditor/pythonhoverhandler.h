/*
 * Copyright (c) 2011 Instituto Nokia de Tecnologia
 *   Author: Paulo Alcantara <paulo.alcantara@openbossa.org>
 *
 * This software may be redistributed and/or modified under the terms of
 * the GNU General Public License ("GPL") version 2 as published by the
 * Free Software Foundation.
 */

#ifndef _PYTHONHOVERHANDLER_H_
#define _PYTHONHOVERHANDLER_H_

#include <texteditor/basehoverhandler.h>

#include <QtCore/QObject>

namespace Core {
class IEditor;
}

namespace TextEditor {
class ITextEditor;
}

namespace PythonEditor {
namespace Internal {

class PyHoverHandler : public TextEditor::BaseHoverHandler
{
    Q_OBJECT
public:
    PyHoverHandler(QObject *parent = 0);
    virtual ~PyHoverHandler();

private:
    virtual bool acceptEditor(Core::IEditor *editor);
    virtual void identifyMatch(TextEditor::ITextEditor *editor, int pos);
    virtual void decorateToolTip(void);
};

}
}

#endif /* _PYTHONHOVERHANDLER_H_ */
