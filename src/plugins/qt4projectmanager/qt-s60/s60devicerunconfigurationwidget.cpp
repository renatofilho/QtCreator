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

#include "s60devicerunconfigurationwidget.h"
#include "s60devicerunconfiguration.h"

#include <utils/debuggerlanguagechooser.h>
#include <utils/detailswidget.h>

#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QFormLayout>

namespace Qt4ProjectManager {
namespace Internal {

S60DeviceRunConfigurationWidget::S60DeviceRunConfigurationWidget(
            S60DeviceRunConfiguration *runConfiguration,
            QWidget *parent)
    : QWidget(parent),
    m_runConfiguration(runConfiguration),
    m_detailsWidget(new Utils::DetailsWidget),
    m_argumentsLineEdit(new QLineEdit(m_runConfiguration->commandLineArguments()))
{
    m_detailsWidget->setState(Utils::DetailsWidget::NoSummary);
    QVBoxLayout *mainBoxLayout = new QVBoxLayout();
    mainBoxLayout->setMargin(0);
    setLayout(mainBoxLayout);
    mainBoxLayout->addWidget(m_detailsWidget);
    QWidget *detailsContainer = new QWidget;
    m_detailsWidget->setWidget(detailsContainer);

    QVBoxLayout *detailsBoxLayout = new QVBoxLayout();
    detailsBoxLayout->setMargin(0);
    detailsContainer->setLayout(detailsBoxLayout);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setMargin(0);
    detailsBoxLayout->addLayout(formLayout);
    formLayout->addRow(tr("Arguments:"), m_argumentsLineEdit);

    QLabel *debuggerLabel = new QLabel(tr("Debugger:"), this);
    debuggerLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);

    m_debuggerLanguageChooser = new Utils::DebuggerLanguageChooser(this);
    formLayout->addRow(debuggerLabel, m_debuggerLanguageChooser);

    m_debuggerLanguageChooser->setCppChecked(m_runConfiguration->useCppDebugger());
    m_debuggerLanguageChooser->setQmlChecked(m_runConfiguration->useQmlDebugger());
    m_debuggerLanguageChooser->setQmlDebugServerPort(m_runConfiguration->qmlDebugServerPort());

    connect(m_argumentsLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(argumentsEdited(QString)));

    connect(m_runConfiguration, SIGNAL(isEnabledChanged(bool)),
            this, SLOT(runConfigurationEnabledChange(bool)));

    connect(m_debuggerLanguageChooser, SIGNAL(cppLanguageToggled(bool)),
            this, SLOT(useCppDebuggerToggled(bool)));
    connect(m_debuggerLanguageChooser, SIGNAL(qmlLanguageToggled(bool)),
            this, SLOT(useQmlDebuggerToggled(bool)));
    connect(m_debuggerLanguageChooser, SIGNAL(qmlDebugServerPortChanged(uint)),
            this, SLOT(qmlDebugServerPortChanged(uint)));

    setEnabled(m_runConfiguration->isEnabled());
}

void S60DeviceRunConfigurationWidget::argumentsEdited(const QString &text)
{
    m_runConfiguration->setCommandLineArguments(text.trimmed());
}

void S60DeviceRunConfigurationWidget::runConfigurationEnabledChange(bool enabled)
{
    setEnabled(enabled);
}

void S60DeviceRunConfigurationWidget::useCppDebuggerToggled(bool enabled)
{
    m_runConfiguration->setUseCppDebugger(enabled);
}

void S60DeviceRunConfigurationWidget::useQmlDebuggerToggled(bool enabled)
{
    m_runConfiguration->setUseQmlDebugger(enabled);
}

void S60DeviceRunConfigurationWidget::qmlDebugServerPortChanged(uint port)
{
    m_runConfiguration->setQmlDebugServerPort(port);
}

} // namespace Internal
} // namespace Qt4ProjectManager
