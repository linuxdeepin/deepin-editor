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

#include "linebar.h"
#include "utils.h"

#include <QDebug>

LineBar::LineBar(QLineEdit *parent) : QLineEdit(parent)
{
    // Init.
    autoSaveInternal = 500;

    autoSaveTimer = new QTimer(this);
    autoSaveTimer->setSingleShot(true);
    
    connect(autoSaveTimer, &QTimer::timeout, this, &LineBar::handleTextChangeTimer);
    connect(this, &QLineEdit::textChanged, this, &LineBar::handleTextChanged, Qt::QueuedConnection);
}

void LineBar::handleTextChangeTimer()
{
    // Emit contentChanged signal.
    contentChanged();
}

void LineBar::handleTextChanged()
{
    // Stop timer if new character is typed, avoid unused timer run.
    if (autoSaveTimer->isActive()) {
        autoSaveTimer->stop();
    }
    
    // Start new timer.
    autoSaveTimer->start(autoSaveInternal);
}

void LineBar::focusOutEvent(QFocusEvent *)
{
    // Emit focus out signal.
    focusOut();
}

void LineBar::keyPressEvent(QKeyEvent *e)
{
    QString key = Utils::getKeyshortcut(e);
    
    if (key == "Esc") {
        pressEsc();
    } else if (key == "Return") {
        pressEnter();
    } else if (key == "Ctrl + Return") {
        pressCtrlEnter();
    } else if (key == "Alt + Return") {
        pressAltEnter();
    } else if (key == "Meta + Return") {
        pressMetaEnter();
    } else {
        // Pass event to QLineEdit continue, otherwise you can't type anything after here. ;)
        QLineEdit::keyPressEvent(e);
    }
}
