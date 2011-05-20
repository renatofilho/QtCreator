/*
 * Copyright (c) 2011 Instituto Nokia de Tecnologia
 *   Author: Paulo Alcantara <paulo.alcantara@openbossa.org>
 *
 * This software may be redistributed and/or modified under the terms of
 * the GNU General Public License ("GPL") version 2 as published by the
 * Free Software Foundation.
 */

#ifndef _PYTHONFILEWIZARD_H_
#define _PYTHONFILEWIZARD_H_

#include <coreplugin/basefilewizard.h>
#include <utils/filewizarddialog.h>
#include <utils/qtcassert.h>
#include <utils/filewizarddialog.h>

namespace PythonEditor {

class PyFileWizardDialog;

class PyFileWizard: public Core::BaseFileWizard {
    Q_OBJECT

public:
    typedef Core::BaseFileWizardParameters BaseFileWizardParameters;

    explicit PyFileWizard(const BaseFileWizardParameters &parameters,
                            QObject *parent = 0);

protected:
    QString fileContents(const QString &baseName) const;

    virtual QWizard *createWizardDialog(QWidget *parent,
                                const QString &defaultPath,
                                const WizardPageList &extensionPages) const;

    virtual Core::GeneratedFiles generateFiles(const QWizard *w,
                                               QString *errorMessage) const;
    QString preferredSuffix(void) const;
};

class PyFileWizardDialog : public Utils::FileWizardDialog {
    Q_OBJECT

public:
    PyFileWizardDialog(QWidget *parent = 0)
        : Utils::FileWizardDialog(parent)
    {
    }
};

}

#endif /* _PYTHONFILEWIZARD_H_ */
