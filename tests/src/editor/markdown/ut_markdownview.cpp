// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_markdownview.h"
#include "markdownview.h"
#include "markdownbridge.h"

#include <QSignalSpy>

UT_MarkdownView::UT_MarkdownView()
{
}

// 构造后（未 init/load）默认未就绪
TEST_F(UT_MarkdownView, Construct_NotReadyByDefault)
{
    MarkdownView v;
    EXPECT_FALSE(v.isReady());
}

// setMarkdown 应转发为 bridge 的 setMarkdownRequested（经 MarkdownView 内部 bridge）
// 这里不监听内部 bridge，而是通过暴露的 bridge 访问器验证转发链路。
TEST_F(UT_MarkdownView, SetMarkdown_ForwardsToBridge)
{
    MarkdownView v;
    QSignalSpy spy(v.bridge(), &MarkdownBridge::setMarkdownRequested);

    v.setMarkdown(QStringLiteral("# title"));

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toString(), QStringLiteral("# title"));
}

// setMode(ReadOnly) 应转发 setModeRequested(false)
TEST_F(UT_MarkdownView, SetMode_ReadOnly_ForwardsFalse)
{
    MarkdownView v;
    QSignalSpy spy(v.bridge(), &MarkdownBridge::setModeRequested);

    v.setMode(static_cast<int>(MarkdownView::Mode::ReadOnly));

    ASSERT_EQ(spy.count(), 1);
    EXPECT_FALSE(spy.takeFirst().at(0).toBool());
}

// setMode(Editable) 应转发 setModeRequested(true)（阶段二用，本期接口先到位）
TEST_F(UT_MarkdownView, SetMode_Editable_ForwardsTrue)
{
    MarkdownView v;
    QSignalSpy spy(v.bridge(), &MarkdownBridge::setModeRequested);

    v.setMode(static_cast<int>(MarkdownView::Mode::Editable));

    ASSERT_EQ(spy.count(), 1);
    EXPECT_TRUE(spy.takeFirst().at(0).toBool());
}

// setLayout 应转发 maxContentWidth / center
TEST_F(UT_MarkdownView, SetLayout_ForwardsArgs)
{
    MarkdownView v;
    QSignalSpy spy(v.bridge(), &MarkdownBridge::setLayoutRequested);

    v.setLayout(800, true);

    ASSERT_EQ(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    EXPECT_EQ(args.at(0).toInt(), 800);
    EXPECT_TRUE(args.at(1).toBool());
}

// setLayout(maxContentWidth=0) 表示不限最大宽（实时阅览半幅场景）
TEST_F(UT_MarkdownView, SetLayout_ZeroMaxWidth_ForwardsZero)
{
    MarkdownView v;
    QSignalSpy spy(v.bridge(), &MarkdownBridge::setLayoutRequested);

    v.setLayout(0, false);

    ASSERT_EQ(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    EXPECT_EQ(args.at(0).toInt(), 0);
    EXPECT_FALSE(args.at(1).toBool());
}

// scrollToRatio 应转发 ratio
TEST_F(UT_MarkdownView, ScrollToRatio_ForwardsRatio)
{
    MarkdownView v;
    QSignalSpy spy(v.bridge(), &MarkdownBridge::scrollToRatioRequested);

    v.scrollToRatio(0.75);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_DOUBLE_EQ(spy.takeFirst().at(0).toDouble(), 0.75);
}

// bridge.onReady() → MarkdownView 内部置 ready 标志 + 发 ready 信号
TEST_F(UT_MarkdownView, BridgeReady_SetsReadyAndEmitsSignal)
{
    MarkdownView v;
    QSignalSpy spy(&v, &MarkdownView::ready);
    ASSERT_FALSE(v.isReady());

    v.bridge()->onReady();

    EXPECT_TRUE(v.isReady());
    EXPECT_EQ(spy.count(), 1);
}

// bridge 的 scrollRatioChanged 应穿透为 MarkdownView::scrollRatioChanged（预留，本期不连上层）
TEST_F(UT_MarkdownView, BridgeScrollRatio_PassesThrough)
{
    MarkdownView v;
    QSignalSpy spy(&v, &MarkdownView::scrollRatioChanged);

    v.bridge()->onScrollRatio(0.4);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_DOUBLE_EQ(spy.takeFirst().at(0).toDouble(), 0.4);
}

// bridge 的 contentChanged 应穿透为 MarkdownView::contentChanged（预留，本期不连上层）
TEST_F(UT_MarkdownView, BridgeContentChanged_PassesThrough)
{
    MarkdownView v;
    QSignalSpy spy(&v, &MarkdownView::contentChanged);

    v.bridge()->onContentChanged(QStringLiteral("body"));

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toString(), QStringLiteral("body"));
}

// §4.7/§5.3：页面加载是异步的，ready 前 applyTheme 的主题须缓存，ready 后自动补发
TEST_F(UT_MarkdownView, ApplyTheme_BeforeReady_ReemittedAfterReady)
{
    MarkdownView v;
    QVariantMap themeMap;
    themeMap["editor-colors"] = QVariantMap{
        {"background-color", "#1e1e1e"},
        {"text-color", "#d8d8d8"},
    };
    QSignalSpy spy(v.bridge(), &MarkdownBridge::applyThemeRequested);

    v.applyTheme(themeMap);          // 页面未 ready：直发一次（若 JS 未接线则丢失）
    ASSERT_EQ(spy.count(), 1);

    v.bridge()->onReady();           // 页面就绪：应补发缓存主题
    ASSERT_EQ(spy.count(), 2);
    const auto args = spy.takeLast();
    EXPECT_EQ(args.at(1).toBool(), true);   // 深色背景 → isDark=true
}

// ready 之后的 applyTheme 不重复补发
TEST_F(UT_MarkdownView, ApplyTheme_AfterReady_NoReemit)
{
    MarkdownView v;
    v.bridge()->onReady();
    QVariantMap themeMap;
    themeMap["editor-colors"] = QVariantMap{
        {"background-color", "#ffffff"},
        {"text-color", "#1f1f1f"},
    };
    QSignalSpy spy(v.bridge(), &MarkdownBridge::applyThemeRequested);
    v.applyTheme(themeMap);
    EXPECT_EQ(spy.count(), 1);
}
