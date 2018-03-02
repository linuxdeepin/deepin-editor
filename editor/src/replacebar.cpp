/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 *               2011 ~ 2018 Wang Yong
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Wang Yong <wangyong@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "replacebar.h"

#include <QDebug>

ReplaceBar::ReplaceBar(QWidget *parent) : QWidget(parent)
{
    // Init.
    setWindowFlags(Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint);
    setFixedHeight(40);

    // Init layout and widgets.
    layout = new QHBoxLayout(this);
    replaceLabel = new QLabel("Replace: ");
    replaceLine = new LineBar();
    withLabel = new QLabel("With: ");
    withLine = new LineBar();
    replaceButton = new QPushButton("Replace");
    replaceSkipButton = new QPushButton("Skip");
    replaceRestButton = new QPushButton("Replace Rest");
    replaceAllButton = new QPushButton("Replace All");
    
    layout->addWidget(replaceLabel);
    layout->addWidget(replaceLine);
    layout->addWidget(withLabel);
    layout->addWidget(withLine);
    layout->addWidget(replaceButton);
    layout->addWidget(replaceSkipButton);
    layout->addWidget(replaceRestButton);
    layout->addWidget(replaceAllButton);
    
    // Make button don't grab keyboard focus after click it.
    replaceButton->setFocusPolicy(Qt::NoFocus);
    replaceSkipButton->setFocusPolicy(Qt::NoFocus);
    replaceRestButton->setFocusPolicy(Qt::NoFocus);
    replaceAllButton->setFocusPolicy(Qt::NoFocus);
    
    connect(replaceLine, &LineBar::pressEsc, this, &ReplaceBar::replaceCancel, Qt::QueuedConnection);
    connect(withLine, &LineBar::pressEsc, this, &ReplaceBar::replaceCancel, Qt::QueuedConnection);
    
    connect(replaceLine, &LineBar::pressEnter, this, &ReplaceBar::handleReplaceNext, Qt::QueuedConnection);
    connect(withLine, &LineBar::pressEnter, this, &ReplaceBar::handleReplaceNext, Qt::QueuedConnection);
    
    connect(replaceLine, &LineBar::pressCtrlEnter, this, &ReplaceBar::replaceSkip, Qt::QueuedConnection);
    connect(withLine, &LineBar::pressCtrlEnter, this, &ReplaceBar::replaceSkip, Qt::QueuedConnection);

    connect(replaceLine, &LineBar::pressAltEnter, this, &ReplaceBar::handleReplaceRest, Qt::QueuedConnection);
    connect(withLine, &LineBar::pressAltEnter, this, &ReplaceBar::handleReplaceRest, Qt::QueuedConnection);

    connect(replaceLine, &LineBar::pressMetaEnter, this, &ReplaceBar::handleReplaceAll, Qt::QueuedConnection);
    connect(withLine, &LineBar::pressMetaEnter, this, &ReplaceBar::handleReplaceAll, Qt::QueuedConnection);
    
    connect(replaceLine, &LineBar::contentChanged, this, &ReplaceBar::handleContentChanged, Qt::QueuedConnection);
    
    connect(replaceButton, &QPushButton::clicked, this, &ReplaceBar::handleReplaceNext, Qt::QueuedConnection);
    connect(replaceSkipButton, &QPushButton::clicked, this, &ReplaceBar::replaceSkip, Qt::QueuedConnection);
    connect(replaceRestButton, &QPushButton::clicked, this, &ReplaceBar::handleReplaceRest, Qt::QueuedConnection);
    connect(replaceAllButton, &QPushButton::clicked, this, &ReplaceBar::handleReplaceAll, Qt::QueuedConnection);
}

bool ReplaceBar::isFocus()
{
    return replaceLine->hasFocus();
}

void ReplaceBar::focus()
{
    replaceLine->setFocus();
}

void ReplaceBar::activeInput(QString text, QString file, int row, int column, int scrollOffset)
{
    // Try fill keyword with select text.
    replaceLine->clear();
    replaceLine->insert(text);
    replaceLine->selectAll();

    // Show.
    show();
    
    // Save file info for back to position.
    replaceFile = file;
    replaceFileRow = row;
    replaceFileColumn = column;
    replaceFileSrollOffset = scrollOffset;
    
    // Focus.
    focus();
}

void ReplaceBar::replaceCancel()
{
    hide();
    
    backToPosition(replaceFile, replaceFileRow, replaceFileColumn, replaceFileSrollOffset);
}

void ReplaceBar::handleContentChanged()
{
    updateSearchKeyword(replaceFile, replaceLine->text());
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

void ReplaceBar::hideEvent(QHideEvent *)
{
    removeSearchKeyword();
}

void ReplaceBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setOpacity(1);
    QPainterPath path;
    path.addRect(rect());
    painter.fillPath(path, QColor("#202020"));
}

bool ReplaceBar::focusNextPrevChild(bool)
{
    // Make keyword jump between two EditLine widgets.
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
