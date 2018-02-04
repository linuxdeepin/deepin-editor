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

#ifndef REPLACEBAR_H
#define REPLACEBAR_H

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include "dtextbutton.h"
#include "linebar.h"

DWIDGET_USE_NAMESPACE

class ReplaceBar : public QWidget
{
    Q_OBJECT
    
public:
    ReplaceBar(QWidget *parent = 0);
    
    void activeInput(QString text, QString file, int row, int column, int scrollOffset);
    
    void focus();
    
    bool isFocus();
    
signals:
    void backToPosition(QString file, int row, int column, int scrollOffset);
    void updateSearchKeyword(QString file, QString keyword);
    void replaceNext(QString replaceText, QString withText);
    void replaceSkip();
    void replaceRest(QString replaceText, QString withText);
    void replaceAll(QString replaceText, QString withText);
    void cleanMatchKeyword();
    
public slots:
    void back();
    void handleContentChanged();
    void handleReplaceNext();
    void handleReplaceRest();
    void handleReplaceAll();
    
protected:
    void paintEvent(QPaintEvent *event);
    bool focusNextPrevChild(bool next);
    void hideEvent(QHideEvent *event);
    
private:
    QHBoxLayout *layout;
    
    QLabel *replaceLabel;
    LineBar *replaceLine;
    
    QLabel *withLabel;
    LineBar *withLine;
    
    DTextButton *replaceButton;
    DTextButton *replaceSkipButton;
    DTextButton *replaceRestButton;
    DTextButton *replaceAllButton;
    
    QString replaceFile;
    int replaceFileRow;
    int replaceFileColumn;
    int replaceFileSrollOffset;
};

#endif
