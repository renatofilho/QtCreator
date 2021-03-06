/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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

#include "memchecksettings.h"
#include "memcheckconfigwidget.h"

#include <valgrind/xmlprotocol/error.h>

#include <utils/qtcassert.h>

using namespace Analyzer;

static const char numCallersC[]  = "Analyzer.Valgrind.NumCallers";
static const char trackOriginsC[] = "Analyzer.Valgrind.TrackOrigins";
static const char suppressionFilesC[] = "Analyzer.Valgrind.SupressionFiles";
static const char removedSuppressionFilesC[] = "Analyzer.Valgrind.RemovedSuppressionFiles";
static const char addedSuppressionFilesC[] = "Analyzer.Valgrind.AddedSuppressionFiles";
static const char filterExternalIssuesC[] = "Analyzer.Valgrind.FilterExternalIssues";
static const char visibleErrorKindsC[] = "Analyzer.Valgrind.VisibleErrorKinds";

static const char lastSuppressionDirectoryC[] = "Analyzer.Valgrind.LastSuppressionDirectory";
static const char lastSuppressionHistoryC[] = "Analyzer.Valgrind.LastSuppressionHistory";

namespace Valgrind {
namespace Internal {

QVariantMap AbstractMemcheckSettings::defaults() const
{
    QVariantMap map;
    map.insert(QLatin1String(numCallersC), 25);
    map.insert(QLatin1String(trackOriginsC), true);
    map.insert(QLatin1String(filterExternalIssuesC), true);

    QVariantList defaultErrorKinds;
    for (int i = 0; i < Valgrind::XmlProtocol::MemcheckErrorKindCount; ++i)
        defaultErrorKinds << i;
    map.insert(QLatin1String(visibleErrorKindsC), defaultErrorKinds);

    return map;
}

bool AbstractMemcheckSettings::fromMap(const QVariantMap &map)
{
    setIfPresent(map, QLatin1String(numCallersC), &m_numCallers);
    setIfPresent(map, QLatin1String(trackOriginsC), &m_trackOrigins);
    setIfPresent(map, QLatin1String(filterExternalIssuesC), &m_filterExternalIssues);

    // if we get more of these try a template specialization of setIfPresent for lists...
    if (map.contains(QLatin1String(visibleErrorKindsC))) {
        m_visibleErrorKinds.clear();
        foreach (const QVariant &val, map.value(QLatin1String(visibleErrorKindsC)).toList())
            m_visibleErrorKinds << val.toInt();
    }

    return true;
}

QVariantMap AbstractMemcheckSettings::toMap() const
{
    QVariantMap map;
    map.insert(QLatin1String(numCallersC), m_numCallers);
    map.insert(QLatin1String(trackOriginsC), m_trackOrigins);
    map.insert(QLatin1String(filterExternalIssuesC), m_filterExternalIssues);

    QVariantList errorKinds;
    foreach (int i, m_visibleErrorKinds)
        errorKinds << i;
    map.insert(QLatin1String(visibleErrorKindsC), errorKinds);

    return map;
}

void AbstractMemcheckSettings::setNumCallers(int numCallers)
{
    if (m_numCallers != numCallers) {
        m_numCallers = numCallers;
        emit numCallersChanged(numCallers);
    }
}

void AbstractMemcheckSettings::setTrackOrigins(bool trackOrigins)
{
    if (m_trackOrigins != trackOrigins) {
        m_trackOrigins = trackOrigins;
        emit trackOriginsChanged(trackOrigins);
    }
}

void AbstractMemcheckSettings::setFilterExternalIssues(bool filterExternalIssues)
{
    if (m_filterExternalIssues != filterExternalIssues) {
        m_filterExternalIssues = filterExternalIssues;
        emit filterExternalIssuesChanged(filterExternalIssues);
    }
}

void AbstractMemcheckSettings::setVisibleErrorKinds(const QList<int> &visibleErrorKinds)
{
    if (m_visibleErrorKinds != visibleErrorKinds) {
        m_visibleErrorKinds = visibleErrorKinds;
        emit visibleErrorKindsChanged(visibleErrorKinds);
    }
}

QString AbstractMemcheckSettings::id() const
{
    return "Analyzer.Valgrind.Settings.Memcheck";
}

QString AbstractMemcheckSettings::displayName() const
{
    return tr("Memory Analysis");
}

QWidget *AbstractMemcheckSettings::createConfigWidget(QWidget *parent)
{
    return new MemcheckConfigWidget(this, parent);
}

QStringList MemcheckGlobalSettings::suppressionFiles() const
{
    return m_suppressionFiles;
}

void MemcheckGlobalSettings::addSuppressionFiles(const QStringList &suppressions)
{
    foreach (const QString &s, suppressions)
        if (!m_suppressionFiles.contains(s))
            m_suppressionFiles.append(s);
}

void MemcheckGlobalSettings::removeSuppressionFiles(const QStringList &suppressions)
{
    foreach (const QString &s, suppressions)
        m_suppressionFiles.removeAll(s);
}

QString MemcheckGlobalSettings::lastSuppressionDialogDirectory() const
{
    return m_lastSuppressionDirectory;
}

void MemcheckGlobalSettings::setLastSuppressionDialogDirectory(const QString &directory)
{
    m_lastSuppressionDirectory = directory;
}

QStringList MemcheckGlobalSettings::lastSuppressionDialogHistory() const
{
    return m_lastSuppressionHistory;
}

void MemcheckGlobalSettings::setLastSuppressionDialogHistory(const QStringList &history)
{
    m_lastSuppressionHistory = history;
}

QVariantMap MemcheckGlobalSettings::defaults() const
{
    QVariantMap ret = AbstractMemcheckSettings::defaults();
    ret.insert(suppressionFilesC, QStringList());
    ret.insert(lastSuppressionDirectoryC, QString());
    ret.insert(lastSuppressionHistoryC, QStringList());
    return ret;
}

bool MemcheckGlobalSettings::fromMap(const QVariantMap &map)
{
    AbstractMemcheckSettings::fromMap(map);
    m_suppressionFiles = map.value(suppressionFilesC).toStringList();
    m_lastSuppressionDirectory = map.value(lastSuppressionDirectoryC).toString();
    m_lastSuppressionHistory = map.value(lastSuppressionHistoryC).toStringList();
    return true;
}

QVariantMap MemcheckGlobalSettings::toMap() const
{
    QVariantMap map = AbstractMemcheckSettings::toMap();
    map.insert(suppressionFilesC, m_suppressionFiles);
    map.insert(lastSuppressionDirectoryC, m_lastSuppressionDirectory);
    map.insert(lastSuppressionHistoryC, m_lastSuppressionHistory);
    return map;
}

MemcheckGlobalSettings *globalMemcheckSettings()
{
    MemcheckGlobalSettings *ret = AnalyzerGlobalSettings::instance()->subConfig<MemcheckGlobalSettings>();
    QTC_ASSERT(ret, return 0);
    return ret;
}


QVariantMap MemcheckProjectSettings::defaults() const
{
    QVariantMap ret = AbstractMemcheckSettings::defaults();
    ret.insert(addedSuppressionFilesC, QStringList());
    ret.insert(removedSuppressionFilesC, QStringList());
    return ret;
}

bool MemcheckProjectSettings::fromMap(const QVariantMap &map)
{
    AbstractMemcheckSettings::fromMap(map);
    setIfPresent(map, addedSuppressionFilesC, &m_addedSuppressionFiles);
    setIfPresent(map, removedSuppressionFilesC, &m_disabledGlobalSuppressionFiles);
    return true;
}

QVariantMap MemcheckProjectSettings::toMap() const
{
    QVariantMap map = AbstractMemcheckSettings::toMap();
    map.insert(addedSuppressionFilesC, m_addedSuppressionFiles);
    map.insert(removedSuppressionFilesC, m_disabledGlobalSuppressionFiles);
    return map;
}

void MemcheckProjectSettings::addSuppressionFiles(const QStringList &suppressions)
{
    QStringList globalSuppressions = globalMemcheckSettings()->suppressionFiles();
    foreach (const QString &s, suppressions) {
        if (m_addedSuppressionFiles.contains(s))
            continue;
        m_disabledGlobalSuppressionFiles.removeAll(s);
        if (!globalSuppressions.contains(s))
            m_addedSuppressionFiles.append(s);
    }
}

void MemcheckProjectSettings::removeSuppressionFiles(const QStringList &suppressions)
{
    QStringList globalSuppressions = globalMemcheckSettings()->suppressionFiles();
    foreach (const QString &s, suppressions) {
        m_addedSuppressionFiles.removeAll(s);
        if (globalSuppressions.contains(s))
            m_disabledGlobalSuppressionFiles.append(s);
    }
}

QStringList MemcheckProjectSettings::suppressionFiles() const
{
    QStringList ret = globalMemcheckSettings()->suppressionFiles();
    foreach (const QString &s, m_disabledGlobalSuppressionFiles)
        ret.removeAll(s);
    ret.append(m_addedSuppressionFiles);
    return ret;
}

} // namespace Internal
} // namespace Valgrind
