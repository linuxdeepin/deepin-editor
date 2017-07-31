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

#include "main_window.h"
#include <DApplication>
#include <DMainWindow>
#include <QApplication>
#include <QScreen>
#include <QDesktopWidget>
#include <DWidgetUtil>
#include <iostream>
#include "utils.h"

DWIDGET_USE_NAMESPACE

int main(int argc, char *argv[]) 
{
    DApplication::loadDXcbPlugin();
    
    const char *descriptionText = QT_TRANSLATE_NOOP("MainWindow", 
                                                    "Deepin Note是一款简单的文本编辑器"
                                                    );

    const QString acknowledgementLink = "https://www.deepin.org/acknowledgments/deepin-note";

    DApplication app(argc, argv);

    app.loadTranslator();
        
    app.setOrganizationName("deepin");
    app.setApplicationName(QObject::tr("deepin-note"));
    app.setApplicationVersion("1.0");
        
    app.setProductIcon(QPixmap::fromImage(QImage(Utils::getQrcPath("logo_96.svg"))));
    app.setProductName(DApplication::translate("MainWindow", "Deepin Note"));
    app.setApplicationDescription(DApplication::translate("MainWindow", descriptionText) + "\n");
    app.setApplicationAcknowledgementPage(acknowledgementLink);
        
    app.setWindowIcon(QIcon(Utils::getQrcPath("logo_48.png")));
        
    MainWindow window;
        
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect  screenGeometry = screen->geometry();
    window.setMinimumSize(QSize(screenGeometry.width() * 2 / 3, screenGeometry.height() * 2 / 3));
    Dtk::Widget::moveToCenter(&window);
    window.show();

    return app.exec();

    return 0;
}
