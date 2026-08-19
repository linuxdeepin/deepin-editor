// SPDX-FileCopyrightText: 2026 UnionCTechnology Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_scrollsync.h"
#include "scrollsync.h"

UT_ScrollSync::UT_ScrollSync()
{
}

// value=min → ratio=0
TEST_F(UT_ScrollSync, RatioFromBar_AtMin_Zero)
{
    EXPECT_DOUBLE_EQ(ScrollSync::ratioFromScrollBar(0, 0, 100), 0.0);
}

// value=max → ratio=1
TEST_F(UT_ScrollSync, RatioFromBar_AtMax_One)
{
    EXPECT_DOUBLE_EQ(ScrollSync::ratioFromScrollBar(100, 0, 100), 1.0);
}

// value=mid → ratio=0.5
TEST_F(UT_ScrollSync, RatioFromBar_MidPoint_Half)
{
    EXPECT_DOUBLE_EQ(ScrollSync::ratioFromScrollBar(50, 0, 100), 0.5);
}

// max=0（无滚动范围）→ ratio=0（除零保护）
TEST_F(UT_ScrollSync, RatioFromBar_MaxZero_NoDivByZero)
{
    EXPECT_DOUBLE_EQ(ScrollSync::ratioFromScrollBar(0, 0, 0), 0.0);
}

// value < min → 钳制 0
TEST_F(UT_ScrollSync, RatioFromBar_BelowMin_ClampedZero)
{
    EXPECT_DOUBLE_EQ(ScrollSync::ratioFromScrollBar(-10, 0, 100), 0.0);
}

// value > max → 钳制 1
TEST_F(UT_ScrollSync, RatioFromBar_AboveMax_ClampedOne)
{
    EXPECT_DOUBLE_EQ(ScrollSync::ratioFromScrollBar(150, 0, 100), 1.0);
}

// 非 0 起点 min：ratio 按 (value-min)/(max-min) 计算
TEST_F(UT_ScrollSync, RatioFromBar_NonZeroMin_Normalized)
{
    EXPECT_DOUBLE_EQ(ScrollSync::ratioFromScrollBar(60, 10, 110), 0.5);
}

// clampRatio：越界值钳制 [0,1]
TEST_F(UT_ScrollSync, ClampRatio_OutOfRange_Clamped)
{
    EXPECT_DOUBLE_EQ(ScrollSync::clampRatio(-0.5), 0.0);
    EXPECT_DOUBLE_EQ(ScrollSync::clampRatio(1.5), 1.0);
    EXPECT_DOUBLE_EQ(ScrollSync::clampRatio(0.3), 0.3);
    EXPECT_DOUBLE_EQ(ScrollSync::clampRatio(0.0), 0.0);
    EXPECT_DOUBLE_EQ(ScrollSync::clampRatio(1.0), 1.0);
}
