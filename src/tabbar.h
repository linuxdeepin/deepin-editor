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
                      
signals:
    void doubleClicked();
    void switchToFile(QString filepath);
    
public slots:
    void handleTabbarDoubleClick();
    void handleCurrentIndexChanged(int index);
    
private:
    QHBoxLayout *layout;
    DTabBar *tabbar;
    QList<QString> tabFiles;
};

#endif
