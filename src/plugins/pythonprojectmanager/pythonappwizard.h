#ifndef _PYTHONAPPWIZARD_H_
#define _PYTHONAPPWIZARD_H_

#include <coreplugin/basefilewizard.h>
#include <projectexplorer/baseprojectwizarddialog.h>

namespace PythonProjectManager {
namespace Internal {

class PythonAppWizardDialog : public ProjectExplorer::BaseProjectWizardDialog
{
    Q_OBJECT
public:
    explicit PythonAppWizardDialog(QWidget *parent = 0);
};

class PythonAppWizard : public Core::BaseFileWizard
{
    Q_OBJECT

public:
    PythonAppWizard();
    virtual ~PythonAppWizard();

    static Core::BaseFileWizardParameters parameters(void);

protected:
    virtual QWizard *createWizardDialog(QWidget *parent,
                                        const QString &defaultPath,
                                        const WizardPageList &extensionPages) const;

    virtual Core::GeneratedFiles generateFiles(const QWizard *w,
                                               QString *errorMessage) const;

    virtual bool postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage);
};

}
}

#endif /* _PYTHONAPPWIZARD_H_ */
