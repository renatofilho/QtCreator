/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "maemodeviceconfigwizard.h"
#include "ui_maemodeviceconfigwizardkeycreationpage.h"
#include "ui_maemodeviceconfigwizardkeydeploymentpage.h"
#include "ui_maemodeviceconfigwizardlogindatapage.h"
#include "ui_maemodeviceconfigwizardpreviouskeysetupcheckpage.h"
#include "ui_maemodeviceconfigwizardreusekeyscheckpage.h"
#include "ui_maemodeviceconfigwizardstartpage.h"

#include "linuxdeviceconfigurations.h"
#include "maemoglobal.h"
#include "maemokeydeployer.h"

#include <utils/fileutils.h>
#include <utils/ssh/sshkeygenerator.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtGui/QButtonGroup>
#include <QtGui/QDesktopServices>
#include <QtGui/QMessageBox>
#include <QtGui/QWizardPage>

using namespace Utils;

namespace RemoteLinux {
namespace Internal {
namespace {

struct WizardData
{
    QString configName;
    QString hostName;
    QString osType;
    SshConnectionParameters::AuthenticationType authType;
    LinuxDeviceConfiguration::DeviceType deviceType;
    QString privateKeyFilePath;
    QString publicKeyFilePath;
    QString userName;
    QString password;
};

enum PageId {
    StartPageId, LoginDataPageId, PreviousKeySetupCheckPageId,
    ReuseKeysCheckPageId, KeyCreationPageId, KeyDeploymentPageId, FinalPageId
};

class MaemoDeviceConfigWizardStartPage : public QWizardPage
{
    Q_OBJECT
public:
    MaemoDeviceConfigWizardStartPage(QWidget *parent = 0)
        : QWizardPage(parent), m_ui(new Ui::MaemoDeviceConfigWizardStartPage)
    {
        m_ui->setupUi(this);
        setTitle(tr("General Information"));
        setSubTitle(QLatin1String(" ")); // For Qt bug (background color)
        m_ui->fremantleButton->setText(MaemoGlobal::osTypeToString(LinuxDeviceConfiguration::Maemo5OsType));
        m_ui->harmattanButton->setText(MaemoGlobal::osTypeToString(LinuxDeviceConfiguration::HarmattanOsType));
        m_ui->meegoButton->setText(MaemoGlobal::osTypeToString(LinuxDeviceConfiguration::MeeGoOsType));
        m_ui->genericLinuxButton->setText(MaemoGlobal::osTypeToString(LinuxDeviceConfiguration::GenericLinuxOsType));

        QButtonGroup *buttonGroup = new QButtonGroup(this);
        buttonGroup->setExclusive(true);
        buttonGroup->addButton(m_ui->hwButton);
        buttonGroup->addButton(m_ui->qemuButton);
        connect(buttonGroup, SIGNAL(buttonClicked(int)),
           SLOT(handleDeviceTypeChanged()));

        buttonGroup = new QButtonGroup(this);
        buttonGroup->setExclusive(true);
        buttonGroup->addButton(m_ui->fremantleButton);
        buttonGroup->addButton(m_ui->harmattanButton);
        buttonGroup->addButton(m_ui->meegoButton);
        buttonGroup->addButton(m_ui->genericLinuxButton);
        connect(buttonGroup, SIGNAL(buttonClicked(int)),
           SLOT(handleOsTypeChanged()));

        m_ui->nameLineEdit->setText(QLatin1String("(New Configuration)"));
        m_ui->harmattanButton->setChecked(true);
        m_ui->hwButton->setChecked(true);
        handleDeviceTypeChanged();
        m_ui->hostNameLineEdit->setText(LinuxDeviceConfiguration::defaultHost(deviceType(),
            osType()));
        connect(m_ui->nameLineEdit, SIGNAL(textChanged(QString)), this,
            SIGNAL(completeChanged()));
        connect(m_ui->hostNameLineEdit, SIGNAL(textChanged(QString)), this,
            SIGNAL(completeChanged()));
    }

    virtual bool isComplete() const
    {
        return !configName().isEmpty() && !hostName().isEmpty();
    }

    QString configName() const { return m_ui->nameLineEdit->text().trimmed(); }

    QString hostName() const
    {
        return deviceType() == LinuxDeviceConfiguration::Emulator
            ? LinuxDeviceConfiguration::defaultHost(LinuxDeviceConfiguration::Emulator, osType())
            : m_ui->hostNameLineEdit->text().trimmed();
    }

    QString osType() const
    {
        return m_ui->fremantleButton->isChecked() ? LinuxDeviceConfiguration::Maemo5OsType
            : m_ui->harmattanButton->isChecked() ? LinuxDeviceConfiguration::HarmattanOsType
            : m_ui->meegoButton->isChecked() ? LinuxDeviceConfiguration::MeeGoOsType
            : LinuxDeviceConfiguration::GenericLinuxOsType;
    }

    LinuxDeviceConfiguration::DeviceType deviceType() const
    {
        return m_ui->hwButton->isChecked()
            ? LinuxDeviceConfiguration::Physical : LinuxDeviceConfiguration::Emulator;
    }

private slots:
    void handleDeviceTypeChanged()
    {
        const bool enable = deviceType() == LinuxDeviceConfiguration::Physical;
        m_ui->hostNameLabel->setEnabled(enable);
        m_ui->hostNameLineEdit->setEnabled(enable);
    }

    void handleOsTypeChanged()
    {
        if (osType() == LinuxDeviceConfiguration::GenericLinuxOsType) {
            m_ui->hwButton->setChecked(true);
            m_ui->hwButton->setEnabled(false);
            m_ui->qemuButton->setEnabled(false);
        } else {
            m_ui->hwButton->setEnabled(true);
            m_ui->qemuButton->setEnabled(true);
        }
        handleDeviceTypeChanged();
    }

private:
    const QScopedPointer<Ui::MaemoDeviceConfigWizardStartPage> m_ui;
};

class MaemoDeviceConfigWizardLoginDataPage : public QWizardPage
{
    Q_OBJECT

public:
    MaemoDeviceConfigWizardLoginDataPage(WizardData &wizardData, QWidget *parent)
        : QWizardPage(parent),
          m_ui(new Ui::MaemoDeviceConfigWizardLoginDataPage),
          m_wizardData(wizardData)
    {
        m_ui->setupUi(this);
        setTitle(tr("Login Data"));
        m_ui->privateKeyPathChooser->setExpectedKind(PathChooser::File);
        setSubTitle(QLatin1String(" ")); // For Qt bug (background color)
        connect(m_ui->userNameLineEdit, SIGNAL(textChanged(QString)),
            SIGNAL(completeChanged()));
        connect(m_ui->privateKeyPathChooser, SIGNAL(validChanged()),
            SIGNAL(completeChanged()));
        connect(m_ui->passwordButton, SIGNAL(toggled(bool)),
            SLOT(handleAuthTypeChanged()));
    }

    virtual bool isComplete() const
    {
        return !userName().isEmpty()
            && (authType() == SshConnectionParameters::AuthenticationByPassword
                || m_ui->privateKeyPathChooser->isValid());
    }

    virtual void initializePage()
    {
        m_ui->userNameLineEdit->setText(LinuxDeviceConfiguration::defaultUser(m_wizardData.osType));
        m_ui->passwordButton->setChecked(true);
        m_ui->passwordLineEdit->clear();
        m_ui->privateKeyPathChooser->setPath(LinuxDeviceConfiguration::defaultPrivateKeyFilePath());
        handleAuthTypeChanged();
    }

    SshConnectionParameters::AuthenticationType authType() const
    {
        return m_ui->passwordButton->isChecked()
            ? SshConnectionParameters::AuthenticationByPassword
            : SshConnectionParameters::AuthenticationByKey;
    }

    QString userName() const { return m_ui->userNameLineEdit->text().trimmed(); }
    QString password() const { return m_ui->passwordLineEdit->text(); }
    QString privateKeyFilePath() const { return m_ui->privateKeyPathChooser->path(); }

private:
    Q_SLOT void handleAuthTypeChanged()
    {
        m_ui->passwordLineEdit->setEnabled(authType() == SshConnectionParameters::AuthenticationByPassword);
        m_ui->privateKeyPathChooser->setEnabled(authType() == SshConnectionParameters::AuthenticationByKey);
        emit completeChanged();
    }

    const QScopedPointer<Ui::MaemoDeviceConfigWizardLoginDataPage> m_ui;
    const WizardData &m_wizardData;
};

class MaemoDeviceConfigWizardPreviousKeySetupCheckPage : public QWizardPage
{
    Q_OBJECT
public:
    MaemoDeviceConfigWizardPreviousKeySetupCheckPage(QWidget *parent)
        : QWizardPage(parent),
          m_ui(new Ui::MaemoDeviceConfigWizardCheckPreviousKeySetupPage)
    {
        m_ui->setupUi(this);
        m_ui->privateKeyFilePathChooser->setExpectedKind(PathChooser::File);
        setTitle(tr("Device Status Check"));
        setSubTitle(QLatin1String(" ")); // For Qt bug (background color)
        QButtonGroup * const buttonGroup = new QButtonGroup(this);
        buttonGroup->setExclusive(true);
        buttonGroup->addButton(m_ui->keyWasSetUpButton);
        buttonGroup->addButton(m_ui->keyWasNotSetUpButton);
        connect(buttonGroup, SIGNAL(buttonClicked(int)),
            SLOT(handleSelectionChanged()));
        connect(m_ui->privateKeyFilePathChooser, SIGNAL(changed(QString)),
            this, SIGNAL(completeChanged()));
    }

    virtual bool isComplete() const
    {
        return !keyBasedLoginWasSetup()
            || m_ui->privateKeyFilePathChooser->isValid();
    }

    virtual void initializePage()
    {
        m_ui->keyWasNotSetUpButton->setChecked(true);
        m_ui->privateKeyFilePathChooser->setPath(LinuxDeviceConfiguration::defaultPrivateKeyFilePath());
        handleSelectionChanged();
    }

    bool keyBasedLoginWasSetup() const {
        return m_ui->keyWasSetUpButton->isChecked();
    }

    QString privateKeyFilePath() const {
        return m_ui->privateKeyFilePathChooser->path();
    }

private:
    Q_SLOT void handleSelectionChanged()
    {
        m_ui->privateKeyFilePathChooser->setEnabled(keyBasedLoginWasSetup());
        emit completeChanged();
    }

    const QScopedPointer<Ui::MaemoDeviceConfigWizardCheckPreviousKeySetupPage> m_ui;
};

class MaemoDeviceConfigWizardReuseKeysCheckPage : public QWizardPage
{
    Q_OBJECT
public:
    MaemoDeviceConfigWizardReuseKeysCheckPage(QWidget *parent)
        : QWizardPage(parent),
          m_ui(new Ui::MaemoDeviceConfigWizardReuseKeysCheckPage)
    {
        m_ui->setupUi(this);
        setTitle(tr("Existing Keys Check"));
        setSubTitle(QLatin1String(" ")); // For Qt bug (background color)
        m_ui->publicKeyFilePathChooser->setExpectedKind(PathChooser::File);
        m_ui->privateKeyFilePathChooser->setExpectedKind(PathChooser::File);
        QButtonGroup * const buttonGroup = new QButtonGroup(this);
        buttonGroup->setExclusive(true);
        buttonGroup->addButton(m_ui->reuseButton);
        buttonGroup->addButton(m_ui->dontReuseButton);
        connect(buttonGroup, SIGNAL(buttonClicked(int)),
            SLOT(handleSelectionChanged()));
        connect(m_ui->privateKeyFilePathChooser, SIGNAL(changed(QString)),
            this, SIGNAL(completeChanged()));
        connect(m_ui->publicKeyFilePathChooser, SIGNAL(changed(QString)),
            this, SIGNAL(completeChanged()));
    }

    virtual bool isComplete() const
    {
        return !reuseKeys() || (m_ui->publicKeyFilePathChooser->isValid()
            && m_ui->privateKeyFilePathChooser->isValid());
    }

    virtual void initializePage()
    {
        m_ui->dontReuseButton->setChecked(true);
        m_ui->privateKeyFilePathChooser->setPath(LinuxDeviceConfiguration::defaultPrivateKeyFilePath());
        m_ui->publicKeyFilePathChooser->setPath(LinuxDeviceConfiguration::defaultPublicKeyFilePath());
        handleSelectionChanged();
    }

    bool reuseKeys() const { return m_ui->reuseButton->isChecked(); }

    QString privateKeyFilePath() const {
        return m_ui->privateKeyFilePathChooser->path();
    }

    QString publicKeyFilePath() const {
        return m_ui->publicKeyFilePathChooser->path();
    }

private:
    Q_SLOT void handleSelectionChanged()
    {
        m_ui->privateKeyFilePathLabel->setEnabled(reuseKeys());
        m_ui->privateKeyFilePathChooser->setEnabled(reuseKeys());
        m_ui->publicKeyFilePathLabel->setEnabled(reuseKeys());
        m_ui->publicKeyFilePathChooser->setEnabled(reuseKeys());
        emit completeChanged();
    }

    const QScopedPointer<Ui::MaemoDeviceConfigWizardReuseKeysCheckPage> m_ui;
};

class MaemoDeviceConfigWizardKeyCreationPage : public QWizardPage
{
    Q_OBJECT
public:
    MaemoDeviceConfigWizardKeyCreationPage(QWidget *parent)
        : QWizardPage(parent),
          m_ui(new Ui::MaemoDeviceConfigWizardKeyCreationPage)
    {
        m_ui->setupUi(this);
        setTitle(tr("Key Creation"));
        setSubTitle(QLatin1String(" ")); // For Qt bug (background color)
        connect(m_ui->createKeysButton, SIGNAL(clicked()), SLOT(createKeys()));
    }

    QString privateKeyFilePath() const {
        return m_ui->keyDirPathChooser->path() + QLatin1String("/qtc_id_rsa");
    }

    QString publicKeyFilePath() const {
        return privateKeyFilePath() + QLatin1String(".pub");
    }

    virtual void initializePage()
    {
        m_isComplete = false;
        const QString &dir = QDesktopServices::storageLocation(QDesktopServices::HomeLocation)
           + QLatin1String("/.ssh");
        m_ui->keyDirPathChooser->setPath(dir);
        enableInput();
    }

    virtual bool isComplete() const { return m_isComplete; }

private:

    Q_SLOT void createKeys()
    {
        const QString &dirPath = m_ui->keyDirPathChooser->path();
        QFileInfo fi(dirPath);
        if (fi.exists() && !fi.isDir()) {
            QMessageBox::critical(this, tr("Cannot Create Keys"),
                tr("The path you have entered is not a directory."));
            return;
        }
        if (!fi.exists() && !QDir::root().mkpath(dirPath)) {
            QMessageBox::critical(this, tr("Cannot Create Keys"),
                tr("The directory you have entered does not exist and "
                   "cannot be created."));
            return;
        }

        m_ui->keyDirPathChooser->setEnabled(false);
        m_ui->createKeysButton->setEnabled(false);
        m_ui->statusLabel->setText(tr("Creating keys ... "));
        SshKeyGenerator keyGenerator;
        if (!keyGenerator.generateKeys(SshKeyGenerator::Rsa,
             SshKeyGenerator::OpenSsl, 1024)) {
            QMessageBox::critical(this, tr("Cannot Create Keys"),
                tr("Key creation failed: %1").arg(keyGenerator.error()));
            enableInput();
            return;
        }

        if (!saveFile(privateKeyFilePath(), keyGenerator.privateKey())
                || !saveFile(publicKeyFilePath(), keyGenerator.publicKey())) {
            enableInput();
            return;
        }
        QFile::setPermissions(privateKeyFilePath(),
            QFile::ReadOwner | QFile::WriteOwner);

        m_ui->statusLabel->setText(m_ui->statusLabel->text() + tr("Done."));
        m_isComplete = true;
        emit completeChanged();
    }

    bool saveFile(const QString &filePath, const QByteArray &data)
    {
        Utils::FileSaver saver(filePath);
        saver.write(data);
        if (!saver.finalize()) {
            QMessageBox::critical(this, tr("Could Not Save Key File"), saver.errorString());
            return false;
        }
        return true;
    }

    void enableInput()
    {
        m_ui->keyDirPathChooser->setEnabled(true);
        m_ui->createKeysButton->setEnabled(true);
        m_ui->statusLabel->clear();
    }

    const QScopedPointer<Ui::MaemoDeviceConfigWizardKeyCreationPage> m_ui;
    bool m_isComplete;
};

class MaemoDeviceConfigWizardKeyDeploymentPage : public QWizardPage
{
    Q_OBJECT
public:
    MaemoDeviceConfigWizardKeyDeploymentPage(const WizardData &wizardData,
        QWidget *parent = 0)
            : QWizardPage(parent),
              m_ui(new Ui::MaemoDeviceConfigWizardKeyDeploymentPage),
              m_wizardData(wizardData),
              m_keyDeployer(new MaemoKeyDeployer(this))
    {
        m_ui->setupUi(this);
        m_instructionTextTemplate = m_ui->instructionLabel->text();
        setTitle(tr("Key Deployment"));
        setSubTitle(QLatin1String(" ")); // For Qt bug (background color)
        connect(m_ui->deviceAddressLineEdit, SIGNAL(textChanged(QString)),
            SLOT(enableOrDisableButton()));
        connect(m_ui->passwordLineEdit, SIGNAL(textChanged(QString)),
            SLOT(enableOrDisableButton()));
        connect(m_ui->deployButton, SIGNAL(clicked()), SLOT(deployKey()));
        connect(m_keyDeployer, SIGNAL(error(QString)),
            SLOT(handleKeyDeploymentError(QString)));
        connect(m_keyDeployer, SIGNAL(finishedSuccessfully()),
            SLOT(handleKeyDeploymentSuccess()));
    }

    virtual void initializePage()
    {
        m_isComplete = false;
        m_ui->deviceAddressLineEdit->setText(m_wizardData.hostName);
        m_ui->instructionLabel->setText(QString(m_instructionTextTemplate)
            .replace(QLatin1String("%%%maddev%%%"),
                MaemoGlobal::madDeveloperUiName(m_wizardData.osType)));
        m_ui->passwordLineEdit->clear();
        enableInput();
    }

    virtual bool isComplete() const { return m_isComplete; }

    QString hostAddress() const {
        return m_ui->deviceAddressLineEdit->text().trimmed();
    }

private:
    Q_SLOT void enableOrDisableButton()
    {
        m_ui->deployButton->setEnabled(!hostAddress().isEmpty()
            && !password().isEmpty());
    }

    Q_SLOT void deployKey()
    {
        using namespace Utils;
        m_ui->deviceAddressLineEdit->setEnabled(false);
        m_ui->passwordLineEdit->setEnabled(false);
        m_ui->deployButton->setEnabled(false);
        SshConnectionParameters sshParams(SshConnectionParameters::NoProxy);
        sshParams.authenticationType = SshConnectionParameters::AuthenticationByPassword;
        sshParams.host = hostAddress();
        sshParams.port = LinuxDeviceConfiguration::defaultSshPort(LinuxDeviceConfiguration::Physical);
        sshParams.password = password();
        sshParams.timeout = 30;
        sshParams.userName = LinuxDeviceConfiguration::defaultUser(m_wizardData.osType);
        m_ui->statusLabel->setText(tr("Deploying... "));
        m_keyDeployer->deployPublicKey(sshParams, m_wizardData.publicKeyFilePath);
    }

    Q_SLOT void handleKeyDeploymentError(const QString &errorMsg)
    {
        QMessageBox::critical(this, tr("Key Deployment Failure"), errorMsg);
        enableInput();
    }

    Q_SLOT void handleKeyDeploymentSuccess()
    {
        QMessageBox::information(this, tr("Key Deployment Success"),
            tr("The key was successfully deployed. You may now close "
               "the \"%1\" application and continue.")
               .arg(MaemoGlobal::madDeveloperUiName(m_wizardData.osType)));
        m_ui->statusLabel->setText(m_ui->statusLabel->text() + tr("Done."));
        m_isComplete = true;
        emit completeChanged();
    }

    void enableInput()
    {
        m_ui->deviceAddressLineEdit->setEnabled(true);
        m_ui->passwordLineEdit->setEnabled(true);
        m_ui->statusLabel->clear();
        enableOrDisableButton();
    }

    QString password() const {
        return m_ui->passwordLineEdit->text().trimmed();
    }


    const QScopedPointer<Ui::MaemoDeviceConfigWizardKeyDeploymentPage> m_ui;
    bool m_isComplete;
    const WizardData &m_wizardData;
    MaemoKeyDeployer * const m_keyDeployer;
    QString m_instructionTextTemplate;
};

class MaemoDeviceConfigWizardFinalPage : public QWizardPage
{
    Q_OBJECT
public:
    MaemoDeviceConfigWizardFinalPage(const WizardData &wizardData,
        QWidget *parent)
            : QWizardPage(parent),
              m_infoLabel(new QLabel(this)),
              m_wizardData(wizardData)
    {
        setTitle(tr("Setup Finished"));
        setSubTitle(QLatin1String(" ")); // For Qt bug (background color)
        m_infoLabel->setWordWrap(true);
        QVBoxLayout * const layout = new QVBoxLayout(this);
        layout->addWidget(m_infoLabel);
    }

    virtual void initializePage()
    {
        QString infoText;
        if (m_wizardData.deviceType == LinuxDeviceConfiguration::Physical) {
            infoText = tr("The new device configuration will now be "
                "created and a test procedure will be run to check whether "
                "Qt Creator can connect to the device and to provide some "
                "information about its features.");
        } else {
            infoText = tr("The new device configuration will now be created.");
        }
        m_infoLabel->setText(infoText);
    }

private:
    QLabel * const m_infoLabel;
    const WizardData &m_wizardData;
};

} // anonymous namespace

struct MaemoDeviceConfigWizardPrivate
{
    MaemoDeviceConfigWizardPrivate(LinuxDeviceConfigurations *devConfigs,
            QWidget *parent)
        : devConfigs(devConfigs),
          startPage(parent),
          loginDataPage(wizardData, parent),
          previousKeySetupPage(parent),
          reuseKeysCheckPage(parent),
          keyCreationPage(parent),
          keyDeploymentPage(wizardData, parent),
          finalPage(wizardData, parent)
    {
    }

    WizardData wizardData;
    LinuxDeviceConfigurations * const devConfigs;
    MaemoDeviceConfigWizardStartPage startPage;
    MaemoDeviceConfigWizardLoginDataPage loginDataPage;
    MaemoDeviceConfigWizardPreviousKeySetupCheckPage previousKeySetupPage;
    MaemoDeviceConfigWizardReuseKeysCheckPage reuseKeysCheckPage;
    MaemoDeviceConfigWizardKeyCreationPage keyCreationPage;
    MaemoDeviceConfigWizardKeyDeploymentPage keyDeploymentPage;
    MaemoDeviceConfigWizardFinalPage finalPage;
};


MaemoDeviceConfigWizard::MaemoDeviceConfigWizard(LinuxDeviceConfigurations *devConfigs,
    QWidget *parent)
        : QWizard(parent),
          d(new MaemoDeviceConfigWizardPrivate(devConfigs, this))
{
    setWindowTitle(tr("New Device Configuration Setup"));
    setPage(StartPageId, &d->startPage);
    setPage(LoginDataPageId, &d->loginDataPage);
    setPage(PreviousKeySetupCheckPageId, &d->previousKeySetupPage);
    setPage(ReuseKeysCheckPageId, &d->reuseKeysCheckPage);
    setPage(KeyCreationPageId, &d->keyCreationPage);
    setPage(KeyDeploymentPageId, &d->keyDeploymentPage);
    setPage(FinalPageId, &d->finalPage);
    d->finalPage.setCommitPage(true);
}

MaemoDeviceConfigWizard::~MaemoDeviceConfigWizard() {}

void MaemoDeviceConfigWizard::createDeviceConfig()
{
    QString name = d->wizardData.configName;
    if (d->devConfigs->hasConfig(name)) {
        const QString nameTemplate = name + QLatin1String(" (%1)");
        int suffix = 2;
        do
            name = nameTemplate.arg(QString::number(suffix++));
        while (d->devConfigs->hasConfig(name));
    }

    if (d->wizardData.osType == LinuxDeviceConfiguration::GenericLinuxOsType) {
        if (d->wizardData.authType == SshConnectionParameters::AuthenticationByPassword) {
           d->devConfigs->addGenericLinuxConfigurationUsingPassword(name,
               d->wizardData.hostName, d->wizardData.userName,
               d->wizardData.password);
        } else {
            d->devConfigs->addGenericLinuxConfigurationUsingKey(name,
                d->wizardData.hostName, d->wizardData.userName,
                d->wizardData.privateKeyFilePath);
        }
    } else if (d->wizardData.deviceType == LinuxDeviceConfiguration::Physical) {
        d->devConfigs->addHardwareDeviceConfiguration(name,
            d->wizardData.osType, d->wizardData.hostName,
            d->wizardData.privateKeyFilePath);
    } else {
        d->devConfigs->addEmulatorDeviceConfiguration(name,
            d->wizardData.osType);
    }
}

int MaemoDeviceConfigWizard::nextId() const
{
    switch (currentId()) {
    case StartPageId:
        d->wizardData.configName = d->startPage.configName();
        d->wizardData.osType = d->startPage.osType();
        d->wizardData.deviceType = d->startPage.deviceType();
        d->wizardData.hostName = d->startPage.hostName();

        if (d->wizardData.deviceType == LinuxDeviceConfiguration::Emulator) {
            return FinalPageId;
        } else if (d->wizardData.osType == LinuxDeviceConfiguration::GenericLinuxOsType) {
            return LoginDataPageId;
        } else {
            return PreviousKeySetupCheckPageId;
        }
    case LoginDataPageId:
        d->wizardData.userName = d->loginDataPage.userName();
        d->wizardData.authType = d->loginDataPage.authType();
        if (d->wizardData.authType == SshConnectionParameters::AuthenticationByPassword)
            d->wizardData.password = d->loginDataPage.password();
        else
            d->wizardData.privateKeyFilePath = d->loginDataPage.privateKeyFilePath();
        return FinalPageId;
    case PreviousKeySetupCheckPageId:
        if (d->previousKeySetupPage.keyBasedLoginWasSetup()) {
            d->wizardData.privateKeyFilePath
                = d->previousKeySetupPage.privateKeyFilePath();
            return FinalPageId;
        } else {
            return ReuseKeysCheckPageId;
        }
    case ReuseKeysCheckPageId:
        if (d->reuseKeysCheckPage.reuseKeys()) {
            d->wizardData.privateKeyFilePath
                = d->reuseKeysCheckPage.privateKeyFilePath();
            d->wizardData.publicKeyFilePath
                = d->reuseKeysCheckPage.publicKeyFilePath();
            return KeyDeploymentPageId;
        } else {
            return KeyCreationPageId;
        }
    case KeyCreationPageId:
        d->wizardData.privateKeyFilePath
            = d->keyCreationPage.privateKeyFilePath();
        d->wizardData.publicKeyFilePath
            = d->keyCreationPage.publicKeyFilePath();
        return KeyDeploymentPageId;
    case KeyDeploymentPageId:
        d->wizardData.hostName = d->keyDeploymentPage.hostAddress();
        return FinalPageId;
    case FinalPageId: return -1;
    default:
        Q_ASSERT(false);
        return -1;
    }
}

} // namespace Internal
} // namespace RemoteLinux

#include "maemodeviceconfigwizard.moc"
