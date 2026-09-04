// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "stubext.h"
#include "scrollsync.h"

#include <climits>

// 分支清单（来源：scrollsync.h）
// clampRatio:
//   C1: ratio < 0.0 → 0.0
//   C2: ratio > 1.0 → 1.0
//   C3: [0,1] 区间 → 原值
// ratioFromScrollBar:
//   S1: max <= min → 0.0（除零保护，含 max<min 与相等）
//   S2: 正常 (value-min)/(max-min) → 钳制后值
//   S3: value 越出 [min,max] → 钳制到 0/1
//   S4: min 非 0（负 min 边界）
//
// 用例映射：
// - ClampRatio_BoundaryValues_ClampsToUnitInterval（TEST_P 7 组）      → C1/C2/C3
// - RatioFromScrollBar_DegenerateRange_ReturnsZero（TEST_P 3 组）      → S1
// - RatioFromScrollBar_NormalRanges_ReturnsNormalized（TEST_P 5 组）   → S2/S4
// - RatioFromScrollBar_ValueOutOfRange_ClampedToUnitInterval（TEST_P 3 组）→ S3
//
// 最小清单自检：1 每公开方法≥1用例 ✔（2 方法全覆盖）2 等价类 ✔ 3 边界值显式成组 ✔
// 4 TEST_P ×3（各≥3 组）✔ 5 分支映射 ✔ 6 全分支覆盖 ✔ 7 无异常 8 负面（退化区间/越界）✔
// 9 纯函数无状态 ✔ 10 无外部依赖，无需 stub ✔

// —— C1/C2/C3：[0,1] 钳制边界值成组 ——
namespace {
struct ClampCase {
    double input;
    double expected;
};
const ClampCase kClampCases[] = {
    { -0.5, 0.0 },     // 下越界
    { -0.001, 0.0 },   // 贴近下界的负值
    { 0.0, 0.0 },      // 下边界
    { 0.3, 0.3 },      // 区间内
    { 1.0, 1.0 },      // 上边界
    { 1.0001, 1.0 },   // 贴近上界的越界
    { 2.5, 1.0 },      // 上越界
};
} // namespace

class ClampRatioParamTest : public ::testing::TestWithParam<ClampCase> {
};

TEST_P(ClampRatioParamTest, ClampRatio_BoundaryValues_ClampsToUnitInterval)
{
    // Arrange
    const auto &c = GetParam();

    // Act
    const double out = ScrollSync::clampRatio(c.input);

    // Assert
    EXPECT_NEAR(out, c.expected, 1e-12);   // 期望值见参数表
    EXPECT_GE(out, 0.0);                   // 恒 >= 0
    EXPECT_LE(out, 1.0);                   // 恒 <= 1
}

INSTANTIATE_TEST_SUITE_P(ClampBoundaries, ClampRatioParamTest,
                         ::testing::ValuesIn(kClampCases));

// —— S1：退化区间（max<=min，除零保护）——
namespace {
struct BarCase {
    int value;
    int min;
    int max;
};
const BarCase kDegenerateCases[] = {
    { 0, 10, 10 },    // max == min
    { 5, 10, 5 },     // max < min
    { 7, 0, -3 },     // max 为负
};
} // namespace

class DegenerateBarParamTest : public ::testing::TestWithParam<BarCase> {
};

TEST_P(DegenerateBarParamTest, RatioFromScrollBar_DegenerateRange_ReturnsZero)
{
    // Arrange
    const auto &c = GetParam();

    // Act
    const double out = ScrollSync::ratioFromScrollBar(c.value, c.min, c.max);

    // Assert：除零保护路径恒 0（期望 0.0 边）
    EXPECT_DOUBLE_EQ(out, 0.0);
    EXPECT_GE(out, 0.0);
}

INSTANTIATE_TEST_SUITE_P(DegenerateRanges, DegenerateBarParamTest,
                         ::testing::ValuesIn(kDegenerateCases));

// —— S2/S4：正常区间（含非零 min）——
namespace {
struct RatioCase {
    int value;
    int min;
    int max;
    double expected;
};
const RatioCase kNormalCases[] = {
    { 0, 0, 100, 0.0 },     // 顶边界
    { 50, 0, 100, 0.5 },    // 中点
    { 100, 0, 100, 1.0 },   // 底边界
    { 10, 10, 110, 0.0 },   // 非零 min：value==min
    { 60, 10, 110, 0.5 },   // 非零 min：中点
};
} // namespace

class NormalBarParamTest : public ::testing::TestWithParam<RatioCase> {
};

TEST_P(NormalBarParamTest, RatioFromScrollBar_NormalRanges_ReturnsNormalized)
{
    // Arrange
    const auto &c = GetParam();

    // Act
    const double out = ScrollSync::ratioFromScrollBar(c.value, c.min, c.max);

    // Assert
    EXPECT_NEAR(out, c.expected, 1e-12);
    // 与 clampRatio 组合语义一致（独立公式复核）
    const double raw = double(c.value - c.min) / double(c.max - c.min);
    EXPECT_NEAR(out, ScrollSync::clampRatio(raw), 1e-12);
}

INSTANTIATE_TEST_SUITE_P(NormalRanges, NormalBarParamTest,
                         ::testing::ValuesIn(kNormalCases));

// —— S3：value 越出区间 → 钳制 ——
namespace {
const RatioCase kOutOfRangeCases[] = {
    { -50, 0, 100, 0.0 },    // 下越界
    { 200, 0, 100, 1.0 },    // 上越界
    { INT_MIN, 0, 100, 0.0 },// 极小值边界
};
} // namespace

class OutOfRangeBarParamTest : public ::testing::TestWithParam<RatioCase> {
};

TEST_P(OutOfRangeBarParamTest, RatioFromScrollBar_ValueOutOfRange_ClampedToUnitInterval)
{
    // Arrange
    const auto &c = GetParam();

    // Act
    const double out = ScrollSync::ratioFromScrollBar(c.value, c.min, c.max);

    // Assert
    EXPECT_NEAR(out, c.expected, 1e-12);
    EXPECT_LE(out, 1.0);
    EXPECT_GE(out, 0.0);
}

INSTANTIATE_TEST_SUITE_P(OutOfRangeValues, OutOfRangeBarParamTest,
                         ::testing::ValuesIn(kOutOfRangeCases));
