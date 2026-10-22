// SPDX-FileCopyrightText: 2017 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddlog.h"
#include "themelistview.h"
#include "themelistmodel.h"
#include <QScrollBar>
#include <QApplication>
#include <QEvent>
#include <QDebug>

ThemeListView::ThemeListView(QWidget *parent)
    : QListView(parent)
{
    qCDebug(appLog) << "ThemeListView constructor";
    installEventFilter(this);

    setVerticalScrollMode(ScrollPerPixel);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, &ThemeListView::adjustScrollbarMargins);
}

ThemeListView::~ThemeListView()
{
    qCDebug(appLog) << "ThemeListView destructor";
}

void ThemeListView::adjustScrollbarMargins()
{
    if (!isVisible()) {
        qCDebug(appLog) << "View not visible, skipping scrollbar adjustment";
        return;
    }

    qCInfo(appLog) << "Adjusting scrollbar margins";
    QEvent event(QEvent::LayoutRequest);
    QApplication::sendEvent(this, &event);

    // 原先 if/else 两分支 margins 完全相同，条件判断无实际效果，已简化；
    // 预留右侧 5px 给竖向滚动条
    setViewportMargins(0, 0, 5, 0);
}

bool ThemeListView::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::FocusOut) {
        qCDebug(appLog) << "Focus out event detected";
        emit focusOut();
    }

    return false;
}

void ThemeListView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    qCDebug(appLog) << "Selection changed";
    QListView::selectionChanged(selected, deselected);

    QModelIndexList list = selectionModel()->selectedIndexes();
    for (const QModelIndex &index : list) {
        const QString &themePath = index.data(ThemeListModel::ThemePath).toString();
        qCDebug(appLog) << "Selected theme path:" << themePath;

        emit themeChanged(themePath);
    }
}
