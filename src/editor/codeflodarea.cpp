// SPDX-FileCopyrightText: 2019 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "codeflodarea.h"
#include "leftareaoftextedit.h"

#include <QDebug>


CodeFlodArea::CodeFlodArea(LeftAreaTextEdit *leftAreaWidget)
{
    qDebug() << "CodeFlodArea created";
    m_pLeftAreaWidget = leftAreaWidget;
}

CodeFlodArea::~CodeFlodArea()
{
    qDebug() << "CodeFlodArea destroyed";

}

void CodeFlodArea::paintEvent(QPaintEvent *e)
{
    qDebug() << "CodeFlodArea paintEvent";
    m_pLeftAreaWidget->codeFlodAreaPaintEvent(e);
}

