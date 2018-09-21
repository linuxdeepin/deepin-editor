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

#ifndef TITLEBAR_H
#define TITLEBAR_H

#include "tabwidget.h"

#include <QHBoxLayout>
#include <QWidget>

DWIDGET_USE_NAMESPACE

class Tabbar : public QWidget
{
    Q_OBJECT

public:
    Tabbar(QWidget *parent = nullptr);

    int getTabIndex(const QString &filepath);
    QString getTabName(int index);
    QString getTabPath(int index);

    int getActiveTabIndex();
    QString getActiveTabName();
    QString getActiveTabPath();
    void activeTabWithIndex(int index);

    void addTab(const QString &filepath, const QString &tabName);
    void addTabWithIndex(int index, const QString &filepath, const QString &tabName);
    void closeActiveTab();
    void closeOtherTabs();
    void closeOtherTabsExceptFile(const QString &filepath);

    void selectNextTab();
    void selectPrevTab();

    void updateTabWithIndex(int index, const QString &filepath, const QString &tabName);
    void setTabActiveColor(const QString &color);

    int getTabCount();

    TabWidget *tabbar;

signals:
    void doubleClicked();
    void tabReleaseRequested(const QString &tabName, const QString &filepaht, int index);
    void requestHistorySaved(const QString &filePath, int index);

public slots:
    void closeTabWithIndex(int closeIndex);

    void handleCloseOtherTabs(int index);

    void handleTabDroped(int index, Qt::DropAction action, QObject *target);
    void handleTabMoved(int fromIndex, int toIndex);
    void handleTabReleaseRequested(int index);

private:
    QHBoxLayout *m_layout;
};

#endif
