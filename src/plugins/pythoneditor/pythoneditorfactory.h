/*
 * Copyright (c) 2011 Instituto Nokia de Tecnologia
 *   Author: Paulo Alcantara <paulo.alcantara@openbossa.org>
 *
 * This software may be redistributed and/or modified under the terms of
 * the GNU General Public License ("GPL") version 2 as published by the
 * Free Software Foundation.
 */

#ifndef _PYTHONEDITORFACTORY_H_
#define _PYTHONEDITORFACTORY_H_

#include <coreplugin/editormanager/ieditorfactory.h>

#include <QtCore/QStringList>

#include "pythoneditor.h"

namespace TextEditor {
class TextEditorActionHandler;
}

namespace PythonEditor {
class PyTextEditorWidget;

namespace Internal {

class PyEditorActionHandler;

class PyEditorFactory : public Core::IEditorFactory {
    Q_OBJECT

public:
    PyEditorFactory(QObject *parent);
    ~PyEditorFactory();

    virtual QStringList mimeTypes(void) const;
    /* IEditorFactory */
    QString id(void) const;
    QString displayName(void) const;
    Core::IFile *open(const QString &fileName);
    Core::IEditor *createEditor(QWidget *parent);

private slots:
    void updateEditorInfoBar(Core::IEditor *editor);

private:
    QStringList m_mimeTypes;
};

}
}

#endif /* _PYTHONEDITORFACTORY_H_ */
