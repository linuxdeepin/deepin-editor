// SPDX-FileCopyrightText: 2017 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "toolbar.h"
#include <QLabel>
#include <QDebug>
#include <DHiDPIHelper>

DWIDGET_USE_NAMESPACE

ToolBar::ToolBar(QWidget *parent)
    : QWidget(parent),
      m_layout(new QHBoxLayout(this))
{
    qDebug() << "ToolBar constructor start";
    m_layout->setContentsMargins(0, 0, 0, 0);
    qDebug() << "ToolBar layout initialized with zero margins";
}

ToolBar::~ToolBar()
{
    qDebug() << "ToolBar destructor";
}

void ToolBar::setTabbar(QWidget *w)
{
    qDebug() << "Setting tabbar widget";
    m_layout->addWidget(w, 0);
    m_layout->addSpacing(70);
    qDebug() << "Tabbar widget added to toolbar with spacing";
}
