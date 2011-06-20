/**************************************************************************
**
** Copyright (c) 2011 INdT
**
** Contact: contact@pyside.org
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef PYTHONPROJECTFILE_H
#define PYTHONPROJECTFILE_H

#include <coreplugin/ifile.h>

namespace PythonProjectManager {

class PythonProject;

namespace Internal {

class PythonProjectFile : public Core::IFile
{
    Q_OBJECT

public:
    PythonProjectFile(PythonProject *parent, QString fileName);
    virtual ~PythonProjectFile();

    virtual bool save(QString *errorString, const QString &fileName, bool autoSave);
    virtual QString fileName() const;
    virtual void rename(const QString &newName);

    virtual QString defaultPath() const;
    virtual QString suggestedFileName() const;
    virtual QString mimeType() const;

    virtual bool isModified() const;
    virtual bool isReadOnly() const;
    virtual bool isSaveAsAllowed() const;

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type);

private:
    PyhtonProject *m_project;
    QString m_fileName;
};

} // namespace Internal
} // namespace PythonProjectManager

#endif // QMLPROJECTFILE_H
