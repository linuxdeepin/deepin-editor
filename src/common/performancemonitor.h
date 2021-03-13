/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     guoshaoyu <guoshaoyu@uniontech.com>
*
* Maintainer: guoshaoyu <guoshaoyu@uniontech.com>
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

#ifndef PERFORMANCEMONITOR_H
#define PERFORMANCEMONITOR_H


#include <QTime>
#include <QDateTime>
#include <QDebug>

class PerformanceMonitor
{

public:
    explicit PerformanceMonitor();

    static void initializeAppStart();
    static void initializAppFinish();
    static void closeAppStart();
    static void closeAPPFinish();
    static void openFileStart();
    static void openFileFinish(const QString &strFileName, qint64 iFileSize);

private:
    Q_DISABLE_COPY(PerformanceMonitor)

    static qint64 initializeAppStartMs;
    static qint64 inittalizeApoFinishMs;
    static qint64 closeAppStartMs;
    static qint64 closeAppFinishMs;
    static qint64 openFileStartMs;
    static qint64 openFileFinishMs;
};

#endif // PERFORMANCEMONITOR_H
