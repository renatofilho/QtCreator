/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qmljseditoreditable.h"
#include "qmljseditor.h"
#include "qmljseditorconstants.h"

#include <texteditor/texteditorconstants.h>
#include <qmldesigner/qmldesignerconstants.h>

#include <coreplugin/mimedatabase.h>
#include <coreplugin/icore.h>
#include <coreplugin/designmode.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/coreconstants.h>

namespace QmlJSEditor {
namespace Internal {

QmlJSEditorEditable::QmlJSEditorEditable(QmlJSTextEditor *editor)
    : BaseTextEditorEditable(editor)
{
    m_context.add(QmlJSEditor::Constants::C_QMLJSEDITOR_ID);
    m_context.add(TextEditor::Constants::C_TEXTEDITOR);
}

// Use preferred mode from Bauhaus settings
static bool openInDesignMode()
{
    static bool bauhausDetected = false;
    static bool bauhausPresent = false;
    // Check if Bauhaus is loaded, that is, a Design mode widget is
    // registered for the QML mime type.
    if (!bauhausDetected) {
        if (const Core::IMode *dm = Core::ModeManager::instance()->mode(QLatin1String(Core::Constants::MODE_DESIGN)))
            if (const Core::DesignMode *designMode = qobject_cast<const Core::DesignMode *>(dm))
                bauhausPresent = designMode->registeredMimeTypes().contains(QLatin1String(QmlJSEditor::Constants::QML_MIMETYPE));
        bauhausDetected =  true;
    }
    if (!bauhausPresent)
        return false;

    return bool(QmlDesigner::Constants::QML_OPENDESIGNMODE_DEFAULT);
}

QString QmlJSEditorEditable::preferredModeType() const
{
    Core::ModeManager *modeManager = Core::ModeManager::instance();
    if (modeManager->currentMode()
            && (modeManager->currentMode()->type() == Core::Constants::MODE_DESIGN_TYPE
                || modeManager->currentMode()->type() == Core::Constants::MODE_EDIT_TYPE))
    {
        return modeManager->currentMode()->type();
    }

    // if we are in other mode than edit or design, use the hard-coded default.
    // because the editor opening decision is modal, it would be confusing to
    // have the user also access to this failsafe setting.
    if (editor()->mimeType() == QLatin1String(QmlJSEditor::Constants::QML_MIMETYPE)
        && openInDesignMode())
        return QLatin1String(Core::Constants::MODE_DESIGN_TYPE);
    return QString();
}

} // namespace Internal
} // namespace QmlJSEditor