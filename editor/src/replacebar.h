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

#include <QPushButton>
#include "linebar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QWidget>

DWIDGET_USE_NAMESPACE

class ReplaceBar : public QWidget
{
    Q_OBJECT
    
public:
    ReplaceBar(QWidget *parent = 0);
    
    bool isFocus();
    void focus();
    
    void activeInput(QString text, QString file, int row, int column, int scrollOffset);
    
signals:
    void replaceNext(QString replaceText, QString withText);
    void replaceSkip();
    void replaceRest(QString replaceText, QString withText);
    void replaceAll(QString replaceText, QString withText);
    
    void backToPosition(QString file, int row, int column, int scrollOffset);
    
    void removeSearchKeyword();
    void updateSearchKeyword(QString file, QString keyword);
    
public slots:
    void replaceCancel();
    void handleContentChanged();
    void handleReplaceAll();
    void handleReplaceNext();
    void handleReplaceRest();
    
protected:
    void hideEvent(QHideEvent *event);
    void paintEvent(QPaintEvent *event);
    bool focusNextPrevChild(bool next);
    
private:
    QPushButton *replaceAllButton;
    QPushButton *replaceButton;
    QPushButton *replaceRestButton;
    QPushButton *replaceSkipButton;
    LineBar *replaceLine;
    LineBar *withLine;
    QHBoxLayout *layout;
    QLabel *replaceLabel;
    QLabel *withLabel;
    QString replaceFile;
    int replaceFileColumn;
    int replaceFileRow;
    int replaceFileSrollOffset;
};

#endif
