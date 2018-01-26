#ifndef TABBAR_H
#define TABBAR_H

#include <QWidget>
#include <QHBoxLayout>
#include "tabwidget.h"

DWIDGET_USE_NAMESPACE

class Tabbar : public QWidget
{
    Q_OBJECT
    
public:
    Tabbar();
    
    void addTab(QString filepath, QString tabName);
    int currentIndex();
    int isTabExist(QString filepath);
    void activeTab(int index);
    
    void selectNextTab();
    void selectPrevTab();
    void closeTab();
    void closeOtherTabs();
    void closeTabWithIndex(int index);
    
    QString getActiveTabName();
    QString getActiveTabPath();
    
    QString getTabName(int index);
    QString getTabPath(int index);
    
    void updateTab(int index, QString filepath, QString tabName);
                      
signals:
    void doubleClicked();
    void switchToFile(QString filepath);
    void closeFile(QString filepath);
    void tabAddRequested();
    void tabReleaseRequested(QString tabName, QString filepaht, int index);
                          
public slots:
    void handleTabbarDoubleClick();
    void handleCurrentIndexChanged(int index);
    void handleTabMoved(int fromIndex, int toIndex);
    void handleTabClosed(int closeIndex);
    void handleTabReleaseRequested(int index);
    void handleTabDroped(int index, Qt::DropAction action, QObject *target);
    
private:
    QHBoxLayout *layout;
    TabWidget *tabbar;
};

#endif
