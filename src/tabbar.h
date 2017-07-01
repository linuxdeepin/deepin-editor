#ifndef TABBAR_H
#define TABBAR_H

#include <QWidget>
#include <QFontMetrics>
#include <QPaintEvent>

class Tabbar : public QWidget
{
    Q_OBJECT
    
public:
    Tabbar(QWidget *parent = 0);
    ~Tabbar();
    
    void addTab(QString tabName);
    
protected:
    void paintEvent(QPaintEvent *event);
    
private:
    QList<QString> tabNames;
    QList<int> tabNameWidths;
    int currentTabIndex;
    QFontMetrics *fontMetrics;
    int tabNameSize;
    int tabNameLeftPadding;
    int tabNameRightPadding;
    int tabCloseButtonWidth;
    int tabCloseButtonRightPadding;
    int tabUnderlineHeight;
    int tabNameTopPadding;
};

#endif
