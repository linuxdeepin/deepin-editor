#include "performancemonitor.h"

const QString LOG_FLAG = "[PerformanceMonitor]";

const QString GRAB_POINT     = "[GRABPOINT]";
const QString APP_NAME       = "DEEPIN_EDITOR";
const QString INIT_APP_TIME  = "0001";
const QString CLOSE_APP_TIME = "0002";
const QString OOPEN_FILE_TIME = "0004";

qint64 PerformanceMonitor::initializeAppStartMs  = 0;
qint64 PerformanceMonitor::inittalizeApoFinishMs = 0;
qint64 PerformanceMonitor::closeAppStartMs       = 0;
qint64 PerformanceMonitor::closeAppFinishMs      = 0;
qint64 PerformanceMonitor::openFileStartMs       = 0;
qint64 PerformanceMonitor::openFileFinishMs      = 0;

PerformanceMonitor::PerformanceMonitor(QObject *parent) : QObject(parent)
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
    qInfo() << QString("%1 %2-%3 %4 #(Init app time)").arg(GRAB_POINT).arg(APP_NAME).arg(INIT_APP_TIME).arg(time);
}

void PerformanceMonitor::closeAppStart()
{
    QDateTime current = QDateTime::currentDateTime();
    qDebug() << "LOG_FLAG"
             << QDateTime::currentDateTime().toString(Qt::ISODateWithMs)
             << "start to close app";
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
    qInfo() << QString("%1 %2-%3 %4 #(Close app time)").arg(GRAB_POINT).arg(APP_NAME).arg(CLOSE_APP_TIME).arg(time);
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
    qInfo() << QString("%1 %2-%3 %4 #(Open file time) file name:%5, file size:%6 ").arg(GRAB_POINT).arg(APP_NAME).arg(OOPEN_FILE_TIME).arg(time).arg(strFileName).arg(iFileSize);
}
