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
    setFixedSize(nJumpLineBarWidth, nJumpLineBarHeight);

    // Init layout and widgets.
    m_layout = new QHBoxLayout();
    m_layout->setContentsMargins(10, 6, 10, 6);
    m_layout->setSpacing(0);

    m_closeButton = new DIconButton(DStyle::SP_CloseButton);
    m_closeButton->setIconSize(QSize(30, 30));
    m_closeButton->setFixedSize(30, 30);
    m_closeButton->setEnabledCircle(true);
    m_closeButton->setFlat(true);
    m_closeButton->installEventFilter(this);

    m_label = new QLabel();
    m_label->setText(tr("Go to Line: "));
    m_pSpinBoxInput = new DSpinBox;
    m_pSpinBoxInput->lineEdit()->clear();
    m_pSpinBoxInput->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_pSpinBoxInput->installEventFilter(this);

    m_layout->addWidget(m_label);
    m_layout->addWidget(m_pSpinBoxInput);
    m_layout->addWidget(m_closeButton);
    this->setLayout(m_layout);

    connect(this, &JumpLineBar::pressEsc, this, &JumpLineBar::jumpCancel, Qt::QueuedConnection);
    connect(m_pSpinBoxInput->lineEdit(), &QLineEdit::returnPressed, this, &JumpLineBar::jumpConfirm, Qt::QueuedConnection);
    connect(m_pSpinBoxInput->lineEdit(), &QLineEdit::textChanged, this, &JumpLineBar::handleLineChanged, Qt::QueuedConnection);
    connect(m_closeButton, &DIconButton::clicked, this, &JumpLineBar::close, Qt::QueuedConnection);
}

JumpLineBar::~JumpLineBar()
{}

void JumpLineBar::focus()
{
    m_pSpinBoxInput->lineEdit()->setFocus();
}

bool JumpLineBar::isFocus()
{
    return m_pSpinBoxInput->lineEdit()->hasFocus();
}

void JumpLineBar::activeInput(QString file, int row, int column, int lineCount, int scrollOffset)
{
    // Save file info for back to line.
    m_jumpFile = file;
    m_rowBeforeJump = row;
    m_columnBeforeJump = column;
    m_jumpFileScrollOffset = scrollOffset;
    m_pSpinBoxInput->setRange(1, lineCount);
    m_pSpinBoxInput->clear();
    setFixedSize(nJumpLineBarWidth + QString::number(lineCount).size() * fontMetrics().width('9'), nJumpLineBarHeight);

    // Clear line number.
    if (m_pSpinBoxInput->lineEdit()->text().toInt() > lineCount)
        m_pSpinBoxInput->lineEdit()->setText("");
}

void JumpLineBar::handleFocusOut()
{
    //hide();
    lostFocusExit();
}

void JumpLineBar::handleLineChanged()
{
    QString content = m_pSpinBoxInput->lineEdit()->text();
    if (content != "") {
        if (content.toInt() == 0) {
            m_pSpinBoxInput->clear();
            return;
        }
        jumpToLine(m_jumpFile, content.toInt(), false);
    }
}

void JumpLineBar::jumpCancel()
{
    hide();
    // esc键不跳转　返回当前
    // backToPosition(m_jumpFile, m_rowBeforeJump, m_columnBeforeJump, m_jumpFileScrollOffset);
}

void JumpLineBar::jumpConfirm()
{
    QString content = m_pSpinBoxInput->lineEdit()->text();
    if (content != "") {
        jumpToLine(m_jumpFile, content.toInt(), true);
    }
}

void JumpLineBar::slotFocusChanged(bool bFocus)
{
    if (bFocus == false) {
        lostFocusExit();
    }
}

// Hide 跳转到行窗口时，需要清空编辑框中的内容
void JumpLineBar::hide()
{
    m_pSpinBoxInput->clear();
    DFloatingWidget::hide();
}

bool JumpLineBar::eventFilter(QObject *pObject, QEvent *pEvent)
{
    if (pEvent->type() == QEvent::FocusOut) {
        if (pObject == m_pSpinBoxInput) {
            /**
             * 规避当DSpinBox输入框里为空且失去focus焦点时会显示上一次输入的数值内容的问题
             */
            if (m_pSpinBoxInput->lineEdit()->text() == "") {
                m_pSpinBoxInput->lineEdit()->clear();
            }

            if (m_closeButton != qApp->focusObject()) {
                handleFocusOut();
            }
        }

        // 当前控件的关闭按钮角度丢失后触发退出信号
        if (pObject == m_closeButton) {
            handleFocusOut();
        }
    }

    return DFloatingWidget::eventFilter(pObject, pEvent);
}

