/*
 * Copyright (C) 2015 ~ 2017 Deepin Technology Co., Ltd.
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

#include "wordcompletionitem.h"
#include <QColor>
#include <QDebug>

DWIDGET_USE_NAMESPACE

WordCompletionItem::WordCompletionItem(QString itemName)
{
    name = itemName;
}

bool WordCompletionItem::sameAs(DSimpleListItem *item)
{
    return name == (static_cast<WordCompletionItem*>(item))->name;
}

void WordCompletionItem::drawBackground(QRect rect, QPainter *painter, int, bool isSelect, bool)
{
    QPainterPath path;
    path.addRect(QRectF(rect));
    
    painter->setOpacity(1);
    if (isSelect) {
        painter->fillPath(path, QColor(selectedBackgroundColor));
    } else {
        painter->fillPath(path, QColor(normalBackgroundColor));
    }
}

void WordCompletionItem::drawForeground(QRect rect, QPainter *painter, int, int, bool isSelect, bool)
{
    painter->setOpacity(1);
    if (isSelect) {
        painter->setPen(QPen(QColor(selectedTextColor)));    
    } else {
        painter->setPen(QPen(QColor(normalTextColor)));
    }
    
    int padding = 10;
    QFont font = painter->font();
    font.setPointSize(12);
    painter->setFont(font);
    painter->drawText(QRect(rect.x() + padding, rect.y(), rect.width() - padding * 2, rect.height()), Qt::AlignLeft | Qt::AlignVCenter, name);
}


void WordCompletionItem::setColors(QString sBackgroundColor, QString sTextColor, QString nBackgroundColor, QString nTextColor)
{
    selectedBackgroundColor = sBackgroundColor;
    selectedTextColor = sTextColor;
    normalBackgroundColor = nBackgroundColor;
    normalTextColor = nTextColor;
}
