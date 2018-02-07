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

#include "findbar.h"

#include <QDebug>

FindBar::FindBar(QWidget *parent) : QWidget(parent)
{
    // Init.
    setWindowFlags(Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint);
    setFixedHeight(40);
    
    // Init layout and widgets.
    layout = new QHBoxLayout(this);
    findLabel = new QLabel("Find: ");
    editLine = new LineBar();
    findNextButton = new DTextButton("Next");
    findPrevButton = new DTextButton("Previous");
    
    layout->addWidget(findLabel);
    layout->addWidget(editLine);
    layout->addWidget(findNextButton);
    layout->addWidget(findPrevButton);
    
    // Make button don't grab keyboard focus after click it.
    findNextButton->setFocusPolicy(Qt::NoFocus);
    findPrevButton->setFocusPolicy(Qt::NoFocus);
    
    connect(editLine, &LineBar::pressEsc, this, &FindBar::findCancel, Qt::QueuedConnection);
    connect(editLine, &LineBar::pressEnter, this, &FindBar::findNext, Qt::QueuedConnection);
    connect(editLine, &LineBar::pressCtrlEnter, this, &FindBar::findPrev, Qt::QueuedConnection);
    connect(editLine, &LineBar::contentChanged, this, &FindBar::handleContentChanged, Qt::QueuedConnection);
    
    connect(findNextButton, &DTextButton::clicked, this, &FindBar::findNext, Qt::QueuedConnection);
    connect(findPrevButton, &DTextButton::clicked, this, &FindBar::findPrev, Qt::QueuedConnection);
}

bool FindBar::isFocus()
{
    return editLine->hasFocus();
}

void FindBar::focus()
{
    editLine->setFocus();
}

void FindBar::activeInput(QString text, QString file, int row, int column, int scrollOffset)
{
    // Try fill keyword with select text.
    editLine->clear();
    editLine->insert(text);
    editLine->selectAll();
    
    // Show.
    show();
    
    // Save file info for back to position.
    findFile = file;
    findFileRow = row;
    findFileColumn = column;
    findFileSrollOffset = scrollOffset;
    
    // Focus.
    focus();
}

void FindBar::findCancel()
{
    hide();
    
    backToPosition(findFile, findFileRow, findFileColumn, findFileSrollOffset);
}

void FindBar::handleContentChanged()
{
    updateSearchKeyword(findFile, editLine->text());
}

void FindBar::hideEvent(QHideEvent *)
{
    removeSearchKeyword();
}

void FindBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setOpacity(1);
    QPainterPath path;
    path.addRect(rect());
    painter.fillPath(path, QColor("#202020"));
}
