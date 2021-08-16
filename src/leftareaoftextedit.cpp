/*
* Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
*
* Author:     liumaochuan <liumaochuan@uniontech.com>
* Maintainer: liumaochuan <liumaochuan@uniontech.com>
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "leftareaoftextedit.h"
#include "linenumberarea.h"
#include "bookmarkwidget.h"
#include "codeflodarea.h"
#include "dtextedit.h"
#include <QHBoxLayout>
#include <QDebug>

leftareaoftextedit::leftareaoftextedit(TextEdit *textEdit)
{
    QHBoxLayout *hLayout = new QHBoxLayout;
    m_bookMarkArea = new bookmarkwidget(this);
    m_flodArea = new CodeFlodArea(this);
    m_linenumberarea = new LineNumberArea(this);

    m_bookMarkArea->setContentsMargins(0,0,0,0);
    m_flodArea->setContentsMargins(0,0,0,0);
    m_linenumberarea->setContentsMargins(0,0,0,0);
   // m_bookMarkArea->setFixedWidth(18);
    //m_flodArea->setFixedWidth(26);


    //m_linenumberarea->setFixedWidth(textEdit->cursorRect(textEdit->textCursor()).height());
    m_bookMarkArea->setFixedWidth(textEdit->cursorRect(textEdit->textCursor()).height());
    m_flodArea->setFixedWidth(textEdit->cursorRect(textEdit->textCursor()).height());
    hLayout->addWidget(m_bookMarkArea);
    hLayout->addWidget(m_linenumberarea);
    hLayout->addWidget(m_flodArea);
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->setSpacing(0);
    this->setLayout(hLayout);
    m_textEdit = textEdit;
}

leftareaoftextedit::~leftareaoftextedit()
{

}

void leftareaoftextedit::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    m_textEdit->lineNumberAreaPaintEvent(event);
}

int leftareaoftextedit::lineNumberAreaWidth()
{
    return m_textEdit->lineNumberAreaWidth();
}

//LineNumberArea *leftareaoftextedit::getLineNumberArea()
//{
//    return m_linenumberarea;
//}

//bookmarkwidget *leftareaoftextedit::getBookMarkWidget()
//{
//    return m_bookMarkArea;
//}

void leftareaoftextedit::bookMarkAreaPaintEvent(QPaintEvent *event)
{
    m_textEdit->bookMarkAreaPaintEvent(event);
}

void leftareaoftextedit::codeFlodAreaPaintEvent(QPaintEvent *event)
{
    m_textEdit->codeFLodAreaPaintEvent(event);
}

void leftareaoftextedit::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    QColor bdColor;
    if (m_textEdit->getBackColor().lightness() < 128) {
        bdColor = palette().brightText().color();
        bdColor.setAlphaF(0.06);
    } else {
        bdColor = palette().brightText().color();
        bdColor.setAlphaF(0.03);
    }
    painter.fillRect(e->rect(), bdColor);
}

//void leftareaoftextedit::mousePressEvent(QMouseEvent *event)
//{

//}

//QSize leftareaoftextedit::sizeHint() const
//{
//    //return QSize(m_textEdit->lineNumberAreaWidth(), 0);
// //   return QSize(m_bookMarkArea->width() + m_linenumberarea->width(), 0);
//}
