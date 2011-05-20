/*
 * Copyright (c) 2011 Instituto Nokia de Tecnologia
 *   Author: Paulo Alcantara <paulo.alcantara@openbossa.org>
 *
 * This software may be redistributed and/or modified under the terms of
 * the GNU General Public License ("GPL") version 2 as published by the
 * Free Software Foundation.
 */

#ifndef _PYTHONEDITOREDITABLE_H_
#define _PYTHONEDITOREDITABLE_H_

#include <texteditor/basetexteditor.h>

namespace PythonEditor {
class PyTextEditorWidget;

namespace Internal {

class PyEditorEditable : public TextEditor::BaseTextEditor {
    Q_OBJECT

public:
    explicit PyEditorEditable(PyTextEditorWidget *);

    bool duplicateSupported(void) const { return true; }
    Core::IEditor *duplicate(QWidget *parent);
    QString id(void) const;
    bool isTemporary(void) const { return false; }
    virtual bool open(QString *errorString, const QString &fileName,
                        const QString &realFileName);
    virtual QString preferredModeType(void) const;
};

}
}

#endif /* _PYTHONEDITOREDITABLE_H_ */
