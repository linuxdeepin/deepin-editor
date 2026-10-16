// SPDX-FileCopyrightText: 2026 UnionCTechnology Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "stubext.h"
#include "scrollsync.h"

#include <climits>

// 分支清单（来源：scrollsync.h，VS Code 式行号同步纯函数）
// clampLine:
//   C1: line < lo → lo
//   C2: line > hi → hi
//   C3: [lo,hi] 区间 → 原值
//   C4: 退化区间 lo > hi → lo
// scrollValueForLine:
//   S1: 带小数四舍五入（<0.5 舍 / >=0.5 入）
//   S2: 越下界 → min
//   S3: 越上界 → max
//   S4: 退化区间 min > max → min（除零/负区间保护）
//   S5: 区间内整数值 → 原值
//
// 用例映射：
// - ClampLine_BoundaryValues_ClampsToRange（TEST_P 7 组）                 → C1/C2/C3
// - ClampLine_DegenerateRange_ReturnsLo（TEST_F）                          → C4
// - ScrollValueForLine_FractionalLines_RoundsHalfUp（TEST_P 4 组）         → S1/S5
// - ScrollValueForLine_OutOfRange_ClampsToRange（TEST_P 4 组）             → S2/S3
// - ScrollValueForLine_DegenerateRange_ReturnsMin（TEST_P 3 组）           → S4
//
// 最小清单自检：1 每公开方法≥1用例 ✔（2 方法全覆盖）2 等价类 ✔ 3 边界值显式成组 ✔
// 4 TEST_P ×3（各≥3 组）✔ 5 分支映射 ✔ 6 全分支覆盖 ✔ 7 无异常 8 负面（退化区间/越界）✔
// 9 纯函数无状态 ✔ 10 无外部依赖，无需 stub ✔

// —— C1/C2/C3：区间钳制边界值成组 ——
namespace {
struct ClampCase {
    double input;
    double expected;
};
const ClampCase kClampCases[] = {
    { -5.0, 0.0 },     // 下越界（负行号）
    { -0.5, 0.0 },     // 贴近下界的负值
    { 0.0, 0.0 },      // 下边界
    { 42.5, 42.5 },    // 区间内（小数原样保留）
    { 100.0, 100.0 },  // 上边界
    { 100.5, 100.0 },  // 贴近上界的越界
    { 250.0, 100.0 },  // 上越界
};
} // namespace

class ClampLineParamTest : public ::testing::TestWithParam<ClampCase> {
};

TEST_P(ClampLineParamTest, ClampLine_BoundaryValues_ClampsToRange)
{
    // Arrange：固定区间 [0, 100]
    const auto &c = GetParam();

    // Act
    const double out = ScrollSync::clampLine(c.input, 0.0, 100.0);

    // Assert
    EXPECT_NEAR(out, c.expected, 1e-12);   // 期望值见参数表
    EXPECT_GE(out, 0.0);                   // 恒 >= lo
    EXPECT_LE(out, 100.0);                 // 恒 <= hi
}

INSTANTIATE_TEST_SUITE_P(ClampBoundaries, ClampLineParamTest,
                         ::testing::ValuesIn(kClampCases));

// —— C4：退化区间（lo > hi）返回 lo ——
TEST(ScrollSyncStaticTest, ClampLine_DegenerateRange_ReturnsLo)
{
    EXPECT_NEAR(ScrollSync::clampLine(50.0, 100.0, 0.0), 100.0, 1e-12);
    EXPECT_NEAR(ScrollSync::clampLine(-1.0, 10.0, 5.0), 10.0, 1e-12);
}

// —— S1/S5：小数四舍五入与整数原值 ——
namespace {
struct RoundCase {
    double line;
    int expected;
};
const RoundCase kRoundCases[] = {
    { 10.4, 10 },   // 小数部分 < 0.5 → 舍
    { 10.5, 11 },   // 小数部分 = 0.5 → 入（半值远离零方向）
    { 10.7, 11 },   // 小数部分 > 0.5 → 入
    { 7.0, 7 },     // 整数 → 原值
};
} // namespace

class RoundLineParamTest : public ::testing::TestWithParam<RoundCase> {
};

TEST_P(RoundLineParamTest, ScrollValueForLine_FractionalLines_RoundsHalfUp)
{
    // Arrange：宽区间 [0, 1000]，排除钳制干扰只验证取整
    const auto &c = GetParam();

    // Act
    const int out = ScrollSync::scrollValueForLine(c.line, 0, 1000);

    // Assert
    EXPECT_EQ(out, c.expected);
}

INSTANTIATE_TEST_SUITE_P(RoundLines, RoundLineParamTest,
                         ::testing::ValuesIn(kRoundCases));

// —— S2/S3：越界钳制 ——
namespace {
struct OutOfRangeCase {
    double line;
    int min;
    int max;
    int expected;
};
const OutOfRangeCase kOutOfRangeCases[] = {
    { -3.7, 0, 200, 0 },      // 下越界（带小数）→ min
    { 0.2, 0, 200, 0 },       // 越下界四舍五入前先钳制（非 0）
    { 205.9, 0, 200, 200 },   // 上越界 → max
    { 100.0, 10, 90, 90 },    // 非零 min 下越上界
};
} // namespace

class OutOfRangeParamTest : public ::testing::TestWithParam<OutOfRangeCase> {
};

TEST_P(OutOfRangeParamTest, ScrollValueForLine_OutOfRange_ClampsToRange)
{
    const auto &c = GetParam();
    EXPECT_EQ(ScrollSync::scrollValueForLine(c.line, c.min, c.max), c.expected);
}

INSTANTIATE_TEST_SUITE_P(OutOfRangeClamp, OutOfRangeParamTest,
                         ::testing::ValuesIn(kOutOfRangeCases));

// —— S4：退化区间（min >= max）→ min ——
namespace {
struct DegenerateCase {
    double line;
    int min;
    int max;
};
const DegenerateCase kDegenerateCases[] = {
    { 5.0, 10, 10 },    // min == max
    { 50.0, 10, 5 },    // min > max
    { -7.5, 0, -3 },    // max 为负
};
} // namespace

class DegenerateRangeParamTest : public ::testing::TestWithParam<DegenerateCase> {
};

TEST_P(DegenerateRangeParamTest, ScrollValueForLine_DegenerateRange_ReturnsMin)
{
    const auto &c = GetParam();
    EXPECT_EQ(ScrollSync::scrollValueForLine(c.line, c.min, c.max), c.min);
}

INSTANTIATE_TEST_SUITE_P(DegenerateRanges, DegenerateRangeParamTest,
                         ::testing::ValuesIn(kDegenerateCases));
