// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_editwrapper_markdown.h"
#include "editwrapper.h"
#include "markdown/viewmodefsm.h"
#include "markdown/renderthrottle.h"

#include <QSignalSpy>
#include <QTest>
#include <QScrollBar>
#include "markdown/markdownview.h"
#include "markdown/markdownbridge.h"

using ::testing::_;

UT_EditWrapper_Markdown::UT_EditWrapper_Markdown()
{
}

// setViewMode 委托 FSM：非 md 文件切 LivePreview 应被拒（返回 false）
TEST(UT_EditWrapper_Markdown, SetViewMode_NonMarkdownLivePreview_Rejected)
{
    EditWrapper wra;
    wra.m_isMarkdown = false;   // -fno-access-control 直接访问私有成员
    EXPECT_FALSE(wra.setViewMode(ViewMode::LivePreview));
    EXPECT_EQ(wra.viewMode(), ViewMode::Edit);   // 未改变
}

// setViewMode：非 md 文件可切 Edit / ReadView
TEST(UT_EditWrapper_Markdown, SetViewMode_NonMarkdown_EditAndReadView_Accepted)
{
    EditWrapper wra;
    wra.m_isMarkdown = false;
    EXPECT_TRUE(wra.setViewMode(ViewMode::ReadView));
    EXPECT_EQ(wra.viewMode(), ViewMode::ReadView);
    EXPECT_TRUE(wra.setViewMode(ViewMode::Edit));
    EXPECT_EQ(wra.viewMode(), ViewMode::Edit);
}

// setViewMode：md 文件可切 LivePreview
TEST(UT_EditWrapper_Markdown, SetViewMode_MarkdownLivePreview_Accepted)
{
    EditWrapper wra;
    wra.m_isMarkdown = true;
    EXPECT_TRUE(wra.setViewMode(ViewMode::LivePreview));
    EXPECT_EQ(wra.viewMode(), ViewMode::LivePreview);
}

// setViewMode：Wysiwyg 阶段二不可达，应被拒
TEST(UT_EditWrapper_Markdown, SetViewMode_Wysiwyg_Rejected)
{
    EditWrapper wra;
    wra.m_isMarkdown = true;
    EXPECT_FALSE(wra.setViewMode(ViewMode::Wysiwyg));
}

// setViewMode 切换后发 viewModeChanged 信号
TEST(UT_EditWrapper_Markdown, SetViewMode_EmitsViewModeChanged)
{
    EditWrapper wra;
    wra.m_isMarkdown = true;
    QSignalSpy spy(&wra, &EditWrapper::viewModeChanged);
    wra.setViewMode(ViewMode::ReadView);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).value<ViewMode>(), ViewMode::ReadView);
}

// updateMarkdownRecognition：md 文件 → m_isMarkdown=true + 发 markdownAvailabilityChanged(true)
TEST(UT_EditWrapper_Markdown, UpdateRecognition_MarkdownFile_SetsFlagAndEmits)
{
    EditWrapper wra;
    wra.m_isMarkdown = false;
    QSignalSpy spy(&wra, &EditWrapper::markdownAvailabilityChanged);
    wra.updateMarkdownRecognition(QStringLiteral("readme.md"), QStringLiteral("Markdown"));
    EXPECT_TRUE(wra.isMarkdownFile());
    ASSERT_EQ(spy.count(), 1);
    EXPECT_TRUE(spy.takeFirst().at(0).toBool());
}

// updateMarkdownRecognition：非 md 文件 → m_isMarkdown=false，无信号（因为初始就 false）
TEST(UT_EditWrapper_Markdown, UpdateRecognition_NonMarkdown_NoSignalWhenUnchanged)
{
    EditWrapper wra;
    wra.m_isMarkdown = false;
    QSignalSpy spy(&wra, &EditWrapper::markdownAvailabilityChanged);
    wra.updateMarkdownRecognition(QStringLiteral("x.txt"), QStringLiteral("Plain Text"));
    EXPECT_FALSE(wra.isMarkdownFile());
    EXPECT_EQ(spy.count(), 0);   // 未变化，不发信号
}

// updateMarkdownRecognition：从 md 切到非 md，当前在 LivePreview → 回退到 Edit
TEST(UT_EditWrapper_Markdown, UpdateRecognition_LostMarkdownFromLivePreview_FallbackToEdit)
{
    EditWrapper wra;
    wra.m_isMarkdown = true;
    wra.setViewMode(ViewMode::LivePreview);
    ASSERT_EQ(wra.viewMode(), ViewMode::LivePreview);
    // 模拟语言切走
    wra.updateMarkdownRecognition(QStringLiteral("x.txt"), QStringLiteral("Plain Text"));
    EXPECT_FALSE(wra.isMarkdownFile());
    EXPECT_EQ(wra.viewMode(), ViewMode::Edit);   // FSM 回退
}

// updateMarkdownRecognition：从 md 切到非 md，当前在 ReadView → 保持 ReadView（纯文本只读）
TEST(UT_EditWrapper_Markdown, UpdateRecognition_LostMarkdownFromReadView_StayReadView)
{
    EditWrapper wra;
    wra.m_isMarkdown = true;
    wra.setViewMode(ViewMode::ReadView);
    ASSERT_EQ(wra.viewMode(), ViewMode::ReadView);
    wra.updateMarkdownRecognition(QStringLiteral("x.txt"), QStringLiteral("Plain Text"));
    EXPECT_EQ(wra.viewMode(), ViewMode::ReadView);
}

// 2026-08-19：语言切为 Markdown → Edit 自动跃迁 LivePreview（新建 txt 手动切语言场景）
TEST(UT_EditWrapper_Markdown, UpdateRecognition_GainedMarkdownFromEdit_ElevateToLivePreview)
{
    EditWrapper wra;
    wra.m_isMarkdown = false;   // 新建 txt，当前 Edit
    ASSERT_EQ(wra.viewMode(), ViewMode::Edit);
    // 模拟手动把语言切为 Markdown
    wra.updateMarkdownRecognition(QStringLiteral("untitled.txt"), QStringLiteral("Markdown"));
    EXPECT_TRUE(wra.isMarkdownFile());
    EXPECT_EQ(wra.viewMode(), ViewMode::LivePreview);   // 自动切入实时预览
}

// 识别结果未变化时不打扰用户手动选择的 Edit（仅 false→true 变化才跃迁）
TEST(UT_EditWrapper_Markdown, UpdateRecognition_UnchangedMarkdown_KeepsUserEditMode)
{
    EditWrapper wra;
    wra.m_isMarkdown = true;
    EXPECT_TRUE(wra.setViewMode(ViewMode::Edit));   // 用户手动切到 Edit
    // 再次上报同一 md 识别结果（未变化）
    wra.updateMarkdownRecognition(QStringLiteral("x.md"), QStringLiteral("Markdown"));
    EXPECT_EQ(wra.viewMode(), ViewMode::Edit);   // 不强切回 LivePreview
}

// 测试注入 Mock：注入后 ensureMarkdownViewCreated 不创建真实 view（m_pRendererInjected=true）
TEST(UT_EditWrapper_Markdown, InjectMock_PreventsRealViewCreation)
{
    EditWrapper wra;
    MockMarkdownRenderer mock;
    wra.setMarkdownRendererForTest(&mock);
    wra.m_isMarkdown = true;
    // 调 setViewMode(ReadView) 会触发 ensureMarkdownViewCreated，但注入了 mock 不应创建真实 view
    EXPECT_CALL(mock, setMode(_)).Times(::testing::AnyNumber());
    EXPECT_CALL(mock, setLayout(_, _)).Times(::testing::AnyNumber());
    EXPECT_TRUE(wra.setViewMode(ViewMode::ReadView));
    EXPECT_EQ(wra.m_pMarkdownView, nullptr);   // 未创建真实 view
    EXPECT_EQ(wra.m_pRenderer, &mock);         // 使用注入的 mock
}

// §5'.5：切 LivePreview → setLayout(0,false) + setMode(ReadOnly) + 立即 setMarkdown（首切不等 300ms）
TEST(UT_EditWrapper_Markdown, SetViewMode_LivePreview_RendererCallsAndImmediateFlush)
{
    EditWrapper wra;
    MockMarkdownRenderer mock;
    wra.setMarkdownRendererForTest(&mock);
    wra.m_isMarkdown = true;
    wra.textEditor()->setPlainText(QStringLiteral("# hi"));
    EXPECT_CALL(mock, isReady()).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(mock, setMode(_)).Times(::testing::AnyNumber());
    EXPECT_CALL(mock, setLayout(0, false)).Times(1);
    EXPECT_CALL(mock, setMarkdown(QStringLiteral("# hi"))).Times(1);
    EXPECT_TRUE(wra.setViewMode(ViewMode::LivePreview));
}

// §4.4：md 文件「查看视图」走渲染页（Page1），懒创建真实 view（单测不 init，不启动 WebEngine）
TEST(UT_EditWrapper_Markdown, SetViewMode_ReadViewMarkdown_ShowsRenderPage)
{
    EditWrapper wra;
    wra.m_isMarkdown = true;
    EXPECT_TRUE(wra.setViewMode(ViewMode::ReadView));
    EXPECT_NE(wra.m_pMarkdownView, nullptr);
    EXPECT_EQ(wra.m_viewStack->currentWidget(), wra.m_pReadPage);
}

// §4.4：非 md「查看视图」= 纯文本只读（不进渲染页），切回 Edit 恢复可编辑
TEST(UT_EditWrapper_Markdown, SetViewMode_ReadViewNonMarkdown_TextReadOnlyThenRestore)
{
    EditWrapper wra;
    wra.m_isMarkdown = false;
    EXPECT_TRUE(wra.setViewMode(ViewMode::ReadView));
    EXPECT_TRUE(wra.textEditor()->getReadOnlyMode());
    EXPECT_EQ(wra.m_pMarkdownView, nullptr);   // 非 md 不创建渲染 view
    EXPECT_TRUE(wra.setViewMode(ViewMode::Edit));
    EXPECT_FALSE(wra.textEditor()->getReadOnlyMode());
}

// §4.4：LivePreview 页把编辑页挪入分栏左栏（同一 TextEdit 实例，不重建）
TEST(UT_EditWrapper_Markdown, SetViewMode_LivePreview_EditorPageInSplitter)
{
    EditWrapper wra;
    wra.m_isMarkdown = true;
    EXPECT_TRUE(wra.setViewMode(ViewMode::LivePreview));
    EXPECT_EQ(wra.m_viewStack->currentWidget(), wra.m_pLiveSplitter);
    EXPECT_EQ(wra.m_pEditPage->parentWidget(), wra.m_pLiveSplitter);
    // 切回 Edit：编辑页回到视图栈 Page0
    EXPECT_TRUE(wra.setViewMode(ViewMode::Edit));
    EXPECT_EQ(wra.m_viewStack->currentWidget(), wra.m_pEditPage);
}

// §4.4/§4.6：识别变化时 ReadView 实现随之切换（渲染页 ↔ 纯文本只读），模式本身保持
TEST(UT_EditWrapper_Markdown, UpdateRecognition_ReadViewSwitchesImplementation)
{
    EditWrapper wra;
    wra.m_isMarkdown = true;
    EXPECT_TRUE(wra.setViewMode(ViewMode::ReadView));
    ASSERT_EQ(wra.m_viewStack->currentWidget(), wra.m_pReadPage);

    // 语言切走（md → txt）：ReadView 模式保持，但实现切为纯文本只读
    wra.updateMarkdownRecognition(QStringLiteral("x.txt"), QStringLiteral("Plain Text"));
    EXPECT_EQ(wra.viewMode(), ViewMode::ReadView);
    EXPECT_EQ(wra.m_viewStack->currentWidget(), wra.m_pEditPage);
    EXPECT_TRUE(wra.textEditor()->getReadOnlyMode());

    // 语言切回（txt → md）：实现恢复渲染页，解除只读
    wra.updateMarkdownRecognition(QStringLiteral("x.md"), QStringLiteral("Markdown"));
    EXPECT_EQ(wra.viewMode(), ViewMode::ReadView);
    EXPECT_EQ(wra.m_viewStack->currentWidget(), wra.m_pReadPage);
    EXPECT_FALSE(wra.textEditor()->getReadOnlyMode());
}

// §4.6：LivePreview 下左栏滚动按比例转发 renderer
TEST(UT_EditWrapper_Markdown, LeftScroll_ForwardsRatioToRenderer)
{
    EditWrapper wra;
    MockMarkdownRenderer mock;
    wra.setMarkdownRendererForTest(&mock);
    wra.m_isMarkdown = true;
    // 足够多行使滚动条 maximum > 0
    QString longText;
    for (int i = 0; i < 300; ++i)
        longText += QStringLiteral("line %1\n").arg(i);
    wra.textEditor()->setPlainText(longText);
    EXPECT_CALL(mock, isReady()).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(mock, setMode(_)).Times(::testing::AnyNumber());
    EXPECT_CALL(mock, setLayout(_, _)).Times(::testing::AnyNumber());
    EXPECT_CALL(mock, setMarkdown(_)).Times(::testing::AnyNumber());
    EXPECT_CALL(mock, scrollToRatio(_)).Times(::testing::AnyNumber());   // 进入时首同步（§5.2）
    EXPECT_TRUE(wra.setViewMode(ViewMode::LivePreview));

    ::testing::Mock::VerifyAndClearExpectations(&mock);
    double captured = -1.0;
    EXPECT_CALL(mock, scrollToRatio(::testing::_))
        .WillRepeatedly([&captured](double r) { captured = r; });
    auto sb = wra.textEditor()->verticalScrollBar();
    sb->setValue(sb->maximum() / 2);
    EXPECT_GT(captured, 0.0);
    EXPECT_LE(captured, 1.0);
    sb->setValue(sb->maximum());
    EXPECT_DOUBLE_EQ(captured, 1.0);
}

// §8.1：渲染视图（ReadView/LivePreview 右栏）右键弹「视图模式」菜单——TextEdit 动作可复用
TEST(UT_EditWrapper_Markdown, RenderView_HasViewModeContextMenu)
{
    EditWrapper wra;
    wra.m_isMarkdown = true;
    EXPECT_TRUE(wra.setViewMode(ViewMode::ReadView));
    ASSERT_NE(wra.m_pMarkdownView, nullptr);
    // CustomContextMenu 策略：右键事件不再落入 Chromium 默认菜单，由 EditWrapper 弹视图模式菜单
    EXPECT_EQ(wra.m_pMarkdownView->contextMenuPolicy(), Qt::CustomContextMenu);
}

// §4.5：非 Edit 模式下文本变化经 RenderThrottle（ready 后）转发 renderer
TEST(UT_EditWrapper_Markdown, TextChange_ThrottledToRenderer)
{
    EditWrapper wra;
    MockMarkdownRenderer mock;
    wra.setMarkdownRendererForTest(&mock);
    wra.m_isMarkdown = true;
    EXPECT_CALL(mock, isReady()).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(mock, setMode(_)).Times(::testing::AnyNumber());
    EXPECT_CALL(mock, setLayout(_, _)).Times(::testing::AnyNumber());
    EXPECT_CALL(mock, setMarkdown(_)).Times(::testing::AnyNumber());
    EXPECT_TRUE(wra.setViewMode(ViewMode::LivePreview));

    ::testing::Mock::VerifyAndClearExpectations(&mock);
    EXPECT_CALL(mock, isReady()).WillRepeatedly(::testing::Return(true));
    QString captured;
    EXPECT_CALL(mock, setMarkdown(::testing::_))
        .WillRepeatedly([&captured](const QString &md) { captured = md; });
    // 节流前沿：首次变更同步立即发出（事件驱动兜底：极端时序下允许 300ms 内到达）
    QSignalSpy spy(&wra.m_renderThrottle, &RenderThrottle::renderRequested);
    wra.textEditor()->setPlainText(QStringLiteral("changed"));
    if (spy.count() == 0)
        ASSERT_TRUE(spy.wait(5000));
    EXPECT_EQ(captured, QStringLiteral("changed"));
}

// §4.4 实机几何验证：LivePreview 左右两栏都必须可见且宽度合理（用户反馈"只有 markdown 显示"）
TEST(UT_EditWrapper_Markdown, LivePreview_BothPanesVisibleGeometry)
{
    EditWrapper wra;
    wra.resize(1000, 600);
    wra.show();
    QApplication::processEvents();
    wra.m_isMarkdown = true;
    ASSERT_TRUE(wra.setViewMode(ViewMode::LivePreview));
    QApplication::processEvents();

    auto *splitter = wra.m_pLiveSplitter;
    ASSERT_NE(splitter, nullptr);
    ASSERT_EQ(splitter->count(), 2);
    EXPECT_EQ(splitter->widget(0), wra.m_pEditPage);
    EXPECT_EQ(splitter->widget(1), wra.m_pMarkdownView);

    qDebug() << "[geom] splitter size =" << splitter->size()
             << "sizes =" << splitter->sizes()
             << "editPage =" << wra.m_pEditPage->size()
             << "mdView =" << wra.m_pMarkdownView->size();

    EXPECT_TRUE(wra.m_pEditPage->isVisibleTo(&wra));
    EXPECT_TRUE(wra.m_pMarkdownView->isVisibleTo(&wra));
    EXPECT_GT(wra.m_pEditPage->width(), 100);
    EXPECT_GT(wra.m_pMarkdownView->width(), 100);
    EXPECT_GT(splitter->sizes().first(), 100);
}

// §4.4 多次往返切换（LivePreview→Edit→ReadView→LivePreview）后左右两栏仍可见
TEST(UT_EditWrapper_Markdown, LivePreview_GeometryAfterModeCycling)
{
    EditWrapper wra;
    wra.resize(1000, 600);
    wra.show();
    QApplication::processEvents();
    wra.m_isMarkdown = true;

    ASSERT_TRUE(wra.setViewMode(ViewMode::LivePreview));
    EXPECT_TRUE(wra.m_pEditPage->isVisibleTo(&wra));
    EXPECT_GT(wra.m_pEditPage->width(), 100);

    ASSERT_TRUE(wra.setViewMode(ViewMode::Edit));
    ASSERT_TRUE(wra.setViewMode(ViewMode::ReadView));
    EXPECT_TRUE(wra.m_pMarkdownView->isVisibleTo(&wra));
    EXPECT_GT(wra.m_pMarkdownView->width(), 100);

    ASSERT_TRUE(wra.setViewMode(ViewMode::LivePreview));
    QApplication::processEvents();
    EXPECT_TRUE(wra.m_pEditPage->isVisibleTo(&wra));
    EXPECT_TRUE(wra.m_pMarkdownView->isVisibleTo(&wra));
    EXPECT_GT(wra.m_pEditPage->width(), 100);
    EXPECT_GT(wra.m_pMarkdownView->width(), 100);
}

// 图片路径改写接线：推送渲染的内容经 MarkdownLogic::resolveImagePaths 按 md 文件目录改写
TEST(UT_EditWrapper_Markdown, ImagePath_RewrittenBeforeRender)
{
    EditWrapper wra;
    MockMarkdownRenderer mock;
    wra.setMarkdownRendererForTest(&mock);
    wra.m_isMarkdown = true;
    wra.textEditor()->setFilePath(QStringLiteral("/tmp/docs/demo.md"));
    wra.textEditor()->setPlainText(QStringLiteral("# t\n\n![alt](sample.png)\n"));
    EXPECT_CALL(mock, isReady()).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(mock, setMode(_)).Times(::testing::AnyNumber());
    EXPECT_CALL(mock, setLayout(_, _)).Times(::testing::AnyNumber());
    QString captured;
    EXPECT_CALL(mock, setMarkdown(::testing::_))
        .WillRepeatedly([&captured](const QString &md) { captured = md; });
    EXPECT_TRUE(wra.setViewMode(ViewMode::LivePreview));
    EXPECT_TRUE(captured.contains(QStringLiteral("mdimg:///tmp/docs/sample.png")));
}

// §4.6 反向同步：右栏滚动通知（scrollRatioChanged）驱动左栏 TextEdit 按比例滚动
TEST(UT_EditWrapper_Markdown, ReverseScroll_SyncsLeftEditor)
{
    EditWrapper wra;
    wra.m_isMarkdown = true;
    QString longText;
    for (int i = 0; i < 300; ++i)
        longText += QStringLiteral("line %1\n").arg(i);
    wra.textEditor()->setPlainText(longText);
    ASSERT_TRUE(wra.setViewMode(ViewMode::LivePreview));
    ASSERT_NE(wra.m_pMarkdownView, nullptr);

    auto sb = wra.textEditor()->verticalScrollBar();
    sb->setValue(0);
    const int half = sb->minimum() + (sb->maximum() - sb->minimum()) / 2;

    QSignalSpy outSpy(wra.m_pMarkdownView->bridge(), &MarkdownBridge::scrollToRatioRequested);
    emit wra.m_pMarkdownView->bridge()->scrollRatioChanged(0.5);   // 模拟 JS 端用户滚动
    EXPECT_GE(sb->value(), half - 2);                              // 左栏应跟到约一半
    // 防回环：应用反向同步期间不应再向右栏转发 scrollToRatio
    EXPECT_EQ(outSpy.count(), 0);
}

// §4.6 反向同步仅在 LivePreview 生效（ReadView 下右栏滚动不影响编辑器——编辑器不可见）
TEST(UT_EditWrapper_Markdown, ReverseScroll_InactiveInReadView)
{
    EditWrapper wra;
    wra.m_isMarkdown = true;
    QString longText;
    for (int i = 0; i < 300; ++i)
        longText += QStringLiteral("line %1\n").arg(i);
    wra.textEditor()->setPlainText(longText);
    ASSERT_TRUE(wra.setViewMode(ViewMode::ReadView));

    auto sb = wra.textEditor()->verticalScrollBar();
    sb->setValue(0);
    emit wra.m_pMarkdownView->bridge()->scrollRatioChanged(0.5);
    EXPECT_EQ(sb->value(), 0);
}
