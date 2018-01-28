#include "linebar.h"
#include "utils.h"
#include <QDebug>

LineBar::LineBar(DLineEdit *parent) : DLineEdit(parent)
{
    
}

void LineBar::focusOutEvent(QFocusEvent *)
{
    focusOut();
}

void LineBar::keyPressEvent(QKeyEvent *e)
{
    QString key = Utils::getKeymap(e);
    
    if (key == "Esc") {
        pressEsc();
    } else if (key == "Return") {
        pressEnter();
    } else {
        DLineEdit::keyPressEvent(e);
    }
    
}
