// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * PerformanceMonitor 单元测试
 *
 * 分支清单（来源：PerformanceMonitor 六个静态打点方法）
 * B1 : initializeAppStart/initializAppFinish → 记录时间差并输出 POINT-01 startduration
 * B2 : closeAppStart/closeAPPFinish           → POINT-02 closeduration
 * B3 : openFileStart/openFileFinish(file,size)→ POINT-04 filename/filezise/opentime
 * B4 : 构造函数                                → 仅 qDebug
 *
 * 用例映射：
 * - InitializeApp_StartThenFinish_LogsPoint01Duration     → B1
 * - CloseApp_StartThenFinish_LogsPoint02Duration          → B2
 * - OpenFile_StartThenFinish_LogsPoint04Info              → B3
 * - Constructor_NewInstance_LogsCreated                   → B4
 *
 * 隔离：QDateTime::currentDateTime 全部 stub 为固定队列时间（确定性时间差），
 * qInfo/qDebug 经 qInstallMessageHandler 捕获后断言（TearDown 恢复默认 handler）。
 */

#include <gtest/gtest.h>
#include "stubext.h"

#include "performancemonitor.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QStringList>
#include <QVector>

namespace {

QCoreApplication *ensureApp()
{
    if (!QCoreApplication::instance()) {
        static int argc = 1;
        static char argv0[] = "test_common2";
        static char *argv[2] = { argv0, nullptr };
        static QCoreApplication app(argc, argv);
        return &app;
    }
    return QCoreApplication::instance();
}

} // namespace

// 消息捕获：静态指针指向当前用例的捕获缓冲（TearDown 置空，避免跨用例污染）
static QStringList *g_capturedLogs = nullptr;

static void utCaptureHandler(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    if (g_capturedLogs)
        g_capturedLogs->append(msg);
    // 保持默认行为需要的最小输出抑制：不再回显
    Q_UNUSED(type);
}

class PerformanceMonitorTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ensureApp();
        stub.clear();
        logs.clear();
        timeQueue.clear();
        timeCalls = 0;
        g_capturedLogs = &logs;
        qInstallMessageHandler(utCaptureHandler);
        stub.set_lamda(static_cast<QDateTime (*)()>(&QDateTime::currentDateTime),
                       [this]() -> QDateTime {
                           ++timeCalls;
                           const qint64 ms = timeQueue.isEmpty() ? 0 : timeQueue.takeFirst();
                           return QDateTime::fromMSecsSinceEpoch(ms);
                       });
    }

    void TearDown() override
    {
        qInstallMessageHandler(nullptr); // 恢复默认 handler
        g_capturedLogs = nullptr;
        stub.clear();
    }

    // 便捷：压入两次相同时间（每个方法内部会取 currentDateTime 两次：赋值 + 日志格式化）
    void pushTime(qint64 ms)
    {
        timeQueue.append(ms);
        timeQueue.append(ms);
    }

    bool logsContain(const QString &sub) const
    {
        for (const QString &l : logs)
            if (l.contains(sub, Qt::CaseSensitive))
                return true;
        return false;
    }

    stub_ext::StubExt stub;
    QStringList logs;
    QVector<qint64> timeQueue;
    int timeCalls = 0;
};

// B1
TEST_F(PerformanceMonitorTest, InitializeApp_StartThenFinish_LogsPoint01Duration)
{
    // Arrange: start=1000ms, finish=2000ms → duration 1000ms
    pushTime(1000);
    pushTime(2000);
    // Act
    PerformanceMonitor::initializeAppStart();
    PerformanceMonitor::initializAppFinish();
    // Assert
    EXPECT_TRUE(logsContain(QStringLiteral("[GRABPOINT] POINT-01 startduration=1000ms")));
    EXPECT_TRUE(logsContain(QStringLiteral("start to initialize app")));
    EXPECT_EQ(timeCalls, 4);
}

// B2
TEST_F(PerformanceMonitorTest, CloseApp_StartThenFinish_LogsPoint02Duration)
{
    // Arrange: closeAppStart 仅取一次当前时间；closeAPPFinish 取两次（赋值+日志格式化）
    timeQueue.append(3000);
    pushTime(4500);
    // Act
    PerformanceMonitor::closeAppStart();
    PerformanceMonitor::closeAPPFinish();
    // Assert
    EXPECT_TRUE(logsContain(QStringLiteral("[GRABPOINT] POINT-02 closeduration=1500ms")));
    EXPECT_TRUE(logsContain(QStringLiteral("finish to close app")));
    EXPECT_EQ(timeCalls, 3);
}

// B3
TEST_F(PerformanceMonitorTest, OpenFile_StartThenFinish_LogsPoint04Info)
{
    // Arrange: 2MB 文件，耗时 500ms
    pushTime(100000);
    pushTime(100500);
    // Act
    PerformanceMonitor::openFileStart();
    PerformanceMonitor::openFileFinish(QStringLiteral("test-doc.txt"), 2LL * 1024 * 1024);
    // Assert
    EXPECT_TRUE(logsContain(QStringLiteral("[GRABPOINT] POINT-04")));
    EXPECT_TRUE(logsContain(QStringLiteral("filename=test-doc.txt")));
    EXPECT_TRUE(logsContain(QStringLiteral("filezise=2.000000M")));
    EXPECT_TRUE(logsContain(QStringLiteral("opentime=500ms")));
    EXPECT_EQ(timeCalls, 4);
}

// B4
TEST_F(PerformanceMonitorTest, Constructor_NewInstance_LogsCreated)
{
    // Arrange/Act
    {
        PerformanceMonitor monitor;
    }
    // Assert: 构造与析构不依赖任何外部资源，仅日志
    EXPECT_TRUE(logsContain(QStringLiteral("PerformanceMonitor instance created")));
    EXPECT_EQ(timeCalls, 0);
}
