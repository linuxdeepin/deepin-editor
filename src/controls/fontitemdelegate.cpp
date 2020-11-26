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

#include "fontitemdelegate.h"
#include <QPainter>

FontItemDelegate::FontItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{

}

FontItemDelegate::~FontItemDelegate()
{
}

void FontItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const QString text = index.data(Qt::DisplayRole).toString();
    bool isSelected = option.state & QStyle::State_Selected;

    painter->setPen(Qt::black);

    if (isSelected) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor("#2CA7F8"));
        painter->drawRect(option.rect);
        painter->setPen(Qt::white);
    }

    QFont font(painter->font());
    font.setFamily(text);
    painter->setFont(font);

    QRect textRect = option.rect;
    painter->drawText(QRect(textRect.x() + 10,
                            textRect.y(),
                            textRect.width() - 10,
                            textRect.height()), Qt::AlignVCenter | Qt::AlignLeft, text);
}

QSize FontItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return QSize(-1, 30);
}
