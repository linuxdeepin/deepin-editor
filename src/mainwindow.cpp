/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2017 Deepin, Inc.
 *               2011 ~ 2017 Wang Yong
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

#include "mainwindow.h"
#include <DTitlebar>
#include <QLabel>
#include <QDebug>
#include <QSvgWidget>
#include "dthememanager.h"
#include "utils.h"

MainWindow::MainWindow(DMainWindow *parent) : DMainWindow(parent)
{
    DThemeManager::instance()->setTheme("dark");

    installEventFilter(this);   // add event filter

    layoutWidget = new QWidget();
    layout = new QVBoxLayout(layoutWidget);
    layout->setContentsMargins(0, 0, 0, 0);

    this->setCentralWidget(layoutWidget);

    editor = new Editor();
    layout->addWidget(editor);

    QLabel *label = new QLabel();
    layout->addWidget(label, 0, Qt::AlignBottom);

    tabbarWidget = new QWidget();
    tabbarLayout = new QHBoxLayout(tabbarWidget);
    tabbarLayout->setContentsMargins(0, 0, 0, 0);

    QPixmap iconPixmap = QPixmap(Utils::getQrcPath("logo_24.svg"));
    QLabel *iconLabel = new QLabel();
    iconLabel->setPixmap(iconPixmap);
    iconLabel->setFixedSize(24, 40);

    tabbar = new DTabBar();
    tabbar->setMovable(true);
    tabbar->setTabsClosable(true);
    tabbar->setVisibleAddButton(true);

    tabbarLayout->addSpacing(10);
    tabbarLayout->addWidget(iconLabel, 0, Qt::AlignTop);
    tabbarLayout->addSpacing(10);
    tabbarLayout->addWidget(tabbar, 0, Qt::AlignTop);
    tabbarLayout->addSpacing(40);

    this->titlebar()->setCustomWidget(tabbarWidget, Qt::AlignVCenter, false);
    this->titlebar()->setSeparatorVisible(true);

    tabbar->addTab("Bob Dylan");
    tabbar->addTab("Neil Young");
    tabbar->addTab("Passanger");
}

MainWindow::~MainWindow()
{
    // We don't need clean pointers because application has exit here.
}

void MainWindow::keyPressEvent(QKeyEvent *keyEvent)
{
    if (keyEvent->modifiers() == Qt::ControlModifier) {
        if (keyEvent->key() == Qt::Key_F) {
            tabbar->addTab("Bob Dylan");
        } else if (keyEvent->key() == Qt::Key_N) {
            tabbar->addTab("Neil Young");
        } else if (keyEvent->key() == Qt::Key_P) {
            tabbar->addTab("Passanger");
        }
    }

    if (keyEvent->modifiers() && Qt::ControlModifier) {
        if (keyEvent->key() == Qt::Key_Backtab) {
            // tabbar->selectPrevTab();
        } else if (keyEvent->key() == Qt::Key_Tab) {
            // tabbar->selectNextTab();
        }
    }

    if ((keyEvent->modifiers() && Qt::ControlModifier) && (keyEvent->modifiers() && Qt::MetaModifier)) {
        if (keyEvent->key() == Qt::Key_Home) {
            // tabbar->selectFirstTab();
        } else if (keyEvent->key() == Qt::Key_End) {
            // tabbar->selectLastTab();
        }
    }
}

