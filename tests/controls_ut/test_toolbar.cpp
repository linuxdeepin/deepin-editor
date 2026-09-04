// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// ToolBar（src/controls/toolbar.cpp）单元测试
//
// 类特征：QWidget 子类（GUI 类，无 Q_OBJECT），offscreen 无头构造。
//
// 方法映射（公开方法全集）：
// - ToolBar::ToolBar    → Constructor_DefaultParent_CreatesZeroMarginLayout
// - ~ToolBar            → TearDown delete 链覆盖
// - setTabbar           → SetTabbar_WidgetOrNull_AddsLayoutItems / SetTabbar_NullWidget_StillAdds (TEST_P)
//
// 分支清单（来源：toolbar.cpp）：无 if/loop 分支（构造 + addWidget + addSpacing）
//
// 最小清单完成情况：
// | 1 | 每个公开方法 ≥1 用例 | 完成 |
// | 2 | 等价类（普通子件 / 空指针） | 完成 |
// | 3 | 边界值（布局计数 0 → 2） | 完成 |
// | 4 | TEST_P | N/A（仅 2 组输入，不满足 ≥3 强制条件） |
// | 5 | 分支清单 | 无分支 |
// | 6 | 分支覆盖 | N/A |
// | 7 | 异常路径 | EXPECT_NO_THROW（含 nullptr 负面输入） |
// | 8 | 负面（setTabbar(nullptr) 不崩溃不损坏） | 完成 |
// | 9 | 强异常安全（二次 setTabbar 布局持续追加） | 完成 |
// | 10 | stub_ext（无外部依赖） | 完成 |

#include <gtest/gtest.h>

#include <QHBoxLayout>
#include <QWidget>

#include "test_env.h"
#include "toolbar.h"

namespace {

class ToolBarTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { controlsut::ensureApp(); }

    void SetUp() override
    {
        stub.clear();
        toolbar = new ToolBar();
    }

    void TearDown() override
    {
        delete toolbar;
        toolbar = nullptr;
        stub.clear();
    }

    stub_ext::StubExt stub;
    ToolBar *toolbar = nullptr;
};

TEST_F(ToolBarTest, Constructor_DefaultParent_CreatesZeroMarginLayout)
{
    // Assert：水平布局已建、零边距、初始无布局项
    auto *layout = qobject_cast<QHBoxLayout *>(toolbar->layout());
    ASSERT_NE(layout, nullptr);
    EXPECT_EQ(layout->contentsMargins(), QMargins(0, 0, 0, 0));
    EXPECT_EQ(layout->count(), 0);
}

struct TabbarWidgetCase {
    bool nullWidget;
};

class ToolBarSetTabbarTest : public ToolBarTest,
                             public ::testing::WithParamInterface<TabbarWidgetCase> {
};

TEST_P(ToolBarSetTabbarTest, SetTabbar_WidgetOrNull_AddsLayoutItems)
{
    const auto &c = GetParam();
    QWidget child;
    QWidget *toAdd = c.nullWidget ? nullptr : &child;

    // Act
    EXPECT_NO_THROW(toolbar->setTabbar(toAdd));

    // Assert：非空 → 子件 + 间距共 2 项；空指针 → Qt 布局拒绝添加，仅剩间距 1 项
    auto *layout = toolbar->layout();
    if (c.nullWidget) {
        EXPECT_EQ(layout->count(), 1);
        EXPECT_NE(layout->itemAt(0)->spacerItem(), nullptr);
    } else {
        ASSERT_EQ(layout->count(), 2);
        EXPECT_EQ(layout->itemAt(0)->widget(), &child);
        EXPECT_NE(layout->itemAt(1)->spacerItem(), nullptr);  // 第二项为间距
        EXPECT_EQ(child.parentWidget(), toolbar);
    }
}

INSTANTIATE_TEST_SUITE_P(
    WidgetCases, ToolBarSetTabbarTest,
    ::testing::Values(TabbarWidgetCase{ false }, TabbarWidgetCase{ true }));

TEST_F(ToolBarTest, SetTabbar_CalledTwice_AppendsBoth)
{
    // Arrange：强异常安全——重复调用不损坏既有布局
    QWidget first, second;
    toolbar->setTabbar(&first);

    // Act
    toolbar->setTabbar(&second);

    // Assert：布局持续追加（2 + 2 = 4 项）
    EXPECT_EQ(toolbar->layout()->count(), 4);
    EXPECT_EQ(first.parentWidget(), toolbar);
    EXPECT_EQ(second.parentWidget(), toolbar);
}

}  // namespace
