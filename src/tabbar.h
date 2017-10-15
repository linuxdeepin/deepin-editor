#ifndef TABBAR_H
#define TABBAR_H

#include <QWidget>

struct TabNode {
    QString tabId;
    QString tabName;
    int offsetLeft;
    int offsetRight;
    int nameWidth;
    int tabWidth;
};

class Tabbar : public QWidget
{
    Q_OBJECT
    
public:
    Tabbar(QWidget *parent=0);
	~Tabbar(); 
    
    void addTab(QString tabId, QString tabName, int pos=-1);
    void selectNextTab();
    void selectPrevTab();
    void selectFirstTab();
    void selectLastTab();
    
protected:
    void paintEvent(QPaintEvent *);
    
private:
    QList<TabNode> *tabList;
    int fontSize;
    int currentIndex;
    int tabUnderlineHeight;
};	

#endif
