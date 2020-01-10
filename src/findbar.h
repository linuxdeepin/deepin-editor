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

#ifndef FINDBAR_H
#define FINDBAR_H

#include "linebar.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QPainter>
#include <QWidget>
#include "dimagebutton.h"
#include <QColor>
#include <DIconButton>
#include <DApplicationHelper>
#include <DFloatingWidget>

#include <DPalette>
#include <DAbstractDialog>

DWIDGET_USE_NAMESPACE

class FindBar : public DAbstractDialog
{
    Q_OBJECT

public:
    FindBar(QWidget *parent = 0);

    bool isFocus();
    void focus();

    void activeInput(QString text, QString file, int row, int column, int scrollOffset);
    void setMismatchAlert(bool isAlert);

signals:
    void findNext();
    void findPrev();

    void removeSearchKeyword();
    void updateSearchKeyword(QString file, QString keyword);

    //add guoshao
    void sigFindbarClose();

public slots:
    void findCancel();
    void handleContentChanged();

protected:
    void hideEvent(QHideEvent *event) override;

private:
    QPushButton *m_findNextButton;
    QPushButton *m_findPrevButton;
    DIconButton *m_closeButton;
    LineBar *m_editLine;
    QHBoxLayout *m_layout;
    QLabel *m_findLabel;
    QString m_findFile;
    int m_findFileColumn;
    int m_findFileRow;
    int m_findFileSrollOffset;
    QColor m_backgroundColor;
};

#endif
