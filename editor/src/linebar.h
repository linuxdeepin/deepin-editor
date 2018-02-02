#ifndef LINEBAR_H
#define LINEBAR_H

#include "dlineedit.h"
#include <QTimer>

DWIDGET_USE_NAMESPACE

class LineBar : public DLineEdit
{
    Q_OBJECT
    
public:
    LineBar(DLineEdit *parent = 0);
    
public slots:
    void handleTextChanged();
    void handleTextChangeTimer();
    
signals:
    void focusOut();
    void pressEsc();
    void pressEnter();
    void pressCtrlEnter();
    void contentChanged();
    
protected:
    virtual void focusOutEvent(QFocusEvent *e);    
    virtual void keyPressEvent(QKeyEvent *e);
    
private:
    QTimer *autoSaveTimer;
    int autoSaveInternal;
};

#endif
