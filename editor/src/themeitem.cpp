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

    auto filePath = QDir(themeDir).filePath("editor.theme");
    
    QFile fileObject(filePath);
    if(!fileObject.open(QIODevice::ReadOnly)){
        qDebug()<<"Failed to open "<<filePath;
    }

    QTextStream file_text(&fileObject);
    QString jsonString;
    jsonString = file_text.readAll();
    fileObject.close();
    QByteArray jsonBytes = jsonString.toLocal8Bit();

    auto jsonDocument = QJsonDocument::fromJson(jsonBytes);

    if(jsonDocument.isNull()){
        qDebug()<<"Failed to create JSON doc.";
    }
    
    if(!jsonDocument.isObject()){
        qDebug()<<"JSON is not an object.";
    }

    QJsonObject jsonObject = jsonDocument.object();

    if(jsonObject.isEmpty()){
        qDebug()<<"JSON object is empty.";
    }

    QVariantMap jsonMap = jsonObject.toVariantMap();
    
    importColor = jsonMap["text-styles"].toMap()["Import"].toMap()["text-color"].toString();
    stringColor = jsonMap["text-styles"].toMap()["String"].toMap()["text-color"].toString();
    builtInColor = jsonMap["text-styles"].toMap()["BuiltIn"].toMap()["text-color"].toString();
    keywordColor = jsonMap["text-styles"].toMap()["Keyword"].toMap()["text-color"].toString();
    commentColor = jsonMap["text-styles"].toMap()["Comment"].toMap()["text-color"].toString();
    functionColor = jsonMap["text-styles"].toMap()["Function"].toMap()["text-color"].toString();
    normalColor = jsonMap["text-styles"].toMap()["Normal"].toMap()["text-color"].toString();
    backgroundColor = jsonMap["editor-colors"].toMap()["background-color"].toString();
    
    qDebug() << themeName << importColor << stringColor << builtInColor << keywordColor << commentColor <<  functionColor << normalColor << backgroundColor;
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
            "<font color='%1'>#include　\"deepin.h\"</font><br/>"
            "<font color='%2'>QString</font> <font color='%3'>theme</font><font color='%4'>() {</font><br/>"
            "<font color='%5'>%6// Return theme name.</font><br/>"
            "<font color='%7'>%8return</font> <font color='%9'>\"%10\"</font><font color='%11'>;</font><br/>"
            "<font color='%12'>}</font>"
            "</code></font>").arg(importColor).arg(keywordColor).arg(functionColor).arg(normalColor).arg(commentColor).arg(indentConent).arg(builtInColor).arg(indentConent).arg(stringColor).arg(themeName).arg(normalColor).arg(normalColor);
        td.setHtml(htmlConent);
        td.drawContents(painter);
        
        painter->restore();
    }
}
