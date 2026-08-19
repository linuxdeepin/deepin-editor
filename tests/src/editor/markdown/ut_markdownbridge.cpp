// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_markdownbridge.h"
#include "markdownbridge.h"

#include <QSignalSpy>

UT_MarkdownBridge::UT_MarkdownBridge()
{
}

// onReady 应触发 ready 信号（JS 侧 Milkdown 创建完成后调用）
TEST_F(UT_MarkdownBridge, OnReady_EmitsReadySignal)
{
    MarkdownBridge bridge;
    QSignalSpy spy(&bridge, &MarkdownBridge::ready);
    ASSERT_TRUE(spy.isValid());

    bridge.onReady();

    EXPECT_EQ(spy.count(), 1);
}

// onContentChanged 应转发 contentChanged 信号并原样携带 md（阶段二启用，接口先到位）
TEST_F(UT_MarkdownBridge, OnContentChanged_EmitsContentChangedWithPayload)
{
    MarkdownBridge bridge;
    QSignalSpy spy(&bridge, &MarkdownBridge::contentChanged);

    bridge.onContentChanged(QStringLiteral("# hello"));

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toString(), QStringLiteral("# hello"));
}

// onScrollRatio 应转发 scrollRatioChanged 信号并携带 ratio
TEST_F(UT_MarkdownBridge, OnScrollRatio_EmitsScrollRatioChangedWithPayload)
{
    MarkdownBridge bridge;
    QSignalSpy spy(&bridge, &MarkdownBridge::scrollRatioChanged);

    bridge.onScrollRatio(0.5);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_DOUBLE_EQ(spy.takeFirst().at(0).toDouble(), 0.5);
}

// onOpenLink 应转发 openLinkRequested 信号并原样携带 url（JS 点击拦截外部链接 → 系统浏览器）
TEST_F(UT_MarkdownBridge, OnOpenLink_EmitsOpenLinkRequestedWithPayload)
{
    MarkdownBridge bridge;
    QSignalSpy spy(&bridge, &MarkdownBridge::openLinkRequested);

    bridge.onOpenLink(QStringLiteral("https://www.deepin.org/index.shtml"));

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toString(), QStringLiteral("https://www.deepin.org/index.shtml"));
}

// C++ → JS 信号：setMarkdownRequested 应可被 QSignalSpy 捕获（验证信号存在且可连接）
TEST_F(UT_MarkdownBridge, SetMarkdownRequested_IsEmitable)
{
    MarkdownBridge bridge;
    QSignalSpy spy(&bridge, &MarkdownBridge::setMarkdownRequested);

    emit bridge.setMarkdownRequested(QStringLiteral("body"));

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toString(), QStringLiteral("body"));
}

// applyThemeRequested 信号存在性 + 双参数载荷（jsonColors / isDark）
TEST_F(UT_MarkdownBridge, ApplyThemeRequested_IsEmitableWithTwoArgs)
{
    MarkdownBridge bridge;
    QSignalSpy spy(&bridge, &MarkdownBridge::applyThemeRequested);

    emit bridge.applyThemeRequested(QStringLiteral("{\"--bg\":\"#fff\"}"), true);

    ASSERT_EQ(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    EXPECT_EQ(args.at(0).toString(), QStringLiteral("{\"--bg\":\"#fff\"}"));
    EXPECT_TRUE(args.at(1).toBool());
}

// setModeRequested 信号载荷（editable 开关）
TEST_F(UT_MarkdownBridge, SetModeRequested_IsEmitable)
{
    MarkdownBridge bridge;
    QSignalSpy spy(&bridge, &MarkdownBridge::setModeRequested);

    emit bridge.setModeRequested(true);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_TRUE(spy.takeFirst().at(0).toBool());
}

// setLayoutRequested 信号载荷（maxW / center）
TEST_F(UT_MarkdownBridge, SetLayoutRequested_IsEmitableWithTwoArgs)
{
    MarkdownBridge bridge;
    QSignalSpy spy(&bridge, &MarkdownBridge::setLayoutRequested);

    emit bridge.setLayoutRequested(800, true);

    ASSERT_EQ(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    EXPECT_EQ(args.at(0).toInt(), 800);
    EXPECT_TRUE(args.at(1).toBool());
}

// scrollToRatioRequested 信号载荷
TEST_F(UT_MarkdownBridge, ScrollToRatioRequested_IsEmitable)
{
    MarkdownBridge bridge;
    QSignalSpy spy(&bridge, &MarkdownBridge::scrollToRatioRequested);

    emit bridge.scrollToRatioRequested(0.25);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_DOUBLE_EQ(spy.takeFirst().at(0).toDouble(), 0.25);
}
