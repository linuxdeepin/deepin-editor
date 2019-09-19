/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Rekols    <rekols@foxmail.com>
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

JumpLineBar::JumpLineBar(QWidget *parent)
    : DFloatingWidget(parent)
{
    // Init.
    setFixedSize(200, 68);

    // Init layout and widgets.
    m_layout = new QHBoxLayout();

    m_label = new QLabel();
    m_label->setText(tr("Go to Line: "));
    m_editLine = new LineBar();

    m_lineValidator = new QIntValidator;
    m_editLine->setValidator(m_lineValidator);

    m_layout->addWidget(m_label);
    m_layout->addWidget(m_editLine);
    this->setLayout(m_layout);

    connect(m_editLine, &LineBar::pressEsc, this, &JumpLineBar::jumpCancel, Qt::QueuedConnection);
    connect(m_editLine, &LineBar::pressEnter, this, &JumpLineBar::jumpConfirm, Qt::QueuedConnection);
    connect(m_editLine, &LineBar::textChanged, this, &JumpLineBar::handleLineChanged, Qt::QueuedConnection);
    connect(m_editLine, &LineBar::focusOut, this, &JumpLineBar::handleFocusOut, Qt::QueuedConnection);
}

void JumpLineBar::focus()
{
    m_editLine->setFocus();
}

bool JumpLineBar::isFocus()
{
    return m_editLine->hasFocus();
}

void JumpLineBar::activeInput(QString file, int row, int column, int lineCount, int scrollOffset)
{
    // Save file info for back to line.
    m_jumpFile = file;
    m_rowBeforeJump = row;
    m_columnBeforeJump = column;
    m_jumpFileScrollOffset = scrollOffset;
    m_lineValidator->setRange(1, lineCount);

    // Clear line number.
    m_editLine->setText("");

    // Show jump line bar.
    show();
    raise();

    // Focus default.
    m_editLine->setFocus();
}

void JumpLineBar::handleFocusOut()
{
    hide();

    lostFocusExit();
}

void JumpLineBar::handleLineChanged()
{
    QString content = m_editLine->text();
    if (content != "") {
        jumpToLine(m_jumpFile, content.toInt(), false);
    }
}

void JumpLineBar::jumpCancel()
{
    hide();

    backToPosition(m_jumpFile, m_rowBeforeJump, m_columnBeforeJump, m_jumpFileScrollOffset);
}

void JumpLineBar::jumpConfirm()
{
    hide();

    QString content = m_editLine->text();
    if (content != "") {
        jumpToLine(m_jumpFile, content.toInt(), true);
    }
}

