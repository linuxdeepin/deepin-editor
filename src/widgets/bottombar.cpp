/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     rekols <rekols@foxmail.com>
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

#include "bottombar.h"
#include <QLabel>
#include <QPainter>
#include <QHBoxLayout>

BottomBar::BottomBar(QWidget *parent)
    : QWidget(parent),
      m_positionLabel(new QLabel),
      m_rowStr(tr("Row")),
      m_columnStr(tr("Column"))
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 1, 5, 0);
    layout->addWidget(m_positionLabel);

    m_positionLabel->setText(QString("Row %1 , Column %1").arg(0));

    setFixedHeight(30);
}

BottomBar::~BottomBar()
{
}

void BottomBar::updatePosition(int row, int column)
{
    m_positionLabel->setText(QString("%1 %2 , %3 %4").arg(m_rowStr, QString::number(row),
                                                         m_columnStr, QString::number(column)));
}

void BottomBar::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    painter.setOpacity(1);

    QColor backgroundColor = palette().color(QPalette::Background);
    QPainterPath path;
    path.addRect(rect());
    painter.fillPath(path, backgroundColor);

    QColor splitLineColor;
    if (backgroundColor.lightness() < 128) {
        splitLineColor = QColor("#ffffff");
    } else {
        splitLineColor = QColor("#000000");
    }

    QPainterPath framePath;
    framePath.addRect(QRect(rect().x(), rect().y(), rect().width(), 1));
    painter.setOpacity(0.1);
    painter.fillPath(framePath, splitLineColor);
}
