/*
 * Copyright (c) 2011 Instituto Nokia de Tecnologia
 *   Author: Paulo Alcantara <paulo.alcantara@openbossa.org>
 *
 * This software may be redistributed and/or modified under the terms of
 * the GNU General Public License ("GPL") version 2 as published by the
 * Free Software Foundation.
 */

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/basetextdocument.h>
#include <texteditor/fontsettings.h>
#include <texteditor/tabsettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/generichighlighter/manager.h>
#include <texteditor/generichighlighter/highlighter.h>
#include <texteditor/generichighlighter/highlightdefinition.h>
#include <texteditor/generichighlighter/highlightersettings.h>
#include <texteditor/generichighlighter/highlighterexception.h>
#include <texteditor/generichighlighter/context.h>
#include <texteditor/generichighlighter/highlightdefinitionhandler.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/plaintexteditor.h>
#include <texteditor/refactoroverlay.h>
#include <texteditor/normalindenter.h>
#include <texteditor/generichighlighter/highlightersettingspage.h>
#include <texteditor/tooltip/tooltip.h>
#include <qmldesigner/qmldesignerconstants.h>
#include <utils/changeset.h>
#include <utils/uncommentselection.h>

#include <QtCore/QFileInfo>
#include <QtCore/QSignalMapper>
#include <QtCore/QTimer>
#include <QtCore/QDebug>

#include <QtGui/QMenu>
#include <QtGui/QComboBox>
#include <QtGui/QHeaderView>
#include <QtGui/QInputDialog>
#include <QtGui/QMainWindow>
#include <QtGui/QToolBar>
#include <QtGui/QTreeView>
#include <QtCore/QSharedPointer>
#include <QtCore/QFileInfo>

#include "pythoneditor.h"
#include "pythoneditoreditable.h"
#include "pythoneditorplugin.h"
#include "pythoneditorconstants.h"
#include "pythonindenter.h"

enum {
    UPDATE_DOCUMENT_DEFAULT_INTERVAL = 150,
};

using namespace PythonEditor;
using namespace PythonEditor::Internal;

PyTextEditorWidget::PyTextEditorWidget(QWidget *parent)
    : TextEditor::PlainTextEditorWidget(parent),
    m_outlineCombo(0)
{
    qDebug() << "PyTextEditorWidget::PyTextEditorWidget";
    setParenthesesMatchingEnabled(true);
    setCodeFoldingSupported(true);
    setRevisionsVisible(true);
    setMarksVisible(true);
    setRequestMarkEnabled(false);
    setLineSeparatorsAllowed(true);

    //setMimeType(QLatin1String(TextEditor::Constants::C_TEXTEDITOR_MIMETYPE_TEXT));
    //setDisplayName(tr(Core::Constants::K_DEFAULT_TEXT_EDITOR_DISPLAY_NAME));

    setIndenter(new Indenter);

    /*
    m_updateDocumentTimer = new QTimer(this);
    m_updateDocumentTimer->setInterval(UPDATE_DOCUMENT_DEFAULT_INTERVAL);
    m_updateDocumentTimer->setSingleShot(true);
    connect(m_updateDocumentTimer, SIGNAL(timeout()), this, SLOT(updateDocumentNow()));
    connect(this, SIGNAL(textChanged()), this, SLOT(updateDocument()));
    */
    //connect(file(), SIGNAL(changed()), this, SLOT(configure()));
    //connect(Manager::instance(), SIGNAL(mimeTypesRegistered()), this, SLOT(configure()));

//    if (m_modelManager) {
//        m_semanticHighlighter->setModelManager(m_modelManager);
//        connect(m_modelManager, SIGNAL(documentUpdated(Py::Document::Ptr)),
//                this, SLOT(onDocumentUpdated(Py::Document::Ptr)));
//        connect(m_modelManager, SIGNAL(libraryInfoUpdated(QString,Py::LibraryInfo)),
//                this, SLOT(forceSemanticRehighlight()));
//        connect(this->document(), SIGNAL(modificationChanged(bool)), this, SLOT(modificationChanged(bool)));
//    }
}

void PyTextEditorWidget::updateDocument(void)
{
    m_updateDocumentTimer->start();
}

void PyTextEditorWidget::updateDocumentNow(void)
{
    m_updateDocumentTimer->stop();
}

PyTextEditorWidget::~PyTextEditorWidget(void)
{
}

int PyTextEditorWidget::editorRevision(void) const
{
    //return document()->revision();
    return 0;
}

bool PyTextEditorWidget::isOutdated(void) const
{
//    if (m_semanticInfo.revision() != editorRevision())
//        return true;

    return false;
}

Core::IEditor *PyEditorEditable::duplicate(QWidget *parent)
{
    qDebug() << "PyEditorEditable::duplicate()";

    PyTextEditorWidget *newEditor = new PyTextEditorWidget(parent);
    newEditor->duplicateFrom(editorWidget());
    PyEditorPlugin::instance()->initializeEditor(newEditor);

    return newEditor->editor();
}

QString PyEditorEditable::id(void) const
{
    return QLatin1String(PythonEditor::Constants::PYTHONEDITOR_ID);
}

bool PyEditorEditable::open(QString *errorString, const QString &fileName,
                                const QString &realFileName)
{
    editorWidget()->setMimeType(
        Core::ICore::instance()->mimeDatabase()->findByFile(QFileInfo(
                                                            fileName)).type());
    bool b = TextEditor::BaseTextEditor::open(errorString, fileName,
                                                realFileName);
    return b;
}

QString PyTextEditorWidget::wordUnderCursor(void) const
{
    QTextCursor tc = textCursor();
    const QChar ch = characterAt(tc.position() - 1);
    // make sure that we're not at the start of the next word.
    if (ch.isLetterOrNumber() || ch == QLatin1Char('_'))
        tc.movePosition(QTextCursor::Left);
    tc.movePosition(QTextCursor::StartOfWord);
    tc.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    const QString word = tc.selectedText();

    return word;
}

TextEditor::BaseTextEditor *PyTextEditorWidget::createEditor(void)
{
    PyEditorEditable *editable = new PyEditorEditable(this);
    createToolBar(editable);

    return editable;
}

void PyTextEditorWidget::createToolBar(PyEditorEditable *editor)
{
    m_outlineCombo = new QComboBox;
    m_outlineCombo->setMinimumContentsLength(22);

    // ### m_outlineCombo->setModel(m_outlineModel);

    QTreeView *treeView = new QTreeView;
    treeView->header()->hide();
    treeView->setItemsExpandable(false);
    treeView->setRootIsDecorated(false);
    m_outlineCombo->setView(treeView);
    treeView->expandAll();

    //m_outlineCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    // Make the combo box prefer to expand
    QSizePolicy policy = m_outlineCombo->sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Expanding);
    m_outlineCombo->setSizePolicy(policy);

    editor->insertExtraToolBarWidget(TextEditor::BaseTextEditor::Left,
                                        m_outlineCombo);
}

TextEditor::IAssistInterface *PyTextEditorWidget::createAssistInterface(
    TextEditor::AssistKind kind,
    TextEditor::AssistReason reason) const
{
    /* hacked up! */
    Q_UNUSED(kind);
    Q_UNUSED(reason);

    /*
    if (kind == TextEditor::Completion)
        return new PyCompletionAssistInterface(document(),
                                                 position(),
                                                 editor()->file(),
                                                 reason,
                                                 mimeType(),
                                                 glslDocument());
    */
    //return BaseTextEditorWidget::createAssistInterface(kind, reason);
    return 0;
}
