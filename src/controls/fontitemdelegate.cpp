// SPDX-FileCopyrightText: 2017 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fontitemdelegate.h"
#include <QPainter>
#include <QDebug>

FontItemDelegate::FontItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
    qDebug() << "FontItemDelegate constructor";
}

FontItemDelegate::~FontItemDelegate()
{
    qDebug() << "FontItemDelegate destructor";
}

void FontItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // qDebug() << "FontItemDelegate paint";
    const QString text = index.data(Qt::DisplayRole).toString();
    bool isSelected = option.state & QStyle::State_Selected;
    // qDebug() << "isSelected" << isSelected;
    painter->setPen(Qt::black);
    // qDebug() << "setPen";
    if (isSelected) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor("#2CA7F8"));
        painter->drawRect(option.rect);
        painter->setPen(Qt::white);
        // qDebug() << "setPen white";
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
    // qDebug() << "FontItemDelegate sizeHint";
    return QSize(-1, 30);
}
