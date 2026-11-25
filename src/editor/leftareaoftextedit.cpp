// SPDX-FileCopyrightText: 2019-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddlog.h"
#include "leftareaoftextedit.h"
#include "linenumberarea.h"
#include "bookmarkwidget.h"
#include "codeflodarea.h"
#include "dtextedit.h"

#include <QHBoxLayout>
#include <QDebug>


LeftAreaTextEdit::LeftAreaTextEdit(TextEdit *textEdit) :
    m_pTextEdit(textEdit)
{
    qCDebug(appLog) << "LeftAreaTextEdit created with textEdit:" << textEdit;
    QHBoxLayout *pHLayout = new QHBoxLayout(this);
    m_pLineNumberArea = new LineNumberArea(this);
    m_pBookMarkArea = new BookMarkWidget(this);
    m_pFlodArea = new CodeFlodArea(this);

    m_pBookMarkArea->setContentsMargins(0, 0, 0, 0);
    m_pFlodArea->setContentsMargins(0, 0, 0, 0);
    m_pLineNumberArea->setContentsMargins(0, 0, 0, 0);
    //m_pBookMarkArea->setFixedWidth(14);
   // m_pFlodArea->setFixedWidth(18);
    //m_pBookMarkArea->setFixedWidth(textEdit->cursorRect(textEdit->textCursor()).height());
   // m_pFlodArea->setFixedWidth(textEdit->cursorRect(textEdit->textCursor()).height());

    pHLayout->addWidget(m_pBookMarkArea);
    pHLayout->addWidget(m_pLineNumberArea);
    pHLayout->addWidget(m_pFlodArea);
    pHLayout->setContentsMargins(0, 0, 0, 0);
    pHLayout->setSpacing(0);
    this->setLayout(pHLayout);
    qCDebug(appLog) << "LeftAreaTextEdit layout initialized with 3 areas";
}

LeftAreaTextEdit::~LeftAreaTextEdit()
{
    qCDebug(appLog) << "LeftAreaTextEdit destroyed";
}

void LeftAreaTextEdit::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    qCDebug(appLog) << "LeftAreaTextEdit lineNumberAreaPaintEvent";
    m_pTextEdit->lineNumberAreaPaintEvent(event);
}

int LeftAreaTextEdit::lineNumberAreaWidth()
{
    qCDebug(appLog) << "LeftAreaTextEdit lineNumberAreaWidth";
    int digits = 1;
    int max = qMax(1, m_pTextEdit->document()->blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    qCDebug(appLog) << "LeftAreaTextEdit lineNumberAreaWidth return";
    return m_pTextEdit->lineNumberAreaWidth();
}


void LeftAreaTextEdit::bookMarkAreaPaintEvent(QPaintEvent *event)
{
    qCDebug(appLog) << "LeftAreaTextEdit bookMarkAreaPaintEvent";
    m_pTextEdit->bookMarkAreaPaintEvent(event);
}

void LeftAreaTextEdit::codeFlodAreaPaintEvent(QPaintEvent *event)
{
    qCDebug(appLog) << "LeftAreaTextEdit codeFlodAreaPaintEvent";
    m_pTextEdit->codeFLodAreaPaintEvent(event);
}

void LeftAreaTextEdit::updateLineNumber()
{
    qCDebug(appLog) << "LeftAreaTextEdit updateLineNumber";
    if (m_pLineNumberArea) {
        qCDebug(appLog) << "LeftAreaTextEdit updateLineNumber m_pLineNumberArea";
        m_pLineNumberArea->update();
    }
}

void LeftAreaTextEdit::updateBookMark()
{
    qCDebug(appLog) << "LeftAreaTextEdit updateBookMark";
    if (m_pBookMarkArea) {
        qCDebug(appLog) << "LeftAreaTextEdit updateBookMark m_pBookMarkArea";
        m_pBookMarkArea->update();
    }
}

void LeftAreaTextEdit::updateCodeFlod()
{
    qCDebug(appLog) << "LeftAreaTextEdit updateCodeFlod";
    if (m_pFlodArea) {
        qCDebug(appLog) << "LeftAreaTextEdit updateCodeFlod m_pFlodArea";
        m_pFlodArea->update();
    }
}

void LeftAreaTextEdit::updateAll()
{
    qCDebug(appLog) << "LeftAreaTextEdit updateAll";
    if (m_pLineNumberArea) {
        qCDebug(appLog) << "LeftAreaTextEdit updateAll m_pLineNumberArea";
        m_pLineNumberArea->update();
    }
    if (m_pBookMarkArea) {
        qCDebug(appLog) << "LeftAreaTextEdit updateAll m_pBookMarkArea";
        m_pBookMarkArea->update();
    }
    if (m_pFlodArea) {
        qCDebug(appLog) << "LeftAreaTextEdit updateAll m_pFlodArea";
        m_pFlodArea->update();
    }
    qCDebug(appLog) << "LeftAreaTextEdit updateAll exit";
}

void LeftAreaTextEdit::paintEvent(QPaintEvent *event)
{
    qCDebug(appLog) << "LeftAreaTextEdit paintEvent";
    QPainter painter(this);
    QColor bdColor;
    if (m_pTextEdit->getBackColor().lightness() < 128) {
        qCDebug(appLog) << "LeftAreaTextEdit paintEvent getBackColor lightness < 128";
        bdColor = palette().brightText().color();
        bdColor.setAlphaF(0.06);
    } else {
        qCDebug(appLog) << "LeftAreaTextEdit paintEvent getBackColor lightness >= 128";
        bdColor = palette().brightText().color();
        bdColor.setAlphaF(0.03);
    }
    painter.fillRect(event->rect(), bdColor);
    qCDebug(appLog) << "LeftAreaTextEdit paintEvent exit";
}


TextEdit* LeftAreaTextEdit::getEdit()
{
    qCDebug(appLog) << "LeftAreaTextEdit getEdit";
    return m_pTextEdit;
}


