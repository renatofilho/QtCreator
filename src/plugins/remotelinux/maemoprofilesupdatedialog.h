/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
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
#ifndef MAEMOPROFILESUPDATEDIALOG_H
#define MAEMOPROFILESUPDATEDIALOG_H

#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtGui/QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
    class MaemoProFilesUpdateDialog;
}
QT_END_NAMESPACE

namespace RemoteLinux {
namespace Internal {
class MaemoDeployableListModel;

class MaemoProFilesUpdateDialog : public QDialog
{
    Q_OBJECT

public:
    typedef QPair<MaemoDeployableListModel *, bool> UpdateSetting;

    explicit MaemoProFilesUpdateDialog(const QList<MaemoDeployableListModel *> &models,
        QWidget *parent = 0);
    ~MaemoProFilesUpdateDialog();
    QList<UpdateSetting> getUpdateSettings() const;

private:
    Q_SLOT void checkAll();
    Q_SLOT void uncheckAll();
    void setCheckStateForAll(Qt::CheckState checkState);

    const QList<MaemoDeployableListModel *> m_models;
    Ui::MaemoProFilesUpdateDialog *ui;
};

} // namespace RemoteLinux
} // namespace Internal

#endif // MAEMOPROFILESUPDATEDIALOG_H
