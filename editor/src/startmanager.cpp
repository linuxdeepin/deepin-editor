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

StartManager::StartManager(QObject *parent)
    : QObject(parent)
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
        for (const QString &file : files) {
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
        if (m_windows.size() == 0) {
            QDir blankDirectory = QDir(QDir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first()).filePath("blank-files"));
            QStringList blankFiles = blankDirectory.entryList(QStringList(), QDir::Files);

            Window *window = createWindow(true);

            // Open blank files of last session.
            if (!blankFiles.isEmpty()) {
                QTimer::singleShot(50, [=] {
                                           for (const QString &blankFile : blankFiles) {
                                               window->addBlankTab(QDir(blankDirectory).filePath(blankFile));
                                           }
                                       });
            }
            // Just open new window with blank tab if no blank files in last session.
            else {
                window->addBlankTab();
            }
        }
        // Just active first window if no file is need opened.
        else {
            m_windows[0]->activateWindow();
        }
    } else {
        for (const QString &file : files) {
            FileTabInfo info = getFileTabInfo(file);

            // Open exist tab if file has opened.
            if (info.windowIndex != -1) {
                popupExistTabs(info);

                qDebug() << "Open " << file << " in exist tab";
            }
            // Create new window with file if haven't window exist.
            else if (m_windows.size() == 0) {
                createWindow(true)->addTab(file);

                qDebug() << "Open " << file << " with new window";
            }
            // Open file tab in first window of window list.
            else {
                m_windows[0]->addTab(file);

                qDebug() << "Open " << file << " in first window";
            }

        }
    }
}

void StartManager::createWindowFromTab(QString tabName, QString filepath, QString content)
{
    Window *window = createWindow();
    window->addTabWithContent(tabName, filepath, content);
    window->move(QCursor::pos() - window->topLevelWidget()->pos());
}

void StartManager::loadTheme(const QString &themeName)
{
    for (Window *window : m_windows) {
        window->loadTheme(themeName);
    }
}

Window* StartManager::createWindow(bool alwaysCenter)
{
    // Create window.
    Window *window = new Window();
    connect(window, &Window::dropTabOut, this, &StartManager::createWindowFromTab, Qt::QueuedConnection);
    connect(window, &Window::themeChanged, this, &StartManager::loadTheme, Qt::QueuedConnection);

    // Quit application if close last window.
    connect(window, &Window::close, this,
            [=]() {
                int windowIndex = m_windows.indexOf(window);
                qDebug() << "Close window " << windowIndex;

                if (windowIndex >= 0) {
                    m_windows.takeAt(windowIndex);
                }

                if (m_windows.isEmpty()) {
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
    m_windows << window;

    return window;
}

void StartManager::initWindowPosition(Window *window, bool alwaysCenter)
{
    if (m_windows.size() == 0 || alwaysCenter) {
        Dtk::Widget::moveToCenter(window);
    } else {
        // Add window offset to avoid all editor window popup at same coordinate.
        int windowOffset = 50;
        window->move(m_windows.size() * windowOffset, m_windows.size() * windowOffset);
    }
}

void StartManager::popupExistTabs(FileTabInfo info)
{
    m_windows[info.windowIndex]->activeTab(info.tabIndex);
}

StartManager::FileTabInfo StartManager::getFileTabInfo(QString file)
{
    FileTabInfo info = {-1, -1};

    qDebug() << "Windows size: " << m_windows.size();

    for (Window *window : m_windows) {
        int tabIndex = window->getTabIndex(file);
        if (tabIndex >= 0) {
            info.windowIndex = m_windows.indexOf(window);
            info.tabIndex = tabIndex;
            break;
        }
    }

    return info;
}
