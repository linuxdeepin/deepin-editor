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


LeftAreaTextEdit::LeftAreaTextEdit(TextEdit *textEdit) :
       m_pTextEdit(textEdit)
{
    QHBoxLayout *pHLayout = new QHBoxLayout(this);
    m_pLineNumberArea = new LineNumberArea(this);
    m_pBookMarkArea = new BookMarkWidget(this);
    m_pFlodArea = new CodeFlodArea(this);

    m_pBookMarkArea->setContentsMargins(0,0,0,0);
    m_pFlodArea->setContentsMargins(0,0,0,0);
    m_pLineNumberArea->setContentsMargins(0,0,0,0);
    m_pBookMarkArea->setFixedWidth(14);
    m_pFlodArea->setFixedWidth(14);
    //m_pBookMarkArea->setMinimumWidth(20);
//    m_pFlodArea->setMinimumWidth(20);
    pHLayout->addWidget(m_pBookMarkArea);
    pHLayout->addWidget(m_pLineNumberArea);
    pHLayout->addWidget(m_pFlodArea);
    pHLayout->setContentsMargins(0, 0, 0, 0);
    pHLayout->setSpacing(0);
    this->setLayout(pHLayout);
}

LeftAreaTextEdit::~LeftAreaTextEdit()
{

}

void LeftAreaTextEdit::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    m_pTextEdit->lineNumberAreaPaintEvent(event);
}

int LeftAreaTextEdit::lineNumberAreaWidth()
{
    return m_pTextEdit->lineNumberAreaWidth();
}


void LeftAreaTextEdit::bookMarkAreaPaintEvent(QPaintEvent *event)
{
    m_pTextEdit->bookMarkAreaPaintEvent(event);
}

void LeftAreaTextEdit::codeFlodAreaPaintEvent(QPaintEvent *event)
{
    m_pTextEdit->codeFLodAreaPaintEvent(event);
}


