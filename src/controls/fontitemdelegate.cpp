// SPDX-FileCopyrightText: 2017-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddlog.h"
#include "fontitemdelegate.h"
#include <QPainter>
#include <QDebug>

FontItemDelegate::FontItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
    qCDebug(appLog) << "FontItemDelegate constructor";
}

FontItemDelegate::~FontItemDelegate()
{
    qCDebug(appLog) << "FontItemDelegate destructor";
}

void FontItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // qCDebug(appLog) << "FontItemDelegate paint";
    const QString text = index.data(Qt::DisplayRole).toString();
    bool isSelected = option.state & QStyle::State_Selected;
    // qCDebug(appLog) << "isSelected" << isSelected;
    painter->setPen(Qt::black);
    // qCDebug(appLog) << "setPen";
    if (isSelected) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor("#2CA7F8"));
        painter->drawRect(option.rect);
        painter->setPen(Qt::white);
        // qCDebug(appLog) << "setPen white";
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
    // qCDebug(appLog) << "FontItemDelegate sizeHint";
    return QSize(-1, 30);
}
