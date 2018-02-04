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

#ifndef TABBAR_H
#define TABBAR_H

#include "tabwidget.h"

#include <QHBoxLayout>
#include <QWidget>

DWIDGET_USE_NAMESPACE

class Tabbar : public QWidget
{
    Q_OBJECT
    
public:
    Tabbar();
    
    void addTab(QString filepath, QString tabName);
    int currentIndex();
    int isTabExist(QString filepath);
    void activeTab(int index);
    
    void selectNextTab();
    void selectPrevTab();
    void closeTab();
    void closeOtherTabs();
    void closeOtherTabsExceptFile(QString filepath);
    void closeTabWithIndex(int index);
    
    QString getActiveTabName();
    QString getActiveTabPath();
    
    QString getTabName(int index);
    QString getTabPath(int index);
    
    void updateTab(int index, QString filepath, QString tabName);
                      
signals:
    void doubleClicked();
    void switchToFile(QString filepath);
    void closeFile(QString filepath);
    void tabAddRequested();
    void tabReleaseRequested(QString tabName, QString filepaht, int index);
                          
public slots:
    void handleTabbarDoubleClick();
    void handleCurrentIndexChanged(int index);
    void handleTabMoved(int fromIndex, int toIndex);
    void handleTabClosed(int closeIndex);
    void handleTabReleaseRequested(int index);
    void handleTabDroped(int index, Qt::DropAction action, QObject *target);
    void handleCloseTab(int index);
    void handleCloseOtherTabs(int index);
    
private:
    QHBoxLayout *layout;
    TabWidget *tabbar;
};

#endif
