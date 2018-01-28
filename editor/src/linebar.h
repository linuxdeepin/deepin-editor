#ifndef LINEBAR_H
#define LINEBAR_H

#include "dlineedit.h"

DWIDGET_USE_NAMESPACE

class LineBar : public DLineEdit
{
    Q_OBJECT
    
public:
    LineBar(DLineEdit *parent = 0);
    
signals:
    void focusOut();
    void pressEsc();
    void pressEnter();
    
protected:
    virtual void focusOutEvent(QFocusEvent *e);    
    virtual void keyPressEvent(QKeyEvent *e);
};

#endif
