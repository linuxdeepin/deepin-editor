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


#include "performancemonitor.h"

const QString LOG_FLAG = "[PerformanceMonitor]";

const QString GRAB_POINT_INIT_APP_TIME  = "[GRABPOINT] POINT-01";
const QString GRAB_POINT_CLOSE_APP_TIME = "[GRABPOINT] POINT-02";
const QString GRAB_POINT_OPEN_FILE_TIME = "[GRABPOINT] POINT-04";

qint64 PerformanceMonitor::initializeAppStartMs  = 0;
qint64 PerformanceMonitor::inittalizeApoFinishMs = 0;
qint64 PerformanceMonitor::closeAppStartMs       = 0;
qint64 PerformanceMonitor::closeAppFinishMs      = 0;
qint64 PerformanceMonitor::openFileStartMs       = 0;
qint64 PerformanceMonitor::openFileFinishMs      = 0;

PerformanceMonitor::PerformanceMonitor()
{

}

void PerformanceMonitor::initializeAppStart()
{
    QDateTime current = QDateTime::currentDateTime();
    qDebug() << "LOG_FLAG"
             << QDateTime::currentDateTime().toString(Qt::ISODateWithMs)
             << "start to initaalize app";
    initializeAppStartMs = current.toMSecsSinceEpoch();
}

void PerformanceMonitor::initializAppFinish()
{
    QDateTime current = QDateTime::currentDateTime();
    qDebug() << "LOG_FLAG"
             << QDateTime::currentDateTime().toString(Qt::ISODateWithMs)
             << " finish to initialize app";

    inittalizeApoFinishMs = current.toMSecsSinceEpoch();
    qint64 time = inittalizeApoFinishMs - initializeAppStartMs;
    qInfo() << QString("%1 startduration=%2ms #(Init app time)").arg(GRAB_POINT_INIT_APP_TIME).arg(time);
}

void PerformanceMonitor::closeAppStart()
{
    QDateTime current = QDateTime::currentDateTime();
    closeAppStartMs = current.toMSecsSinceEpoch();
}

void PerformanceMonitor::closeAPPFinish()
{
    QDateTime current = QDateTime::currentDateTime();
    qDebug() << "LOG_FLAG"
             << QDateTime::currentDateTime().toString(Qt::ISODateWithMs)
             << " finish to close app";

    closeAppFinishMs = current.toMSecsSinceEpoch();
    qint64 time = closeAppFinishMs - closeAppStartMs;
    qInfo() << QString("%1 closeduration=%2ms #(Close app time)").arg(GRAB_POINT_CLOSE_APP_TIME).arg(time);
}

void PerformanceMonitor::openFileStart()
{
    QDateTime current = QDateTime::currentDateTime();
    qDebug() << "LOG_FLAG"
             << QDateTime::currentDateTime().toString(Qt::ISODateWithMs)
             << "start to open file";
    openFileStartMs = current.toMSecsSinceEpoch();
}

void PerformanceMonitor::openFileFinish(const QString &strFileName, qint64 iFileSize)
{
    QDateTime current = QDateTime::currentDateTime();
    qDebug() << "LOG_FLAG"
             << QDateTime::currentDateTime().toString(Qt::ISODateWithMs)
             << " finish to open file";

    openFileFinishMs = current.toMSecsSinceEpoch();
    qint64 time = openFileFinishMs - openFileStartMs;
    float fFilesize = iFileSize;
    qInfo() << QString("%1 filename=%2 filezise=%3M opentime=%4ms #(Open file time)").arg(GRAB_POINT_OPEN_FILE_TIME).arg(strFileName).arg(QString::number(fFilesize/(1024*1024), 'f', 6)).arg(time);
}
