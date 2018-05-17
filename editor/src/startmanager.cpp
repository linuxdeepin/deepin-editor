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

#include <DApplication>
#include <DWidgetUtil>
#include <QDebug>
#include <QScreen>

DWIDGET_USE_NAMESPACE

StartManager::StartManager(QObject *parent) : QObject(parent)
{
    // Create blank directory if it not exist.
    QString blankFileDir = QDir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first()).filePath("blank-files");
    if (!QFileInfo(blankFileDir).exists()) {
        QDir().mkpath(blankFileDir);

        qDebug() << "Create blank file dir: " << blankFileDir;
    }
}

void StartManager::openFilesInWindow(QStringList files)
{
    // Open window with blank tab if no files need open.
    if (files.size() == 0) {
        Window *window = createWindow();

        window->addBlankTab();
        window->activateWindow();
    } else {
        foreach (QString file, files) {
            FileTabInfo info = getFileTabInfo(file);

            // Open exist tab if file has opened.
            if (info.windowIndex != -1) {
                popupExistTabs(info);
            }
            // Add new tab in current window.
            else {
                createWindow()->addTab(file);
            }
        }
    }
}

void StartManager::openFilesInTab(QStringList files)
{
    if (files.size() == 0) {
        if (windows.size() == 0) {
            QDir blankDirectory = QDir(QDir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first()).filePath("blank-files"));
            QStringList blankFiles = blankDirectory.entryList(QStringList(), QDir::Files);

            QDir readonlyDirectory = QDir(QDir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first()).filePath("readonly-files"));
            QStringList readonlyFiles = readonlyDirectory.entryList(QStringList(), QDir::Files);
            
            Window *window = createWindow(true);

            // Open blank files of last session.
            if (blankFiles.size() > 0 || readonlyFiles.size() > 0) {
                foreach(QString blankFile, blankFiles) {
                    window->addBlankTab(QDir(blankDirectory).filePath(blankFile));
                }
                
                foreach(QString readonlyFile, readonlyFiles) {
                    QString readonlyFilePath = QDir(readonlyDirectory).filePath(readonlyFile);
                    QString realpath = QFileInfo(readonlyFilePath).fileName().replace(" !_! ", QDir().separator());
                    
                    window->addTab(realpath);
                }
            }
            // Just open new window with blank tab if no blank files in last session.
            else {
                window->addBlankTab();
            }
        }
        // Just active first window if no file is need opened.
        else {
            windows[0]->activateWindow();
        }
    } else {
        foreach (QString file, files) {
            FileTabInfo info = getFileTabInfo(file);

            // Open exist tab if file has opened.
            if (info.windowIndex != -1) {
                popupExistTabs(info);
            }
            // Create new window with file if haven't window exist.
            else if (windows.size() == 0) {
                createWindow(true)->addTab(file);
            }
            // Open file tab in first window of window list.
            else {
                windows[0]->addTab(file);
            }

        }
    }
}

void StartManager::createWindowFromTab(QString tabName, QString filepath, QString content)
{
    createWindow()->addTabWithContent(tabName, filepath, content);
}

Window* StartManager::createWindow(bool alwaysCenter)
{
    // Create window.
    Window *window = new Window();
    connect(window, &Window::dropTabOut, this, &StartManager::createWindowFromTab, Qt::QueuedConnection);
    
    // Quit application if close last window.
    connect(window, &Window::close, this, 
            [=]() {
                if (windows.size() <= 1) {
                    QApplication::quit();
                }
            });

    // Init window position.
    initWindowPosition(window, alwaysCenter);
    
    connect(window, &Window::newWindow, this, 
            [=] () {
                openFilesInWindow(QStringList());
            });

    // Append window in window list.
    windows << window;

    return window;
}

void StartManager::initWindowPosition(Window *window, bool alwaysCenter)
{
    if (windows.size() == 0 || alwaysCenter) {
        Dtk::Widget::moveToCenter(window);
    } else {
        // Add window offset to avoid all editor window popup at same coordinate.
        int windowOffset = 50;
        window->move(windows.size() * windowOffset, windows.size() * windowOffset);
    }
}

void StartManager::popupExistTabs(FileTabInfo info)
{
    windows[info.windowIndex]->activeTab(info.tabIndex);
}

StartManager::FileTabInfo StartManager::getFileTabInfo(QString file)
{
    FileTabInfo info = {-1, -1};

    foreach (Window *window, windows) {
        int tabIndex = window->getTabIndex(file);
        if (tabIndex >= 0) {
            info.windowIndex = windows.indexOf(window);
            info.tabIndex = tabIndex;
            break;
        }
    }

    return info;
}
