/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#ifndef MAEMOSETTINGSPAGES_H
#define MAEMOSETTINGSPAGES_H

#include <coreplugin/dialogs/ioptionspage.h>

namespace RemoteLinux{
namespace Internal {

class MaemoDeviceConfigurationsSettingsWidget;
class MaemoQemuSettingsWidget;

class MaemoDeviceConfigurationsSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT
public:
    MaemoDeviceConfigurationsSettingsPage(QObject *parent = 0);
    ~MaemoDeviceConfigurationsSettingsPage();

    virtual QString id() const;
    virtual QString displayName() const;
    virtual QString category() const;
    virtual QString displayCategory() const;
    virtual QIcon categoryIcon() const;
    virtual bool matches(const QString &searchKeyWord) const;
    virtual QWidget *createPage(QWidget *parent);
    virtual void apply();
    virtual void finish();

    static const QString Id;
    static const QString Category;

private:
    QString m_keywords;
    MaemoDeviceConfigurationsSettingsWidget *m_widget;
};

class MaemoQemuSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT
public:
    MaemoQemuSettingsPage(QObject *parent = 0);
    ~MaemoQemuSettingsPage();

    virtual QString id() const;
    virtual QString displayName() const;
    virtual QString category() const;
    virtual QString displayCategory() const;
    virtual QIcon categoryIcon() const;
    virtual bool matches(const QString &searchKeyWord) const;
    virtual QWidget *createPage(QWidget *parent);
    virtual void apply();
    virtual void finish();

    static const QString Id;
    static const QString Category;

    static void showQemuCrashDialog();

private:
    QString m_keywords;
    MaemoQemuSettingsWidget *m_widget;
};

} // namespace Internal
} // namespace RemoteLinux

#endif // MAEMOSETTINGSPAGES_H
