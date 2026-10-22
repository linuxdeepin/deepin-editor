// SPDX-FileCopyrightText: 2019-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddlog.h"
#include "bookmarkwidget.h"
#include "leftareaoftextedit.h"
#include <QMouseEvent>
#include <QDebug>

BookMarkWidget::BookMarkWidget(LeftAreaTextEdit *leftAreaWidget)
{
    qCDebug(appLog) << "BookMarkWidget constructor";
    m_leftAreaWidget = leftAreaWidget;
}

BookMarkWidget::~BookMarkWidget()
{
    qCDebug(appLog) << "BookMarkWidget destructor";
}

void BookMarkWidget::paintEvent(QPaintEvent *event)
{
    m_leftAreaWidget->bookMarkAreaPaintEvent(event);
}
