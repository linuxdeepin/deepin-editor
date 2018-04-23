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

#include "utils.h"

#include <DSettings>
#include <DSettingsOption>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFontMetrics>
#include <QPainter>
#include <QString>
#include <QtMath>
#include <QWidget>

QT_BEGIN_NAMESPACE
extern Q_WIDGETS_EXPORT void qt_blurImage(QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0);
QT_END_NAMESPACE

QString Utils::getQrcPath(QString imageName)
{
    return QString(":/image/%1").arg(imageName);
}

QString Utils::getQssPath(QString qssName)
{
    return QString(":/qss/%1").arg(qssName);
}

QSize Utils::getRenderSize(int fontSize, QString string)
{
    QFont font;
    font.setPointSize(fontSize);
    QFontMetrics fm(font);

    int width = 0;
    int height = 0;
    foreach (auto line, string.split("\n")) {
        int lineWidth = fm.width(line);
        int lineHeight = fm.height();

        if (lineWidth > width) {
            width = lineWidth;
        }
        height += lineHeight;
    }

    return QSize(width, height);
}

void Utils::setFontSize(QPainter &painter, int textSize)
{
    QFont font = painter.font() ;
    font.setPointSize(textSize);
    painter.setFont(font);
}

void Utils::applyQss(QWidget *widget, QString qssName)
{
    QFile file(Utils::getQssPath(qssName));
    file.open(QFile::ReadOnly);
    QTextStream filetext(&file);
    QString stylesheet = filetext.readAll();
    widget->setStyleSheet(stylesheet);
    file.close();
}


QString Utils::getFileContent(QString filepath)
{
    QString content;
    
    QFile file(filepath);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        content = file.readAll();
    }
    
    file.close();
    
    return content;
}

bool Utils::fileExists(QString path) {
    QFileInfo check_file(path);
    
    return check_file.exists() && check_file.isFile();
}

bool Utils::fileIsWritable(QString path)
{
    QFileDevice::Permissions permissions = QFile(path).permissions();
    
    return permissions & QFileDevice::WriteUser;
}

QString Utils::getKeyshortcut(QKeyEvent *keyEvent)
{
    QStringList keys;
    Qt::KeyboardModifiers modifiers = keyEvent->modifiers();
    if (modifiers != Qt::NoModifier){
        if (modifiers.testFlag(Qt::ControlModifier)) {
            keys.append("Ctrl");
        }
        
        if (modifiers.testFlag(Qt::AltModifier)) {
            keys.append("Alt");
        }
        
        if (modifiers.testFlag(Qt::MetaModifier)) {
            keys.append("Meta");
        }
        
        if (modifiers.testFlag(Qt::ShiftModifier)) {
            keys.append("Shift");
        }
    }
    
    if(keyEvent->key() !=0 && keyEvent->key() != Qt::Key_unknown){
        keys.append(QKeySequence(keyEvent->key()).toString());
    }
    
    return keys.join("+");
}

QString Utils::getKeyshortcutFromKeymap(Settings* settings, QString keyCategory, QString keyName)
{
    return settings->settings->option(QString("shortcuts.%1.%2").arg(keyCategory).arg(keyName))->value().toString();
}

QPixmap Utils::dropShadow(const QPixmap &source, qreal radius, const QColor &color, const QPoint &offset)
{
    QImage shadow = dropShadow(source, radius, color);
    QPainter pa(&shadow);
    pa.setCompositionMode(QPainter::CompositionMode_SourceOver);
    pa.drawPixmap(radius - offset.x(), radius - offset.y(), source);
    pa.end();
    
    return QPixmap::fromImage(shadow);
}

QImage Utils::dropShadow(const QPixmap &px, qreal radius, const QColor &color)
{
    if (px.isNull()) {
        return QImage();
    }
    
    QImage tmp(px.size() + QSize(radius * 2, radius * 2), QImage::Format_ARGB32_Premultiplied);
    tmp.fill(0);
    QPainter tmpPainter(&tmp);
    tmpPainter.setCompositionMode(QPainter::CompositionMode_Source);
    tmpPainter.drawPixmap(QPoint(radius, radius), px);
    tmpPainter.end();
    
    // Blur the alpha channel.
    QImage blurred(tmp.size(), QImage::Format_ARGB32_Premultiplied);
    blurred.fill(0);
    QPainter blurPainter(&blurred);
    qt_blurImage(&blurPainter, tmp, radius, false, true);
    blurPainter.end();
    if (color == QColor(Qt::black)) {
        return blurred;
    }
    tmp = blurred;
    
    // Blacken the image...
    tmpPainter.begin(&tmp);
    tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    tmpPainter.fillRect(tmp.rect(), color);
    tmpPainter.end();
    
    return tmp;
}

qreal Utils::easeInOut(qreal x)
{
    return (1 - qCos(M_PI * x)) / 2;
}

qreal Utils::easeInQuad(qreal x)
{
    return qPow(x, 2);
}

qreal Utils::easeOutQuad(qreal x)
{
    return -1 * qPow(x - 1, 2) + 1;
}

qreal Utils::easeInQuint(qreal x)
{
    return qPow(x, 5);
}

qreal Utils::easeOutQuint(qreal x)
{
    return qPow(x - 1, 5) + 1;
}

