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

#include "qmljstoolsplugin.h"
#include "qmljsmodelmanager.h"
#include "qmljsfunctionfilter.h"
#include "qmljslocatordata.h"
#include "qmljscodestylesettingspage.h"
#include "qmljstoolsconstants.h"
#include "qmljstoolssettings.h"
#include "qmljscodestylesettingsfactory.h"

#include <texteditor/texteditorsettings.h>
#include <texteditor/tabsettings.h>
#include <texteditor/codestylepreferencesmanager.h>

#include <extensionsystem/pluginmanager.h>

#include <coreplugin/icore.h>

#include <QtCore/QtPlugin>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QSettings>

using namespace QmlJSTools::Internal;

enum { debug = 0 };

QmlJSToolsPlugin *QmlJSToolsPlugin::m_instance = 0;

QmlJSToolsPlugin::QmlJSToolsPlugin()
    : m_modelManager(0)
{
    m_instance = this;
}

QmlJSToolsPlugin::~QmlJSToolsPlugin()
{
    m_instance = 0;
    m_modelManager = 0; // deleted automatically
}

bool QmlJSToolsPlugin::initialize(const QStringList &arguments, QString *error)
{
    Q_UNUSED(arguments)
    Q_UNUSED(error)
//    Core::ICore *core = Core::ICore::instance();

    m_settings = new QmlJSToolsSettings(this); // force registration of qmljstools settings

    // Objects
    m_modelManager = new ModelManager(this);
//    Core::VCSManager *vcsManager = core->vcsManager();
//    Core::FileManager *fileManager = core->fileManager();
//    connect(vcsManager, SIGNAL(repositoryChanged(QString)),
//            m_modelManager, SLOT(updateModifiedSourceFiles()));
//    connect(fileManager, SIGNAL(filesChangedInternally(QStringList)),
//            m_modelManager, SLOT(updateSourceFiles(QStringList)));
    addAutoReleasedObject(m_modelManager);

    LocatorData *locatorData = new LocatorData;
    addAutoReleasedObject(locatorData);
    addAutoReleasedObject(new FunctionFilter(locatorData));
    addAutoReleasedObject(new QmlJSCodeStyleSettingsPage);

    TextEditor::CodeStylePreferencesManager::instance()->registerFactory(
                new QmlJSTools::QmlJSCodeStylePreferencesFactory());

    return true;
}

void QmlJSToolsPlugin::extensionsInitialized()
{
    m_modelManager->delayedInitialization();
}

ExtensionSystem::IPlugin::ShutdownFlag QmlJSToolsPlugin::aboutToShutdown()
{
    return SynchronousShutdown;
}

Q_EXPORT_PLUGIN(QmlJSToolsPlugin)
