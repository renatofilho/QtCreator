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

#ifndef MAEMOSSHRUNNER_H
#define MAEMOSSHRUNNER_H

#include "linuxdeviceconfiguration.h"
#include "maemomountspecification.h"

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QStringList>

namespace Utils {
    class SshConnection;
    class SshRemoteProcess;
}

namespace RemoteLinux {
class RemoteLinuxRunConfiguration;

namespace Internal {
class MaemoRemoteMounter;
class MaemoUsedPortsGatherer;

class MaemoSshRunner : public QObject
{
    Q_OBJECT
public:
    MaemoSshRunner(QObject *parent, RemoteLinuxRunConfiguration *runConfig,
        bool debugging);
    ~MaemoSshRunner();

    void start();
    void stop();

    void startExecution(const QByteArray &remoteCall);

    QSharedPointer<Utils::SshConnection> connection() const { return m_connection; }
    const MaemoUsedPortsGatherer *usedPortsGatherer() const { return m_portsGatherer; }
    PortList *freePorts() { return &m_freePorts; }
    QString remoteExecutable() const { return m_remoteExecutable; }
    QString arguments() const { return m_appArguments; }
    QString commandPrefix() const { return m_commandPrefix; }
    const QSharedPointer<const LinuxDeviceConfiguration> devConfig() const { return m_devConfig; }

    static const qint64 InvalidExitCode;

signals:
    void error(const QString &error);
    void mountDebugOutput(const QString &output);
    void readyForExecution();
    void remoteOutput(const QByteArray &output);
    void remoteErrorOutput(const QByteArray &output);
    void reportProgress(const QString &progressOutput);
    void remoteProcessStarted();
    void remoteProcessFinished(qint64 exitCode);

private slots:
    void handleConnected();
    void handleConnectionFailure();
    void handleCleanupFinished(int exitStatus);
    void handleRemoteProcessFinished(int exitStatus);
    void handleMounted();
    void handleUnmounted();
    void handleMounterError(const QString &errorMsg);
    void handlePortsGathererError(const QString &errorMsg);
    void handleUsedPortsAvailable();

private:
    enum State { Inactive, Connecting, PreRunCleaning, PostRunCleaning,
        PreMountUnmounting, Mounting, ReadyForExecution,
        ProcessStarting, StopRequested, GatheringPorts
    };

    void setState(State newState);
    void emitError(const QString &errorMsg, bool force = false);

    void cleanup();
    bool isConnectionUsable() const;
    void mount();
    void unmount();

    MaemoRemoteMounter * const m_mounter;
    MaemoUsedPortsGatherer * const m_portsGatherer;
    const QSharedPointer<const LinuxDeviceConfiguration> m_devConfig;
    const QString m_remoteExecutable;
    const QString m_appArguments;
    const QString m_commandPrefix;
    const PortList m_initialFreePorts;
    QList<MaemoMountSpecification> m_mountSpecs;

    QSharedPointer<Utils::SshConnection> m_connection;
    QSharedPointer<Utils::SshRemoteProcess> m_runner;
    QSharedPointer<Utils::SshRemoteProcess> m_cleaner;
    QStringList m_procsToKill;
    PortList m_freePorts;

    int m_exitStatus;
    State m_state;
};

} // namespace Internal
} // namespace RemoteLinux

#endif // MAEMOSSHRUNNER_H
