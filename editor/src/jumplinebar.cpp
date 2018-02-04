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

#include "jumplinebar.h"
#include <QDebug>

JumpLineBar::JumpLineBar(QWidget *parent) : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint);
    
    layout = new QHBoxLayout(this);
    
    label = new QLabel();
    label->setText("跳到行: ");
    editLine = new LineBar();
    
    lineValidator = new QIntValidator;
    editLine->setValidator(lineValidator);    
    
    layout->addWidget(label);
    layout->addWidget(editLine);
    
    setFixedSize(200, 40);
    
    connect(editLine, &LineBar::pressEsc, this, &JumpLineBar::back, Qt::QueuedConnection);
    connect(editLine, &LineBar::pressEnter, this, &JumpLineBar::jump, Qt::QueuedConnection);
    connect(editLine, &LineBar::textChanged, this, &JumpLineBar::tempJump, Qt::QueuedConnection);
    connect(editLine, &LineBar::focusOut, this, &JumpLineBar::cancel, Qt::QueuedConnection);
}

void JumpLineBar::activeInput(QString file, int line, int lineCount, int scrollOffset)
{
    jumpFile = file;
    lineBeforeJump = line;
    jumpFileScrollOffset = scrollOffset;
    lineValidator->setRange(1, lineCount);
    
    editLine->setText("");
    
    show();
    raise();
    
    editLine->setFocus();
}

void JumpLineBar::cancel()
{
    hide();
    
    cancelJump();
}

void JumpLineBar::back()
{
    hide();
    
    backToLine(jumpFile, lineBeforeJump, jumpFileScrollOffset);
}

void JumpLineBar::tempJump()
{
    QString content = editLine->text();
    if (content != "") {
        tempJumpToLine(jumpFile, content.toInt());
    }
}

void JumpLineBar::jump()
{
    hide();
    
    QString content = editLine->text();
    if (content != "") {
        jumpToLine(jumpFile, content.toInt());
    }
}

void JumpLineBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setOpacity(1);
    QPainterPath path;
    path.addRect(rect());
    painter.fillPath(path, QColor("#202020"));
}
