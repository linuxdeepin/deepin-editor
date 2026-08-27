// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_renderthrottle.h"
#include "renderthrottle.h"

#include <QSignalSpy>
#include <QTest>

UT_RenderThrottle::UT_RenderThrottle()
{
}

// 默认去抖间隔 300ms
TEST_F(UT_RenderThrottle, DefaultInterval_300ms)
{
    RenderThrottle t;
    EXPECT_EQ(t.interval(), 300);
}

// ready 默认 false
TEST_F(UT_RenderThrottle, DefaultNotReady)
{
    RenderThrottle t;
    EXPECT_FALSE(t.isReady());
}

// 冷却窗口内多次输入：前沿渲染首个内容 + 后沿渲染最新内容（共 2 次，各自延迟 ≤300ms）
// 注：输入必须背靠背同步调用——同步调用之间事件循环不运转，300ms 定时器不可能触发，
// 从而保证 b/c 确定性地落在冷却窗口内。若用 qWait 分隔输入，高负载（ASan+gcov 全量
// 运行）下 qWait 超时会使输入落到冷却期外而成为新前沿，得到 3 次渲染（时序抖动，非缺陷）
TEST_F(UT_RenderThrottle, MultipleInputs_Within300ms_LeadingAndTrailing)
{
    RenderThrottle t;
    t.setReady(true);
    QSignalSpy spy(&t, &RenderThrottle::renderRequested);

    t.noteContent(QStringLiteral("a"));   // 前沿：立即渲染 "a"，进入 300ms 冷却
    ASSERT_EQ(spy.count(), 1);
    t.noteContent(QStringLiteral("b"));   // 冷却期内累积
    t.noteContent(QStringLiteral("c"));   // pending 更新为最新 "c"

    // 轮询等待后沿补发（单次 qWait 在事件循环被饿死时可能错过定时器投递）
    for (int i = 0; i < 250 && spy.count() < 2; ++i)
        QTest::qWait(20);

    ASSERT_EQ(spy.count(), 2);                       // 前沿 "a" + 后沿 "c"
    EXPECT_EQ(spy.takeFirst().at(0).toString(), QStringLiteral("a"));
    EXPECT_EQ(spy.takeFirst().at(0).toString(), QStringLiteral("c"));
}

// 冷却期外的首次变更立即渲染（前沿），不等 300ms
TEST_F(UT_RenderThrottle, ReadyAndNote_EmitsImmediately_LeadingEdge)
{
    RenderThrottle t;
    t.setReady(true);
    QSignalSpy spy(&t, &RenderThrottle::renderRequested);

    t.noteContent(QStringLiteral("hello"));
    EXPECT_EQ(spy.count(), 1);   // 前沿：立即发出
    EXPECT_EQ(spy.takeFirst().at(0).toString(), QStringLiteral("hello"));
}

// 连续输入期间每 300ms 渲染一次（不停顿也能实时刷新，2026-08-19 修订的节流语义）
TEST_F(UT_RenderThrottle, ContinuousInput_RendersPeriodically)
{
    RenderThrottle t;
    t.setReady(true);
    QSignalSpy spy(&t, &RenderThrottle::renderRequested);

    // 每 50ms 输入一次，持续约 700ms（模拟连续击键）
    for (int i = 0; i <= 14; ++i) {
        t.noteContent(QStringLiteral("v%1").arg(i));
        QTest::qWait(50);
    }
    QTest::qWait(350);   // 等最后一次后沿补发

    // 700ms 连续输入 + 节流 300ms：至少 3 次渲染（前沿 + 每 300ms 后沿）
    EXPECT_GE(spy.count(), 3);
    // 最后发出的应为最新内容 v14
    EXPECT_EQ(spy.takeLast().at(0).toString(), QStringLiteral("v14"));
}

// 输入停止后：最后一个周期无新内容，定时器自然停止，不再多发
TEST_F(UT_RenderThrottle, InputStops_NoExtraEmitAfterTrailing)
{
    RenderThrottle t;
    t.setReady(true);
    QSignalSpy spy(&t, &RenderThrottle::renderRequested);

    t.noteContent(QStringLiteral("a"));
    QTest::qWait(50);
    t.noteContent(QStringLiteral("b"));
    QTest::qWait(400);   // 前沿 "a" + 后沿 "b"，共 2 次
    ASSERT_EQ(spy.count(), 2);
    spy.clear();

    QTest::qWait(600);   // 之后无输入：不再有任何渲染
    EXPECT_EQ(spy.count(), 0);
}

// 相同内容跳过（md == lastEmitted 不 emit）
TEST_F(UT_RenderThrottle, SameContent_Skipped)
{
    RenderThrottle t;
    t.setReady(true);
    QSignalSpy spy(&t, &RenderThrottle::renderRequested);

    t.noteContent(QStringLiteral("same"));
    QTest::qWait(350);
    ASSERT_EQ(spy.count(), 1);
    spy.clear();

    t.noteContent(QStringLiteral("same"));   // 相同
    QTest::qWait(350);
    EXPECT_EQ(spy.count(), 0);
}

// ready 前缓存：noteContent 后 timer 到期不 emit（因为 not ready）
TEST_F(UT_RenderThrottle, NotReady_NoteContent_CachedNoEmit)
{
    RenderThrottle t;
    t.setReady(false);
    QSignalSpy spy(&t, &RenderThrottle::renderRequested);

    t.noteContent(QStringLiteral("pending"));
    QTest::qWait(350);
    EXPECT_EQ(spy.count(), 0);   // 未 ready，缓存不发出
}

// ready 前缓存的内容，setReady(true) 后立即 flush
TEST_F(UT_RenderThrottle, PendingContent_FlushedOnReady)
{
    RenderThrottle t;
    t.setReady(false);
    QSignalSpy spy(&t, &RenderThrottle::renderRequested);

    t.noteContent(QStringLiteral("pending"));
    QTest::qWait(350);
    ASSERT_EQ(spy.count(), 0);

    t.setReady(true);   // flush
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toString(), QStringLiteral("pending"));
}

// 首切立即触发：flushNow 跳过去抖，立即 emit
TEST_F(UT_RenderThrottle, FlushNow_EmitsImmediately)
{
    RenderThrottle t;
    t.setReady(true);
    QSignalSpy spy(&t, &RenderThrottle::renderRequested);

    t.noteContent(QStringLiteral("first"));
    t.flushNow();   // 不等 300ms
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toString(), QStringLiteral("first"));
}

// flushNow 未 ready 时缓存（不 emit，等 ready）
TEST_F(UT_RenderThrottle, FlushNow_NotReady_CachedOnly)
{
    RenderThrottle t;
    t.setReady(false);
    QSignalSpy spy(&t, &RenderThrottle::renderRequested);

    t.noteContent(QStringLiteral("x"));
    t.flushNow();
    EXPECT_EQ(spy.count(), 0);   // 未 ready 不 emit
}

// setReady(true) 但无 pending 内容，不 emit
TEST_F(UT_RenderThrottle, SetReady_NoPending_NoEmit)
{
    RenderThrottle t;
    t.setReady(false);
    QSignalSpy spy(&t, &RenderThrottle::renderRequested);

    t.setReady(true);
    EXPECT_EQ(spy.count(), 0);
}
