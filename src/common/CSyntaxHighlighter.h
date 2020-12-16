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
#pragma once

#include <QThread>
#include <QString>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/Definition>

class CSyntaxHighlighter : public QThread
{
    Q_OBJECT

public:
    CSyntaxHighlighter(const QString &filepath, QObject *QObject = nullptr);
    ~CSyntaxHighlighter();

    void run();
signals:
    //void loadFinished(const QByteArray &encode, const QByteArray &content);
    void sigDefinition(KSyntaxHighlighting::Definition def);
private:
    QString m_sFilePath;
};
