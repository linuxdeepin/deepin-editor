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

#include <QtDBus/QDBusConnection>
#include <QCoreApplication>
#include "dbus_adaptor.h"
#include "dbus.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    dbus *dbus1 = new dbus;
    new DbusAdaptor(dbus1);

    QDBusConnection connection = QDBusConnection::systemBus();
    if (connection.isConnected()) {
        qDebug() << "Build deepin-editor daemon success.";
    }
    
    if(!connection.registerService("com.deepin.editor.daemon") || !connection.registerObject("/", dbus1)){
        qDebug() << connection.lastError();
        app.exit(1);
        
        return 1;
    }

    return app.exec();
}
