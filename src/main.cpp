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

#include "environments.h"
#include "utils.h"
#include "window.h"
#include "urlinfo.h"

#include <DApplication>
#include <DMainWindow>
#include <DWidgetUtil>
#include <DLog>

#include <QApplication>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDebug>
#include <QDesktopWidget>
#include <QScreen>
#include <iostream>
#include <DApplicationSettings>
#include"editorapplication.h"

DWIDGET_USE_NAMESPACE

int main(int argc, char *argv[])
{
    using namespace Dtk::Core;

    EditorApplication app(argc, argv);
    //save theme
    DApplicationSettings savetheme;

    Dtk::Core::DLogManager::registerConsoleAppender();
    Dtk::Core::DLogManager::registerFileAppender();

    // Parser input arguments.
    QCommandLineParser parser;
    const QCommandLineOption newWindowOption("w", "Open file in new window");
    const QCommandLineOption helpOption = parser.addHelpOption();
    parser.addOption(newWindowOption);
    parser.process(app);

    QStringList urls;
    QStringList arguments = parser.positionalArguments();

    for (const QString &path : arguments) {
        UrlInfo info(path);
        urls << info.url.toLocalFile();
    }

    bool hasWindowFlag = parser.isSet(newWindowOption);

    // Start.
    QDBusConnection dbus = QDBusConnection::sessionBus();
    // Start editor process if not found any editor use DBus.
    if (dbus.registerService("com.deepin.Editor")) {
        StartManager *startManager = StartManager::instance();

        if (hasWindowFlag) {
            startManager->openFilesInWindow(urls);
        } else {
            startManager->openFilesInTab(urls);
        }

        dbus.registerObject("/com/deepin/Editor", startManager, QDBusConnection::ExportScriptableSlots);

        return app.exec();
    }
    // Just send dbus message to exist editor process.
    else {
        QDBusInterface notification("com.deepin.Editor",
                                    "/com/deepin/Editor",
                                    "com.deepin.Editor",
                                    QDBusConnection::sessionBus());

        QList<QVariant> args;
        args << urls;

        if (hasWindowFlag) {
            notification.callWithArgumentList(QDBus::AutoDetect, "openFilesInWindow", args);
        } else {
            notification.callWithArgumentList(QDBus::AutoDetect, "openFilesInTab", args);
        }
    }

    return 0;
}
