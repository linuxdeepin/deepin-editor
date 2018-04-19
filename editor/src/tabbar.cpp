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

#include "tabbar.h"
#include "utils.h"

#include <QDebug>
#include <QLabel>
#include <DHiDPIHelper>

Tabbar::Tabbar()
{
    // Init.
    layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    QPixmap iconPixmap = DHiDPIHelper::loadNxPixmap(Utils::getQrcPath("logo_24.svg"));
    QLabel *iconLabel = new QLabel();
    iconLabel->setPixmap(iconPixmap);
    iconLabel->setFixedSize(24, 40);

    tabbar = new TabWidget();

    layout->addSpacing(10);
    layout->addWidget(iconLabel, 0, Qt::AlignTop);
    layout->addSpacing(10);
    layout->addWidget(tabbar, 0, Qt::AlignTop);
    layout->addSpacing(70);

    connect(tabbar, &TabWidget::closeOtherTabs, this, &Tabbar::handleCloseOtherTabs, Qt::QueuedConnection);
    connect(tabbar, &TabWidget::closeTab, this, &Tabbar::closeTabWithIndex, Qt::QueuedConnection);
    connect(tabbar, &TabWidget::currentChanged, this, &Tabbar::handleCurrentIndexChanged, Qt::QueuedConnection);
    connect(tabbar, &TabWidget::tabAddRequested, this, &Tabbar::tabAddRequested, Qt::QueuedConnection);
    connect(tabbar, &TabWidget::tabBarDoubleClicked, this, &Tabbar::doubleClicked, Qt::QueuedConnection);
    connect(tabbar, &TabWidget::tabCloseRequested, this, &Tabbar::tabCloseRequested, Qt::QueuedConnection);
    connect(tabbar, &TabWidget::tabDroped, this, &Tabbar::handleTabDroped, Qt::QueuedConnection);
    connect(tabbar, &TabWidget::tabMoved, this, &Tabbar::handleTabMoved, Qt::QueuedConnection);
    connect(tabbar, &TabWidget::tabReleaseRequested, this, &Tabbar::handleTabReleaseRequested, Qt::QueuedConnection);
}

int Tabbar::getTabIndex(QString filepath)
{
    for (int i = 0; i < tabbar->tabFiles.size(); i++) {
        if (tabbar->tabFiles[i] == filepath) {
            return i;
        }
    }

    return -1;
}

QString Tabbar::getTabName(int index)
{
    return tabbar->tabText(index);
}

QString Tabbar::getTabPath(int index)
{
    return tabbar->tabFiles.value(index);
}

int Tabbar::getActiveTabIndex()
{
    return tabbar->currentIndex();
}

QString Tabbar::getActiveTabName()
{
    return tabbar->tabText(getActiveTabIndex());
}

QString Tabbar::getActiveTabPath()
{
    return tabbar->tabFiles.value(getActiveTabIndex());
}

void Tabbar::activeTabWithIndex(int index)
{
    tabbar->setCurrentIndex(index);
}

void Tabbar::addTab(QString filepath, QString tabName)
{
    int index = getActiveTabIndex();

    tabbar->tabFiles.insert(index + 1, filepath);
    tabbar->insertTab(index + 1, tabName);
    tabbar->setCurrentIndex(index + 1);
}

void Tabbar::addTabWithIndex(int index, QString filepath, QString tabName)
{
    if (index == -1) {
        index = getActiveTabIndex();
    }
    
    tabbar->tabFiles.insert(index + 1, filepath);
    tabbar->insertTab(index + 1, tabName);
    tabbar->setCurrentIndex(index + 1);
}

void Tabbar::closeActiveTab()
{
    closeTabWithIndex(tabbar->currentIndex());
}

void Tabbar::closeOtherTabs()
{
    closeOtherTabsExceptFile(tabbar->tabFiles[tabbar->currentIndex()]);
}

void Tabbar::closeOtherTabsExceptFile(QString filepath)
{
    while (tabbar->tabFiles.size() > 1) {
        QString firstPath = tabbar->tabFiles[0];
        if (firstPath != filepath) {
            closeTabWithIndex(0);
        } else {
            closeTabWithIndex(1);
        }
    }
}

void Tabbar::selectNextTab()
{
    int currentIndex = tabbar->currentIndex();
    if (currentIndex >= tabbar->count() - 1) {
        tabbar->setCurrentIndex(0);
    } else {
        tabbar->setCurrentIndex(currentIndex + 1);
    }
}

void Tabbar::selectPrevTab()
{
    int currentIndex = tabbar->currentIndex();
    if (currentIndex <= 0) {
        tabbar->setCurrentIndex(tabbar->count() - 1);
    } else {
        tabbar->setCurrentIndex(currentIndex - 1);
    }
}

void Tabbar::updateTabWithIndex(int index, QString filepath, QString tabName)
{
    tabbar->setTabText(index, tabName);
    tabbar->tabFiles[index] = filepath;
}

void Tabbar::closeTabWithIndex(int closeIndex)
{
    QString filepath = tabbar->tabFiles[closeIndex];

    tabbar->tabFiles.takeAt(closeIndex);
    tabbar->removeTab(closeIndex);

    closeFile(filepath);
}

void Tabbar::handleCloseOtherTabs(int index)
{
    closeOtherTabsExceptFile(tabbar->tabFiles[index]);
}

void Tabbar::handleCurrentIndexChanged(int index)
{
    switchToFile(tabbar->tabFiles.value(index));
}

void Tabbar::handleTabDroped(int index, Qt::DropAction, QObject *target)
{
    // Remove match tab if tab drop to tabbar of deepin-editor.
    auto *tabWidget = qobject_cast<TabWidget*>(target);
    if (tabWidget != nullptr) {
        closeTabWithIndex(index);
    }
}

void Tabbar::handleTabMoved(int fromIndex, int toIndex)
{
    tabbar->tabFiles.swap(fromIndex, toIndex);
}

void Tabbar::handleTabReleaseRequested(int index)
{
    // Just send tabReleaseRequested signal have two or above tabs.
    if (tabbar->count() > 1) {
        tabReleaseRequested(getTabName(index), getTabPath(index), index);
    }
}

int Tabbar::getTabCount()
{
    return tabbar->count();
}
