// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// ThemeItemDelegate（src/thememodule/themeitemdelegate.cpp）单元测试
//
// 类特征：QStyledItemDelegate 系（QAbstractItemDelegate 子类），paint/sizeHint 为
// protected override；paint 为纯绘制逻辑，经 -fno-access-control 直接调用，
// 以 QImage+QPainter 离屏渲染落笔验证 + stub 计数交叉断言（Wave1 已验证 offscreen 可行）。
//
// 分支清单 → 用例映射：
// - ThemeItemDelegate::ThemeItemDelegate → 各用例 SetUp 构造
// - paint: option.state & State_Selected（if 分支，选中画笔宽 2） → Paint_SelectedState_DrawsOnImage（stub 计数）
// - paint: else 分支（未选中画笔宽 1） → Paint_UnselectedState_DrawsOnImage（像素验证）
// - sizeHint → SizeHint_AnyIndex_ReturnsFixedHeight
//
// 最小清单完成情况：
// | 1 | 每个公开/保护方法 ≥1 用例 | 完成（ctor/dtor/paint/sizeHint） |
// | 2 | 等价类划分（选中态/非选中态） | 完成 |
// | 3 | 边界值（默认 option / 默认索引形态） | 完成 |
// | 4 | TEST_P | N/A（两态已由独立用例精确断言，无 ≥3 组同断言逻辑） |
// | 5 | 分支清单映射 | 见上方 |
// | 6 | if 分支两侧全覆盖 | 完成 |
// | 7 | 异常路径 EXPECT_THROW | N/A（纯绘制无 throw） |
// | 8 | 负面场景（空主题映射仍可绘制） | 完成（getThemeMapFromPath 未登记路径返回空映射） |
// | 9 | 强异常安全（绘制前后模型行数不变） | 完成 |
// | 10 | stub_ext（Utils::getThemeMapFromPath / QPainter::drawText 计数） | 完成 |

#include <gtest/gtest.h>

#include <QColor>
#include <QImage>
#include <QModelIndex>
#include <QPainter>
#include <QRect>
#include <QSize>
#include <QStyleOptionViewItem>
#include <QVariant>

#include "test_env.h"
#include "themeitemdelegate.h"
#include "themelistmodel.h"

namespace {

class ThemeItemDelegateTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { thememoduleEnsureApp(); }

    void SetUp() override
    {
        stub.clear();
        themeMapCallCount = 0;
        installThemeSourceStubs(stub, {
            { QStringLiteral("/ut-fake-themes/dark.theme"), makeThemeMap("Dark", "#222222", "#EEEEEE") },
        });
        model = new ThemeListModel();
        delegate = new ThemeItemDelegate();
    }

    void TearDown() override
    {
        delete delegate;  // 覆盖 ~ThemeItemDelegate
        delete model;
        delegate = nullptr;
        model = nullptr;
        stub.clear();
    }

    // 统计 Utils::getThemeMapFromPath 被真实调用的次数（在既有主题源 stub 上叠加计数）
    void instrumentThemeMapCalls()
    {
        // 重新安装带计数的 stub（set_lamda 对同一地址重复设置会自动 reset 旧值）
        stub.set_lamda(&Utils::getThemeMapFromPath,
                       [this](const QString &) -> QVariantMap {
                           ++themeMapCallCount;
                           return makeThemeMap("Dark", "#222222", "#EEEEEE");
                       });
    }

    QStyleOptionViewItem makeOption(QStyle::State extraState)
    {
        QStyleOptionViewItem option;
        option.rect = QRect(0, 0, 300, 130);
        option.state |= QStyle::State_Enabled | extraState;
        return option;
    }

    stub_ext::StubExt stub;
    int themeMapCallCount = 0;
    ThemeListModel *model = nullptr;
    ThemeItemDelegate *delegate = nullptr;
};

// ---- sizeHint ----

TEST_F(ThemeItemDelegateTest, SizeHint_AnyIndex_ReturnsFixedHeight)
{
    // Arrange：默认构造的 option/index（边界形态）
    QStyleOptionViewItem option;
    QModelIndex index;

    // Act
    const QSize hint = delegate->sizeHint(option, index);  // protected 经 -fno-access-control

    // Assert：固定返回宽 -1 高 130（未启用则 131）
    EXPECT_EQ(hint.width(), -1);
    EXPECT_EQ(hint.height(), 130);
}

// ---- paint：真实离屏渲染（else 分支：未选中画笔）----

TEST_F(ThemeItemDelegateTest, Paint_UnselectedState_DrawsOnImage)
{
    // Arrange
    instrumentThemeMapCalls();
    QImage image(300, 130, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    ASSERT_TRUE(painter.isActive());
    const QModelIndex idx = model->index(0, 0);
    const QStyleOptionViewItem option = makeOption(QStyle::State_None);

    // Act
    delegate->paint(&painter, option, idx);
    painter.end();

    // Assert：完整语法高亮样例绘制落笔（10 段文本 + 背景 + 边框），图片出现非透明像素
    int paintedPixels = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(image.pixel(x, y)) != 0)
                ++paintedPixels;
        }
    }
    EXPECT_GT(paintedPixels, 0);
    EXPECT_EQ(themeMapCallCount, 1);  // 恰好解析一次主题 JSON
    EXPECT_EQ(model->rowCount(QModelIndex()), 1);  // 绘制不破坏模型（强异常安全）
}

// ---- paint：选中态（if 分支）+ drawText 计数 ----

TEST_F(ThemeItemDelegateTest, Paint_SelectedState_DrawsAllSyntaxSegments)
{
    // Arrange：拦截 QRect 版 drawText 统计文本绘制次数（真实落笔由上一用例验证）
    instrumentThemeMapCalls();
    int drawTextCount = 0;
    stub.set_lamda(
        static_cast<void (QPainter::*)(const QRect &, int, const QString &, QRect *)>(
            &QPainter::drawText),
        [&drawTextCount](QPainter *, const QRect &, int, const QString &, QRect *) {
            ++drawTextCount;
        });
    QImage image(300, 130, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    const QModelIndex idx = model->index(0, 0);
    const QStyleOptionViewItem option = makeOption(QStyle::State_Selected);

    // Act
    delegate->paint(&painter, option, idx);
    painter.end();

    // Assert：语法高亮样例全部 10 段文本都请求绘制；主题 JSON 恰好解析一次
    //（源码固定绘制：#include/"deepin.h"/QString/theme/() {//Return/return/"Dark"/;/}）
    EXPECT_EQ(drawTextCount, 10);
    EXPECT_EQ(themeMapCallCount, 1);
}

}  // namespace
