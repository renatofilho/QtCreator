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

#ifndef ANALYZEROUTPUTPANE_H
#define ANALYZEROUTPUTPANE_H

#include <coreplugin/ioutputpane.h>

QT_FORWARD_DECLARE_CLASS(QStackedLayout)
QT_FORWARD_DECLARE_CLASS(QStackedWidget)

namespace Utils {
class StyledSeparator;
}

namespace Analyzer {
class IAnalyzerTool;
class IAnalyzerOutputPaneAdapter;

namespace Internal {

class AnalyzerOutputPane : public Core::IOutputPane
{
    Q_OBJECT
public:
    explicit AnalyzerOutputPane(QObject *parent = 0);

    void setTool(IAnalyzerTool *tool);
    // IOutputPane
    virtual QWidget *outputWidget(QWidget *parent);
    virtual QList<QWidget *> toolBarWidgets() const;
    virtual QString displayName() const;

    virtual int priorityInStatusBar() const;

    virtual void clearContents();
    virtual void visibilityChanged(bool visible);

    virtual void setFocus();
    virtual bool hasFocus();
    virtual bool canFocus();
    virtual bool canNavigate();
    virtual bool canNext();
    virtual bool canPrevious();
    virtual void goToNext();
    virtual void goToPrev();

private slots:
    void slotPopup(bool withFocus);
    void slotNavigationStatusChanged();

private:
    void clearTool();
    int currentIndex() const;
    IAnalyzerOutputPaneAdapter *currentAdapter() const;
    void setCurrentIndex(int );
    void addAdapter(IAnalyzerOutputPaneAdapter *adapter);
    void addToWidgets(IAnalyzerOutputPaneAdapter *adapter);
    void createWidgets(QWidget *paneParent);
    bool isInitialized() const { return m_paneWidget != 0; }

    QWidget *m_paneWidget;
    QStackedLayout *m_paneStackedLayout;
    QList<IAnalyzerOutputPaneAdapter *> m_adapters;
    QStackedWidget *m_toolbarStackedWidget;
    Utils::StyledSeparator *m_toolBarSeparator;
    // tracks selected index during !isInitialized() state
    int m_delayedCurrentIndex;
};

} // namespace Internal
} // namespace Analyzer

#endif // ANALYZEROUTPUTPANE_H
