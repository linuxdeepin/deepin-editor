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

#ifndef THEMEBAR_H
#define THEMEBAR_H

#include <QWidget>
#include <QGraphicsOpacityEffect>
#include <QPainter>
#include <QTimer>
#include "themeview.h"

class ThemeBar : public QWidget
{
    Q_OBJECT
    
public:
    ThemeBar(QWidget *parent=0);
    
    void setBackground(QString color);
    
    ThemeView *themeView;
    QList<DSimpleListItem*> items;
    
protected:    
    void paintEvent(QPaintEvent *);
                                  
public slots:
    void popup();
    void expand();
    void shrink();
    void handleFocusOut();
    void handleThemeChanged(DSimpleListItem* item, int columnIndex, QPoint pos);
    
signals:
    void changeTheme(QString themeName);
    
private:
    QTimer *expandTimer;
    int expandTicker;
    
    QTimer *shrinkTimer;
    int shrinkTicker;
    
    QGraphicsOpacityEffect *opacityEffect;
    
    int barWidth;
    int animationDuration;
    int animationFrames;
    
    QColor backgroundColor;
    QColor frameLightColor = QColor("#000000");
    QColor frameDarkColor = QColor("#ffffff");
    QColor frameColor;
};

#endif
