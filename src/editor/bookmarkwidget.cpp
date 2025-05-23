// SPDX-FileCopyrightText: 2019 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "bookmarkwidget.h"
#include "leftareaoftextedit.h"
#include <QMouseEvent>

BookMarkWidget::BookMarkWidget(LeftAreaTextEdit *leftAreaWidget)
{
    qDebug() << "BookMarkWidget constructor";
    m_leftAreaWidget = leftAreaWidget;
}

BookMarkWidget::~BookMarkWidget()
{
    qDebug() << "BookMarkWidget destructor";
}

void BookMarkWidget::paintEvent(QPaintEvent *event)
{
    m_leftAreaWidget->bookMarkAreaPaintEvent(event);
}
