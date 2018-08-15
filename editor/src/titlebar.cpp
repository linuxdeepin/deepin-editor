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

#include "titlebar.h"
#include "utils.h"

#include <QDebug>
#include <QLabel>
#include <DHiDPIHelper>

Titlebar::Titlebar(QWidget *parent)
    : QWidget(parent)
{
    // Init.
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);

    QPixmap iconPixmap = DHiDPIHelper::loadNxPixmap(Utils::getQrcPath("logo_24.svg"));
    QLabel *iconLabel = new QLabel();
    iconLabel->setPixmap(iconPixmap);
    iconLabel->setFixedSize(24, 40);

    tabbar = new Tabbar;

    m_layout->addSpacing(10);
    m_layout->addWidget(iconLabel, 0, Qt::AlignTop);
    m_layout->addSpacing(10);
    m_layout->addWidget(tabbar, 0, Qt::AlignTop);
    m_layout->addSpacing(70);

    connect(tabbar, &Tabbar::closeOtherTabs, this, &Titlebar::handleCloseOtherTabs, Qt::QueuedConnection);
    connect(tabbar, &Tabbar::closeTab, this, &Titlebar::closeTabWithIndex, Qt::QueuedConnection);
    // connect(tabbar, &DTabBar::tabDroped, this, &Titlebar::handleTabDroped, Qt::QueuedConnection);
    connect(tabbar, &DTabBar::tabBarDoubleClicked, this, &Titlebar::doubleClicked, Qt::QueuedConnection);
    connect(tabbar, &DTabBar::tabMoved, this, &Titlebar::handleTabMoved, Qt::QueuedConnection);
    connect(tabbar, &DTabBar::tabReleaseRequested, this, &Titlebar::handleTabReleaseRequested, Qt::QueuedConnection);
}

int Titlebar::getTabIndex(QString filepath)
{
    for (int i = 0; i < tabbar->tabFiles.size(); i++) {
        if (tabbar->tabFiles[i] == filepath) {
            return i;
        }
    }

    return -1;
}

QString Titlebar::getTabName(int index)
{
    return tabbar->tabText(index);
}

QString Titlebar::getTabPath(int index)
{
    return tabbar->tabFiles.value(index);
}

int Titlebar::getActiveTabIndex()
{
    return tabbar->currentIndex();
}

QString Titlebar::getActiveTabName()
{
    return tabbar->tabText(getActiveTabIndex());
}

QString Titlebar::getActiveTabPath()
{
    return tabbar->tabFiles.value(getActiveTabIndex());
}

void Titlebar::activeTabWithIndex(int index)
{
    tabbar->setCurrentIndex(index);
}

void Titlebar::addTab(QString filepath, QString tabName)
{
    addTabWithIndex(getActiveTabIndex() + 1, filepath, tabName);
}

void Titlebar::addTabWithIndex(int index, QString filepath, QString tabName)
{
    tabbar->tabFiles.insert(index, filepath);
    tabbar->insertTab(index, tabName);
    tabbar->setCurrentIndex(index);
    tabbar->setTabMaximumSize(index, QSize(150, 100));
}

void Titlebar::closeActiveTab()
{
    closeTabWithIndex(tabbar->currentIndex());
}

void Titlebar::closeOtherTabs()
{
    closeOtherTabsExceptFile(tabbar->tabFiles[tabbar->currentIndex()]);
}

void Titlebar::closeOtherTabsExceptFile(QString filepath)
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

void Titlebar::selectNextTab()
{
    int currentIndex = tabbar->currentIndex();
    if (currentIndex >= tabbar->count() - 1) {
        tabbar->setCurrentIndex(0);
    } else {
        tabbar->setCurrentIndex(currentIndex + 1);
    }
}

void Titlebar::selectPrevTab()
{
    int currentIndex = tabbar->currentIndex();
    if (currentIndex <= 0) {
        tabbar->setCurrentIndex(tabbar->count() - 1);
    } else {
        tabbar->setCurrentIndex(currentIndex - 1);
    }
}

void Titlebar::updateTabWithIndex(int index, QString filepath, QString tabName)
{
    tabbar->setTabText(index, tabName);
    tabbar->tabFiles[index] = filepath;
}

void Titlebar::closeTabWithIndex(int closeIndex)
{
    qDebug() << "*** closeTabWithIndex " << closeIndex;
    tabbar->removeTab(closeIndex);
    tabbar->tabFiles.removeAt(closeIndex);

    qDebug() << "-----------------";
    for (int i = 0; i < tabbar->tabFiles.size(); i++) {
        qDebug() << "!!!! tabFiles " << i << tabbar->tabFiles[i];
    }
}

void Titlebar::handleCloseOtherTabs(int index)
{
    closeOtherTabsExceptFile(tabbar->tabFiles[index]);
}

void Titlebar::handleTabDroped(int index, Qt::DropAction, QObject *target)
{
    // Remove match tab if tab drop to tabbar of deepin-editor.
    auto *tabWidget = qobject_cast<Tabbar *>(target);
    if (tabWidget != nullptr) {
        closeTabWithIndex(index);
    }
}

void Titlebar::handleTabMoved(int fromIndex, int toIndex)
{
    tabbar->tabFiles.swap(fromIndex, toIndex);
}

void Titlebar::handleTabReleaseRequested(int index)
{
    // Just send tabReleaseRequested signal have two or above tabs.
    if (tabbar->count() > 1) {
        emit tabReleaseRequested(getTabName(index), getTabPath(index), index);
    }
}

void Titlebar::handleTabCloseRequested(int index)
{
    const QString filePath = tabbar->tabFiles.value(index);

    tabbar->tabFiles.takeAt(index);
}

int Titlebar::getTabCount()
{
    return tabbar->count();
}

void Titlebar::setTabActiveColor(QString color)
{
    QPalette pa = tabbar->palette();
    pa.setColor(QPalette::Active, QPalette::Text, QColor(color));
    tabbar->setPalette(pa);
}
