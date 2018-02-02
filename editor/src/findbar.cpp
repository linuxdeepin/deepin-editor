#include "findbar.h"
#include <QDebug>

FindBar::FindBar(QWidget *parent) : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint);
    
    layout = new QHBoxLayout(this);
    findLabel = new QLabel("Find: ");
    editLine = new LineBar();
    findNextButton = new DTextButton("Next");
    findPrevButton = new DTextButton("Previous");
    
    layout->addWidget(findLabel);
    layout->addWidget(editLine);
    layout->addWidget(findNextButton);
    layout->addWidget(findPrevButton);
    
    findNextButton->setFocusPolicy(Qt::NoFocus);
    findPrevButton->setFocusPolicy(Qt::NoFocus);
    
    setFixedHeight(40);
    
    connect(editLine, &LineBar::pressEsc, this, &FindBar::back, Qt::QueuedConnection);
    connect(editLine, &LineBar::pressEnter, this, &FindBar::findNext, Qt::QueuedConnection);
    connect(editLine, &LineBar::pressCtrlEnter, this, &FindBar::findPrev, Qt::QueuedConnection);
    connect(editLine, &LineBar::contentChanged, this, &FindBar::handleContentChanged, Qt::QueuedConnection);
    
    connect(findNextButton, &DTextButton::clicked, this, &FindBar::findNext, Qt::QueuedConnection);
    connect(findPrevButton, &DTextButton::clicked, this, &FindBar::findPrev, Qt::QueuedConnection);
}

void FindBar::handleContentChanged()
{
    updateSearchKeyword(findFile, editLine->text());
}

void FindBar::hideEvent(QHideEvent *)
{
    cleanMatchKeyword();
}

void FindBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setOpacity(1);
    QPainterPath path;
    path.addRect(rect());
    painter.fillPath(path, QColor("#202020"));
}

void FindBar::activeInput(QString text, QString file, int row, int column, int scrollOffset)
{
    editLine->clear();
    editLine->insert(text);
    editLine->selectAll();
    
    show();
    
    findFile = file;
    findFileRow = row;
    findFileColumn = column;
    findFileSrollOffset = scrollOffset;
    
    editLine->setFocus();
}

void FindBar::back()
{
    hide();
    
    backToPosition(findFile, findFileRow, findFileColumn, findFileSrollOffset);
}

void FindBar::focus()
{
    editLine->setFocus();
}

bool FindBar::isFocus()
{
    return editLine->hasFocus();
}

