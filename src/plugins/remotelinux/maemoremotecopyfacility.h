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

#ifndef MAEMOREMOTECOPYFACILITY_H
#define MAEMOREMOTECOPYFACILITY_H

#include "maemodeployable.h"

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>

namespace Utils {
class SshConnection;
class SshRemoteProcessRunner;
}

namespace RemoteLinux {
class LinuxDeviceConfiguration;

namespace Internal {

class MaemoRemoteCopyFacility : public QObject
{
    Q_OBJECT
public:
    explicit MaemoRemoteCopyFacility(QObject *parent = 0);
    ~MaemoRemoteCopyFacility();

    void copyFiles(const QSharedPointer<Utils::SshConnection> &connection,
        const QSharedPointer<const LinuxDeviceConfiguration> &devConf,
        const QList<MaemoDeployable> &deployables, const QString &mountPoint);
    void cancel();

signals:
    void stdoutData(const QString &output);
    void stderrData(const QString &output);
    void progress(const QString &message);
    void fileCopied(const MaemoDeployable &deployable);
    void finished(const QString &errorMsg = QString());

private slots:
    void handleConnectionError();
    void handleCopyFinished(int exitStatus);
    void handleRemoteStdout(const QByteArray &output);
    void handleRemoteStderr(const QByteArray &output);

private:
    void copyNextFile();
    void setFinished();

    QSharedPointer<Utils::SshRemoteProcessRunner> m_copyRunner;
    QSharedPointer<const LinuxDeviceConfiguration> m_devConf;
    QList<MaemoDeployable> m_deployables;
    QString m_mountPoint;
    bool m_isCopying;
};

} // namespace Internal
} // namespace RemoteLinux

#endif // MAEMOREMOTECOPYFACILITY_H
