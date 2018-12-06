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
#include "../utils.h"
#include "../editwrapper.h"
#include <QLabel>
#include <QPainter>
#include <QHBoxLayout>
#include <QMenu>

BottomBar::BottomBar(QWidget *parent)
    : QWidget(parent),
      m_wrapper(static_cast<EditWrapper *>(parent)),
      m_positionLabel(new QLabel),
      m_encodeMenu(new DDropdownMenu),
      m_rowStr(tr("Row")),
      m_columnStr(tr("Column"))
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 1, 5, 0);
    layout->addWidget(m_positionLabel);

    m_positionLabel->setText(QString("%1 %2 , %3 %4").arg(m_rowStr, "0",
                                                          m_columnStr, "0"));
    m_encodeMenu->addActions(Utils::getEncodeList());
    m_encodeMenu->setCurrentText("UTF-8");

    layout->addStretch();
    layout->addWidget(m_encodeMenu);

    setFixedHeight(30);

    connect(m_encodeMenu, &DDropdownMenu::currentTextChanged, this, &BottomBar::handleEncodeChanged);
}

BottomBar::~BottomBar()
{
}

void BottomBar::updatePosition(int row, int column)
{
    m_positionLabel->setText(QString("%1 %2 , %3 %4").arg(m_rowStr, QString::number(row),
                                                          m_columnStr, QString::number(column)));
}

void BottomBar::setEncodeName(const QString &name)
{
    m_encodeMenu->setCurrentText(name);
}

void BottomBar::setPalette(const QPalette &palette)
{
    m_positionLabel->setStyleSheet(QString("QLabel { color: %1; }").
                                   arg(palette.color(QPalette::Text).name()));

    if (palette.color(QPalette::Background).lightness() < 128) {
        m_encodeMenu->setTheme("dark");
    } else {
        m_encodeMenu->setTheme("light");
    }

    QWidget::setPalette(palette);
}

void BottomBar::handleEncodeChanged(const QString &name)
{
    m_wrapper->setTextCodec(QTextCodec::codecForName(name.toUtf8()));
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
