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

#include <QCoreApplication>
#include <QPlainTextEdit>
#include <QTextDocument>
#include <QTextStream>
#include <QFile>
#include <QDebug>
#include <QTextCodec>
#include "encoding.h"



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

void FileLoadThread::setEncodeInfo(QStringList pathList,QStringList codeList)
{
    m_pathList = pathList;
    m_codeList = codeList;
}

QString FileLoadThread::getCodec()
{
    QFile file(m_sFilePath);

    if (!QFile::exists (m_sFilePath))
    {
        return "";
    }

    if (!file.open (QFile::ReadOnly))
    {
        return "";
    }

    bool enforced = !m_sCharset.isEmpty();
    bool hasNull = false;
    QByteArray data;
    char c;
    qint64 charSize = sizeof (char); // 1
    int num = 0;
    if (enforced)
    { // no need to check for the null character here
        while (file.read (&c, charSize) > 0)
        {
            if (c == '\n' || c == '\r')
                num = 0;
            if (num < 500004) // a multiple of 4 (for UTF-16/32)
                data.append (c);
            else
                m_bForceUneditable = true;
            ++num;
        }
    }
    else
    {
        unsigned char C[4];
        /* checking 4 bytes is enough to guess
           whether the encoding is UTF-16 or UTF-32 */
        while (num < 4 && file.read (&c, charSize) > 0)
        {
            data.append (c);
            if (c == '\0')
                hasNull = true;
            C[num] = static_cast<unsigned char>(c);
            ++ num;
        }
        if (num == 2 && ((C[0] != '\0' && C[1] == '\0') || (C[0] == '\0' && C[1] != '\0')))
            m_sCharset = "UTF-16"; // single character
        else if (num == 4)
        {
            if (hasNull)
            {
                if ((C[0] == 0xFF && C[1] == 0xFE && C[2] != '\0' && C[3] == '\0') // le
                    || (C[0] == 0xFE && C[1] == 0xFF && C[2] == '\0' && C[3] != '\0') // be
                    || (C[0] != '\0' && C[1] == '\0' && C[2] != '\0' && C[3] == '\0') // le
                    || (C[0] == '\0' && C[1] != '\0' && C[2] == '\0' && C[3] != '\0')) // be
                {
                    m_sCharset = "UTF-16";
                }
                /*else if ((C[0] == 0xFF && C[1] == 0xFE && C[2] == '\0' && C[3] == '\0')
                          || (C[0] == '\0' && C[1] == '\0' && C[2] == 0xFE && C[3] == 0xFF))*/
                else if ((C[0] != '\0' && C[1] != '\0' && C[2] == '\0' && C[3] == '\0') // le
                         || (C[0] == '\0' && C[1] == '\0' && C[2] != '\0' && C[3] != '\0')) // be
                {
                    m_sCharset = "UTF-32";
                }
            }
            /* reading may still be possible */
            if (m_sCharset.isEmpty() && !hasNull)
            {
                num = 5; // 4 characters are already read
                while (file.read (&c, charSize) > 0)
                {
                    if (c == '\0')
                    {
                        if (!hasNull)
                        {
                            if (m_bSkipNonText)
                            {
                                file.close();
                                return "UTF-8";
                            }
                            hasNull = true;
                        }
                    }
                    else if (c == '\n' || c == '\r')
                        num = 0;
                    if (num <= 500000)
                        data.append (c);
                    else if (num == 500001)
                    {
                        data += QByteArray ("    HUGE LINE TRUNCATED: NO LINE WITH MORE THAN 500000 CHARACTERS");
                       m_bForceUneditable = true;
                    }
                    ++num;
                }
            }
            else
            { // the meaning of null characters was determined before
                if (m_bSkipNonText && hasNull && m_sCharset.isEmpty())
                {
                    file.close();
                    return "UTF-8";
                }
                num = 0;
                while (file.read (&c, charSize) > 0)
                {
                    if (c == '\n' || c == '\r')
                        num = 0;
                    if (num < 500004) // a multiple of 4 (for UTF-16/32)
                        data.append (c);
                    else
                        m_bForceUneditable = true;
                    ++num;
                }
            }
        }
    }
    file.close();
    if (m_bSkipNonText && hasNull && m_sCharset.isEmpty())
    {
        return "UTF-8";
    }

    if (m_sCharset.isEmpty())
    {
        if (hasNull)
        {
            m_bForceUneditable = true;
            m_sCharset = "UTF-8"; // always open non-text files as UTF-8
        }
        else
            m_sCharset = detectCharset (data);
    }

    QTextCodec *codec = QTextCodec::codecForName (m_sCharset.toUtf8()); // or m_sCharset.toStdString().c_str()
    if (!codec) // prevent any chance of crash if there's a bug
    {
        m_sCharset = "UTF-8";
        codec = QTextCodec::codecForName ("UTF-8");
    }
    return m_sCharset;
}

