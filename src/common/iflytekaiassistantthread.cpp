// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "iflytekaiassistantthread.h"

#include <QDBusInterface>

IflytekAiassistantThread::IflytekAiassistantThread(QObject *parent)
    : QThread(parent)
{
}

IflytekAiassistantThread::~IflytekAiassistantThread() {}

void IflytekAiassistantThread::run()
{
    QDBusConnection connection = QDBusConnection::sessionBus();
    QDBusConnectionInterface *bus = connection.interface();
    bool bIsRegistUosAiAssistant = bus->isServiceRegistered("com.iflytek.aiassistant");
    if (!bIsRegistUosAiAssistant) {
        qInfo() << "com.iflytek.aiassistant service no registered, try get introspect.";

        QDBusInterface introspect("com.iflytek.aiassistant", "/aiassistant/deepinmain", "org.freedesktop.DBus.Introspectable");
        QDBusReply<QString> version = introspect.call("Introspect");
        if (version.isValid()) {
            bIsRegistUosAiAssistant = true;
            qInfo() << "com.iflytek.aiassistant Introspect success.";
        } else {
            qInfo() << "com.iflytek.aiassistant Introspect also failed. " << version.error().message();
        }
    }

    emit sigIsRegisteredIflytekAiassistant(bIsRegistUosAiAssistant);
}
