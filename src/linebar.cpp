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

#include "linebar.h"
#include "utils.h"

#include <QDebug>

LineBar::LineBar(DLineEdit *parent)
    : DLineEdit(parent)
{
    // Init.
    setClearButtonEnabled(true);

    m_autoSaveInternal = 50;
    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setSingleShot(true);

    connect(m_autoSaveTimer, &QTimer::timeout, this, &LineBar::handleTextChangeTimer);
    connect(this, &DLineEdit::textEdited, this, &LineBar::sendText, Qt::QueuedConnection);
    connect(this, &DLineEdit::textChanged, this, &LineBar::handleTextChanged, Qt::QueuedConnection);
}

void LineBar::handleTextChangeTimer()
{
    // Emit contentChanged signal.
    contentChanged();
}

void LineBar::handleTextChanged()
{
    // Stop timer if new character is typed, avoid unused timer run.
    if (m_autoSaveTimer->isActive()) {
        m_autoSaveTimer->stop();
    }

    // Start new timer.
    m_autoSaveTimer->start(m_autoSaveInternal);
}

void LineBar::sendText(QString t)
{
    qDebug()<<"ppppppttttt"<<t;
    emit signal_sentText(t);
}

void LineBar::focusOutEvent(QFocusEvent *e)
{
    // Emit focus out signal.
    focusOut();

    // Throw event out avoid DLineEdit can't hide cursor after lost focus.
    DLineEdit::focusOutEvent(e);
}

void LineBar::keyPressEvent(QKeyEvent *e)
{
    QString key = Utils::getKeyshortcut(e);
    Qt::KeyboardModifiers modifiers = e->modifiers();

     if (key == "Esc") {
        pressEsc();
     }else if(modifiers == Qt::ControlModifier && e->text() == "\r"){
        pressCtrlEnter();
     }else if(modifiers == Qt::AltModifier && e->text() == "\r"){
        pressAltEnter();
     }else if(modifiers == Qt::MetaModifier && e->text() == "\r"){
        pressMetaEnter();
     }else if(modifiers == Qt::NoModifier && e->text() == "\r"){
        pressEnter();
     }else {
       // Pass event to DLineEdit continue, otherwise you can't type anything after here. ;)
        DLineEdit::keyPressEvent(e);
     }
}
