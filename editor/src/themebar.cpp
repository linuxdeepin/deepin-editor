/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 *               2011 ~ 2018 Wang Yong
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Wang Yong <wangyong@deepin.com>
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

#include "themebar.h"
#include "utils.h"
#include <QDebug>
#include <QVBoxLayout>
#include "themeitem.h"

ThemeBar::ThemeBar(QWidget *parent) : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint);
    setFixedWidth(0);
    
    animationDuration = 25;
    animationFrames = 10;
    expandWidth = 250;
        
    expandTimer = new QTimer();
    connect(expandTimer, &QTimer::timeout, this, &ThemeBar::expand);
    
    // connect(this, &ThemeBar::focusOut, this, &ThemeBar::handleFocusOut, Qt::QueuedConnection);
    
    themeView = new ThemeView();
    themeView->setRowHeight(130);
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    layout->addWidget(themeView);
    
    QStringList themes;
    themes << "Andy" << "Stewart" << "Bob" << "Dylan";
    QList<DSimpleListItem*> items;
    for (auto theme : themes) {
        ThemeItem *item = new ThemeItem(theme);
        items << item;
    }
    
    themeView->addItems(items);
}

void ThemeBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setOpacity(0.5);
    
    QPainterPath path;
    path.addRect(rect());
    painter.fillPath(path, QColor("#000000"));
}

void ThemeBar::popup()
{
    show();
    raise();
    setFocus();
    
    expandTicker = 0;
    expandTimer->start(animationDuration);
}

void ThemeBar::expand()
{
    expandTicker++;
    setFixedWidth(expandWidth * Utils::easeOutQuad(expandTicker * 1.0 / animationFrames));
    
    if (expandTicker > animationFrames) {
        expandTimer->stop();
    }
}

void ThemeBar::handleFocusOut()
{
    qDebug() << "**************";
}
