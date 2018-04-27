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
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextDocument>
#include <QFileInfo>

DWIDGET_USE_NAMESPACE

ThemeItem::ThemeItem(QString themeDir)
{
    themeName = QFileInfo(themeDir).fileName();

    QVariantMap jsonMap = Utils::getThemeNodeMap(themeName);
    
    importColor = jsonMap["text-styles"].toMap()["Import"].toMap()["text-color"].toString();
    stringColor = jsonMap["text-styles"].toMap()["String"].toMap()["text-color"].toString();
    builtInColor = jsonMap["text-styles"].toMap()["BuiltIn"].toMap()["text-color"].toString();
    keywordColor = jsonMap["text-styles"].toMap()["Keyword"].toMap()["text-color"].toString();
    commentColor = jsonMap["text-styles"].toMap()["Comment"].toMap()["text-color"].toString();
    functionColor = jsonMap["text-styles"].toMap()["Function"].toMap()["text-color"].toString();
    normalColor = jsonMap["text-styles"].toMap()["Normal"].toMap()["text-color"].toString();
    otherColor = jsonMap["text-styles"].toMap()["Others"].toMap()["text-color"].toString();
    backgroundColor = jsonMap["editor-colors"].toMap()["background-color"].toString();
}

bool ThemeItem::sameAs(DSimpleListItem *item)
{
    return themeName == ((static_cast<ThemeItem*>(item)))->themeName;
}

void ThemeItem::drawBackground(QRect, QPainter*, int, bool, bool)
{

}

void ThemeItem::drawForeground(QRect rect, QPainter *painter, int column, int, bool isSelect, bool)
{
    // Init opacity and font size.
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setOpacity(1);
    Utils::setFontSize(*painter, 12);
    
    if (column == 0) {
        // Build path.
        QPainterPath path;
        path.addRoundedRect(QRect(rect.x() + 20, rect.y() + 5, rect.width() - 40, rect.height() - 10), 6, 6);
        
        // Draw background.
        painter->fillPath(path, QColor(backgroundColor));
        
        // Draw frame.
        painter->save();
        
        QPen framePen;
        if (isSelect) {
            framePen.setColor("#2CA7F8");
        } else {
            framePen.setColor("#000000");
        }
        painter->setOpacity(1);
        painter->setPen(framePen);
        painter->drawPath(path);
        
        painter->restore();
        
        // Draw syntax highlight.
        painter->save();
        
        int htmlPaddingLeft = 30;
        int htmlPaddingTop = 10;

        painter->translate(QPointF(rect.x() + htmlPaddingLeft, rect.y() + htmlPaddingTop));
        QTextDocument td;
        QString indentConent = "&nbsp;&nbsp;&nbsp;&nbsp;";
        QString htmlConent = QString(
            "<font size='4'><code>"
            "<font color='%1'>#include</font> "
            "<font color='%2'>\"deepin.h\"</font><br/>"
            "<font color='%3'>QString</font> <font color='%4'>theme</font><font color='%5'>() {</font><br/>"
            "<font color='%6'>%7// Return theme name.</font><br/>"
            "<font color='%8'>%9return</font> <font color='%10'>\"%11\"</font><font color='%12'>;</font><br/>"
            "<font color='%13'>}</font>"
            "</code></font>").arg(otherColor).arg(importColor).arg(keywordColor).arg(functionColor).arg(normalColor).arg(commentColor).arg(indentConent).arg(builtInColor).arg(indentConent).arg(stringColor).arg(themeName).arg(normalColor).arg(normalColor);
        td.setHtml(htmlConent);
        td.drawContents(painter);
        
        painter->restore();
    }
}

bool ThemeItem::sortByLightness(const DSimpleListItem *item1, const DSimpleListItem *item2, bool descendingSort)
{
    auto lightness1 = QColor((static_cast<const ThemeItem*>(item1))->backgroundColor).lightness();
    auto lightness2 = QColor((static_cast<const ThemeItem*>(item2))->backgroundColor).lightness();
    
    bool sortOrder = lightness1 < lightness2;
    
    return descendingSort ? sortOrder : !sortOrder;
}
