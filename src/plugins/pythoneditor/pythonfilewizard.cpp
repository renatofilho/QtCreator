/*
 * Copyright (c) 2011 Instituto Nokia de Tecnologia
 *   Author: Paulo Alcantara <paulo.alcantara@openbossa.org>
 *
 * This software may be redistributed and/or modified under the terms of
 * the GNU General Public License ("GPL") version 2 as published by the
 * Free Software Foundation.
 */

#include <utils/filewizarddialog.h>
#include <utils/qtcassert.h>
#include <utils/filewizarddialog.h>

#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>
#include <QtGui/QWizard>
#include <QtGui/QPushButton>

#include "pythonfilewizard.h"
#include "pythoneditorconstants.h"

using namespace PythonEditor;

PyFileWizard::PyFileWizard(const BaseFileWizardParameters &parameters,
                            QObject *parent):
    Core::BaseFileWizard(parameters, parent)
{
}

QString PyFileWizard::preferredSuffix(void) const
{
    return QLatin1String("py");
}

Core::GeneratedFiles PyFileWizard::generateFiles(const QWizard *w,
                                        QString * /*errorMessage*/) const
{
    const PyFileWizardDialog *wizardDialog =
                        qobject_cast<const PyFileWizardDialog *>(w);
    const QString path = wizardDialog->path();
    const QString name = wizardDialog->fileName();

    const QString fileName =
        Core::BaseFileWizard::buildFileName(path, name, preferredSuffix());

    Core::GeneratedFile file(fileName);
    file.setContents(fileContents(fileName));
    file.setAttributes(Core::GeneratedFile::OpenEditorAttribute);

    return Core::GeneratedFiles() << file;
}

QString PyFileWizard::fileContents(const QString &) const
{
    QString contents;
    QTextStream str(&contents);

    str << QLatin1String("# -*- coding: utf-8 -*-\n\n");

    return contents;
}

QWizard *PyFileWizard::createWizardDialog(QWidget *parent,
                                    const QString &defaultPath,
                                    const WizardPageList &extensionPages) const
{
    PyFileWizardDialog *wizardDialog = new PyFileWizardDialog(parent);
    wizardDialog->setWindowTitle(tr("New %1").arg(displayName()));
    setupWizard(wizardDialog);
    wizardDialog->setPath(defaultPath);
    foreach (QWizardPage *p, extensionPages)
        BaseFileWizard::applyExtensionPageShortTitle(wizardDialog,
                                                    wizardDialog->addPage(p));

    return wizardDialog;
}

//#include "pythonfilewizard.moc"
