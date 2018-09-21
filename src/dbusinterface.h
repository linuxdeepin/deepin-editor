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

#ifndef DBUS_INTERFACE_H
#define DBUS_INTERFACE_H

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

class SaveFileInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName() {
        return "com.deepin.editor.daemon";
    }

public:
    SaveFileInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);
    ~SaveFileInterface();

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<bool> saveFile(const QByteArray &filepath, const QByteArray &text, const QByteArray &encoding) {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(filepath) << QVariant::fromValue(text) << QVariant::fromValue(encoding);
        return asyncCallWithArgumentList(QStringLiteral("saveFile"), argumentList);
    }

Q_SIGNALS: // SIGNALS
};

namespace DBusDaemon {
    typedef ::SaveFileInterface dbus;
}
#endif
