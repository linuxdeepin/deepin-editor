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

#include "startmanager.h"
#include <DWidgetUtil>
#include <QScreen>
#include <DApplication>
#include <QDebug>

DWIDGET_USE_NAMESPACE

StartManager::StartManager(QObject *parent) : QObject(parent)
{
}

void StartManager::openFilesInWindow(QStringList files)
{
    if (files.size() == 0) {
        Window *window = createWindow();

        window->addBlankTab();

        window->activateWindow();
    } else {
        foreach (QString file, files) {
            QList<int> fileIndexes = fileIsOpened(file);
            if (fileIndexes.size() > 0) {
                popupExitTab(fileIndexes);
            } else {
                Window *window = createWindow();

                window->addTab(file);
            }
        }
    }
}

void StartManager::openFilesInTab(QStringList files)
{
    if (files.size() == 0) {
        if (windows.size() == 0) {
            Window *window = createWindow(true);

            QDir directory = QDir(QDir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first()).filePath("blank-files"));
            QStringList blankFiles = directory.entryList(QStringList(), QDir::Files);
            
            if (blankFiles.size() > 0) {
                foreach(QString blankFile, blankFiles) {
                    window->addBlankTab(QDir(directory).filePath(blankFile));
                }
            } else {
                window->addBlankTab();
            }
        } else {
            windows[0]->activateWindow();
        }
    } else {
        foreach (QString file, files) {
            QList<int> fileIndexes = fileIsOpened(file);
            if (fileIndexes.size() > 0) {
                popupExitTab(fileIndexes);
            } else {
                if (windows.size() == 0) {
                    Window *window = createWindow(true);
                    
                    window->addTab(file);
                } else {
                    windows[0]->addTab(file);
                }
            }
        }
    }
}

QList<int> StartManager::fileIsOpened(QString file)
{
    QList<int> fileIndexes;
    foreach (Window *window, windows) {
        int tabIndex = window->isFileInTabs(file);
        if (tabIndex >= 0) {
            fileIndexes << windows.indexOf(window) << tabIndex;
            return fileIndexes;
        }
    }

    return fileIndexes;
}

void StartManager::initWindowPosition(Window *window, bool alwaysCenter)
{
    if (windows.size() == 0 || alwaysCenter) {
        Dtk::Widget::moveToCenter(window);
    } else {
        int windowOffset = 50;
        window->move(windows.size() * windowOffset, windows.size() * windowOffset);
    }
}

Window* StartManager::createWindow(bool alwaysCenter)
{
    Window *window = new Window();

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    window->setMinimumSize(QSize(screenGeometry.width() * 2 / 3, screenGeometry.height() * 2 / 3));
    window->show();
    
    initWindowPosition(window, alwaysCenter);

    windows << window;
    
    connect(window, &Window::popTab, this, &StartManager::handlePopTab, Qt::QueuedConnection);
    
    return window;
}

void StartManager::popupExitTab(QList<int> fileIndexes)
{
    int windowIndex = fileIndexes[0];
    int tabIndex = fileIndexes[1];

    windows[windowIndex]->activeTab(tabIndex);
}

void StartManager::handlePopTab(QString tabName, QString filepath, QString content)
{
    Window *window = createWindow();

    window->addTabWithContent(tabName, filepath, content);
}
