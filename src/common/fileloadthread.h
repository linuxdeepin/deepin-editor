// SPDX-FileCopyrightText: 2017-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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

    /**
     * @brief setPreferredEncode 设置优先使用的编码格式，用于跳过自动探测
     * @param encode 优先编码格式，为空时自动探测
     */
    void setPreferredEncode(const QByteArray &encode);

signals:
    // 预处理信号，优先处理文件头，防止出现加载时间过长的情况
    void sigPreProcess(const QByteArray &encode, const QByteArray &content);
    void sigLoadFinished(const QByteArray &encode, const QByteArray &content, bool error = false, bool hasNul = false);

private:
    QString m_strFilePath;
    QByteArray m_preferredEncode;
};

#endif
