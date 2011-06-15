/***************************************************************************
 *   This file is part of KDevelop                                         *
 *   Copyright 2008 Andreas Pakulat <apaku@gmx.de>                         *
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

#include "classdefinitiontest.h"
#include <QtTest/QtTest>
#include <qtest_kde.h>

#include "datahelper.h"


using namespace Python;

Q_DECLARE_METATYPE(CodeAst*)
QTEST_KDEMAIN_CORE( ClassDefinitionTest )

extern CodeAst* simpleClassDefinition();

ClassDefinitionTest::ClassDefinitionTest(QObject* parent )
    : QObject( parent )
{
}

ClassDefinitionTest::~ClassDefinitionTest()
{
}

void ClassDefinitionTest::simpleDefinition_data( )
{
    QTest::addColumn<QString>("project");
    QTest::addColumn<CodeAst*>("expected");
    QTest::newRow( "simple class definition" ) << "class Foo:\n  pass\n" << simpleClassDefinition();
}

void ClassDefinitionTest::simpleDefinition( )
{
    QFETCH( QString, project );
    QFETCH( CodeAst*, expected );
    doTest( project, expected );
}

