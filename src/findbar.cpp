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
    : QWidget(parent)
{
    // Init.
    setWindowFlags(Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint);
    setFixedHeight(45);

    // Init layout and widgets.
    m_layout = new QHBoxLayout(this);
    //modify by guoshaoyu
    m_findLabel = new QLabel(tr("Find"));
    m_editLine = new LineBar();
    m_findNextButton = new QPushButton(tr("Next"));
    m_findPrevButton = new QPushButton(tr("Previous"));
    m_closeButton = new DImageButton();
    m_closeButton->setFixedSize(16, 16);

    m_layout->addWidget(m_findLabel);
    m_layout->addWidget(m_editLine);
    //modify by guoshaoyu
    m_layout->addWidget(m_findPrevButton);
    m_layout->addWidget(m_findNextButton);
    m_layout->addWidget(m_closeButton);

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

    connect(m_closeButton, &DImageButton::clicked, this, &FindBar::findCancel, Qt::QueuedConnection);
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
    emit sigFindbarCancel();
}

void FindBar::handleContentChanged()
{
    updateSearchKeyword(m_findFile, m_editLine->text());
}

void FindBar::hideEvent(QHideEvent *)
{
    removeSearchKeyword();
}

void FindBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setOpacity(1);
    QPainterPath path;
    path.addRect(rect());
    painter.fillPath(path, m_backgroundColor);

    //modify by guoshaoyu
//    QColor splitLineColor;
//    if (m_backgroundColor.lightness() < 128) {
//        splitLineColor = QColor("#ffffff");
//    } else {
//        splitLineColor = QColor("#000000");
//    }

//    QPainterPath framePath;
//    framePath.addRect(QRect(rect().x(), rect().y(), rect().width(), 1));
//    painter.setOpacity(0.05);
//    painter.fillPath(framePath, splitLineColor);
}

void FindBar::setMismatchAlert(bool isAlert)
{
    m_editLine->setAlert(isAlert);
}

void FindBar::setBackground(QString color)
{
    m_backgroundColor = QColor(color);

    if (QColor(m_backgroundColor).lightness() < 128) {
        //m_findLabel->setStyleSheet(QString("QLabel { background-color: %1; color: %2; }").arg(color).arg("#AAAAAA"));

        m_closeButton->setNormalPic(Utils::getQrcPath("bar_close_normal_dark.svg"));
        m_closeButton->setHoverPic(Utils::getQrcPath("bar_close_hover_dark.svg"));
        m_closeButton->setPressPic(Utils::getQrcPath("bar_close_press_dark.svg"));
    } else {
        //m_findLabel->setStyleSheet(QString("QLabel { background-color: %1; color: %2; }").arg(color).arg("#000000"));

        m_closeButton->setNormalPic(Utils::getQrcPath("bar_close_normal_light.svg"));
        m_closeButton->setHoverPic(Utils::getQrcPath("bar_close_hover_light.svg"));
        m_closeButton->setPressPic(Utils::getQrcPath("bar_close_press_light.svg"));
    }

    update();
}
