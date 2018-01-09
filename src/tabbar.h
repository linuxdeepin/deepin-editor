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
    
    void addTab(QString tabName);
    int currentIndex();
                      
signals:
    void doubleClicked();
    
public slots:
    void handleTabbarDoubleClick();
    
private:
    QHBoxLayout *layout;
    DTabBar *tabbar;
    
};

#endif
