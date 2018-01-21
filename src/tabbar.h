#ifndef TABBAR_H
#define TABBAR_H

#include <QWidget>
#include <QHBoxLayout>
#include <DTabBar>

DWIDGET_USE_NAMESPACE

class Tabbar : public QWidget
{
    Q_OBJECT
    
public:
    Tabbar(QWidget *parent=0);
    
    void addTab(QString filepath, QString tabName);
    int currentIndex();
    int isTabExist(QString filepath);
    void activeTab(int index);
    
    void selectNextTab();
    void selectPrevTab();
                      
signals:
    void doubleClicked();
    void switchToFile(QString filepath);
    void closeFile(QString filepath);
    
public slots:
    void handleTabbarDoubleClick();
    void handleCurrentIndexChanged(int index);
    void handleTabMoved(int fromIndex, int toIndex);
    void handleTabClosed(int closeIndex);
    
private:
    QHBoxLayout *layout;
    DTabBar *tabbar;
    QList<QString> tabFiles;
};

#endif
