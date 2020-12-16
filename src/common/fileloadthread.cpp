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
      m_sFilePath(filepath)
{

}

FileLoadThread::~FileLoadThread()
{
}

void FileLoadThread::run()
{
    QFile file(m_sFilePath);

    if (file.open(QIODevice::ReadOnly)) {
        // reads all remaining data from the file.
        QByteArray Indata = file.readAll();
        file.close();
        QByteArray OutData;
        // read the encode.
        QByteArray encode = DetectCode::GetFileEncodingFormat(m_sFilePath);
        QString textEncode =QString::fromLocal8Bit(encode);

         if(textEncode.contains("ASCII",Qt::CaseInsensitive) || textEncode.contains("UTF-8",Qt::CaseInsensitive)){
           emit loadFinished(encode, Indata);
         }else {
           DetectCode::ChangeFileEncodingFormat(Indata,OutData,textEncode,QString("UTF-8"));
           emit loadFinished(encode, OutData);
         }
    }
}
