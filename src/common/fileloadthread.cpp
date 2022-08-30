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
        QByteArray encode;
        QByteArray indata;

        // 判断文件大小是否超过40MB, 超过40MB的文件过大，需要调整读取策略，优先加载头部文件
        static const int s_maxDirectReadLen = 40 * DATA_SIZE_1024 * DATA_SIZE_1024;
        if (file.size() > s_maxDirectReadLen) {
            // 先读取1MB数据
            indata = file.read(DATA_SIZE_1024 * DATA_SIZE_1024);
            encode = DetectCode::GetFileEncodingFormat(m_strFilePath, indata);

            // 发送文件头信息，用于预先加载数据
            QString textEncode = QString::fromLocal8Bit(encode);
            if (textEncode.contains("ASCII", Qt::CaseInsensitive) || textEncode.contains("UTF-8", Qt::CaseInsensitive)) {
                emit sigPreProcess(encode, indata);
            } else {
                QByteArray outHeadData;
                DetectCode::ChangeFileEncodingFormat(indata, outHeadData, textEncode, QString("UTF-8"));
                emit sigPreProcess(encode, outHeadData);
            }
        }

        // reads all remaining data from the file.
        indata += file.read(file.size());
        file.close();

        if (encode.isEmpty()) {
            //编码识别，如果文件数据大于1M，则只裁剪出1M文件数据去做编码探测
            QByteArray dateUsedForCodeIdentify;
            if (indata.length() > DATA_SIZE_1024 * DATA_SIZE_1024) {
                dateUsedForCodeIdentify = indata.mid(0, DATA_SIZE_1024 * DATA_SIZE_1024);
            } else {
                dateUsedForCodeIdentify = indata;
            }
            encode = DetectCode::GetFileEncodingFormat(m_strFilePath, dateUsedForCodeIdentify);
        }

        QString textEncode = QString::fromLocal8Bit(encode);
        if (textEncode.contains("ASCII", Qt::CaseInsensitive) || textEncode.contains("UTF-8", Qt::CaseInsensitive)) {
            emit sigLoadFinished(encode, indata);
        } else {
            QByteArray outData;
            DetectCode::ChangeFileEncodingFormat(indata, outData, textEncode, QString("UTF-8"));
            emit sigLoadFinished(encode, outData);
        }
    }
}
