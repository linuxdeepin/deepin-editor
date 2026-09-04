// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// ============================================================================
// RenderThrottle 单元测试（src/editor/markdown/renderthrottle.h）
//
// 覆盖现状：renderthrottle.h 已由 markdownview/editwrapper TU 间接覆盖
// ctor/interval/setInterval/isReady/setReady/noteContent 前沿/flushNow/
// flushPending（lcov 6/7），唯一 FNDA:0 为私有槽 onTimeout（后沿补发）。
//
// 分支清单 → 用例映射（onTimeout，renderthrottle.h:88）：
//   B1 onTimeout: m_pending != m_lastEmitted && flushPending()==true
//      → 补发最新 pending 并 m_timer.start() 续期下一周期
//      → NoteContent_ContinuousInput_TimeoutEmitsAndRenewsCycle
//        / NoteContent_DuringCooldown_TimeoutEmitsLatestThenStops
//   B2 onTimeout: 条件不满足（pending==lastEmitted 或 flush false）
//      → 不补发、不续期，定时器自然停止
//      → NoteContent_DuringCooldown_TimeoutEmitsLatestThenStops（收敛段）
//
// 实现要点：真实 QTimer 事件循环驱动（QTest::qWait 交付 timeout），
// setInterval(50) 缩短节流周期保证用例速度，节流语义与默认 300ms 同构。
// ============================================================================

#include <gtest/gtest.h>
#include "stubext.h"

#include <QCoreApplication>
#include <QSignalSpy>
#include <QtTest>

#include "renderthrottle.h"

class RenderThrottleTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        if (QCoreApplication::instance() == nullptr) {
            static int argc = 1;
            static char appName[] = "test_renderthrottle";
            static char *argv[] = { appName, nullptr };
            s_app = new QCoreApplication(argc, argv);
        }
    }

    static void TearDownTestSuite()
    {
        // QCoreApplication 保留至进程退出（RenderThrottle 为纯 QObject 逻辑）
    }

    static QCoreApplication *s_app;
};

QCoreApplication *RenderThrottleTest::s_app = nullptr;

// B1+B2：冷却期内累积的变更由 onTimeout 补发最新内容；补发后静默，
// 下一周期无新内容则不续期、定时器自然停止（任何变更可见延迟有上界）
TEST_F(RenderThrottleTest, NoteContent_DuringCooldown_TimeoutEmitsLatestThenStops)
{
    // Arrange
    RenderThrottle throttle;
    throttle.setInterval(50);
    QSignalSpy spy(&throttle, &RenderThrottle::renderRequested);
    throttle.setReady(true);

    // Act 1：前沿立即渲染（冷却期外首次变更）
    throttle.noteContent(QStringLiteral("a"));
    ASSERT_EQ(spy.count(), 1) << "front-edge change must render immediately";

    // Act 2：冷却期内第二笔变更 → pending 累积，onTimeout 后沿补发
    throttle.noteContent(QStringLiteral("b"));
    QTest::qWait(180);   // > interval(50) + 调度缓冲，确保 timeout 已交付

    // Assert：后沿补发最新内容
    EXPECT_EQ(spy.count(), 2);
    EXPECT_EQ(spy.at(1).at(0).toString(), QStringLiteral("b"));

    // Act 3：静默两个周期 → pending==lastEmitted，onTimeout 不再补发/续期
    QTest::qWait(200);

    // Assert：无任何额外发射（定时器自然停止）
    EXPECT_EQ(spy.count(), 2);
}

// B1：连续输入期间每个节流周期最多渲染一次；周期内新内容由下一个
// onTimeout 补发并续期（第二周期），renderRequested 恒为最新 pending
TEST_F(RenderThrottleTest, NoteContent_ContinuousInput_TimeoutEmitsAndRenewsCycle)
{
    // Arrange
    RenderThrottle throttle;
    throttle.setInterval(50);
    QSignalSpy spy(&throttle, &RenderThrottle::renderRequested);
    throttle.setReady(true);

    // Act：第 1 周期（前沿 + 冷却期内变更）
    throttle.noteContent(QStringLiteral("a"));
    ASSERT_EQ(spy.count(), 1);
    throttle.noteContent(QStringLiteral("b"));
    QTest::qWait(90);   // onTimeout #1 → emit b，续期
    ASSERT_EQ(spy.count(), 2) << "trailing edge must emit latest pending";

    // Act：第 2 周期（续期冷却期内又有新变更）
    throttle.noteContent(QStringLiteral("c"));
    QTest::qWait(90);   // onTimeout #2 → emit c

    // Assert
    EXPECT_EQ(spy.count(), 3);
    EXPECT_EQ(spy.at(2).at(0).toString(), QStringLiteral("c"));
    EXPECT_EQ(spy.at(1).at(0).toString(), QStringLiteral("b"));
}

// B2 补充：默认 interval(300) 语义下的后沿补发（不缩短周期，验证出厂值）
TEST_F(RenderThrottleTest, NoteContent_DefaultInterval_TrailingEdgeEmitsAfterCooldown)
{
    // Arrange
    RenderThrottle throttle;
    ASSERT_EQ(throttle.interval(), 300) << "spec: 300ms throttle window";
    QSignalSpy spy(&throttle, &RenderThrottle::renderRequested);
    throttle.setReady(true);

    // Act
    throttle.noteContent(QStringLiteral("first"));
    ASSERT_EQ(spy.count(), 1);
    throttle.noteContent(QStringLiteral("second"));
    QTest::qWait(420);   // > 300ms 冷却 + 调度缓冲

    // Assert
    EXPECT_EQ(spy.count(), 2);
    EXPECT_EQ(spy.at(1).at(0).toString(), QStringLiteral("second"));
}
