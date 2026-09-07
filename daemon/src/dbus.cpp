// SPDX-FileCopyrightText: 2017 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "policykithelper.h"
#include "dbus.h"
#include "utils.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QtCore/QFile>

DBus::DBus(QObject *parent) : QObject(parent)
{
}

bool DBus::saveFile(const QByteArray &path, const QByteArray &text, const QByteArray &encoding)
{
    const QString filepath = QString::fromUtf8(path);

    if (PolicyKitHelper::instance()->checkAuthorization("com.deepin.editor.saveFile", getpid())) {
        // Create file if filepath is not exists.
        if (!Utils::fileExists(filepath)) {
            QString directory = QFileInfo(filepath).dir().absolutePath();

            QDir().mkpath(directory);
            if (QFile(filepath).open(QIODevice::ReadWrite)) {
                qDebug() << QString("File %1 not exists, create one.").arg(filepath);
            }
        }

        // Save content to file.
        QFile file(filepath);
        if(!file.open(QIODevice::WriteOnly | QIODevice::Text)){
            qDebug() << "Can't write file: " << filepath;

            return false;
        }

        QTextStream out(&file);
        out.setCodec(encoding);
        out << text;
        // 检查写入与落盘状态，磁盘错误（ENOSPC、配额等）不得静默上报成功
        out.flush();
        if (out.status() != QTextStream::Ok) {
            qWarning() << "Write stream failed:" << file.errorString();
            file.close();
            return false;
        }
        file.close();
        if (file.error() != QFile::NoError) {
            qWarning() << "Flush to disk failed:" << file.errorString();
            return false;
        }

        return true;
    } else{
        return false;
    }
}
