/*
 * Copyright (c) 2011 Instituto Nokia de Tecnologia
 *   Author: Paulo Alcantara <paulo.alcantara@openbossa.org>
 *
 * This software may be redistributed and/or modified under the terms of
 * the GNU General Public License ("GPL") version 2 as published by the
 * Free Software Foundation.
 */

#include "pythonindenter.h"

#include <texteditor/basetexteditor.h>
#include <texteditor/tabsettings.h>

#include <QtCore/QChar>
#include <QtGui/QTextDocument>
#include <QtGui/QTextBlock>
#include <QtGui/QTextCursor>
#include <QRegExp>
#include <QDebug>

using namespace PythonEditor;
using namespace PythonEditor::Internal;

Indenter::Indenter()
    :   m_tabSize(0)
{
}

Indenter::~Indenter()
{
}

bool Indenter::isElectricCharacter(const QChar &ch) const
{
    qDebug() << "Indenter::isElectricCharacter()";

    return ch == QLatin1Char(':');
}

static size_t wspaces_count(const QString &line)
{
    size_t ret = 0;

    for (int i = 0; i < line.size(); ++i) {
        if (line[i] != ' ')
            break;

        ++ret;
    }

    return ret;
}

void Indenter::indentBlock(QTextDocument *doc,
                           const QTextBlock &block,
                           const QChar &typedChar,
                           TextEditor::BaseTextEditorWidget *editor)
{
    Q_UNUSED(doc)
    Q_UNUSED(editor)
    Q_UNUSED(typedChar)

    qDebug() << "Indenter::indentBlock()";

    const TextEditor::TabSettings &ts = editor->tabSettings();

    int tabSize = ts.m_tabSize;
    if (tabSize > 4)
        tabSize = 4;    /* let's assume 4 for now */

    const QTextBlock pblock = block.previous();
    const QString line = pblock.text();
    qDebug() << "line:" << line;
    const QChar c = QChar(line[line.size() - 1]);
    QRegExp wspacesOnly("^[\\s]*$");
    QRegExp elseKeyword("^[\\s]*else(.*:)$");
    QRegExp elifKeyword("^[\\s]*elif[\\s]*(.*:)$");
    QRegExp exceptKeyword("^[\\s]*except[\\s]*(.*:)$");

    wspacesOnly.setMinimal(true);
    elseKeyword.setMinimal(true);
    elifKeyword.setMinimal(true);
    exceptKeyword.setMinimal(true);

    if (isElectricCharacter(c)) {
        if (elseKeyword.exactMatch(line) &&
            m_tabSize == wspaces_count(line)) {
            unsigned tsize = wspaces_count(line) - tabSize;

            QString s("else");
            s.append(elseKeyword.cap(1));
            pblock.text() = s;
            ts.indentLine(pblock, tsize);
            tsize = wspaces_count(line);
            ts.indentLine(block, tsize);
            m_tabSize = tsize;
        } else if (elifKeyword.exactMatch(line) &&
                    m_tabSize == wspaces_count(line)) {
            unsigned tsize = wspaces_count(line) - tabSize;

            QString s("elif");
            s.append(elifKeyword.cap(1));
            pblock.text() = s;
            ts.indentLine(pblock, tsize);
            tsize = wspaces_count(line);
            ts.indentLine(block, tsize);
            m_tabSize = tsize;
        } else if (exceptKeyword.exactMatch(line) &&
                    m_tabSize == wspaces_count(line)) {
            unsigned tsize = wspaces_count(line) - tabSize;

            QString s("except");
            s.append(exceptKeyword.cap(1));
            pblock.text() = s;
            ts.indentLine(pblock, tsize);
            tsize = wspaces_count(line);
            ts.indentLine(block, tsize);
            m_tabSize = tsize;
        } else {
            m_tabSize = wspaces_count(line) + tabSize;
            ts.indentLine(block, m_tabSize);
        }
    } else if (wspacesOnly.exactMatch(line)) {
        m_tabSize  = m_tabSize > 0 ? m_tabSize - tabSize : m_tabSize;
        ts.indentLine(block, m_tabSize);
    } else {
        m_tabSize = wspaces_count(line);
        ts.indentLine(block, m_tabSize);
    }
}
