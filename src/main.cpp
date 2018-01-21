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

#include "window.h"
#include <QCommandLineParser>
#include <DApplication>
#include <DMainWindow>
#include <QApplication>
#include <QScreen>
#include <QDesktopWidget>
#include <DWidgetUtil>
#include <iostream>
#include "utils.h"
#include <QDBusConnection>
#include <QDebug>
#include <QDBusInterface>
#include "startmanager.h"

DWIDGET_USE_NAMESPACE

int main(int argc, char *argv[])
{
    DApplication::loadDXcbPlugin();

    const char *descriptionText = QT_TRANSLATE_NOOP("MainWindow",
                                                    "Deepin Editor是一款简单的文本编辑器"
        );

    const QString acknowledgementLink = "https://www.deepin.org/acknowledgments/deepin-editor";

    DApplication app(argc, argv);

    app.loadTranslator();

    app.setOrganizationName("deepin");
    app.setApplicationName(QObject::tr("deepin-editor"));
    app.setApplicationVersion("1.0");

    app.setProductIcon(QIcon(Utils::getQrcPath("logo_96.svg")));
    app.setProductName(DApplication::translate("MainWindow", "Deepin Editor"));
    app.setApplicationDescription(DApplication::translate("MainWindow", descriptionText) + "\n");
    app.setApplicationAcknowledgementPage(acknowledgementLink);

    app.setWindowIcon(QIcon(Utils::getQrcPath("logo_48.png")));

    QCommandLineParser parser;
    
    const QCommandLineOption filesOption("f", "Files to open", "files");
    const QCommandLineOption newWindowOption("w", "Open file in new window", "new-window");
    const QCommandLineOption helpOption = parser.addHelpOption();
    parser.addOption(filesOption);
    parser.addOption(newWindowOption);
    
    parser.process(app);
    
    QStringList files;
    if (parser.isSet(filesOption)) {
        files << parser.values(filesOption);
    }
    bool openInWindow = false;
    
    if (parser.isSet(newWindowOption)) {
        openInWindow = true;
    }

    QDBusConnection dbus = QDBusConnection::sessionBus();
    if (dbus.registerService("com.deepin.Editor")) {
        StartManager startManager;

        if (openInWindow) {
            startManager.openFilesInWindow(files);
        } else {
            startManager.openFilesInTab(files);
        }

        dbus.registerObject("/com/deepin/Editor", &startManager, QDBusConnection::ExportScriptableSlots);

        return app.exec();
    } else {
        // Send DBus message to stop screen-recorder if found other screen-recorder DBus service has started.
        QDBusInterface notification("com.deepin.Editor",
                                    "/com/deepin/Editor",
                                    "com.deepin.Editor",
                                    QDBusConnection::sessionBus());

        QList<QVariant> arg;
        arg << files;
        if (openInWindow) {
            notification.callWithArgumentList(QDBus::AutoDetect, "openFilesInWindow", arg);
        } else {
            notification.callWithArgumentList(QDBus::AutoDetect, "openFilesInTab", arg);
        }
    }

    return 0;
}
