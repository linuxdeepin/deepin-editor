/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Rekols    <rekols@foxmail.com>
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

#ifndef STARTMANAGER_H
#define STARTMANAGER_H

#include "widgets/window.h"
#include "editor/editwrapper.h"

#include <com_deepin_dde_daemon_dock.h>
#include <com_deepin_dde_daemon_dock_entry.h>
#include <QObject>

using Dock          = com::deepin::dde::daemon::Dock;
using Entry         = com::deepin::dde::daemon::dock::Entry;


class StartManager : public QObject
{
    Q_OBJECT

    Q_CLASSINFO("D-Bus Interface", "com.deepin.Editor")

public:
    struct FileTabInfo
    {
        int windowIndex;
        int tabIndex;
    };

    static StartManager* instance();
    StartManager(QObject *parent = nullptr);
    bool checkPath(const QString &file);
    bool ifKlu();
    bool isMultiWindow();
    bool isTemFilesEmpty();
    void recoverFile(Window *window);

private:
    void initBlockShutdown();
public slots:
    Q_SCRIPTABLE void openFilesInTab(QStringList files);
    Q_SCRIPTABLE void openFilesInWindow(QStringList files);

    void createWindowFromWrapper(const QString &tabName, const QString &filePath, const QString &qstrTruePath, EditWrapper *buffer, bool isModifyed);
    void loadTheme(const QString &themeName);

    Window* createWindow(bool alwaysCenter = false);
    void initWindowPosition(Window *window, bool alwaysCenter = false);
    void popupExistTabs(FileTabInfo info);
    FileTabInfo getFileTabInfo(QString file);

    void slotCheckUnsaveTab();

private:
    static StartManager *m_instance;
    QList<Window*> m_windows;

    QDBusReply<QDBusUnixFileDescriptor> m_reply;
    QDBusInterface *m_pLoginManager = nullptr;
    QList<QVariant> m_arg;

    QDBusPendingReply<QDBusUnixFileDescriptor> m_inhibitReply;
    QScopedPointer<Dock> m_pDock;
    QScopedPointer<Entry> m_pEntry;
    QStringList m_listFilePath;
    QStringList m_qlistTemFile;
};

#endif
