// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// ThemeListView（src/thememodule/themelistview.cpp）单元测试
//
// 类特征：QListView 子类（GUI 类），offscreen QApplication 无头构造；
// 事件/滚动条/选择模型交互按 test-types §7.5 用 stub_ext 拦截 Qt 内置类。
//
// 分支清单 → 用例映射：
// - ThemeListView::ThemeListView → 各用例 SetUp 构造（滚动模式/滚动条策略经构造后状态断言）
// - adjustScrollbarMargins: !isVisible() 早退 → AdjustScrollbarMargins_HiddenView_SkipsLayoutRequest
// - adjustScrollbarMargins: 可见 + 滚动条区域非空 → AdjustScrollbarMargins_VisibleView_SendsLayoutAndSetsMargins
// - adjustScrollbarMargins: 可见 + 滚动条区域为空（else 分支，与 if 分支语句相同） → 同上用例覆盖（两分支 setViewportMargins 调用相同）
// - eventFilter: FocusOut → focusOut 信号 → EventFilter_ParamEventType_FocusOutEmittedOrNot(TEST_P)
// - eventFilter: 非 FocusOut → 不发射 → 同上 TEST_P "other" 参数
// - selectionChanged: selectedIndexes 非空 → themeChanged → SelectionChanged_SingleSelection_EmitsThemePath
// - selectionChanged: selectedIndexes 多项 → 每项发射 → SelectionChanged_MultiSelection_EmitsPerIndex
// - selectionChanged: selectedIndexes 空 → 不发射 → SelectionChanged_EmptySelection_EmitsNothing
//
// 最小清单完成情况：
// | 1 | 每个公开/保护方法 ≥1 用例 | 完成（ctor/dtor/adjustScrollbarMargins/eventFilter/selectionChanged） |
// | 2 | 等价类划分（可见性/事件类型/选择形态） | 完成 |
// | 3 | 边界值（空选择/多选/未知事件类型） | 完成 |
// | 4 | TEST_P ≥3 组（eventFilter 3 事件类型参数） | 完成 |
// | 5 | 分支清单映射 | 见上方 |
// | 6 | if/early-return 全分支 | 完成 |
// | 7 | 异常路径 EXPECT_THROW | N/A（Qt 风格无 throw） |
// | 8 | 负面场景（空选择/非焦点事件） | 完成 |
// | 9 | 强异常安全（空选择后状态不变） | 完成 |
// | 10 | stub_ext（QApplication::sendEvent 计数） | 完成 |
//
// 已知源码缺陷（只记录不改源码）：
// - adjustScrollbarMargins if/else 两分支 setViewportMargins(0,0,5,0) 完全相同，
//   visibleRegion() 条件判断无实际效果（疑似复制粘贴残留）→ 见批次 session defects。

#include <gtest/gtest.h>

#include <QEvent>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QMargins>
#include <QModelIndex>
#include <QSignalSpy>
#include <QStringList>
#include <QVariant>

#include "test_env.h"
#include "themelistmodel.h"
#include "themelistview.h"

namespace {

// 暴露 protected override 以便直接调用（不修改源码）
class TestableThemeListView : public ThemeListView {
public:
    using ThemeListView::ThemeListView;
    using ThemeListView::eventFilter;          // protected → public
    using ThemeListView::selectionChanged;     // protected → public
};

class ThemeListViewTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { thememoduleEnsureApp(); }

    void SetUp() override
    {
        stub.clear();
        installThemeSourceStubs(stub, {
            { QStringLiteral("/ut-fake-themes/dark.theme"), makeThemeMap("Dark", "#000000") },
            { QStringLiteral("/ut-fake-themes/bright.theme"), makeThemeMap("Bright", "#FFFFFF") },
        });
        model = new ThemeListModel();
        view = new TestableThemeListView();
        view->setModel(model);
    }

    void TearDown() override
    {
        // 先清 stub 再析构控件：避免 Qt 析构路径中的事件派发被计数 stub 吞掉
        stub.clear();
        delete view;  // 覆盖 ~ThemeListView
        delete model;
        view = nullptr;
        model = nullptr;
    }

    // sendEvent 计数器（夹具成员，生命周期覆盖 stub 生效期——禁止栈局部引用捕获）
    void installSendEventCounter()
    {
        totalSendEvent = 0;
        layoutRequestToView = 0;
        stub.set_lamda(&QCoreApplication::sendEvent,
                       [this](QObject *receiver, QEvent *event) -> bool {
                           ++totalSendEvent;
                           if (event && event->type() == QEvent::LayoutRequest
                               && receiver == view)
                               ++layoutRequestToView;
                           return true;
                       });
    }

    stub_ext::StubExt stub;
    int totalSendEvent = 0;
    int layoutRequestToView = 0;
    ThemeListModel *model = nullptr;
    TestableThemeListView *view = nullptr;
};

// ---- 构造 ----

TEST_F(ThemeListViewTest, Constructor_AppliesScrollPolicies)
{
    // Act：构造已完成（SetUp）
    // Assert：逐像素滚动、垂直滚动条常显、水平滚动条常隐（构造函数行为断言）
    EXPECT_EQ(view->verticalScrollMode(), QAbstractItemView::ScrollPerPixel);
    EXPECT_EQ(view->verticalScrollBarPolicy(), Qt::ScrollBarAlwaysOn);
    EXPECT_EQ(view->horizontalScrollBarPolicy(), Qt::ScrollBarAlwaysOff);
}

// ---- adjustScrollbarMargins ----

TEST_F(ThemeListViewTest, AdjustScrollbarMargins_HiddenView_SkipsLayoutRequest)
{
    // Arrange：视图未 show()；拦截 QApplication::sendEvent 计数（不发真实事件）
    installSendEventCounter();

    // Act
    view->adjustScrollbarMargins();

    // Assert：不可见早退——不派发任何事件（含 LayoutRequest）、不改边距（初始边距全 0）
    EXPECT_EQ(totalSendEvent, 0);
    EXPECT_EQ(layoutRequestToView, 0);
    const QMargins margins = view->viewportMargins();  // 经 -fno-access-control 访问 protected
    EXPECT_EQ(margins.left(), 0);
    EXPECT_EQ(margins.right(), 0);
}

TEST_F(ThemeListViewTest, AdjustScrollbarMargins_VisibleView_SendsLayoutAndSetsMargins)
{
    // Arrange：show 后可见；sendEvent 计数（Qt 内部其它事件派发允许存在）
    view->resize(300, 400);
    view->show();
    ASSERT_TRUE(view->isVisible());
    installSendEventCounter();

    // Act
    view->adjustScrollbarMargins();

    // Assert：向本视图恰好派发 1 次 LayoutRequest；右边距被置 5
    //（源码 if/else 两分支调用相同，无论 visibleRegion 哪边都落到同一断言）
    EXPECT_EQ(layoutRequestToView, 1);
    EXPECT_GE(totalSendEvent, 1);
    const QMargins margins = view->viewportMargins();
    EXPECT_EQ(margins.right(), 5);
    EXPECT_EQ(margins.top(), 0);
}

// ---- eventFilter ----

struct EventFilterCase {
    QEvent::Type eventType;
    int expectedSpyCount;
    QString caseName;
};

class ThemeListViewEventFilterTest : public ThemeListViewTest,
                                     public ::testing::WithParamInterface<EventFilterCase> {
};

TEST_P(ThemeListViewEventFilterTest, EventFilter_ParamEventType_FocusOutEmittedOrNot)
{
    // Arrange
    QSignalSpy spy(view, &ThemeListView::focusOut);
    ASSERT_TRUE(spy.isValid());
    QEvent event(GetParam().eventType);

    // Act
    const bool filtered = view->eventFilter(view, &event);

    // Assert：仅 FocusOut 发射一次信号；任何事件都不拦截（恒返回 false）
    EXPECT_EQ(spy.count(), GetParam().expectedSpyCount);
    EXPECT_FALSE(filtered);  // 期望不拦截：返回 false
}

INSTANTIATE_TEST_SUITE_P(
    EventEquivalenceClasses, ThemeListViewEventFilterTest,
    ::testing::Values(
        (EventFilterCase{ QEvent::FocusOut, 1, "focusOutEmits" }),
        (EventFilterCase{ QEvent::Enter, 0, "enterIgnored" }),
        (EventFilterCase{ QEvent::MouseButtonPress, 0, "mousePressIgnored" })),
    [](const ::testing::TestParamInfo<EventFilterCase> &info) { return info.param.caseName.toStdString(); });

// ---- selectionChanged ----

TEST_F(ThemeListViewTest, SelectionChanged_SingleSelection_EmitsThemePath)
{
    // Arrange：模型 2 行（排序后 dark 行 0、bright 行 1）
    QSignalSpy spy(view, &ThemeListView::themeChanged);
    ASSERT_TRUE(spy.isValid());

    // Act：经选择模型选中行 0（触发 selectionChanged 覆盖：先调基类再遍历选中项）
    const QModelIndex idx = model->index(0, 0);
    view->selectionModel()->select(QItemSelection(idx, idx),
                                   QItemSelectionModel::ClearAndSelect);

    // Assert：发射一次，携带行 0 的主题路径
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), QStringLiteral("/ut-fake-themes/dark.theme"));
}

TEST_F(ThemeListViewTest, SelectionChanged_MultiSelection_EmitsPerIndex)
{
    // Arrange：同时选中两行
    QSignalSpy spy(view, &ThemeListView::themeChanged);
    ASSERT_TRUE(spy.isValid());
    const QModelIndex first = model->index(0, 0);
    const QModelIndex second = model->index(1, 0);
    QItemSelection multi;
    multi.append(QItemSelectionRange(first));
    multi.append(QItemSelectionRange(second));

    // Act
    view->selectionModel()->select(multi, QItemSelectionModel::ClearAndSelect);

    // Assert：每个选中索引各发射一次（循环遍历分支）
    EXPECT_EQ(spy.count(), 2);
    EXPECT_EQ(spy.at(0).at(0).toString(), QStringLiteral("/ut-fake-themes/dark.theme"));
    EXPECT_EQ(spy.at(1).at(0).toString(), QStringLiteral("/ut-fake-themes/bright.theme"));
}

TEST_F(ThemeListViewTest, SelectionChanged_EmptySelection_EmitsNothing)
{
    // Arrange：空选择集
    QSignalSpy spy(view, &ThemeListView::themeChanged);
    ASSERT_TRUE(spy.isValid());

    // Act：直接以空选择调用 protected 覆盖（selectedIndexes 为空 → 循环体不进入）
    view->selectionChanged(QItemSelection(), QItemSelection());

    // Assert：不发射；选择模型状态未受影响（强异常安全）
    EXPECT_EQ(spy.count(), 0);
    EXPECT_TRUE(view->selectionModel()->selectedIndexes().isEmpty());
}

}  // namespace
