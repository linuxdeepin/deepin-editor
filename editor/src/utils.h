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

#include "settings.h"
#include <QKeyEvent>
#include <QObject>
#include <QPainter>
#include <QString>
#include <QImage>

class Utils : public QObject
{
    Q_OBJECT
    
public:
    static QSize getRenderSize(int fontSize, QString string);
    static QString getKeyshortcut(QKeyEvent *keyEvent);
    static QString getKeyshortcutFromKeymap(Settings* settings, QString keyCategory, QString keyName);
    static QString getQrcPath(QString imageName);
    static QString getQssPath(QString qssName);
    static QString getFileContent(QString filepath);
    static bool fileExists(QString path);
    static bool fileIsWritable(QString path);
    static void applyQss(QWidget *widget, QString qssName);
    static void passInputEvent(int wid);
    static void setFontSize(QPainter &painter, int textSize);
    static QPixmap dropShadow(const QPixmap &source, qreal radius, const QColor &color, const QPoint &offset);
    static QImage dropShadow(const QPixmap &px, qreal radius, const QColor &color);
    static qreal easeInOut(qreal x);
    static qreal easeInQuad(qreal x);
    static qreal easeInQuint(qreal x);
    static qreal easeOutQuad(qreal x);
    static qreal easeOutQuint(qreal x);
    static QVariantMap getThemeNodeMap(QString themeName);
    static QByteArray getFileEncode(QByteArray fileContent);
    static bool isEditableFile(QString filepath);
};
