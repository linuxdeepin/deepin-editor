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
    
    setFixedHeight(40);
    
    connect(editLine, &LineBar::focusOut, this, &FindBar::cancel, Qt::QueuedConnection);
}


void FindBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setOpacity(1);
    QPainterPath path;
    path.addRect(rect());
    painter.fillPath(path, QColor("#202020"));
}

void FindBar::activeInput(QString file, int row, int column, int scrollOffset)
{
    show();
    
    findFile = file;
    findFileRow = row;
    findFileColumn = column;
    findFileSrollOffset = scrollOffset;
    
    editLine->setFocus();
}

void FindBar::cancel()
{
    hide();
    
    cancelFind();
}
