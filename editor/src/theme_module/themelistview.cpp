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

#include "themelistview.h"
#include "themelistmodel.h"
#include <QScrollBar>
#include <QApplication>
#include <QEvent>

ThemeListView::ThemeListView(QWidget *parent)
    : QListView(parent)
{
    installEventFilter(this);

    setVerticalScrollMode(ScrollPerPixel);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, &ThemeListView::adjustScrollbarMargins);
}

ThemeListView::~ThemeListView()
{
}

void ThemeListView::adjustScrollbarMargins()
{
    QEvent event(QEvent::LayoutRequest);
    QApplication::sendEvent(this, &event);

    if (!verticalScrollBar()->visibleRegion().isEmpty()) {
        setViewportMargins(0, 0, -verticalScrollBar()->sizeHint().width(), 0);
    } else {
        setViewportMargins(0, 0, 0, 0);
    }
}

bool ThemeListView::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::FocusOut) {
        emit focusOut();
    }

    return false;
}

void ThemeListView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    QListView::selectionChanged(selected, deselected);

    QModelIndexList list = selectionModel()->selectedIndexes();
    for (const QModelIndex &index : list) {
        const QString &themeName = index.data(ThemeListModel::ThemeName).toString();
        const QString &themePath = index.data(ThemeListModel::ThemePath).toString();

        emit themeChanged(themePath);
    }
}
