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

#ifndef URLINFO_H
#define URLINFO_H

#include <QRegularExpression>
#include <QString>
#include <QUrl>
#include <QDir>
#include <QDebug>

class UrlInfo
{
public:
    UrlInfo(QString path)
    {
        // Just check if the path is an existing file.
        if (QFile::exists(path)) {
            url = QUrl::fromLocalFile(QDir::current().absoluteFilePath(path));
            return;
        }

        const auto match = QRegularExpression(QStringLiteral(":(\\d+)(?::(\\d+))?:?$")).match(path);

        if (match.isValid()) {
            // cut away line/column specification from the path.
            path.chop(match.capturedLength());
        }

        // make relative paths absolute using the current working directory
        // prefer local file, if in doubt!
        url = QUrl::fromUserInput(path, QDir::currentPath(), QUrl::AssumeLocalFile);

        // in some cases, this will fail, e.g.
        // assume a local file and just convert it to an url.
        if (!url.isValid()) {
            // create absolute file path, we will e.g. pass this over dbus to other processes
            url = QUrl::fromLocalFile(QDir::current().absoluteFilePath(path));
        }
    }

    QUrl url;
};

#endif // URLINFO_H
