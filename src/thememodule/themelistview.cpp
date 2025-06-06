// SPDX-FileCopyrightText: 2017 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "themelistview.h"
#include "themelistmodel.h"
#include <QScrollBar>
#include <QApplication>
#include <QEvent>

ThemeListView::ThemeListView(QWidget *parent)
    : QListView(parent)
{
    qDebug() << "ThemeListView constructor";
    installEventFilter(this);

    setVerticalScrollMode(ScrollPerPixel);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, &ThemeListView::adjustScrollbarMargins);
}

ThemeListView::~ThemeListView()
{
    qDebug() << "ThemeListView destructor";
}

void ThemeListView::adjustScrollbarMargins()
{
    if (!isVisible()) {
        qDebug() << "View not visible, skipping scrollbar adjustment";
        return;
    }

    qInfo() << "Adjusting scrollbar margins";
    QEvent event(QEvent::LayoutRequest);
    QApplication::sendEvent(this, &event);

    if (!verticalScrollBar()->visibleRegion().isEmpty()) {
        qDebug() << "Setting viewport margins with scrollbar";
        setViewportMargins(0, 0,5, 0);
    } else {
        qDebug() << "Setting viewport margins without scrollbar";
        setViewportMargins(0, 0, 5, 0);
    }
}

bool ThemeListView::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::FocusOut) {
        qDebug() << "Focus out event detected";
        emit focusOut();
    }

    return false;
}

void ThemeListView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    qDebug() << "Selection changed";
    QListView::selectionChanged(selected, deselected);

    QModelIndexList list = selectionModel()->selectedIndexes();
    for (const QModelIndex &index : list) {
        const QString &themePath = index.data(ThemeListModel::ThemePath).toString();
        qDebug() << "Selected theme path:" << themePath;

        emit themeChanged(themePath);
    }
}
