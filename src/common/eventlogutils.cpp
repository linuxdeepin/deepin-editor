// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "eventlogutils.h"
#include <QLibrary>
#include <QDir>
#include <QLibraryInfo>
#include <QJsonDocument>
#include <QDebug>

Eventlogutils *Eventlogutils::m_pInstance = nullptr;
Eventlogutils *Eventlogutils::GetInstance()
{
    qDebug() << "Eventlogutils::GetInstance()";
    if (m_pInstance == nullptr) {
        m_pInstance  = new Eventlogutils();
    }
    return m_pInstance;
}

void Eventlogutils::writeLogs(QJsonObject &data)
{
    if (!writeEventLogFunc) {
        qWarning() << "writeEventLogFunc is not initialized";
        return;
    }

    writeEventLogFunc(QJsonDocument(data).toJson(QJsonDocument::Compact).toStdString());
}

Eventlogutils::Eventlogutils()
{
    qDebug() << "Eventlogutils::Eventlogutils()";
    QLibrary library("libdeepin-event-log.so");
    initFunc = reinterpret_cast<bool (*)(const std::string &, bool)>(library.resolve("Initialize"));
    writeEventLogFunc = reinterpret_cast<void (*)(const std::string &)>(library.resolve("WriteEventLog"));

    if (!initFunc) {
        qWarning() << "Failed to load libdeepin-event-log.so";
        return;
    }

    initFunc("deepin-editor", true);
}
