// SPDX-FileCopyrightText: 2019-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddlog.h"
#include "codeflodarea.h"
#include "leftareaoftextedit.h"

#include <QDebug>


CodeFlodArea::CodeFlodArea(LeftAreaTextEdit *leftAreaWidget)
{
    qCDebug(appLog) << "CodeFlodArea created";
    m_pLeftAreaWidget = leftAreaWidget;
}

CodeFlodArea::~CodeFlodArea()
{
    qCDebug(appLog) << "CodeFlodArea destroyed";

}

void CodeFlodArea::paintEvent(QPaintEvent *e)
{
    qCDebug(appLog) << "CodeFlodArea paintEvent";
    m_pLeftAreaWidget->codeFlodAreaPaintEvent(e);
}

