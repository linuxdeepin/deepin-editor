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

ThemeItem::ThemeItem(QString themeDir, qreal scale)
{
    themeName = QFileInfo(themeDir).fileName();
    screenScale = scale;

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
        // Draw frame.
        painter->save();

        QPainterPath framePath;
        framePath.addRoundedRect(
            QRect(rect.x() + itemPaddingX,
                  rect.y() + itemPaddingY,
                  rect.width() - itemPaddingX * 2,
                  rect.height() - itemPaddingY * 2), frameRadius, frameRadius);
        QString frameColor;
        if (isSelect) {
            frameColor = frameSelectedColor;
        } else {
            frameColor = frameNormalColor;
        }
        painter->setOpacity(1);
        painter->setPen(QPen(QColor(frameColor), 1));
        painter->drawPath(framePath);

        painter->restore();

        // Draw background.
        QPainterPath backgroundPath;
        backgroundPath.addRoundedRect(
            QRect(rect.x() + itemPaddingX,
                  rect.y() + itemPaddingY,
                  rect.width() - itemPaddingX * 2,
                  rect.height() - itemPaddingY * 2), frameRadius, frameRadius);

        painter->setOpacity(0.8);
        painter->fillPath(backgroundPath, QColor(backgroundColor));

        // Draw syntax highlight.
        painter->save();

        painter->setOpacity(1);
        QFont font = painter->font();
        font.setPointSize(fontSize);
        painter->setFont(font);

        QFontMetrics fm(font);

        int includeX = renderX;
        int includeY = renderY;
        painter->setPen(QPen(QColor(otherColor)));
        painter->drawText(QRect(rect.x() + includeX, rect.y() + includeY, rect.width(), lineHeight), Qt::AlignLeft | Qt::AlignTop, "#include");

        int headerX = includeX + fm.width("#include ");
        int headerY = includeY;
        painter->setPen(QPen(QColor(importColor)));
        painter->drawText(QRect(rect.x() + headerX, rect.y() + headerY, rect.width(), lineHeight), Qt::AlignLeft | Qt::AlignTop, "\"deepin.h\"");

        int keywordX = includeX;
        int keywordY = includeY + lineHeight;
        painter->setPen(QPen(QColor(keywordColor)));
        painter->drawText(QRect(rect.x() + keywordX, rect.y() + keywordY, rect.width(), lineHeight), Qt::AlignLeft | Qt::AlignTop, "QString");

        int functionX = includeX + fm.width("QString ");
        int functionY = includeY + lineHeight;
        painter->setPen(QPen(QColor(functionColor)));
        painter->drawText(QRect(rect.x() + functionX, rect.y() + functionY, rect.width(), lineHeight), Qt::AlignLeft | Qt::AlignTop, "theme");

        int functionArgX = includeX + fm.width("QString theme");
        int functionArgY = includeY + lineHeight;
        painter->setPen(QPen(QColor(normalColor)));
        painter->drawText(QRect(rect.x() + functionArgX, rect.y() + functionArgY, rect.width(), lineHeight), Qt::AlignLeft | Qt::AlignTop, "() {");

        int commentX = includeX + fm.width("QStr");
        int commentY = includeY + lineHeight * 2;
        painter->setPen(QPen(QColor(commentColor)));
        painter->drawText(QRect(rect.x() + commentX, rect.y() + commentY, rect.width(), lineHeight), Qt::AlignLeft | Qt::AlignTop, "// Return theme name.");

        int builtInX = includeX + fm.width("QStr");
        int builtInY = includeY + lineHeight * 3;
        painter->setPen(QPen(QColor(builtInColor)));
        painter->drawText(QRect(rect.x() + builtInX, rect.y() + builtInY, rect.width(), lineHeight), Qt::AlignLeft | Qt::AlignTop, "return");

        int stringX = includeX + fm.width("QStr") + fm.width("return ");
        int stringY = includeY + lineHeight * 3;
        painter->setPen(QPen(QColor(stringColor)));
        painter->drawText(QRect(rect.x() + stringX, rect.y() + stringY, rect.width(), lineHeight), Qt::AlignLeft | Qt::AlignTop, QString("\"%1\"").arg(themeName));

        int semicolonX = includeX + fm.width("QStr") + fm.width("return ") + fm.width(QString("\"%1\"").arg(themeName));
        int semicolonY = includeY + lineHeight * 3;
        painter->setPen(QPen(QColor(normalColor)));
        painter->drawText(QRect(rect.x() + semicolonX, rect.y() + semicolonY, rect.width(), lineHeight), Qt::AlignLeft | Qt::AlignTop, ";");

        int bracketX = includeX;
        int bracketY = includeY + lineHeight * 4;
        painter->setPen(QPen(QColor(normalColor)));
        painter->drawText(QRect(rect.x() + bracketX, rect.y() + bracketY, rect.width(), lineHeight), Qt::AlignLeft | Qt::AlignTop, "}");

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

void ThemeItem::setFrameColor(QString selectedColor, QString normalColor)
{
    frameSelectedColor = selectedColor;
    frameNormalColor = normalColor;
}
