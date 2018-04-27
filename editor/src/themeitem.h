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

#ifndef THEMEITEM_H
#define THEMEITEM_H

#include <DSimpleListItem>

DWIDGET_USE_NAMESPACE

class ThemeItem : public DSimpleListItem
{
    Q_OBJECT

public:
    ThemeItem(QString themeDir);
    
    bool sameAs(DSimpleListItem *item);
    void drawBackground(QRect rect, QPainter *painter, int index, bool isSelect, bool isHover);
    void drawForeground(QRect rect, QPainter *painter, int column, int index, bool isSelect, bool isHover);
    
    static bool sortByLightness(const DSimpleListItem *item1, const DSimpleListItem *item2, bool descendingSort);
    
    QString themeName;
    
private:
    QString importColor;
    QString stringColor;
    QString builtInColor;
    QString keywordColor;
    QString commentColor;
    QString functionColor;
    QString normalColor;
    QString backgroundColor;
    QString otherColor;
};

#endif
