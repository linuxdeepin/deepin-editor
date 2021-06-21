/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Rekols    <rekols@foxmail.com>
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

#include "linebar.h"

#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QPainter>
#include <QWidget>
#include <DApplicationHelper>
#include <DFloatingWidget>

DWIDGET_USE_NAMESPACE

const int nJumpLineBarWidth = 200;
const int nJumpLineBarHeight = 58;

class JumpLineBar : public DFloatingWidget
{
    Q_OBJECT

public:
    JumpLineBar(DFloatingWidget *parent = 0);

public slots:
    void focus();
    bool isFocus();

    void activeInput(QString file, int row, int column, int lineCount, int scrollOffset);

    void handleFocusOut();
    void handleLineChanged();

    void jumpCancel();
    void jumpConfirm();

    void slotFocusChanged(bool bFocus);

signals:
    void backToPosition(QString file, int row, int column, int scrollOffset);
    void jumpToLine(QString file, int line, bool focusEditor);
    void lostFocusExit();

protected:

private:
    LineBar *m_editLine;
    QHBoxLayout *m_layout;
    QIntValidator *m_lineValidator;
    QLabel *m_label;
    QString m_jumpFile;
    int m_jumpFileScrollOffset;
    int m_rowBeforeJump;
    int m_columnBeforeJump;
    QColor m_backgroundColor;
};

#endif
