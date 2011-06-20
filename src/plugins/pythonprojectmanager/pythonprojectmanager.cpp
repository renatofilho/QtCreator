/**************************************************************************
**
** Copyright (c) 2011  INdT
**
** Contact: contact@indt.org
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

#include "pythonprojectmanager.h"
#include "pythonprojectconstants.h"
#include "pythonproject.h"

#include <coreplugin/icore.h>
#include <coreplugin/ifile.h>
#include <coreplugin/messagemanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <QtDebug>

namespace PythonProjectManager {
namespace Internal {

Manager::Manager()
{
}

QString Manager::mimeType() const
{
    return QLatin1String(Constants::PYTHONMIMETYPE);
}

ProjectExplorer::Project *Manager::openProject(const QString &fileName)
{
    Core::MessageManager *messageManager = Core::ICore::instance()->messageManager();

    QFileInfo fileInfo(fileName);
    ProjectExplorer::ProjectExplorerPlugin *projectExplorer = ProjectExplorer::ProjectExplorerPlugin::instance();

    foreach (ProjectExplorer::Project *pi, projectExplorer->session()->projects()) {
        if (fileName == pi->file()->fileName()) {
            messageManager->printToOutputPanePopup(tr("Failed opening project '%1': Project already open")
                                                   .arg(QDir::toNativeSeparators(fileName)));
            return 0;
        }
    }

    if (fileInfo.isFile())
        return new PythonProject(this, fileName);

    return 0;
}

void Manager::registerProject(PythonProject *project)
{
    m_projects.append(project);
}

void Manager::unregisterProject(PythonProject *project)
{
    m_projects.removeAll(project);
}

void Manager::notifyChanged(const QString &fileName)
{
    foreach (PythonProject *project, m_projects) {
        if (fileName == project->filesFileName()) {
            project->refresh(PythonProject::Files);
        }
    }
}

} // namespace Internal
} // namespace PythonProjectManager
