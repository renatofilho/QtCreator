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

#include "qmlprojectruncontrol.h"
#include "qmlprojectrunconfiguration.h"
#include "qmlprojectconstants.h"
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/applicationlauncher.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <qtsupport/qtversionmanager.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <debugger/debuggerrunner.h>
#include <debugger/debuggerplugin.h>
#include <debugger/debuggerconstants.h>
#include <debugger/debuggerengine.h>
#include <debugger/debuggerstartparameters.h>
#include <qmljsinspector/qmljsinspectorconstants.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qmlobservertool.h>
#include <qtsupport/qtsupportconstants.h>

#include <QtGui/QApplication>
#include <QtGui/QLabel>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>

#include <QtCore/QDir>

using namespace ProjectExplorer;

namespace QmlProjectManager {
namespace Internal {

QmlProjectRunControl::QmlProjectRunControl(QmlProjectRunConfiguration *runConfiguration, QString mode)
    : RunControl(runConfiguration, mode)
{
    m_applicationLauncher.setEnvironment(runConfiguration->environment());
    m_applicationLauncher.setWorkingDirectory(runConfiguration->workingDirectory());

    if (mode == ProjectExplorer::Constants::RUNMODE) {
        m_executable = runConfiguration->viewerPath();
    } else {
        m_executable = runConfiguration->observerPath();
    }
    m_commandLineArguments = runConfiguration->viewerArguments();

    connect(&m_applicationLauncher, SIGNAL(appendMessage(QString,Utils::OutputFormat)),
            this, SLOT(slotAppendMessage(QString, Utils::OutputFormat)));
    connect(&m_applicationLauncher, SIGNAL(processExited(int)),
            this, SLOT(processExited(int)));
    connect(&m_applicationLauncher, SIGNAL(bringToForegroundRequested(qint64)),
            this, SLOT(slotBringApplicationToForeground(qint64)));
}

QmlProjectRunControl::~QmlProjectRunControl()
{
    stop();
}

void QmlProjectRunControl::start()
{
    m_applicationLauncher.start(ApplicationLauncher::Gui, m_executable,
                                m_commandLineArguments);
    setApplicationProcessHandle(ProcessHandle(m_applicationLauncher.applicationPID()));
    emit started();
    QString msg = tr("Starting %1 %2\n")
        .arg(QDir::toNativeSeparators(m_executable), m_commandLineArguments);
    appendMessage(msg, Utils::NormalMessageFormat);
}

RunControl::StopResult QmlProjectRunControl::stop()
{
    m_applicationLauncher.stop();
    return StoppedSynchronously;
}

bool QmlProjectRunControl::isRunning() const
{
    return m_applicationLauncher.isRunning();
}

QIcon QmlProjectRunControl::icon() const
{
    return QIcon(ProjectExplorer::Constants::ICON_RUN_SMALL);
}

void QmlProjectRunControl::slotBringApplicationToForeground(qint64 pid)
{
    bringApplicationToForeground(pid);
}

void QmlProjectRunControl::slotAppendMessage(const QString &line, Utils::OutputFormat format)
{
    appendMessage(line, format);
}

void QmlProjectRunControl::processExited(int exitCode)
{
    QString msg = tr("%1 exited with code %2\n")
        .arg(QDir::toNativeSeparators(m_executable)).arg(exitCode);
    appendMessage(msg, exitCode ? Utils::ErrorMessageFormat : Utils::NormalMessageFormat);
    emit finished();
}

QmlProjectRunControlFactory::QmlProjectRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
}

QmlProjectRunControlFactory::~QmlProjectRunControlFactory()
{
}

bool QmlProjectRunControlFactory::canRun(RunConfiguration *runConfiguration,
                                  const QString &mode) const
{
    QmlProjectRunConfiguration *config =
        qobject_cast<QmlProjectRunConfiguration*>(runConfiguration);
    if (mode == ProjectExplorer::Constants::RUNMODE)
        return config != 0 && !config->viewerPath().isEmpty();
    else if (mode != Debugger::Constants::DEBUGMODE)
        return false;

    bool qmlDebugSupportInstalled =
            Debugger::DebuggerPlugin::isActiveDebugLanguage(Debugger::QmlLanguage);

    if (config && qmlDebugSupportInstalled) {
        if (!(config->qtVersion() && config->qtVersion()->needsQmlDebuggingLibrary())
                || !config->observerPath().isEmpty())
            return true;
        if (config->qtVersion() && QtSupport::QmlObserverTool::canBuild(config->qtVersion()))
            return true;
    }

    return false;
}

RunControl *QmlProjectRunControlFactory::create(RunConfiguration *runConfiguration,
                                         const QString &mode)
{
    QTC_ASSERT(canRun(runConfiguration, mode), return 0);

    QmlProjectRunConfiguration *config = qobject_cast<QmlProjectRunConfiguration *>(runConfiguration);
    RunControl *runControl = 0;
    if (mode == ProjectExplorer::Constants::RUNMODE)
        runControl = new QmlProjectRunControl(config, mode);
    else if (mode == Debugger::Constants::DEBUGMODE)
        runControl = createDebugRunControl(config);
    return runControl;
}

QString QmlProjectRunControlFactory::displayName() const
{
    return tr("Run");
}

ProjectExplorer::RunConfigWidget *QmlProjectRunControlFactory::createConfigurationWidget(RunConfiguration *runConfiguration)
{
    Q_UNUSED(runConfiguration)
    return 0;
}

RunControl *QmlProjectRunControlFactory::createDebugRunControl(QmlProjectRunConfiguration *runConfig)
{
    Debugger::DebuggerStartParameters params;
    params.startMode = Debugger::StartInternal;
    params.executable = runConfig->observerPath();
    params.qmlServerAddress = "127.0.0.1";
    params.qmlServerPort = runConfig->qmlDebugServerPort();
    params.processArgs = QString("-qmljsdebugger=port:%1,block").arg(runConfig->qmlDebugServerPort());
    params.processArgs += QLatin1Char(' ') + runConfig->viewerArguments();
    params.workingDirectory = runConfig->workingDirectory();
    params.environment = runConfig->environment();
    params.displayName = runConfig->displayName();
    params.projectSourceDirectory = runConfig->target()->project()->projectDirectory();
    params.projectSourceFiles = runConfig->target()->project()->files(Project::ExcludeGeneratedFiles);

    if (params.executable.isEmpty()) {
        showQmlObserverToolWarning();
        return 0;
    }

    return Debugger::DebuggerPlugin::createDebugger(params, runConfig);
}

void QmlProjectRunControlFactory::showQmlObserverToolWarning()
{
    QMessageBox dialog(QApplication::activeWindow());
    QPushButton *qtPref = dialog.addButton(tr("Open Qt4 Options"),
                                           QMessageBox::ActionRole);
    dialog.addButton(tr("Cancel"), QMessageBox::ActionRole);
    dialog.setDefaultButton(qtPref);
    dialog.setWindowTitle(tr("QML Observer Missing"));
    dialog.setText(tr("QML Observer could not be found."));
    dialog.setInformativeText(tr(
                                  "QML Observer is used to offer debugging features for "
                                  "QML applications, such as interactive debugging and inspection tools. "
                                  "It must be compiled for each used Qt version separately. "
                                  "On the Qt4 options page, select the current Qt installation "
                                  "and click Rebuild."));
    dialog.exec();
    if (dialog.clickedButton() == qtPref) {
        Core::ICore::instance()->showOptionsDialog(
                    QtSupport::Constants::QT_SETTINGS_CATEGORY,
                    QtSupport::Constants::QTVERSION_SETTINGS_PAGE_ID);
    }
}

} // namespace Internal
} // namespace QmlProjectManager
