// SPDX-FileCopyrightText: 2017-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddlog.h"
#include "toolbar.h"
#include <QLabel>
#include <QDebug>
#include <DHiDPIHelper>

DWIDGET_USE_NAMESPACE

ToolBar::ToolBar(QWidget *parent)
    : QWidget(parent),
      m_layout(new QHBoxLayout(this))
{
    qCDebug(appLog) << "ToolBar constructor start";
    m_layout->setContentsMargins(0, 0, 0, 0);
    qCDebug(appLog) << "ToolBar layout initialized with zero margins";
}

ToolBar::~ToolBar()
{
    qCDebug(appLog) << "ToolBar destructor";
}

void ToolBar::setTabbar(QWidget *w)
{
    qCDebug(appLog) << "Setting tabbar widget";
    m_layout->addWidget(w, 0);
    m_layout->addSpacing(70);
    qCDebug(appLog) << "Tabbar widget added to toolbar with spacing";
}
