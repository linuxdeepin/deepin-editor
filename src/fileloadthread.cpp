/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     rekols <rekols@foxmail.com>
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

#include "fileloadthread.h"
#include "utils.h"

#include <QCoreApplication>
#include <QPlainTextEdit>
#include <QTextDocument>
#include <QTextStream>
#include <QFile>
#include <QDebug>
#include <QTextCodec>

FileLoadThread::FileLoadThread(const QString &filepath, QObject *parent)
    : QThread(parent),
      m_filePath(filepath)
{

}

FileLoadThread::~FileLoadThread()
{
}

void FileLoadThread::run()
{
    QFile file(m_filePath);

    if (file.open(QIODevice::ReadOnly)) {
        // reads all remaining data from the file.
        QByteArray fileContent = file.readAll();

        // read the encode.
        QByteArray encode = Utils::detectEncode(fileContent);
        QByteArray encodeArray = encode;

        if (m_pathList.contains(m_filePath)) {
            encodeArray = m_codeList.value(m_pathList.indexOf(m_filePath)).toUtf8();
        }

        file.close();
        if (file.open(QIODevice::ReadOnly)) {
            QTextStream stream(&fileContent);
            stream.setCodec(encodeArray);
            QString content = stream.readAll();

            emit loadFinished(encodeArray, content);
            }

            file.close();
            emit toTellFileClosed();
        }
        else {
            file.close();
        }

}

void FileLoadThread::setEncodeInfo(QStringList pathList,QStringList codeList)
{
    m_pathList = pathList;
    m_codeList = codeList;
}
