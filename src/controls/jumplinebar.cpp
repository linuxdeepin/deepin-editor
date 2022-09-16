// SPDX-FileCopyrightText: 2011-2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
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

    m_label = new QLabel();
    m_label->setText(tr("Go to Line: "));
    m_pSpinBoxInput = new DSpinBox;
    m_pSpinBoxInput->lineEdit()->clear();
    m_pSpinBoxInput->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_pSpinBoxInput->installEventFilter(this);

    m_layout->addWidget(m_label);
    m_layout->addWidget(m_pSpinBoxInput);
    this->setLayout(m_layout);

    connect(this, &JumpLineBar::pressEsc, this, &JumpLineBar::jumpCancel, Qt::QueuedConnection);
    connect(m_pSpinBoxInput->lineEdit(), &QLineEdit::returnPressed, this, &JumpLineBar::jumpConfirm, Qt::QueuedConnection);
    connect(m_pSpinBoxInput->lineEdit(), &QLineEdit::textChanged, this, &JumpLineBar::handleLineChanged, Qt::QueuedConnection);
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
    m_lineCount = lineCount;
    // 调整为 0~lineCount ，0已被处理不允许首位输入，不影响仅单行的情况
    // 设置 range 后会自动调整输入范围，不使用 clear() 防止在读取文件时已输入的行号被清空
    m_pSpinBoxInput->setRange(0, lineCount);
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

int JumpLineBar::getLineCount()
{
    return m_lineCount;
}

bool JumpLineBar::eventFilter(QObject *pObject, QEvent *pEvent)
{
    if (pObject == m_pSpinBoxInput) {
        if (pEvent->type() == QEvent::FocusOut) {
            handleFocusOut();
            /**
             * 规避当DSpinBox输入框里为空且失去focus焦点时会显示上一次输入的数值内容的问题
             */
            if (m_pSpinBoxInput->lineEdit()->text() == "") {
                m_pSpinBoxInput->lineEdit()->clear();
            }
        }
    }

    return DFloatingWidget::eventFilter(pObject, pEvent);
}

