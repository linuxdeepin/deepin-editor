// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../../src/common/performancemonitor.h"

#include <gtest/gtest.h>

// Constructor is public and default-constructible.
TEST(PerformanceMonitorTest, Construct_DoesNotCrash)
{
    PerformanceMonitor pm;
    SUCCEED();
}

TEST(PerformanceMonitorTest, InitializeAppStart_SetsTimestamp)
{
    PerformanceMonitor pm;
    pm.initializeAppStart();
    EXPECT_NE(pm.initializeAppStartMs, 0);
}

TEST(PerformanceMonitorTest, InitializAppFinish_SetsTimestamp)
{
    PerformanceMonitor pm;
    pm.initializAppFinish();
    EXPECT_NE(pm.inittalizeApoFinishMs, 0);
}

TEST(PerformanceMonitorTest, CloseAppStart_SetsTimestamp)
{
    PerformanceMonitor pm;
    pm.closeAppStart();
    EXPECT_NE(pm.closeAppStartMs, 0);
}

TEST(PerformanceMonitorTest, CloseAPPFinish_SetsTimestamp)
{
    PerformanceMonitor pm;
    pm.closeAPPFinish();
    EXPECT_NE(pm.closeAppFinishMs, 0);
}

TEST(PerformanceMonitorTest, OpenFileStart_SetsTimestamp)
{
    PerformanceMonitor pm;
    pm.openFileStart();
    EXPECT_NE(pm.openFileStartMs, 0);
}

TEST(PerformanceMonitorTest, OpenFileFinish_SetsTimestamp)
{
    PerformanceMonitor pm;
    pm.openFileFinish(QStringLiteral("test.txt"), 1024);
    EXPECT_NE(pm.openFileFinishMs, 0);
}
