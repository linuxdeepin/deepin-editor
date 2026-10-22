// SPDX-FileCopyrightText: 2011-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddlog.h"
#include "dbusinterface.h"

#include <QDebug>

SaveFileInterface::SaveFileInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent)
    : QDBusAbstractInterface(service, path, staticInterfaceName(), connection, parent)
{
    qCDebug(appLog) << "SaveFileInterface created for" << service << path;
}

SaveFileInterface::~SaveFileInterface()
{
    qCDebug(appLog) << "SaveFileInterface destroyed for" << service() << path();
}
