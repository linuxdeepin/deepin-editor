/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     rekols <rekols@foxmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "toolbar.h"
#include <QLabel>
#include <DHiDPIHelper>

DWIDGET_USE_NAMESPACE

ToolBar::ToolBar(QWidget *parent)
    : QWidget(parent),
      m_layout(new QHBoxLayout(this))
{
    m_layout->setContentsMargins(0, 0, 0, 0);

//    QPixmap iconPixmap = DHiDPIHelper::loadNxPixmap(":/images/logo_24.svg");
//    QLabel *iconLabel = new QLabel;
//    iconLabel->setPixmap(iconPixmap);
//    iconLabel->setFixedSize(32, 32);

//    m_layout->addSpacing(10);
//    m_layout->addWidget(iconLabel);
//    m_layout->addWidget(iconLabel, 10, Qt::AlignTop);
//    m_layout->addSpacing(10);
}

ToolBar::~ToolBar()
{
}

void ToolBar::setTabbar(QWidget *w)
{
    m_layout->addWidget(w, 0);
    m_layout->addSpacing(70);
}
