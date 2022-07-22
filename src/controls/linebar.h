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

#ifndef LINEBAR_H
#define LINEBAR_H

#include "dlineedit.h"

#include <QTimer>

DWIDGET_USE_NAMESPACE

class LineBar : public DLineEdit
{
    Q_OBJECT

public:
    explicit LineBar(DLineEdit *parent = 0);

public slots:
    void handleTextChangeTimer();
    void handleTextChanged(const QString &str="");
    void sendText(QString t);

signals:
    void contentChanged();
    void focusOut();
    void pressAltEnter();
    void pressCtrlEnter();
    void pressEnter();
    void pressMetaEnter();
    void signal_sentText(QString t);

protected:
    virtual void focusOutEvent(QFocusEvent *e);
    virtual void keyPressEvent(QKeyEvent *e);

private:
    QTimer *m_autoSaveTimer;
    int m_autoSaveInternal;
};

#endif
