#ifndef TABBAR_H
#define TABBAR_H

#include <QTabBar>

class Tabbar : public QTabBar
{
    Q_OBJECT
    
public:
    Tabbar(QTabBar *parent=0);
	~Tabbar(); 
};	

#endif
