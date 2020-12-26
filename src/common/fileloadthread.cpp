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
#include "../encodes/detectcode.h"
#include <QFile>
#include <QDebug>

FileLoadThread::FileLoadThread(const QString &filepath, QObject *parent)
    : QThread(parent),
      m_strFilePath(filepath)
{

}

FileLoadThread::~FileLoadThread()
{
}

void FileLoadThread::run()
{
    QFile file(m_strFilePath);

    if (file.open(QIODevice::ReadOnly)) {
        // reads all remaining data from the file.
        QByteArray indata = file.readAll();
        file.close();
        QByteArray outData;
        // read the encode.
        QByteArray encode = DetectCode::GetFileEncodingFormat(m_strFilePath);
        QString textEncode =QString::fromLocal8Bit(encode);

         if (textEncode.contains("ASCII", Qt::CaseInsensitive) || textEncode.contains("UTF-8", Qt::CaseInsensitive)) {
           emit sigLoadFinished(encode, indata);
         } else {
           DetectCode::ChangeFileEncodingFormat(indata, outData, textEncode, QString("UTF-8"));
           emit sigLoadFinished(encode, outData);
         }
    }
}
