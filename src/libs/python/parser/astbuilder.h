/***************************************************************************
 *   This file is part of KDevelop                                         *
 *   Copyright 2007 Andreas Pakulat <apaku@gmx.de>                         *
 *   Copyright 2010-2011 Sven Brauch <svenbrauch@googlemail.com>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef ASTBUILDER_H
#define ASTBUILDER_H

#include "ast.h"
#include <QUrl>
//#include <QDomDocument>
//#include "kdebug.h"
#include "QXmlStreamReader"
#include <QMutex>
//#include <language/duchain/topducontext.h>
#include "parserexport.h"

namespace PythonParser
{
    class Parser;
    class AstNode;
}

namespace Python
{
    class Parser;
    class AstNode;
    class Ast;
    class CodeAst;

typedef QMap<QString, QString> stringDictionary;

class PYTHON_EXPORT AstBuilder
{
    
public:
    CodeAst* parse(QUrl filename, QString& contents);
    //QList<KDevelop::ProblemPointer> m_problems;
private:
    static QMutex pyInitLock;
    static QString pyHomeDir;
};

}

#endif