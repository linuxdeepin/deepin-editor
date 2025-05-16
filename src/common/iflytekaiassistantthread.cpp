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
    bool bIsRegistUosAiAssistant = false;

    // 直接尝试调用 DBus 接口
    QDBusInterface aiInterface("com.iflytek.aiassistant",
                             "/aiassistant/deepinmain",
                             "org.freedesktop.DBus.Introspectable");

    QDBusReply<QString> reply = aiInterface.call("Introspect");
    if (reply.isValid()) {
        bIsRegistUosAiAssistant = true;
        qInfo() << "com.iflytek.aiassistant service is available.";
    } else {
        qInfo() << "Failed to connect to com.iflytek.aiassistant: " << reply.error().message();
    }

    emit sigIsRegisteredIflytekAiassistant(bIsRegistUosAiAssistant);
}