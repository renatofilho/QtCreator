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

#ifndef COMMAND_H
#define COMMAND_H

#include <coreplugin/core_global.h>

#include <QtCore/QObject>

QT_BEGIN_NAMESPACE
class QAction;
class QShortcut;
class QKeySequence;
QT_END_NAMESPACE


namespace Core {

class Context;

class CORE_EXPORT Command : public QObject
{
    Q_OBJECT
public:
    enum CommandAttribute {
        CA_Hide = 1,
        CA_UpdateText = 2,
        CA_UpdateIcon = 4,
        CA_NonConfigurable = 8
    };
    Q_DECLARE_FLAGS(CommandAttributes, CommandAttribute)

    virtual void setDefaultKeySequence(const QKeySequence &key) = 0;
    virtual QKeySequence defaultKeySequence() const = 0;
    virtual QKeySequence keySequence() const = 0;
    virtual void setDefaultText(const QString &text) = 0;
    virtual QString defaultText() const = 0;

    virtual int id() const = 0;

    virtual QAction *action() const = 0;
    virtual QShortcut *shortcut() const = 0;
    virtual Context context() const = 0;

    virtual void setAttribute(CommandAttribute attr) = 0;
    virtual void removeAttribute(CommandAttribute attr) = 0;
    virtual bool hasAttribute(CommandAttribute attr) const = 0;

    virtual bool isActive() const = 0;

    virtual ~Command() {}

    virtual void setKeySequence(const QKeySequence &key) = 0;

    virtual QString stringWithAppendedShortcut(const QString &str) const = 0;

    virtual bool isScriptable() const = 0;
    virtual bool isScriptable(const Context &) const = 0;

signals:
    void keySequenceChanged();
    void activeStateChanged();
};

} // namespace Core

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::Command::CommandAttributes)

#endif // COMMAND_H
