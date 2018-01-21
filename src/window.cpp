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

#include "window.h"
#include <DTitlebar>
#include <QLabel>
#include <QDebug>
#include <QFileDialog>
#include <QDir>
#include <QDateTime>
#include <QApplication>
#include <QSvgWidget>
#include "dthememanager.h"
#include "utils.h"

Window::Window(DMainWindow *parent) : DMainWindow(parent)
{
    DThemeManager::instance()->setTheme("dark");

    installEventFilter(this);   // add event filter

    layoutWidget = new QWidget();
    layout = new QStackedLayout(layoutWidget);
    layout->setContentsMargins(0, 0, 0, 0);

    this->setCentralWidget(layoutWidget);

    tabbar = new Tabbar();

    this->titlebar()->setCustomWidget(tabbar, Qt::AlignVCenter, false);
    this->titlebar()->setSeparatorVisible(true);

    connect(tabbar, SIGNAL(doubleClicked()), this->titlebar(), SIGNAL(doubleClicked()), Qt::QueuedConnection);
    connect(tabbar, SIGNAL(switchToFile(QString)), this, SLOT(handleSwitchToFile(QString)), Qt::QueuedConnection);
    connect(tabbar, SIGNAL(closeFile(QString)), this, SLOT(handleCloseFile(QString)), Qt::QueuedConnection);

    Utils::applyQss(this, "main.qss");
}

Window::~Window()
{
    // We don't need clean pointers because application has exit here.
}

void Window::keyPressEvent(QKeyEvent *keyEvent)
{
    if (keyEvent->modifiers() & Qt::ControlModifier) {
        if (keyEvent->key() == Qt::Key_T) {
            addBlankTab();
        } else if (keyEvent->key() == Qt::Key_Tab) {
            tabbar->selectNextTab();
        } else if (keyEvent->key() == Qt::Key_Backtab) {
            tabbar->selectPrevTab();
        } else if (keyEvent->key() == Qt::Key_W) {
            tabbar->closeTab();
        } else if (keyEvent->key() == Qt::Key_O) {
            openFile();
        }
    }
}

int Window::isFileInTabs(QString file)
{
    return tabbar->isTabExist(file);
}

void Window::addTab(QString file)
{
    if (tabbar->isTabExist(file) == -1) {
        tabbar->addTab(file, QFileInfo(file).fileName());

        if (!editorMap.contains(file)) {
            Editor *editor = new Editor();
            editor->loadFile(file);

            editorMap[file] = editor;

            layout->addWidget(editor);
            layout->setCurrentWidget(editor);
        }
    }

    activateWindow();
}

void Window::addBlankTab()
{
    QString blankTabPath = QString("Blank Tab: %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")) ;

    tabbar->addTab(blankTabPath, "Blank document");
    Editor *editor = new Editor();

    editorMap[blankTabPath] = editor;

    layout->addWidget(editor);
    layout->setCurrentWidget(editor);
}

void Window::handleSwitchToFile(QString filepath)
{
    if (editorMap.contains(filepath)) {
        layout->setCurrentWidget(editorMap[filepath]);
    }
}

void Window::handleCloseFile(QString filepath)
{
    if (editorMap.contains(filepath)) {
        Editor *editor = editorMap[filepath];

        layout->removeWidget(editor);
        editorMap.remove(filepath);

        editor->deleteLater();
    }

    // Exit window after close all tabs.
    if (editorMap.count() == 0) {
        deleteLater();
    }
}

void Window::activeTab(int index)
{
    activateWindow();
    tabbar->activeTab(index);
}

void Window::openFile()
{
    QFileDialog dialog(0, QString(), QDir::homePath());
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    QStringList fileNames;
    if (dialog.exec()) {
        foreach (QString file, dialog.selectedFiles()) {
            addTab(file);
        }
    }
}
