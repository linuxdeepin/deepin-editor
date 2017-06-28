#ifndef TABBAR_H
#define TABBAR_H

#include <QWidget>
#include <QPaintEvent>

class Tabbar : public QWidget
{
    Q_OBJECT
    
public:
    Tabbar(QWidget *parent = 0);
    ~Tabbar();
    
protected:
    void paintEvent(QPaintEvent *event);
};

#endif
