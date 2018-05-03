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
#include <QFileInfoList>
#include <QDir>
#include "themeitem.h"

ThemeBar::ThemeBar(QWidget *parent) : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint);
    setFixedWidth(0);

    animationDuration = 25;
    animationFrames = 10;
    barWidth = 280;

    expandTimer = new QTimer();
    connect(expandTimer, &QTimer::timeout, this, &ThemeBar::expand);

    shrinkTimer = new QTimer();
    connect(shrinkTimer, &QTimer::timeout, this, &ThemeBar::shrink);

    themeView = new ThemeView();
    themeView->setRowHeight(110);
    
    QList<SortAlgorithm> *alorithms = new QList<SortAlgorithm>();
    alorithms->append(&ThemeItem::sortByLightness);
    themeView->setColumnSortingAlgorithms(alorithms, 0, true);
    
    // Set column to make sort list works.
    QList<QString> titles;
    titles << "theme";
    QList<int> widths;
    widths << -1;
    themeView->setColumnTitleInfo(titles, widths, 0);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    layout->addWidget(themeView);

    QStringList filters;
    QFileInfoList themeInfos = QDir(":/theme").entryInfoList(filters, QDir::Dirs | QDir::NoDotAndDotDot);

    for (auto themeInfo : themeInfos) {
        ThemeItem *item = new ThemeItem(themeInfo.absoluteFilePath());
        items << item;
    }

    themeView->addItems(items);
    connect(themeView, &ThemeView::focusOut, this, &ThemeBar::handleFocusOut);
    connect(themeView, &ThemeView::mousePressChanged, this, &ThemeBar::handleThemeChanged);
    
    opacityEffect = new QGraphicsOpacityEffect(this);
    this->setGraphicsEffect(opacityEffect);
}

void ThemeBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setOpacity(0.5);

    QPainterPath path;
    path.addRect(QRect(rect().x() + 1, rect().y(), rect().width() - 1, rect().height()));
    painter.setOpacity(0.8);
    painter.fillPath(path, backgroundColor);
    
    QPainterPath framePath;
    framePath.addRect(QRect(rect().x(), rect().y(), 1, rect().height()));
    painter.setOpacity(0.1);
    painter.fillPath(framePath, frameColor);
}

void ThemeBar::popup()
{
    show();
    raise();
    themeView->setFocus();

    expandTicker = 0;
    expandTimer->start(animationDuration);
}

void ThemeBar::expand()
{
    expandTicker++;
    setFixedWidth(barWidth * Utils::easeOutQuad(expandTicker * 1.0 / animationFrames));
    opacityEffect->setOpacity(Utils::easeOutQuad(expandTicker * 1.0 / animationFrames));

    if (expandTicker > animationFrames) {
        expandTimer->stop();
    }
}

void ThemeBar::shrink()
{
    shrinkTicker++;
    setFixedWidth(barWidth * (1 - Utils::easeOutQuad(shrinkTicker * 1.0 / animationFrames)));
    opacityEffect->setOpacity(1 - Utils::easeOutQuad(shrinkTicker * 1.0 / animationFrames));

    if (shrinkTicker > animationFrames) {
        shrinkTimer->stop();
    }
}

void ThemeBar::handleFocusOut()
{
    shrinkTicker = 0;
    shrinkTimer->start(animationDuration);
}

void ThemeBar::handleThemeChanged(DSimpleListItem* item, int, QPoint)
{
    emit changeTheme((static_cast<ThemeItem*>(item))->themeName);
}

void ThemeBar::setBackground(QString color)
{
    backgroundColor = QColor(color);
    
    if (backgroundColor.lightness() < 128) {
        frameColor = frameDarkColor;
    } else {
        frameColor = frameLightColor;
    }
    
    repaint();
}
