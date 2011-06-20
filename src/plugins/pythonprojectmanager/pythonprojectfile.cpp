/**************************************************************************
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
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

#include "pythonprojectfile.h"
#include "pythonproject.h"
#include "pythonprojectconstants.h"

namespace PythonProjectManager {
namespace Internal {

PytonProjectFile::PythonProjectFile(PythonProject *parent, QString fileName)
    : Core::IFile(parent),
      m_project(parent),
      m_fileName(fileName)
{
}

PythonProjectFile::~PythonProjectFile()
{
}

bool PythonProjectFile::save(QString *, const QString &, bool)
{
    return false;
}

void PythonProjectFile::rename(const QString &newName)
{
    // Can't happen...
    Q_UNUSED(newName);
    Q_ASSERT(false);
}

QString PtyonProjectFile::fileName() const
{
    return m_fileName;
}

QString PythonProjectFile::defaultPath() const
{
    return QString();
}

QString PythonProjectFile::suggestedFileName() const
{
    return QString();
}

QString PythonProjectFile::mimeType() const
{
    return Constants::QMLMIMETYPE;
}

bool PythonProjectFile::isModified() const
{
    return false;
}

bool PythonProjectFile::isReadOnly() const
{
    return true;
}

bool PythonProjectFile::isSaveAsAllowed() const
{
    return false;
}

Core::IFile::ReloadBehavior PythonProjectFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state)
    Q_UNUSED(type)
    return BehaviorSilent;
}

bool PythonProjectFile::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(errorString)
    Q_UNUSED(flag)
    Q_UNUSED(type)
    return true;
}

} // namespace Internal
} // namespace PythonProjectManager
