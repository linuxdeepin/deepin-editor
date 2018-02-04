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

#ifndef DBUS_ADAPTOR_H
#define DBUS_ADAPTOR_H

#include <QtCore/QObject>
#include <QtDBus/QtDBus>
QT_BEGIN_NAMESPACE
class QByteArray;
template<class T> class QList;
template<class Key, class Value> class QMap;
class QString;
class QStringList;
class QVariant;
QT_END_NAMESPACE

class DbusAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.deepin.editor.daemon")
    Q_CLASSINFO("D-Bus Introspection", ""
                "  <interface name=\"com.deepin.editor.daemon\">\n"
                "    <method name=\"saveFile\">\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "      <arg direction=\"in\" type=\"ss\" name=\"text\"/>\n"
                "    </method>\n"
                "  </interface>\n"
                "")
public:
    DbusAdaptor(QObject *parent);
    virtual ~DbusAdaptor();

public: // PROPERTIES
public Q_SLOTS: // METHODS
    bool saveFile(const QString &filepath, const QString &text);
Q_SIGNALS: // SIGNALS
};

#endif
