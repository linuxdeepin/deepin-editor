// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// ThemeListModel（src/thememodule/themelistmodel.cpp）单元测试
//
// 类特征：QAbstractListModel 子类（非直接 GUI 绘制），主题清单来源于
// QDir 扫描 + Utils::getThemeMapFromPath JSON 解析 → 全部 stub_ext 拦截为受控假数据
// （Qt 内置类/项目静态函数均无虚接口可注入，按 test-types §7.5 选 stub_ext）。
//
// 分支清单 → 用例映射：
// - ThemeListModel::ThemeListModel → 各用例 SetUp 构造（间接调 initThemes）
// - initThemes: entryInfoList 空清单 → Constructor_EmptyThemeDir_YieldsZeroRows
// - initThemes: 多主题 + 排序 lambda（≥2 主题触发比较） → Constructor_MultipleThemes_SortedByBgLightness
// - setSelection: path 命中 → SetSelection_MatchingPath_EmitsRequestCurrentIndexOnce
// - setSelection: path 未命中（循环走完不 break） → SetSelection_UnknownPath_EmitsNothing
// - setFrameColor → SetFrameColor_TwoColors_StoredAndServedByData
// - data: switch ThemeName/ThemePath/FrameNormalColor/FrameSelectedColor → Data_ParamRole_ReturnsExpectedValue(TEST_P)
// - data: default 分支（未知角色返回无效 QVariant） → Data_ParamRole_ReturnsExpectedValue(TEST_P "unknown" 参数)
// - rowCount → Constructor_EmptyThemeDir_YieldsZeroRows / Constructor_MultipleThemes_*（多处交叉断言）
//
// 最小清单完成情况：
// | 1 | 每个公开方法 ≥1 用例 | 完成（ctor/dtor/setFrameColor/setSelection/rowCount/data；initThemes 经构造间接全覆盖） |
// | 2 | 等价类划分（角色枚举/路径命中与否/目录空与非空） | 完成 |
// | 3 | 边界值（空目录 0 行 / 首行 index(0,0)） | 完成 |
// | 4 | TEST_P ≥3 组（data 角色 5 参数组） | 完成 |
// | 5 | 分支清单映射 | 见上方 |
// | 6 | if/switch/early-return 全分支 | 完成（sort 比较经多主题用例触发） |
// | 7 | 异常路径 EXPECT_THROW | N/A（Qt 风格无 throw） |
// | 8 | 负面场景（未知路径/未知角色/空目录） | 完成 |
// | 9 | 强异常安全（未命中后模型行数不变） | 完成 |
// | 10 | stub_ext（Qt 内置类 + 项目静态函数） | 完成 |
//
// 已知源码缺陷（只记录不改源码）：
// - data() 未校验 index 行号边界：m_themes.at(r) 对越界 row（含默认 QModelIndex 的 -1）
//   直接 QList::at 越界（UB/断言崩溃），违背 QAbstractItemModel 对无效索引返回
//   QVariant() 的契约 → 见批次 session defects。

#include <gtest/gtest.h>

#include <QAbstractListModel>
#include <QModelIndex>
#include <QSignalSpy>
#include <QStringList>
#include <QVariant>

#include "test_env.h"
#include "themelistmodel.h"

namespace {

// 受控主题源：三个主题文件（entryInfoList 顺序：亮、暗、中），用于排序与命中测试
FakeThemeSource makeThreeThemes()
{
    return {
        { QStringLiteral("/ut-fake-themes/bright.theme"), makeThemeMap("Bright", "#FFFFFF") },
        { QStringLiteral("/ut-fake-themes/dark.theme"), makeThemeMap("Dark", "#000000") },
        { QStringLiteral("/ut-fake-themes/mid.theme"), makeThemeMap("Mid", "#808080") },
    };
}

class ThemeListModelTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { thememoduleEnsureApp(); }

    void SetUp() override
    {
        stub.clear();
        source = makeThreeThemes();
        installThemeSourceStubs(stub, source);
        model = new ThemeListModel();
    }

    void TearDown() override
    {
        delete model;  // 覆盖 ~ThemeListModel
        model = nullptr;
        stub.clear();
    }

    stub_ext::StubExt stub;
    FakeThemeSource source;
    ThemeListModel *model = nullptr;
};

// ---- 构造 + initThemes + rowCount ----

TEST_F(ThemeListModelTest, Constructor_EmptyThemeDir_YieldsZeroRows)
{
    // Arrange：重新用空目录构造（stub 换为空清单）
    delete model;
    installThemeSourceStubs(stub, FakeThemeSource());
    model = new ThemeListModel();

    // Act
    const int rows = model->rowCount(QModelIndex());

    // Assert：空清单 → 0 行（循环体不进入、空容器排序安全）
    EXPECT_EQ(rows, 0);
    EXPECT_EQ(model->rowCount(QModelIndex()), 0);  // 稳定性：重复调用结果一致
}

TEST_F(ThemeListModelTest, Constructor_MultipleThemes_SortedByBgLightness)
{
    // Arrange：entryInfoList 顺序为 亮(255)/暗(0)/中(128)

    // Act：读取 ThemePath 角色按行取值
    const int rows = model->rowCount(QModelIndex());
    QStringList paths;
    for (int row = 0; row < rows; ++row)
        paths << model->data(model->index(row, 0), ThemeListModel::ThemePath).toString();

    // Assert：3 行；按 background-color 亮度升序 → dark(0) < mid(128) < bright(255)
    EXPECT_EQ(rows, 3);
    EXPECT_EQ(paths, (QStringList() << "/ut-fake-themes/dark.theme"
                                    << "/ut-fake-themes/mid.theme"
                                    << "/ut-fake-themes/bright.theme"));
}

// ---- data()：角色等价类参数化（含 default 分支）----

struct DataRoleCase {
    int role;
    QVariant expected;
    QString caseName;
};

class ThemeListModelDataTest : public ThemeListModelTest,
                               public ::testing::WithParamInterface<DataRoleCase> {
};

TEST_P(ThemeListModelDataTest, Data_ParamRole_ReturnsExpectedValue)
{
    // Arrange：首行主题经排序后为 dark 主题
    const QModelIndex idx = model->index(0, 0);
    ASSERT_EQ(model->data(idx, ThemeListModel::ThemePath).toString(),
              QStringLiteral("/ut-fake-themes/dark.theme"));

    // Act
    const QVariant actual = model->data(idx, GetParam().role);

    // Assert：每个角色返回精确期望值；未登记角色（default 分支）返回无效 QVariant
    EXPECT_EQ(actual, GetParam().expected);
    EXPECT_EQ(actual.isValid(), GetParam().expected.isValid());
}

INSTANTIATE_TEST_SUITE_P(
    RoleEquivalenceClasses, ThemeListModelDataTest,
    ::testing::Values(
        // ThemeName = Qt::DisplayRole，取 metadata.name
        (DataRoleCase{ ThemeListModel::ThemeName, QVariant(QStringLiteral("Dark")), "name" }),
        // ThemePath = Qt::UserRole，取排序后首行主题文件路径
        (DataRoleCase{ ThemeListModel::ThemePath,
                       QVariant(QStringLiteral("/ut-fake-themes/dark.theme")), "path" }),
        // FrameNormalColor / FrameSelectedColor 未 setFrameColor 前为空串
        (DataRoleCase{ ThemeListModel::FrameNormalColor, QVariant(QString("")), "normalColorDefault" }),
        (DataRoleCase{ ThemeListModel::FrameSelectedColor, QVariant(QString("")), "selectedColorDefault" }),
        // default 分支：未登记角色返回无效 QVariant
        (DataRoleCase{ Qt::ToolTipRole, QVariant(), "unknownRoleInvalid" })),
    [](const ::testing::TestParamInfo<DataRoleCase> &info) { return info.param.caseName.toStdString(); });

// ---- setFrameColor ----

TEST_F(ThemeListModelTest, SetFrameColor_TwoColors_StoredAndServedByData)
{
    // Arrange
    const QModelIndex idx = model->index(1, 0);  // 排序后第 2 行（mid）

    // Act
    model->setFrameColor("#FF0000", "#00FF00");

    // Assert：两种颜色经 data() 精确回读（状态变更断言）
    EXPECT_EQ(model->data(idx, ThemeListModel::FrameSelectedColor).toString(),
              QStringLiteral("#FF0000"));
    EXPECT_EQ(model->data(idx, ThemeListModel::FrameNormalColor).toString(),
              QStringLiteral("#00FF00"));
    // 其它角色不受影响
    EXPECT_EQ(model->data(idx, ThemeListModel::ThemeName).toString(),
              QStringLiteral("Mid"));
    EXPECT_EQ(model->rowCount(QModelIndex()), 3);
}

// ---- setSelection ----

TEST_F(ThemeListModelTest, SetSelection_MatchingPath_EmitsRequestCurrentIndexOnce)
{
    // Arrange：监听信号
    QSignalSpy spy(model, &ThemeListModel::requestCurrentIndex);
    ASSERT_TRUE(spy.isValid());

    // Act：命中排序后第 2 行（mid，行号 1）
    model->setSelection(QStringLiteral("/ut-fake-themes/mid.theme"));

    // Assert：恰好发射一次，携带行号 1 的索引（命中即 break）
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toModelIndex().row(), 1);
    EXPECT_EQ(spy.at(0).at(0).toModelIndex().column(), 0);
}

TEST_F(ThemeListModelTest, SetSelection_UnknownPath_EmitsNothingAndKeepsRows)
{
    // Arrange
    QSignalSpy spy(model, &ThemeListModel::requestCurrentIndex);
    ASSERT_TRUE(spy.isValid());
    const int rowsBefore = model->rowCount(QModelIndex());

    // Act：未命中路径（循环完整走完，无 break）
    model->setSelection(QStringLiteral("/ut-fake-themes/nonexistent.theme"));

    // Assert：不发射信号；模型状态未损坏（强异常安全）
    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(model->rowCount(QModelIndex()), rowsBefore);
    EXPECT_EQ(model->rowCount(QModelIndex()), 3);
}

}  // namespace
