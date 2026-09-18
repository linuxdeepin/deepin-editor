// SPDX-FileCopyrightText: 2011-2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dbusinterface.h"

#include <QDebug>

SaveFileInterface::SaveFileInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent)
    : QDBusAbstractInterface(service, path, staticInterfaceName(), connection, parent)
{
    qDebug() << "SaveFileInterface created for" << service << path;
}

SaveFileInterface::~SaveFileInterface()
{
    qDebug() << "SaveFileInterface destroyed for" << service() << path();
}
