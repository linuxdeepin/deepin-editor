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

#include "findbar.h"
#include "utils.h"

#include <QDebug>

FindBar::FindBar(QWidget *parent)
    : DFloatingWidget(parent)
{
    // Init.
    setFixedHeight(68);

    // Init layout and widgets.
    m_layout = new QHBoxLayout();
    m_findLabel = new QLabel(tr("Find"));
    m_editLine = new LineBar();
    m_findNextButton = new QPushButton(tr("Next"));
    m_findPrevButton = new QPushButton(tr("Previous"));
    m_closeButton = new DIconButton(DStyle::SP_CloseButton);
    m_closeButton->setFixedSize(20, 20);

    m_layout->addWidget(m_findLabel);
    m_layout->addWidget(m_editLine);
    m_layout->addWidget(m_findPrevButton);
    m_layout->addWidget(m_findNextButton);
    m_layout->addWidget(m_closeButton);
    this->setLayout(m_layout);

    // Make button don't grab keyboard focus after click it.
    m_findNextButton->setFocusPolicy(Qt::NoFocus);
    m_findPrevButton->setFocusPolicy(Qt::NoFocus);
    m_closeButton->setFocusPolicy(Qt::NoFocus);

    connect(m_editLine, &LineBar::pressEsc, this, &FindBar::findCancel, Qt::QueuedConnection);
    connect(m_editLine, &LineBar::pressEnter, this, &FindBar::findNext, Qt::QueuedConnection);
    connect(m_editLine, &LineBar::pressCtrlEnter, this, &FindBar::findPrev, Qt::QueuedConnection);
    connect(m_editLine, &LineBar::contentChanged, this, &FindBar::handleContentChanged, Qt::QueuedConnection);

    connect(m_findNextButton, &QPushButton::clicked, this, &FindBar::findNext, Qt::QueuedConnection);
    connect(m_findPrevButton, &QPushButton::clicked, this, &FindBar::findPrev, Qt::QueuedConnection);

    connect(m_closeButton, &DIconButton::clicked, this, &FindBar::findCancel, Qt::QueuedConnection);
}

bool FindBar::isFocus()
{
    return m_editLine->hasFocus();
}

void FindBar::focus()
{
    m_editLine->setFocus();
    m_editLine->selectAll();
}

void FindBar::activeInput(QString text, QString file, int row, int column, int scrollOffset)
{
    // Try fill keyword with select text.
    m_editLine->clear();
    m_editLine->insert(text);
    m_editLine->selectAll();

    // Show.
    QWidget::show();

    // Save file info for back to position.
    m_findFile = file;
    m_findFileRow = row;
    m_findFileColumn = column;
    m_findFileSrollOffset = scrollOffset;

    // Focus.
    focus();
}

void FindBar::findCancel()
{
    QWidget::hide();
    //add by guoshaoyu
    emit sigFindbarClose();
}

void FindBar::handleContentChanged()
{
    updateSearchKeyword(m_findFile, m_editLine->text());
}

void FindBar::hideEvent(QHideEvent *)
{
    removeSearchKeyword();
}

void FindBar::setMismatchAlert(bool isAlert)
{
    m_editLine->setAlert(isAlert);
}
