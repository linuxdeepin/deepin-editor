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

#include "themeitem.h"
#include "utils.h"
#include <QDebug>
#include <QTextDocument>

DWIDGET_USE_NAMESPACE

ThemeItem::ThemeItem(QString name)
{
    themeName = name;
}

bool ThemeItem::sameAs(DSimpleListItem *item)
{
    return themeName == ((static_cast<ThemeItem*>(item)))->themeName;
}

void ThemeItem::drawBackground(QRect rect, QPainter *painter, int index, bool isSelect, bool isHover)
{
    
}

void ThemeItem::drawForeground(QRect rect, QPainter *painter, int column, int index, bool isSelect, bool isHover)
{
    // Init opacity and font size.
    painter->setOpacity(1);
    Utils::setFontSize(*painter, 12);
    

    if (column == 0) {
        int htmlPaddingLeft = 10;
        int htmlPaddingTop = 10;
            
        painter->translate(QPointF(rect.x() + htmlPaddingLeft, rect.y() + htmlPaddingTop));
        QTextDocument td;
        QString htmlConent;
        htmlConent =
            "<font size='4' color='#ff0000'>#include　\"deepin.h\"</font><br/>"
            "<font size='4' color='#00ffff'>QString</font> <font size='4' color='#ffffff'>theme()</font> <font size='4' color='#666666'>{</font><br/>"
            "<font size='4' color='#333333'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;// Return theme name.</font><br/>"
            "<font size='4' color='#ff00ff'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;return</font> <font size='4' color='#ffff00'>\"amazaing\"</font><font size='4' color='#666666'>;</font><br/>"
            "<font size='4' color='#666666'>}</font>";
        td.setHtml(htmlConent);
        td.drawContents(painter);        
    }
}
