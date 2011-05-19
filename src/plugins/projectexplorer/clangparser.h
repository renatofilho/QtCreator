/**************************************************************************
**
** This file is part of Qt Creator
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

#ifndef CLANGPARSER_H
#define CLANGPARSER_H

#include "ioutputparser.h"

#include <QtCore/QRegExp>

namespace ProjectExplorer {

class ClangParser : public ProjectExplorer::IOutputParser
{
    Q_OBJECT

public:
    ClangParser();
    ~ClangParser();
    virtual void stdError(const QString &line);

private:
    void newTask(Task::TaskType type_, const QString &description_,
                 const QString &file_, int line_, const QString &category_);

    QRegExp m_commandRegExp;
    QRegExp m_inLineRegExp;
    QRegExp m_messageRegExp;
    QString m_codeSnippet;

    Task m_currentTask;
};

} // namespace ProjectExplorer

#endif // CLANGPARSER_H