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

JumpLineBar::JumpLineBar(DFloatingWidget *parent)
    : DFloatingWidget(parent)
{
    // Init.
    setFixedSize(180, 58);

    // Init layout and widgets.
    m_layout = new QHBoxLayout();
    m_layout->setContentsMargins(10, 4, 4, 4);
    m_layout->setSpacing(0);

    m_label = new QLabel();
    m_label->setMinimumHeight(37);
    m_label->setText(tr("Go to Line: "));
    m_editLine = new LineBar();
    m_editLine->lineEdit()->setFixedSize(96, 37);

    m_lineValidator = new QIntValidator;
    m_editLine->lineEdit()->setValidator(m_lineValidator);

    QFont font;
    font.setFamily("SourceHanSansSC-Medium");
    font.setPixelSize(13);
    m_label->setFont(font);
    m_editLine->lineEdit()->setFont(font);

    m_layout->addWidget(m_label);
    m_layout->addWidget(m_editLine);
    this->setLayout(m_layout);

    connect(m_editLine, &LineBar::pressEsc, this, &JumpLineBar::jumpCancel, Qt::QueuedConnection);
    connect(m_editLine, &LineBar::pressEnter, this, &JumpLineBar::jumpConfirm, Qt::QueuedConnection);
    connect(m_editLine, &LineBar::textChanged, this, &JumpLineBar::handleLineChanged, Qt::QueuedConnection);
    connect(m_editLine, &LineBar::focusOut, this, &JumpLineBar::handleFocusOut, Qt::QueuedConnection);
    connect(m_editLine, &LineBar::focusChanged, this, &JumpLineBar::slotFocusChanged, Qt::QueuedConnection);
}

void JumpLineBar::focus()
{
    m_editLine->lineEdit()->setFocus();
}

bool JumpLineBar::isFocus()
{
    return m_editLine->lineEdit()->hasFocus();
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
    m_editLine->lineEdit()->setText("");

    // Show jump line bar.
    show();
    raise();

    // Focus default.
    m_editLine->lineEdit()->setFocus();
}

void JumpLineBar::handleFocusOut()
{
    hide();

    lostFocusExit();
}

void JumpLineBar::handleLineChanged()
{
    QString content = m_editLine->lineEdit()->text();
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

    QString content = m_editLine->lineEdit()->text();
    if (content != "") {
        jumpToLine(m_jumpFile, content.toInt(), true);
    }
}

void JumpLineBar::slotFocusChanged(bool bFocus)
{
    if (bFocus == false) {
        hide();

        lostFocusExit();
    }
}

