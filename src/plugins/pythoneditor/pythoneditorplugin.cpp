/*
 * Copyright (c) 2011 Instituto Nokia de Tecnologia
 *   Author: Paulo Alcantara <paulo.alcantara@openbossa.org>
 *
 * This software may be redistributed and/or modified under the terms of
 * the GNU General Public License ("GPL") version 2 as published by the
 * Free Software Foundation.
 */

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/taskhub.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/fontsettings.h>
#include <texteditor/storagesettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/textfilewizard.h>
#include <texteditor/texteditoractionhandler.h>
#include <utils/qtcassert.h>

#include <QtCore/QtPlugin>
#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtCore/QDir>
#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>
#include <QtGui/QMenu>
#include <QtGui/QAction>
#include <QDebug>

#include "pythoneditorplugin.h"
#include "pythoneditor.h"
#include "pythoneditorfactory.h"
#include "pythoneditorconstants.h"
#include "pythoneditoreditable.h"
#include "pythonfilewizard.h"
#include "pythonhoverhandler.h"

using namespace PythonEditor;
using namespace PythonEditor::Internal;
using namespace PythonEditor::Constants;

PyEditorPlugin *PyEditorPlugin::m_instance = 0;

PyEditorPlugin::PyEditorPlugin() :
    m_editor(0),
    m_actionHandler(0)
{
    m_instance = this;
}

PyEditorPlugin::~PyEditorPlugin()
{
    removeObject(m_editor);
    delete m_actionHandler;
    m_instance = 0;
}

/*! Copied from cppplugin.cpp */
static inline
Core::Command *createSeparator(Core::ActionManager *am,
                               QObject *parent,
                               Core::Context &context,
                               const char *id)
{
    QAction *separator = new QAction(parent);
    separator->setSeparator(true);
    return am->registerAction(separator, Core::Id(id), context);
}

Core::Command *PyEditorPlugin::addToolAction(QAction *a,
                                            Core::ActionManager *am,
                                            Core::Context &context,
                                            const QString &name,
                                            Core::ActionContainer *c1,
                                            const QString &keySequence)
{
    Core::Command *command = am->registerAction(a, name, context);

    if (!keySequence.isEmpty())
        command->setDefaultKeySequence(QKeySequence(keySequence));

    c1->addAction(command);

    return command;
}

/* the fight starts here! :-( */
bool PyEditorPlugin::initialize(const QStringList & /*arguments*/, QString *error_message)
{
    qDebug() << "PyEditorPlugin::initialize()";

    Core::ICore *core = Core::ICore::instance();
    if (!core->mimeDatabase()->addMimeTypes(
            QLatin1String(":/pythoneditor/PythonEditor.mimetypes.xml"),
            error_message))
        return false;

    addAutoReleasedObject(new PyHoverHandler(this));

    Core::Context context(PythonEditor::Constants::PYTHONEDITOR_ID);

    m_editor = new PyEditorFactory(this);
    addObject(m_editor);

    m_actionHandler =
        new TextEditor::TextEditorActionHandler(
                    PythonEditor::Constants::PYTHONEDITOR_ID,
                    TextEditor::TextEditorActionHandler::Format |
                    TextEditor::TextEditorActionHandler::UnCommentSelection |
                    TextEditor::TextEditorActionHandler::UnCollapseAll);

    m_actionHandler->initializeActions();

    Core::ActionManager *am =  core->actionManager();
    Core::ActionContainer *contextMenu =
                    am->createMenu(PythonEditor::Constants::M_CONTEXT);
    Core::ActionContainer *pythonToolsMenu =
                    am->createMenu(Core::Id(Constants::M_TOOLS_PYTHON));
    pythonToolsMenu->setOnAllDisabledBehavior(Core::ActionContainer::Hide);

    QMenu *menu = pythonToolsMenu->menu();
    //: Python sub-menu in the Tools menu
    menu->setTitle(tr("Python"));
    am->actionContainer(Core::Constants::M_TOOLS)->addMenu(pythonToolsMenu);

    Core::Command *cmd = 0;

    // Insert marker for "Refactoring" menu:
    Core::Context globalContext(Core::Constants::C_GLOBAL);
    Core::Command *sep = createSeparator(am, this, globalContext,
                                         Constants::SEPARATOR1);
    sep->action()->setObjectName(Constants::M_REFACTORING_MENU_INSERTION_POINT);
    contextMenu->addAction(sep);
    contextMenu->addAction(createSeparator(am, this, globalContext,
                                           Constants::SEPARATOR2));

    cmd = am->command(TextEditor::Constants::UN_COMMENT_SELECTION);
    contextMenu->addAction(cmd);

    error_message->clear();

    Core::BaseFileWizardParameters fragWizardParameters(
                                        Core::IWizard::FileWizard);
    fragWizardParameters.setCategory(QLatin1String(
                                        Constants::WIZARD_CATEGORY_PYTHON));
    fragWizardParameters.setDisplayCategory(QCoreApplication::translate(
                                    "PythonEditor",
                                    Constants::WIZARD_TR_CATEGORY_PYTHON));
    fragWizardParameters.setDescription(tr("Let's do python :-)"));
    fragWizardParameters.setDisplayName(tr("Python Source File"));
    fragWizardParameters.setId(QLatin1String("PY"));
    addAutoReleasedObject(new PyFileWizard(fragWizardParameters, core));

    qDebug() << "PyEditorPlugin::initialize() will return";
    return true;
}

void PyEditorPlugin::extensionsInitialized(void)
{
}

ExtensionSystem::IPlugin::ShutdownFlag PyEditorPlugin::aboutToShutdown(void)
{
    // delete Py::Icons::instance(); // delete object held by singleton

    return IPlugin::aboutToShutdown();
}

void PyEditorPlugin::initializeEditor(PythonEditor::PyTextEditorWidget *editor)
{
    QTC_ASSERT(m_instance, /**/);

    qDebug() << "PyEditorPlugin::initializeEditor()";
    m_actionHandler->setupActions(editor);

    TextEditor::TextEditorSettings::instance()->initializeEditor(editor);
}

QByteArray PyEditorPlugin::pyFile(const QString &fileName)
{
    QString path = Core::ICore::instance()->resourcePath();
    path += QLatin1String("/py/");
    path += fileName;
    QFile file(path);
    if (file.open(QFile::ReadOnly))
        return file.readAll();

    return QByteArray();
}

Q_EXPORT_PLUGIN(PyEditorPlugin)
