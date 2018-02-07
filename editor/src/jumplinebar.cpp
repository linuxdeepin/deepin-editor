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

#include "dthememanager.h"
#include "jumplinebar.h"

#include <QDebug>

JumpLineBar::JumpLineBar(QWidget *parent) : QWidget(parent)
{
    // Init.
    setWindowFlags(Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint);
    setFixedSize(200, 40);
    
    // Init layout and widgets.
    layout = new QHBoxLayout(this);
    
    label = new QLabel();
    label->setText("跳到行: ");
    editLine = new LineBar();
    
    lineValidator = new QIntValidator;
    editLine->setValidator(lineValidator);    
    
    layout->addWidget(label);
    layout->addWidget(editLine);
    
    connect(editLine, &LineBar::pressEsc, this, &JumpLineBar::jumpCancel, Qt::QueuedConnection);
    connect(editLine, &LineBar::pressEnter, this, &JumpLineBar::jumpConfirm, Qt::QueuedConnection);
    connect(editLine, &LineBar::textChanged, this, &JumpLineBar::handleLineChanged, Qt::QueuedConnection);
    connect(editLine, &LineBar::focusOut, this, &JumpLineBar::handleFocusOut, Qt::QueuedConnection);
}

void JumpLineBar::focus()
{
    editLine->setFocus();
}

bool JumpLineBar::isFocus()
{
    return editLine->hasFocus();
}

void JumpLineBar::activeInput(QString file, int row, int column, int lineCount, int scrollOffset)
{
    // Save file info for back to line.
    jumpFile = file;
    rowBeforeJump = row;
    columnBeforeJump = column;
    jumpFileScrollOffset = scrollOffset;
    lineValidator->setRange(1, lineCount);
    
    // Clear line number.
    editLine->setText("");
    
    // Show jump line bar.
    show();
    raise();
    
    // Focus default.
    editLine->setFocus();
}

void JumpLineBar::handleFocusOut()
{
    hide();
    
    lostFocusExit();
}

void JumpLineBar::handleLineChanged()
{
    QString content = editLine->text();
    if (content != "") {
        jumpToLine(jumpFile, content.toInt(), false);
    }
}

void JumpLineBar::jumpCancel()
{
    hide();
    
    backToPosition(jumpFile, rowBeforeJump, columnBeforeJump, jumpFileScrollOffset);
}

void JumpLineBar::jumpConfirm()
{
    hide();
    
    QString content = editLine->text();
    if (content != "") {
        jumpToLine(jumpFile, content.toInt(), true);
    }
}

void JumpLineBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setOpacity(1);
    QPainterPath path;
    path.addRect(rect());
    painter.fillPath(path, QColor("#ffffff"));
}

