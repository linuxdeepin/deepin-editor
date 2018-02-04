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

#ifndef JUMPLINEBAR_H
#define JUMPLINEBAR_H

#include <QWidget>
#include <QIntValidator>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include "linebar.h"

DWIDGET_USE_NAMESPACE

class JumpLineBar : public QWidget
{
    Q_OBJECT
    
public:
    JumpLineBar(QWidget *parent = 0);
    
    void activeInput(QString file, int line, int lineCount, int scrollOffset);
    
public slots:    
    void cancel();
    void back();
    void jump();
    void tempJump();
    
signals:
    void cancelJump();
    void backToLine(QString file, int line, int scrollOffset);
    void jumpToLine(QString file, int line);
    void tempJumpToLine(QString file, int line);
    
protected:
    void paintEvent(QPaintEvent *event);
    
private:
    QHBoxLayout *layout;
    QLabel *label;
    LineBar *editLine;
    
    QString jumpFile;
    int lineBeforeJump;
    int jumpFileScrollOffset;
    
    QIntValidator *lineValidator;
};

#endif
