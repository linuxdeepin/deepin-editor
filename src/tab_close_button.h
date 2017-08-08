#ifndef TABCLOSEBUTTON_H
#define TABCLOSEBUTTON_H

#include <QPushButton>
#include <QImage>
#include <QEvent>

class TabCloseButton : public QPushButton
{
    Q_OBJECT
    
public:
    TabCloseButton(QPushButton *parent=0);
	~TabCloseButton(); 
    
    void setButtonVisible(bool visible);
    
protected:
    void paintEvent(QPaintEvent *event);
    bool eventFilter(QObject *, QEvent *event);
    
private:
    QImage normalImg;
    QImage hoverImg;
    QImage pressImg;
    
    bool isFocus;
    bool isPress;
    bool isVisible;
};	

#endif
