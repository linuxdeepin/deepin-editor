#include "linebar.h"
#include "utils.h"
#include <QDebug>

LineBar::LineBar(DLineEdit *parent) : DLineEdit(parent)
{
    autoSaveInternal = 500;

    autoSaveTimer = new QTimer(this);
    autoSaveTimer->setSingleShot(true);
    connect(autoSaveTimer, &QTimer::timeout, this, &LineBar::handleTextChangeTimer);

    connect(this, &DLineEdit::textChanged, this, &LineBar::handleTextChanged, Qt::QueuedConnection);
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
    } else if (key == "Ctrl + Return") {
        pressCtrlEnter();
    } else {
        DLineEdit::keyPressEvent(e);
    }
    
}

void LineBar::handleTextChanged()
{
    if (autoSaveTimer->isActive()) {
        autoSaveTimer->stop();
    }
    autoSaveTimer->start(autoSaveInternal);
}

void LineBar::handleTextChangeTimer()
{
    contentChanged();
}
