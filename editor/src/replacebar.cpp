#include "replacebar.h"
#include <QDebug>

ReplaceBar::ReplaceBar(QWidget *parent) : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint);
    
    layout = new QHBoxLayout(this);
    replaceLabel = new QLabel("Replace: ");
    replaceLine = new LineBar();
    withLabel = new QLabel("With: ");
    withLine = new LineBar();
    replaceButton = new DTextButton("Replace");
    skipButton = new DTextButton("Skip");
    replaceRestButton = new DTextButton("Replace Rest");
    replaceAllButton = new DTextButton("Replace All");
    
    layout->addWidget(replaceLabel);
    layout->addWidget(replaceLine);
    layout->addWidget(withLabel);
    layout->addWidget(withLine);
    layout->addWidget(replaceButton);
    layout->addWidget(skipButton);
    layout->addWidget(replaceRestButton);
    layout->addWidget(replaceAllButton);
    
    setFixedHeight(40);
    
    connect(replaceLine, &LineBar::pressEsc, this, &ReplaceBar::back, Qt::QueuedConnection);
    connect(replaceLine, &LineBar::pressEnter, this, &ReplaceBar::handleReplaceNext, Qt::QueuedConnection);
    connect(withLine, &LineBar::pressEnter, this, &ReplaceBar::handleReplaceNext, Qt::QueuedConnection);
    connect(replaceLine, &LineBar::contentChanged, this, &ReplaceBar::handleContentChanged, Qt::QueuedConnection);
    
    connect(replaceButton, &DTextButton::clicked, this, &ReplaceBar::handleReplaceNext, Qt::QueuedConnection);
}

bool ReplaceBar::focusNextPrevChild(bool)
{
    auto *editWidget = qobject_cast<LineBar*>(focusWidget());
    if (editWidget != nullptr) {
        if (editWidget == replaceLine) {
            withLine->setFocus();
            
            return true;
        } else if (editWidget == withLine) {
            replaceLine->setFocus();
            
            return true;
        }
    }
    
    return false;
}

void ReplaceBar::handleContentChanged()
{
    updateSearchKeyword(replaceFile, replaceLine->text());
}

void ReplaceBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setOpacity(1);
    QPainterPath path;
    path.addRect(rect());
    painter.fillPath(path, QColor("#202020"));
}

void ReplaceBar::activeInput(QString text, QString file, int row, int column, int scrollOffset)
{
    replaceLine->clear();
    replaceLine->insert(text);
    replaceLine->selectAll();
    
    show();
    
    replaceFile = file;
    replaceFileRow = row;
    replaceFileColumn = column;
    replaceFileSrollOffset = scrollOffset;
    
    replaceLine->setFocus();
}

void ReplaceBar::back()
{
    hide();
    
    backToPosition(replaceFile, replaceFileRow, replaceFileColumn, replaceFileSrollOffset);
}

void ReplaceBar::focus()
{
    replaceLine->setFocus();
}

bool ReplaceBar::isFocus()
{
    return replaceLine->hasFocus();
}

void ReplaceBar::handleReplaceNext()
{
    replaceNext(replaceLine->text(), withLine->text());
}

