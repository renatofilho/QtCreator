/*
 * Copyright (c) 2011 Instituto Nokia de Tecnologia
 *   Author: Paulo Alcantara <paulo.alcantara@openbossa.org>
 *
 * This software may be redistributed and/or modified under the terms of
 * the GNU General Public License ("GPL") version 2 as published by the
 * Free Software Foundation.
 */

#ifndef _PYTHONEDITOR_H_
#define _PYTHONEDITOR_H_

#include <texteditor/plaintexteditor.h>
#include <texteditor/quickfix.h>
#include <utils/uncommentselection.h>

#include <QtCore/QSharedPointer>
#include <QtCore/QSet>
#include <QtCore/QList>
#include <QtCore/QScopedPointer>

#include "pythoneditoreditable.h"

QT_BEGIN_NAMESPACE
class QComboBox;
class QTimer;
QT_END_NAMESPACE

namespace Core {
class ICore;
class MimeType;
}

namespace PythonEditor {

class Q_DECL_EXPORT PyTextEditorWidget : public TextEditor::PlainTextEditorWidget
{
    Q_OBJECT

public:
    PyTextEditorWidget(QWidget *parent = 0);
    ~PyTextEditorWidget();

    int editorRevision(void) const;
    bool isOutdated(void) const;

    QSet<QString> identifiers(void) const;

    int languageVariant(void) const;

    //Document::Ptr glslDocument() const;

    TextEditor::IAssistInterface *createAssistInterface(TextEditor::AssistKind assistKind,
                                                        TextEditor::AssistReason reason) const;

private slots:
    void updateDocument(void);
    void updateDocumentNow(void);

signals:
    void configured(Core::IEditor *editor);

protected:
    void createToolBar(Internal::PyEditorEditable *editable);
    virtual TextEditor::BaseTextEditor *createEditor();

private:
    void setSelectedElements(void);
    QString wordUnderCursor(void) const;

    QTimer *m_updateDocumentTimer;
    QComboBox *m_outlineCombo;
    //Document::Ptr m_glslDocument;
};

}

#endif /* _PYTHONEDITOR_H_ */
