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

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QWidget>
#include <DApplicationHelper>
#include <DFloatingWidget>
#include <DSpinBox>
#include <QLineEdit>
#include <QEvent>

DWIDGET_USE_NAMESPACE

const int nJumpLineBarWidth = 212;
const int nJumpLineBarHeight = 60;

class JumpLineBar : public DFloatingWidget
{
    Q_OBJECT
public:
    explicit JumpLineBar(DFloatingWidget *parent = 0);
    ~JumpLineBar();

public slots:
    void focus();
    bool isFocus();
    void activeInput(QString file, int row, int column, int lineCount, int scrollOffset);
    void handleFocusOut();
    void handleLineChanged();
    void jumpCancel();
    void jumpConfirm();
    void slotFocusChanged(bool bFocus);
    void hide();

signals:
    void backToPosition(QString file, int row, int column, int scrollOffset);
    void jumpToLine(QString file, int line, bool focusEditor);
    void lostFocusExit();
    void pressEsc();

protected:
    bool eventFilter(QObject *pObject, QEvent *pEvent);

private:
    DSpinBox *m_pSpinBoxInput {nullptr};
    QHBoxLayout *m_layout {nullptr};
    QLabel *m_label {nullptr};
    QString m_jumpFile;
    int m_jumpFileScrollOffset;
    int m_rowBeforeJump;
    int m_columnBeforeJump;
    QColor m_backgroundColor;
};

#endif
