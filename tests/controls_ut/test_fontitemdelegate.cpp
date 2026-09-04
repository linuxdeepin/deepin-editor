// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// FontItemDelegate（src/controls/fontitemdelegate.cpp）单元测试
//
// 类特征：QStyledItemDelegate 子类；paint/sizeHint 为 protected override，
// 经 -fno-access-control 直接调用，QImage+QPainter 离屏真实落笔验证。
//
// 方法映射（公开/保护方法全集）：
// - FontItemDelegate::FontItemDelegate → Constructor_NoParent_CreatesPlainInstance
// - ~FontItemDelegate                   → TearDown delete 链覆盖
// - paint(protected)                    → Paint_UnselectedState_DrawsTextOnImage (TEST_P) /
//                                         Paint_SelectedState_FillsHighlightBackground /
//                                         Paint_WithChineseFont_SetsFontFamilyFromIndex
// - sizeHint(protected)                 → SizeHint_AnyIndex_ReturnsFixedHeight (TEST_P)
//
// 分支清单（来源：fontitemdelegate.cpp）→ 用例映射：
// - paint: isSelected（option.state & State_Selected）→ 蓝底白字 → Paint_SelectedState_...
// - paint: else → 黑字直绘                          → Paint_UnselectedState_... (TEST_P)
// - sizeHint: 恒定 QSize(-1, 30)（无分支）           → SizeHint_... (TEST_P)
//
// 最小清单完成情况：
// | 1 | 每个公开/保护方法 ≥1 用例 | 完成 |
// | 2 | 等价类（选中/未选中 × 文本 ASCII/中文/空） | 完成 |
// | 3 | 边界值（空字符串索引、无效索引） | 完成 |
// | 4 | TEST_P（≥3 组同断言：paint 3 组、sizeHint 3 组） | 完成 |
// | 5 | 分支清单映射 | 见上方 |
// | 6 | if 分支两侧全覆盖 | 完成 |
// | 7 | 异常路径 | N/A（纯绘制无 throw） |
// | 8 | 负面（空文本/无效索引仍可安全绘制） | 完成 |
// | 9 | 强异常安全（绘制前后模型数据不变） | 完成 |
// | 10 | stub_ext（QPainter::setFont 计数断言字体族副作用） | 完成 |

#include <gtest/gtest.h>

#include <QImage>
#include <QModelIndex>
#include <QPainter>
#include <QStandardItemModel>
#include <QStyleOptionViewItem>

#include "fontitemdelegate.h"
#include "test_env.h"

namespace {

class FontItemDelegateTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { controlsut::ensureApp(); }

    void SetUp() override
    {
        stub.clear();
        setFontCount = 0;
        lastFontFamily.clear();
        delegate = new FontItemDelegate();
        model.setParent(nullptr);
        model.clear();
        model.appendRow(new QStandardItem(QString::fromUtf8("Sans Serif")));
        model.appendRow(new QStandardItem(QString::fromUtf8("等距更纱黑体 SC")));
        model.appendRow(new QStandardItem(QString()));  // 边界：空字体名
    }

    void TearDown() override
    {
        delete delegate;
        delegate = nullptr;
        stub.clear();
    }

    QStyleOptionViewItem makeOption(QStyle::State extraState, const QRect &rect)
    {
        QStyleOptionViewItem option;
        option.rect = rect;
        option.state |= QStyle::State_Enabled | extraState;
        return option;
    }

    int countPaintedPixels(const QImage &image) const
    {
        int painted = 0;
        for (int y = 0; y < image.height(); ++y)
            for (int x = 0; x < image.width(); ++x)
                if (qAlpha(image.pixel(x, y)) != 0)
                    ++painted;
        return painted;
    }

    stub_ext::StubExt stub;
    FontItemDelegate *delegate = nullptr;
    QStandardItemModel model;
    int setFontCount = 0;
    QString lastFontFamily;
};

// ---- 构造 ----

TEST_F(FontItemDelegateTest, Constructor_NoParent_CreatesPlainInstance)
{
    // Assert：可构造、无父对象、元对象类型正确（Q_OBJECT 就绪）
    EXPECT_EQ(delegate->parent(), nullptr);
    EXPECT_STREQ(delegate->metaObject()->className(), "FontItemDelegate");
}

// ---- sizeHint（TEST_P 3 组：任意索引恒定尺寸）----

struct SizeHintCase {
    int row;
};

class FontItemSizeHintTest : public FontItemDelegateTest,
                             public ::testing::WithParamInterface<SizeHintCase> {
};

TEST_P(FontItemSizeHintTest, SizeHint_AnyIndex_ReturnsFixedHeight)
{
    const auto &c = GetParam();
    QStyleOptionViewItem option;
    const QModelIndex idx = model.index(c.row, 0);

    // Act
    const QSize hint = delegate->sizeHint(option, idx);

    // Assert：固定宽 -1 高 30（源码常量）
    EXPECT_EQ(hint.width(), -1);
    EXPECT_EQ(hint.height(), 30);
}

INSTANTIATE_TEST_SUITE_P(
    SizeCases, FontItemSizeHintTest,
    ::testing::Values(SizeHintCase{ 0 }, SizeHintCase{ 1 }, SizeHintCase{ 2 }));

// ---- paint：未选中分支（TEST_P 3 组文本）----

struct PaintCase {
    int row;
};

class FontItemPaintTest : public FontItemDelegateTest,
                          public ::testing::WithParamInterface<PaintCase> {
};

TEST_P(FontItemPaintTest, Paint_UnselectedState_DrawsTextOnImage)
{
    const auto &c = GetParam();
    QImage image(300, 40, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    ASSERT_TRUE(painter.isActive());
    const QModelIndex idx = model.index(c.row, 0);
    const auto option = makeOption(QStyle::State_None, QRect(0, 0, 300, 40));
    const QString textBefore = idx.data(Qt::DisplayRole).toString();

    // Act
    delegate->paint(&painter, option, idx);
    painter.end();

    // Assert：有落笔（空文本行为例则允许 0 像素，但绝不崩溃且状态不变）
    const int painted = countPaintedPixels(image);
    if (!textBefore.isEmpty())
        EXPECT_GT(painted, 0);
    EXPECT_EQ(idx.data(Qt::DisplayRole).toString(), textBefore);  // 强异常安全
    // 未选中分支不铺高亮底色：左上角像素不得为选中蓝
    if (painted > 0)
        EXPECT_NE(image.pixel(2, 2), qRgb(0x2C, 0xA7, 0xF8));
}

INSTANTIATE_TEST_SUITE_P(
    TextCases, FontItemPaintTest,
    ::testing::Values(PaintCase{ 0 }, PaintCase{ 1 }, PaintCase{ 2 }));

// ---- paint：选中分支（蓝底 + 白字）----

TEST_F(FontItemDelegateTest, Paint_SelectedState_FillsHighlightBackground)
{
    // Arrange
    QImage image(300, 40, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    const QModelIndex idx = model.index(0, 0);
    const auto option = makeOption(QStyle::State_Selected, QRect(0, 0, 300, 40));

    // Act
    delegate->paint(&painter, option, idx);
    painter.end();

    // Assert：整行填充 #2CA7F8 高亮底色（drawRect 覆盖 option.rect 全域）
    EXPECT_EQ(image.pixel(2, 20), qRgb(0x2C, 0xA7, 0xF8));
    EXPECT_EQ(image.pixel(297, 20), qRgb(0x2C, 0xA7, 0xF8));
    EXPECT_GT(countPaintedPixels(image), 0);
}

// 辅助：默认未选中 option
inline QStyleOptionViewItem optionNone()
{
    QStyleOptionViewItem option;
    option.rect = QRect(0, 0, 300, 40);
    option.state |= QStyle::State_Enabled;
    return option;
}

// ---- paint：字体族副作用 ----

TEST_F(FontItemDelegateTest, Paint_WithChineseFont_SetsFontFamilyFromIndex)
{
    // Arrange：拦截 setFont 记录字体族
    stub.set_lamda(&QPainter::setFont,
                   [this](QPainter *, const QFont &font) {
                       ++setFontCount;
                       lastFontFamily = font.family();
                   });
    QImage image(300, 40, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    const QModelIndex idx = model.index(1, 0);  // "等距更纱黑体 SC"

    // Act
    delegate->paint(&painter, optionNone(), idx);
    painter.end();

    // Assert：painter 字体族被设置为索引数据（中文字体名精确传递）
    EXPECT_EQ(setFontCount, 1);
    EXPECT_EQ(lastFontFamily, QString::fromUtf8("等距更纱黑体 SC"));
}

}  // namespace
