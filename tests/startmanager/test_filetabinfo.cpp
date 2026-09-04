// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// FileTabInfo（src/startmanager.h）单元测试
//
// 类特征：纯聚合数据结构（POD，2 个 int 字段 windowIndex/tabIndex），无成员函数、
// 无信号。测试维度为聚合初始化语义、字段读写、拷贝独立性、按值传参语义、
// 哨兵值（-1）约定（StartManager::getFileTabInfo 未找到时返回 {-1,-1}）。
//
// 方法清单完成情况：
// | 1 | 公开方法 ≥1 用例：无成员方法（POD），按字段语义组织用例 | N/A |
// | 2 | 等价类划分：合法索引(>=0) / 哨兵(-1) / 边界值(INT_MAX/INT_MIN) | 完成 |
// | 3 | 边界值显式覆盖：0、-1、INT_MAX | 完成 |
// | 4 | TEST_P 参数化（≥3 组同质输入） | 完成 |
// | 5-7 | 分支/异常：POD 无分支无异常 | N/A |
// | 8-9 | 负面场景：哨兵值语义即负面路径 | 完成 |
// | 10 | stub 选择：无外部依赖，无需 stub | N/A |
//
// 分支清单（来源：struct FileTabInfo，无控制流分支）：
// - 无 if/switch/throw；字段语义即全部"状态空间"。
//
// 用例映射：
// - AggregateInit_TwoFields_HoldExactValues            → 初始化语义
// - FieldReadWrite_RoundTrip_PreservesValues           → setter/getter 语义（直接字段访问）
// - CopySemantics_IndependentCopy_DoesNotAliasOriginal → 拷贝独立
// - AssignmentOverwrites_PreviousValues_UpdatedAtoms   → 覆盖赋值
// - BoundaryIndices_ExtremeValues_HoldExact            → 边界值（TEST_P）
// - SentinelValues_NotFoundConvention_MinusOne         → 哨兵约定

#include <gtest/gtest.h>

#include "startmanager.h"

#include <limits>

// 边界值参数组：合法 0 / 典型正值 / 哨兵 -1 / int 极值
struct FileTabInfoBoundaryCase {
    int windowIndex;
    int tabIndex;
};

class FileTabInfoBoundaryTest : public ::testing::TestWithParam<FileTabInfoBoundaryCase> {
};

TEST_P(FileTabInfoBoundaryTest, BoundaryIndices_ExtremeValues_HoldExact)
{
    const FileTabInfoBoundaryCase &c = GetParam();

    // Arrange
    StartManager::FileTabInfo info;

    // Act
    info.windowIndex = c.windowIndex;
    info.tabIndex = c.tabIndex;

    // Assert：字段精确往返（两个维度）
    EXPECT_EQ(info.windowIndex, c.windowIndex);
    EXPECT_EQ(info.tabIndex, c.tabIndex);
}

INSTANTIATE_TEST_SUITE_P(
    FileTabInfoCases,
    FileTabInfoBoundaryTest,
    ::testing::Values(
        FileTabInfoBoundaryCase{0, 0},                                    // 下边界：首个窗口首个标签
        FileTabInfoBoundaryCase{3, 7},                                    // 典型值
        FileTabInfoBoundaryCase{-1, -1},                                  // 哨兵：未找到
        FileTabInfoBoundaryCase{std::numeric_limits<int>::max(), std::numeric_limits<int>::max()},   // 上边界
        FileTabInfoBoundaryCase{std::numeric_limits<int>::min(), std::numeric_limits<int>::min()})); // 负边界

TEST(FileTabInfoTest, AggregateInit_TwoFields_HoldExactValues)
{
    // Arrange & Act
    StartManager::FileTabInfo info{2, 5};

    // Assert：聚合初始化按声明序赋值（两个维度交叉验证）
    EXPECT_EQ(info.windowIndex, 2);
    EXPECT_EQ(info.tabIndex, 5);
}

TEST(FileTabInfoTest, FieldReadWrite_RoundTrip_PreservesValues)
{
    // Arrange
    StartManager::FileTabInfo info{0, 0};

    // Act：字段重写（getter/setter 语义：直接字段访问）
    info.windowIndex = 4;
    info.tabIndex = 9;

    // Assert
    EXPECT_EQ(info.windowIndex, 4);
    EXPECT_EQ(info.tabIndex, 9);
}

TEST(FileTabInfoTest, CopySemantics_IndependentCopy_DoesNotAliasOriginal)
{
    // Arrange
    StartManager::FileTabInfo original{1, 3};

    // Act：拷贝后修改副本
    StartManager::FileTabInfo copy = original;
    copy.windowIndex = 8;
    copy.tabIndex = 12;

    // Assert：原对象不受副本修改影响（值语义）
    EXPECT_EQ(original.windowIndex, 1);
    EXPECT_EQ(original.tabIndex, 3);
    EXPECT_EQ(copy.windowIndex, 8);
}

TEST(FileTabInfoTest, AssignmentOverwrites_PreviousValues_UpdatedAtoms)
{
    // Arrange
    StartManager::FileTabInfo info{6, 6};
    const StartManager::FileTabInfo source{0, 1};

    // Act
    info = source;

    // Assert：赋值后逐字段等于源
    EXPECT_EQ(info.windowIndex, 0);
    EXPECT_EQ(info.tabIndex, 1);
}

TEST(FileTabInfoTest, SentinelValues_NotFoundConvention_MinusOne)
{
    // Arrange & Act：与 StartManager::getFileTabInfo 的"未找到"返回值同构
    StartManager::FileTabInfo info{-1, -1};

    // Assert：哨兵约定（两个维度：窗口与标签索引均为 -1）
    EXPECT_EQ(info.windowIndex, -1);
    EXPECT_EQ(info.tabIndex, -1);
}

TEST(FileTabInfoTest, ValueParameterPassing_ByValueCopy_Preserved)
{
    // Arrange：按值传参辅助 lambda（popupExistTabs(FileTabInfo) 即按值传递）
    auto tabIndexOrSentinel = [](StartManager::FileTabInfo info) {
        return info.tabIndex >= 0 ? info.tabIndex : -1;
    };
    StartManager::FileTabInfo found{0, 4};
    StartManager::FileTabInfo notFound{-1, -1};

    // Act & Assert：按值拷贝在函数内保持字段
    EXPECT_EQ(tabIndexOrSentinel(found), 4);
    EXPECT_EQ(tabIndexOrSentinel(notFound), -1);
}
