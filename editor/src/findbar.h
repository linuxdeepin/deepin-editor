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

#ifndef FINDBAR_H
#define FINDBAR_H

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include "dtextbutton.h"
#include "linebar.h"

DWIDGET_USE_NAMESPACE

class FindBar : public QWidget
{
    Q_OBJECT
    
public:
    FindBar(QWidget *parent = 0);
    
    void activeInput(QString text, QString file, int row, int column, int scrollOffset);
    
    void focus();
    
    bool isFocus();
    
signals:
    void backToPosition(QString file, int row, int column, int scrollOffset);
    void updateSearchKeyword(QString file, QString keyword);
    void findNext();
    void findPrev();
    void cleanMatchKeyword();
    
public slots:
    void back();
    void handleContentChanged();
    
protected:
    void paintEvent(QPaintEvent *event);
    void hideEvent(QHideEvent *event);
    
private:
    QHBoxLayout *layout;
    QLabel *findLabel;
    LineBar *editLine;
    
    DTextButton *findNextButton;
    DTextButton *findPrevButton;
    
    QString findFile;
    int findFileRow;
    int findFileColumn;
    int findFileSrollOffset;
};

#endif
