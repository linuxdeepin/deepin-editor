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
#include "utils.h"
#include "window.h"

#include <DApplication>
#include <DMainWindow>
#include <DWidgetUtil>
#include <QApplication>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDebug>
#include <QDesktopWidget>
#include <QScreen>
#include <iostream>

DWIDGET_USE_NAMESPACE

int main(int argc, char *argv[])
{
    // Init DTK.
    DApplication::loadDXcbPlugin();

    const char *descriptionText = QT_TRANSLATE_NOOP("MainWindow",
                                                    "Deepin Editor是一款简单的文本编辑器");
    const QString acknowledgementLink = "https://www.deepin.org/acknowledgments/deepin-editor";

    DApplication app(argc, argv);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);

    app.loadTranslator();

    app.setOrganizationName("deepin");
    app.setApplicationName(QObject::tr("deepin-editor"));
    app.setApplicationDisplayName(QObject::tr("Deepin Editor"));
    app.setApplicationVersion("1.0");

    app.setProductIcon(QIcon(Utils::getQrcPath("logo_96.svg")));
    app.setProductName(DApplication::translate("MainWindow", "Deepin Editor"));
    app.setApplicationDescription(DApplication::translate("MainWindow", descriptionText) + "\n");
    app.setApplicationAcknowledgementPage(acknowledgementLink);

    app.setWindowIcon(QIcon(Utils::getQrcPath("logo_96.svg")));

    // Parser input arguments.
    QCommandLineParser parser;
    
    const QCommandLineOption newWindowOption("w", "Open file in new window");
    const QCommandLineOption helpOption = parser.addHelpOption();
    parser.addOption(newWindowOption);
    
    parser.process(app);
    
    QStringList files;
    QStringList arguments = app.arguments();
    for (int i = 1; i < arguments.size(); i++) {
        if (arguments[i] != "-w") {
            QStringList splitResult = arguments[i].split("file://");
            
            QString file = "";
            if (splitResult.size() == 1) {
                file = splitResult[0];
            } else if (splitResult.size() == 2) {
                file = splitResult[1];
            }
            
            if (Utils::fileExists(file)) {
                files << file;
            }
        }
    }
    
    bool hasWindowFlag = parser.isSet(newWindowOption);

    // Start.
    QDBusConnection dbus = QDBusConnection::sessionBus();
    
    // Start editor process if not found any editor use DBus.
    if (dbus.registerService("com.deepin.Editor")) {
        StartManager startManager;

        if (hasWindowFlag) {
            startManager.openFilesInWindow(files);
        } else {
            startManager.openFilesInTab(files);
        }

        dbus.registerObject("/com/deepin/Editor", &startManager, QDBusConnection::ExportScriptableSlots);

        return app.exec();
    }
    // Just send dbus message to exist editor process.
    else {
        QDBusInterface notification("com.deepin.Editor",
                                    "/com/deepin/Editor",
                                    "com.deepin.Editor",
                                    QDBusConnection::sessionBus());

        QList<QVariant> arg;
        arg << files;
        if (hasWindowFlag) {
            notification.callWithArgumentList(QDBus::AutoDetect, "openFilesInWindow", arg);
        } else {
            notification.callWithArgumentList(QDBus::AutoDetect, "openFilesInTab", arg);
        }
    }

    return 0;
}
