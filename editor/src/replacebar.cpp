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
    replaceSkipButton = new DTextButton("Skip");
    replaceRestButton = new DTextButton("Replace Rest");
    replaceAllButton = new DTextButton("Replace All");
    
    replaceButton->setFocusPolicy(Qt::NoFocus);
    replaceSkipButton->setFocusPolicy(Qt::NoFocus);
    replaceRestButton->setFocusPolicy(Qt::NoFocus);
    replaceAllButton->setFocusPolicy(Qt::NoFocus);
    
    layout->addWidget(replaceLabel);
    layout->addWidget(replaceLine);
    layout->addWidget(withLabel);
    layout->addWidget(withLine);
    layout->addWidget(replaceButton);
    layout->addWidget(replaceSkipButton);
    layout->addWidget(replaceRestButton);
    layout->addWidget(replaceAllButton);
    
    setFixedHeight(40);
    
    connect(replaceLine, &LineBar::pressEsc, this, &ReplaceBar::back, Qt::QueuedConnection);
    connect(withLine, &LineBar::pressEsc, this, &ReplaceBar::back, Qt::QueuedConnection);
    
    connect(replaceLine, &LineBar::pressEnter, this, &ReplaceBar::handleReplaceNext, Qt::QueuedConnection);
    connect(withLine, &LineBar::pressEnter, this, &ReplaceBar::handleReplaceNext, Qt::QueuedConnection);
    
    connect(replaceLine, &LineBar::pressCtrlEnter, this, &ReplaceBar::replaceSkip, Qt::QueuedConnection);
    connect(withLine, &LineBar::pressCtrlEnter, this, &ReplaceBar::replaceSkip, Qt::QueuedConnection);

    connect(replaceLine, &LineBar::pressAltEnter, this, &ReplaceBar::handleReplaceRest, Qt::QueuedConnection);
    connect(withLine, &LineBar::pressAltEnter, this, &ReplaceBar::handleReplaceRest, Qt::QueuedConnection);

    connect(replaceLine, &LineBar::pressMetaEnter, this, &ReplaceBar::handleReplaceAll, Qt::QueuedConnection);
    connect(withLine, &LineBar::pressMetaEnter, this, &ReplaceBar::handleReplaceAll, Qt::QueuedConnection);
    
    connect(replaceLine, &LineBar::contentChanged, this, &ReplaceBar::handleContentChanged, Qt::QueuedConnection);
    
    connect(replaceButton, &DTextButton::clicked, this, &ReplaceBar::handleReplaceNext, Qt::QueuedConnection);
    connect(replaceSkipButton, &DTextButton::clicked, this, &ReplaceBar::replaceSkip, Qt::QueuedConnection);
    connect(replaceRestButton, &DTextButton::clicked, this, &ReplaceBar::handleReplaceRest, Qt::QueuedConnection);
    connect(replaceAllButton, &DTextButton::clicked, this, &ReplaceBar::handleReplaceAll, Qt::QueuedConnection);
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

void ReplaceBar::handleReplaceRest()
{
    replaceRest(replaceLine->text(), withLine->text());
}

void ReplaceBar::handleReplaceAll()
{
    replaceAll(replaceLine->text(), withLine->text());    
}

