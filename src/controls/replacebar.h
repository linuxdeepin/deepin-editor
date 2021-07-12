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

#ifndef REPLACEBAR_H
#define REPLACEBAR_H

#include <DPushButton>
#include "linebar.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QWidget>
#include "dimagebutton.h"
#include <DIconButton>
#include <DApplicationHelper>
#include <DFloatingWidget>
#include <DAbstractDialog>

DWIDGET_USE_NAMESPACE

class ReplaceBar : public DFloatingWidget
{
    Q_OBJECT

public:
    explicit ReplaceBar(QWidget *parent = 0);

    bool isFocus();
    void focus();

    void activeInput(QString text, QString file, int row, int column, int scrollOffset);
    void setMismatchAlert(bool isAlert);
    void setsearched(bool _);
    inline DPushButton *getReplaceAllButton()  { return  m_replaceAllButton; }
    inline DPushButton *getReplaceButton()     { return  m_replaceButton; }
    inline DPushButton *getReplaceRestButton() { return  m_replaceRestButton; }
    inline DPushButton *getReplaceSkipButton() { return  m_replaceSkipButton; }

signals:
    void pressEsc();
    void replaceNext(QString file, QString replaceText, QString withText);
    void replaceSkip(QString file, QString keyword);
    void replaceRest(QString replaceText, QString withText);
    void replaceAll(QString replaceText, QString withText);
    void beforeReplace(QString _);

    void backToPosition(QString file, int row, int column, int scrollOffset);

    void removeSearchKeyword();
    void updateSearchKeyword(QString file, QString keyword);
    void sigReplacebarClose();

public slots:
    void change();
    void replaceClose();
    void handleContentChanged();
    void handleReplaceAll();
    void handleReplaceNext();
    void handleReplaceRest();

protected:
    void hideEvent(QHideEvent *event);
    bool focusNextPrevChild(bool next);
    void keyPressEvent(QKeyEvent *e);

private:
    DPushButton *m_replaceAllButton;
    DPushButton *m_replaceButton;
    DPushButton *m_replaceRestButton;
    DPushButton *m_replaceSkipButton;
    DIconButton *m_closeButton;
    LineBar *m_replaceLine;
    LineBar *m_withLine;
    QHBoxLayout *m_layout;
    QLabel *m_replaceLabel;
    QLabel *m_withLabel;
    QString m_replaceFile;
    int m_replaceFileColumn;
    int m_replaceFileRow;
    int m_replaceFileSrollOffset;
    QColor m_backgroundColor;
    bool searched = false;

    QPoint last;
};

#endif
