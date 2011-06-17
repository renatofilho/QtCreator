#include "qmlproject.h"
#include "qmlprojectfile.h"
#include "fileformat/qmlprojectitem.h"
#include "qmlprojectrunconfiguration.h"
#include "qmlprojecttarget.h"
#include "qmlprojectconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <extensionsystem/pluginmanager.h>
#include <qtsupport/qmldumptool.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtversionmanager.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <utils/fileutils.h>

#include <utils/filesystemwatcher.h>

#include <QtCore/QTextStream>
#include <QtDeclarative/QDeclarativeComponent>
#include <QtCore/QtDebug>

namespace PythonProjectManager {

PythonProject::PythonProject(Internal::Manager *manager, const QString &fileName)
    : m_manager(manager),
      m_fileName(fileName),
      m_modelManager(ExtensionSystem::PluginManager::instance()->getObject<PythonJS::ModelManagerInterface>()),
      m_fileWatcher(new Utils::FileSystemWatcher(this))
{
    m_fileWatcher->setObjectName(QLatin1String("PythonProjectWatcher"));
    setProjectContext(Core::Context(PythonProjectManager::Constants::PROJECTCONTEXT));
    setProjectLanguage(Core::Context(PythonProjectManager::Constants::LANG_QML));

    QFileInfo fileInfo(m_fileName);
    m_projectName = fileInfo.completeBaseName();

    m_file = new Internal::PythonProjectFile(this, fileName);
    m_rootNode = new Internal::PythonProjectNode(this, m_file);

    m_fileWatcher->addFile(fileName, Utils::FileSystemWatcher::WatchModifiedDate);
    connect(m_fileWatcher, SIGNAL(fileChanged(QString)),
            this, SLOT(refreshProjectFile()));

    m_manager->registerProject(this);
}

PythonProject::~PythonProject()
{
    m_manager->unregisterProject(this);

    delete m_projectItem.data();
    delete m_rootNode;
}

QDir PythonProject::projectDir() const
{
    return QFileInfo(file()->fileName()).dir();
}

QString PythonProject::filesFileName() const
{ return m_fileName; }

void PythonProject::parseProject(RefreshOptions options)
{
    Core::MessageManager *messageManager = Core::ICore::instance()->messageManager();
    if (options & Files) {
        if (options & ProjectFile)
            delete m_projectItem.data();
        if (!m_projectItem) {
            Utils::FileReader reader;
            if (reader.fetch(m_fileName)) {
                QDeclarativeComponent *component = new QDeclarativeComponent(&m_engine, this);
                component->setData(reader.data(), QUrl::fromLocalFile(m_fileName));
                if (component->isReady()
                    && qobject_cast<PythonProjectItem*>(component->create())) {
                    m_projectItem = qobject_cast<PythonProjectItem*>(component->create());
                    connect(m_projectItem.data(), SIGNAL(qmlFilesChanged(QSet<QString>, QSet<QString>)),
                            this, SLOT(refreshFiles(QSet<QString>, QSet<QString>)));
                } else {
                    messageManager->printToOutputPane(tr("Error while loading project file %1.").arg(m_fileName));
                    messageManager->printToOutputPane(component->errorString(), true);
                }
            } else {
                messageManager->printToOutputPane(tr("QML project: %1").arg(reader.errorString()), true);
            }
        }
        if (m_projectItem) {
            m_projectItem.data()->setSourceDirectory(projectDir().path());
            m_modelManager->updateSourceFiles(m_projectItem.data()->files(), true);
        }
        m_rootNode->refresh();
    }

    if (options & Configuration) {
        // update configuration
    }

    if (options & Files)
        emit fileListChanged();
}

void PythonProject::refresh(RefreshOptions options)
{
    parseProject(options);

    if (options & Files)
        m_rootNode->refresh();

    PythonJS::ModelManagerInterface::ProjectInfo pinfo(this);
    pinfo.sourceFiles = files();
    pinfo.importPaths = importPaths();
    QtSupport::BaseQtVersion *version = 0;
    if (activeTarget()) {
        if (PythonProjectRunConfiguration *rc = qobject_cast<PythonProjectRunConfiguration *>(activeTarget()->activeRunConfiguration()))
            version = rc->qtVersion();
        QtSupport::PythonDumpTool::pathAndEnvironment(this, version, false, &pinfo.qmlDumpPath, &pinfo.qmlDumpEnvironment);
    }
    m_modelManager->updateProjectInfo(pinfo);
}

QStringList PythonProject::convertToAbsoluteFiles(const QStringList &paths) const
{
    const QDir projectDir(QFileInfo(m_fileName).dir());
    QStringList absolutePaths;
    foreach (const QString &file, paths) {
        QFileInfo fileInfo(projectDir, file);
        absolutePaths.append(fileInfo.absoluteFilePath());
    }
    absolutePaths.removeDuplicates();
    return absolutePaths;
}

QStringList PythonProject::files() const
{
    QStringList files;
    if (m_projectItem) {
        files = m_projectItem.data()->files();
    } else {
        files = m_files;
    }
    return files;
}

QString PythonProject::mainFile() const
{
    if (m_projectItem)
        return m_projectItem.data()->mainFile();
    return QString();
}

bool PythonProject::validProjectFile() const
{
    return !m_projectItem.isNull();
}

QStringList PythonProject::importPaths() const
{
    QStringList importPaths;
    if (m_projectItem)
        importPaths = m_projectItem.data()->importPaths();

    // add the default import path for the target Qt version
    if (activeTarget()) {
        const PythonProjectRunConfiguration *runConfig =
                qobject_cast<PythonProjectRunConfiguration*>(activeTarget()->activeRunConfiguration());
        if (runConfig) {
            const QtSupport::BaseQtVersion *qtVersion = runConfig->qtVersion();
            if (qtVersion && qtVersion->isValid()) {
                const QString qtVersionImportPath = qtVersion->versionInfo().value("QT_INSTALL_IMPORTS");
                if (!qtVersionImportPath.isEmpty())
                    importPaths += qtVersionImportPath;
            }
        }
    }

    return importPaths;
}

bool PythonProject::addFiles(const QStringList &filePaths)
{
    QStringList toAdd;
    foreach (const QString &filePath, filePaths) {
        if (!m_projectItem.data()->matchesFile(filePath))
            toAdd << filePaths;
    }
    return toAdd.isEmpty();
}

void PythonProject::refreshProjectFile()
{
    refresh(PythonProject::ProjectFile | Files);
}

void PythonProject::refreshFiles(const QSet<QString> &/*added*/, const QSet<QString> &removed)
{
    refresh(Files);
    if (!removed.isEmpty())
        m_modelManager->removeFiles(removed.toList());
}

QString PythonProject::displayName() const
{
    return m_projectName;
}

QString PythonProject::id() const
{
    return QLatin1String("PythonProjectManager.PythonProject");
}

Core::IFile *PythonProject::file() const
{
    return m_file;
}

Internal::Manager *PythonProject::projectManager() const
{
    return m_manager;
}

QList<ProjectExplorer::Project *> PythonProject::dependsOn()
{
    return QList<Project *>();
}

QList<ProjectExplorer::BuildConfigWidget*> PythonProject::subConfigWidgets()
{
    return QList<ProjectExplorer::BuildConfigWidget*>();
}

Internal::PythonProjectTarget *PythonProject::activeTarget() const
{
    return static_cast<Internal::PythonProjectTarget *>(Project::activeTarget());
}

Internal::PythonProjectNode *PythonProject::rootProjectNode() const
{
    return m_rootNode;
}

QStringList PythonProject::files(FilesMode) const
{
    return files();
}

bool PythonProject::fromMap(const QVariantMap &map)
{
    if (!Project::fromMap(map))
        return false;

    if (targets().isEmpty()) {
        Internal::PythonProjectTargetFactory *factory
                = ExtensionSystem::PluginManager::instance()->getObject<Internal::PythonProjectTargetFactory>();
        Internal::PythonProjectTarget *target = factory->create(this, QLatin1String(Constants::QML_VIEWER_TARGET_ID));
        addTarget(target);
    }

    refresh(Everything);
    // FIXME workaround to guarantee that run/debug actions are enabled if a valid file exists
    if (activeTarget()) {
        PythonProjectRunConfiguration *runConfig = qobject_cast<PythonProjectRunConfiguration*>(activeTarget()->activeRunConfiguration());
        if (runConfig)
            runConfig->changeCurrentFile(0);
    }

    return true;
}

}

