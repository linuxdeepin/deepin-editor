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

#include "dthememanager.h"
#include "jumplinebar.h"

#include <QDebug>

JumpLineBar::JumpLineBar(QWidget *parent) : QWidget(parent)
{
    // Init.
    setWindowFlags(Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint);
    setFixedSize(200, 40);

    // Init layout and widgets.
    m_layout = new QHBoxLayout(this);

    m_label = new QLabel();
    m_label->setText(tr("Go to line: "));
    m_editLine = new LineBar();

    m_lineValidator = new QIntValidator;
    m_editLine->setValidator(m_lineValidator);

    m_layout->addWidget(m_label);
    m_layout->addWidget(m_editLine);

    connect(m_editLine, &LineBar::pressEsc, this, &JumpLineBar::jumpCancel, Qt::QueuedConnection);
    connect(m_editLine, &LineBar::pressEnter, this, &JumpLineBar::jumpConfirm, Qt::QueuedConnection);
    connect(m_editLine, &LineBar::textChanged, this, &JumpLineBar::handleLineChanged, Qt::QueuedConnection);
    connect(m_editLine, &LineBar::focusOut, this, &JumpLineBar::handleFocusOut, Qt::QueuedConnection);
}

void JumpLineBar::focus()
{
    m_editLine->setFocus();
}

bool JumpLineBar::isFocus()
{
    return m_editLine->hasFocus();
}

void JumpLineBar::activeInput(QString file, int row, int column, int lineCount, int scrollOffset)
{
    // Save file info for back to line.
    m_jumpFile = file;
    m_rowBeforeJump = row;
    m_columnBeforeJump = column;
    m_jumpFileScrollOffset = scrollOffset;
    m_lineValidator->setRange(1, lineCount);

    // Clear line number.
    m_editLine->setText("");

    // Show jump line bar.
    show();
    raise();

    // Focus default.
    m_editLine->setFocus();
}

void JumpLineBar::handleFocusOut()
{
    hide();

    lostFocusExit();
}

void JumpLineBar::handleLineChanged()
{
    QString content = m_editLine->text();
    if (content != "") {
        jumpToLine(m_jumpFile, content.toInt(), false);
    }
}

void JumpLineBar::jumpCancel()
{
    hide();

    backToPosition(m_jumpFile, m_rowBeforeJump, m_columnBeforeJump, m_jumpFileScrollOffset);
}

void JumpLineBar::jumpConfirm()
{
    hide();

    QString content = m_editLine->text();
    if (content != "") {
        jumpToLine(m_jumpFile, content.toInt(), true);
    }
}

void JumpLineBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setOpacity(0.9);
    QPainterPath path;
    int radius = 5;
    path.moveTo(QPointF(rect().x(), rect().y()));
    path.lineTo(QPointF(rect().x(), rect().y() + rect().height() - radius));
    path.arcTo(QRectF(rect().x(), rect().bottom() - radius * 2, radius * 2, radius * 2), 180, 90);
    path.lineTo(QPointF(rect().x() + rect().width(), rect().y() + rect().height()));
    path.lineTo(QPointF(rect().x() + rect().width(), rect().y()));
    painter.fillPath(path, m_backgroundColor);
}

void JumpLineBar::setBackground(QString color)
{
    m_backgroundColor = QColor(color);

    if (QColor(m_backgroundColor).lightness() < 128) {
        m_label->setStyleSheet(QString("QLabel { background-color: %1; color: %2; }").arg(color).arg("#AAAAAA"));
    } else {
        m_label->setStyleSheet(QString("QLabel { background-color: %1; color: %2; }").arg(color).arg("#000000"));
    }

    repaint();
}
