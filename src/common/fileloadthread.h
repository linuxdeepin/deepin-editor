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

#ifndef FILELOADTHREAD_H
#define FILELOADTHREAD_H

#include <QThread>

class FileLoadThread : public QThread
{
    Q_OBJECT
public:
    FileLoadThread(const QString &filepath, QObject *QObject = nullptr);
    ~FileLoadThread();

    void run();

signals:
    // 预处理信号，优先处理文件头，防止出现加载时间过长的情况
    void sigPreProcess(const QByteArray &encode, const QByteArray &content);
    void sigLoadFinished(const QByteArray &encode, const QByteArray &content);

private:
    QString m_strFilePath;
};

#endif
