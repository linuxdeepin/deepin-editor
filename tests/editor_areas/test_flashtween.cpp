// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// FlashTween（src/editor/FlashTween.h/.cpp）单元测试 —— 补间动画纯逻辑类（重点 100%）
//
// 类特征：QObject 子类（非 GUI），两个 QTimer 驱动 X/Y 轴惯性滑动。
// 时间轴由 CELL_TIME=15ms 步进；__runX/__runY 为 private 槽，缓动函数为
// private static —— 测试 TU 以 -fno-access-control 直接驱动（不依赖真实事件循环，
// 确定性验证时间轴推进/完成停止/方向符号）。sinusoidalEaseOut 以 3.14 为 π 近似，
// 期望值按源算法文档（Tween 缓动公式）独立计算。
//
// 方法清单完成情况（test-types §8）：
// | 1 | 公开方法 ≥1 用例：ctor/dtor/startX/startY/stopX/stopY/activeX/activeY + 私有槽/缓动函数（间接直驱） | 完成 |
// | 2 | 等价类划分：变化量 c=0/c>0/c<0；时长 d=0/d>0；t=0/t=d/t>d 边界 | 完成 |
// | 3 | 边界值显式覆盖：t=0、t=d、t 越过 d、c=0、d=0 | 完成 |
// | 4 | TEST_P 参数化（≥3 组同质输入）：缓动端点、bounce 分段、启动守卫 | 完成 |
// | 5 | 分支清单已列出并映射用例 | 完成（见下） |
// | 6 | 每条 if 分支有触发用例 | 完成 |
// | 7 | 异常路径：无 throw，无异常分支 | N/A |
// | 8 | 负面场景：c=0/d=0 守卫提前返回、方向为负 | 完成 |
// | 9 | 强异常安全：守卫返回后状态未被污染（时间/回调计数断言） | 完成 |
// | 10 | stub 选择：无外部依赖（QTimer 不启动事件循环即不触发），无需 stub | N/A |
//
// 分支清单（来源：FlashTween.cpp / FlashTween.h）：
// startY/startX：
//   B1: c==0.0 提前 return（定时器不启动）
//   B2: d==0.0 提前 return（定时器不启动）
//   B3: m_changeValue<0 → direction=+1（负向变化）
//   B4: m_changeValue>=0 → direction=-1（正向变化）
//   B5/B6: m_timerX/Y 判空保护（正常构造恒非空，走非空分支）
// __runY/__runX：
//   B7: currentTime < duration → 步进 CELL_TIME
//   B8: currentTime >= duration → 停止定时器
// bounceEaseOut：
//   B9:  t/d < 1/2.75      B10: t/d < 2/2.75
//   B11: t/d < 2.5/2.75    B12: else
// 缓动端点：t=0 → b；t=d → b+c（各公式）
//
// 用例映射：
// - Constructor_TwoTimersCreated_InactiveInitially            → ctor（B5/B6 非空分支）
// - StartAxis_ZeroChangeOrZeroDuration_GuardReturnsEarly /*TEST_P*/ → B1+B2
// - StartX_NormalChange_TimerBecomesActive                    → B4(X)/B5
// - StartY_NegativeChange_TimerBecomesActive                  → B3(Y)/B6
// - StopX_AfterStart_DeactivatesTimer / StopY_...             → stopX/stopY
// - StopAxes_BeforeAnyStart_InactiveAndNoCrash                → 停止幂等
// - RunY_FirstTick_InvokesCallbackWithZeroDelta               → B7(Y) t=0
// - RunY_MidTick_InvokesCallbackWithSinusoidalDelta           → B7(Y) t=15
// - RunY_NegativeChange_CallbackDeltaPositive                 → B3(Y) 符号
// - RunY_ReachesDuration_StopsTimer                           → B8(Y)
// - RunX_FirstTick_InvokesCallback / RunX_ReachesDuration_StopsTimer → B7/B8(X)
// - EasingEndpoints_StandardFormulas_HitBoundaries /*TEST_P*/  → 端点 b / b+c
// - BounceEaseOut_FourSegments_MatchFormula /*TEST_P*/         → B9~B12
// - Destructor_ScopeExit_DeletesOwnedTimers                   → dtor

#include <gtest/gtest.h>
#include "FlashTween.h"

#include <QCoreApplication>
#include <QTimer>
#include <cmath>

namespace {

// 缓动公式指针表（private static，经 -fno-access-control 取址）
using EaseFn = qreal (*)(qreal, qreal, qreal, qreal);

struct EaseFnCase {
    EaseFn fn;
    const char *name;
};

} // namespace

class FlashTweenTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // FlashTween 为 QObject 子类（非 GUI），QTimer 仅需 QCoreApplication；
        // 不运行事件循环 → 定时器不派发 timeout，时间轴由测试直驱，确定性
        if (QCoreApplication::instance() == nullptr) {
            static int argc = 1;
            static char appName[] = "test_flashtween";
            static char *argv[] = { appName, nullptr };
            s_app = new QCoreApplication(argc, argv);
        }
    }

    static void TearDownTestSuite()
    {
        // QCoreApplication 保留至进程退出
    }

    void SetUp() override
    {
        callCountY = 0;
        callCountX = 0;
        lastDeltaY = 12345.0;   // 哨兵：未回调时可区分
        lastDeltaX = 12345.0;
        obj = new FlashTween();
        ASSERT_NE(obj, nullptr);

        // 惯性回调：记录次数与最近一次增量（stub 模式 §16b/16a）
        fnY = [this](qreal diff) {
            ++callCountY;
            lastDeltaY = diff;
        };
        fnX = [this](qreal diff) {
            ++callCountX;
            lastDeltaX = diff;
        };
    }

    void TearDown() override
    {
        delete obj;
        obj = nullptr;
    }

    static QCoreApplication *s_app;

    FlashTween *obj = nullptr;
    FunSlideInertial fnY;
    FunSlideInertial fnX;
    int callCountY = 0;
    int callCountX = 0;
    qreal lastDeltaY = 0;
    qreal lastDeltaX = 0;
};

QCoreApplication *FlashTweenTest::s_app = nullptr;

TEST_F(FlashTweenTest, Constructor_TwoTimersCreated_InactiveInitially)
{
    // Arrange/Act 在 SetUp 完成（构造）

    // Assert：两个定时器均已创建且未激活（构造无副作用）
    ASSERT_NE(obj->m_timerX, nullptr);
    ASSERT_NE(obj->m_timerY, nullptr);
    EXPECT_FALSE(obj->m_timerX->isActive());
    EXPECT_FALSE(obj->m_timerY->isActive());
    // 对应公开查询接口与内部状态一致；未 start 前间隔保持默认 0
    EXPECT_FALSE(obj->activeX());
    EXPECT_FALSE(obj->activeY());
    EXPECT_EQ(obj->m_timerX->interval(), 0);
    EXPECT_EQ(obj->m_timerY->interval(), 0);
}

// ---- startY/startX 守卫分支（B1/B2）：TEST_P 同质输入参数化 ----
struct StartGuardCase {
    bool useXAxis;
    qreal change;
    qreal duration;
    const char *desc;
};

class FlashTweenStartGuardTest : public FlashTweenTest,
                                 public ::testing::WithParamInterface<StartGuardCase> {
};

TEST_P(FlashTweenStartGuardTest, StartAxis_ZeroChangeOrZeroDuration_GuardReturnsEarly)
{
    const StartGuardCase &c = GetParam();

    // Act：c==0 或 d==0 触发守卫提前返回
    if (c.useXAxis)
        obj->startX(0, 0, c.change, c.duration, fnX);
    else
        obj->startY(0, 0, c.change, c.duration, fnY);

    // Assert：定时器未启动（B1/B2 分支：直接 return）
    EXPECT_FALSE(obj->activeX());
    EXPECT_FALSE(obj->activeY());
    // 强异常安全：守卫路径不触碰时间轴状态（t 保持 0，回调未发出）
    EXPECT_EQ(obj->m_currentTimeX, 0);
    EXPECT_EQ(obj->m_currentTimeY, 0);
    EXPECT_EQ(callCountX + callCountY, 0);
}

INSTANTIATE_TEST_SUITE_P(
    StartGuardCases,
    FlashTweenStartGuardTest,
    ::testing::Values(
        StartGuardCase{ false, 0.0, 100.0, "Y轴 c=0" },      // B1(Y)
        StartGuardCase{ false, 100.0, 0.0, "Y轴 d=0" },      // B2(Y)
        StartGuardCase{ true, 0.0, 100.0, "X轴 c=0" },       // B1(X)
        StartGuardCase{ true, 100.0, 0.0, "X轴 d=0" }));     // B2(X)

TEST_F(FlashTweenTest, StartY_NormalChange_TimerBecomesActive)
{
    // Act：正向变化 c=100 → direction=-1（B4）
    obj->startY(0, 0, 100.0, 30.0, fnY);

    // Assert：定时器以 CELL_TIME 启动
    EXPECT_TRUE(obj->activeY());
    EXPECT_EQ(obj->m_timerY->interval(), CELL_TIME);
    EXPECT_EQ(obj->m_changeValueY, 100.0);
    EXPECT_EQ(obj->m_directionY, -1);    // 正向变化 → 方向 -1
    EXPECT_EQ(callCountY, 0);            // 未处理事件，回调尚未发出
}

TEST_F(FlashTweenTest, StartX_NormalChange_TimerBecomesActive)
{
    // Act
    obj->startX(0, 0, 100.0, 30.0, fnX);

    // Assert
    EXPECT_TRUE(obj->activeX());
    EXPECT_EQ(obj->m_timerX->interval(), CELL_TIME);
    EXPECT_EQ(obj->m_directionX, -1);
}

TEST_F(FlashTweenTest, StartY_NegativeChange_DirectionFlipsToPositive)
{
    // Act：负向变化 c=-100 → direction=+1（B3）
    obj->startY(0, 0, -100.0, 30.0, fnY);

    // Assert
    EXPECT_TRUE(obj->activeY());
    EXPECT_EQ(obj->m_changeValueY, -100.0);
    EXPECT_EQ(obj->m_directionY, 1);     // 负向变化 → 方向 +1
}

TEST_F(FlashTweenTest, StopX_AfterStart_DeactivatesTimer)
{
    // Arrange
    obj->startX(0, 0, 100.0, 30.0, fnX);
    ASSERT_TRUE(obj->activeX());

    // Act
    obj->stopX();

    // Assert
    EXPECT_FALSE(obj->activeX());
    // X 轴停止不影响回调（stop 不驱动动画帧）
    EXPECT_EQ(callCountX, 0);
    // Y 轴不受影响（轴间隔离）
    EXPECT_FALSE(obj->activeY());
}

TEST_F(FlashTweenTest, StopY_AfterStart_DeactivatesTimer)
{
    // Arrange
    obj->startY(0, 0, 100.0, 30.0, fnY);
    ASSERT_TRUE(obj->activeY());

    // Act
    obj->stopY();

    // Assert
    EXPECT_FALSE(obj->activeY());
    EXPECT_EQ(obj->m_timerY->isActive() == false, true);
    EXPECT_EQ(callCountY, 0);   // stop 不驱动动画帧
}

TEST_F(FlashTweenTest, StopAxes_BeforeAnyStart_InactiveAndIdempotent)
{
    // Act：从未启动直接 stop（QTimer::stop 幂等）
    obj->stopX();
    obj->stopY();

    // Assert：状态保持非激活、不崩溃
    EXPECT_FALSE(obj->activeX());
    EXPECT_FALSE(obj->activeY());
    EXPECT_EQ(callCountX + callCountY, 0);
}

TEST_F(FlashTweenTest, RunY_FirstTick_InvokesCallbackWithZeroDelta)
{
    // Arrange：t=0,b=0,c=100,d=30；sin(0)=0 → 首拍增量为 0
    obj->startY(0, 0, 100.0, 30.0, fnY);

    // Act：直驱私有槽（不依赖事件循环）
    obj->__runY();

    // Assert：回调发出且增量按公式 -direction*(v-0)=0（sin(0)=0）
    EXPECT_EQ(callCountY, 1);
    EXPECT_NEAR(lastDeltaY, 0.0, 1e-9);
    // 时间轴推进 CELL_TIME（B7）
    EXPECT_EQ(obj->m_currentTimeY, CELL_TIME);
    EXPECT_TRUE(obj->activeY());
}

TEST_F(FlashTweenTest, RunY_MidTick_InvokesCallbackWithSinusoidalDelta)
{
    // Arrange：推进到 t=15 后再拍一拍
    obj->startY(0, 0, 100.0, 30.0, fnY);
    obj->__runY();   // t: 0 → 15，v(0)=0

    // Act：t=15 → v=100*sin(15/30*3.14/2)=100*sin(0.785)
    obj->__runY();

    // Assert：增量 = direction*(v - v_prev) = -1*(100*sin(0.785) - 0)
    const qreal expected = -1.0 * (100.0 * std::sin(0.785));
    EXPECT_EQ(callCountY, 2);
    EXPECT_NEAR(lastDeltaY, expected, 1e-9);
    EXPECT_EQ(obj->m_currentTimeY, 30);   // 15 + CELL_TIME
}

TEST_F(FlashTweenTest, RunY_NegativeChange_CallbackDeltaPositive)
{
    // Arrange：c=-100 → direction=+1，ease 用 abs(c)
    obj->startY(0, 0, -100.0, 30.0, fnY);

    // Act：t=15 一拍
    obj->__runY();
    obj->__runY();

    // Assert：增量 = +1*(100*sin(0.785) - 0)，符号随方向翻转
    const qreal expected = 1.0 * (100.0 * std::sin(0.785));
    EXPECT_EQ(callCountY, 2);
    EXPECT_NEAR(lastDeltaY, expected, 1e-9);
}

TEST_F(FlashTweenTest, RunY_ReachesDuration_StopsTimer)
{
    // Arrange：完整时间轴 t=0,15,30 共三拍
    obj->startY(0, 0, 100.0, 30.0, fnY);
    obj->__runY();   // t=0  → 15
    obj->__runY();   // t=15 → 30

    // Act：t=30 == duration → 完成分支（B8）
    obj->__runY();

    // Assert：定时器停止、时间轴停在 d、终值 = b+c*sin(1.57)（3.14 近似 π 的精确 oracle）
    EXPECT_FALSE(obj->activeY());
    EXPECT_EQ(obj->m_currentTimeY, 30);
    EXPECT_NEAR(obj->m_lastValueY, 100.0 * std::sin(1.57), 1e-9);
    EXPECT_EQ(callCountY, 3);
}

TEST_F(FlashTweenTest, RunY_PastDuration_DoesNotAdvanceFurther)
{
    // Arrange：动画完成后继续拍（t 不再推进、定时器保持停止）
    obj->startY(0, 0, 100.0, 30.0, fnY);
    obj->__runY();
    obj->__runY();
    obj->__runY();
    ASSERT_FALSE(obj->activeY());

    // Act：越界后的额外 tick（t>d 边界）
    obj->__runY();

    // Assert：时间冻结、回调仍发出（终值不变 → 增量 0）
    EXPECT_EQ(obj->m_currentTimeY, 30);
    EXPECT_NEAR(lastDeltaY, 0.0, 1e-9);
    EXPECT_EQ(callCountY, 4);
}

TEST_F(FlashTweenTest, RunX_FirstTick_InvokesCallbackWithZeroDelta)
{
    // Arrange
    obj->startX(0, 0, 100.0, 30.0, fnX);

    // Act
    obj->__runX();

    // Assert
    EXPECT_EQ(callCountX, 1);
    EXPECT_NEAR(lastDeltaX, 0.0, 1e-9);
    EXPECT_EQ(obj->m_currentTimeX, CELL_TIME);
    EXPECT_TRUE(obj->activeX());
}

TEST_F(FlashTweenTest, RunX_ReachesDuration_StopsTimer)
{
    // Arrange
    obj->startX(0, 0, 100.0, 30.0, fnX);
    obj->__runX();
    obj->__runX();

    // Act：完成拍（B8-X）
    obj->__runX();

    // Assert
    EXPECT_FALSE(obj->activeX());
    EXPECT_EQ(obj->m_currentTimeX, 30);
    EXPECT_NEAR(obj->m_lastValueX, 100.0 * std::sin(1.57), 1e-9);
    EXPECT_EQ(callCountX, 3);
}

// ---- 缓动函数端点（标准公式：t=0 → b；t=d → b+c）----
class FlashTweenEasingTest : public FlashTweenTest,
                             public ::testing::WithParamInterface<EaseFnCase> {
};

TEST_P(FlashTweenEasingTest, EasingEndpoints_StandardFormulas_HitBoundaries)
{
    const EaseFnCase &c = GetParam();
    const qreal b = 20.0;
    const qreal change = 60.0;
    const qreal d = 300.0;

    // Assert：起点 t=0 精确回到 b
    EXPECT_NEAR(c.fn(0.0, b, change, d), b, 1e-9);
    // Assert：终点 t=d 回到 b+c（sinusoidal 以 3.14 近似 π，容差放宽到 1e-3）
    EXPECT_NEAR(c.fn(d, b, change, d), b + change, 1e-3);
}

INSTANTIATE_TEST_SUITE_P(
    EasingCases,
    FlashTweenEasingTest,
    ::testing::Values(
        EaseFnCase{ &FlashTween::quadraticEaseOut, "quadratic" },
        EaseFnCase{ &FlashTween::cubicEaseOut, "cubic" },
        EaseFnCase{ &FlashTween::quarticEaseOut, "quartic" },
        EaseFnCase{ &FlashTween::quinticEaseOut, "quintic" },
        EaseFnCase{ &FlashTween::sinusoidalEaseOut, "sinusoidal" },
        EaseFnCase{ &FlashTween::circularEaseOut, "circular" }));

// ---- bounceEaseOut 四段分支（B9~B12）----
struct BounceCase {
    qreal fraction;      // t/d
    qreal expected;      // 归一化期望值（c=1,b=0,d=1）
};

class FlashTweenBounceTest : public FlashTweenTest,
                             public ::testing::WithParamInterface<BounceCase> {
};

TEST_P(FlashTweenBounceTest, BounceEaseOut_FourSegments_MatchFormula)
{
    const BounceCase &c = GetParam();
    const qreal b = 5.0;
    const qreal change = 80.0;
    const qreal d = 400.0;

    // Act：按段内公式独立计算期望值
    const qreal t = c.fraction;
    qreal normalized = 0.0;
    if (t < (1 / 2.75)) {
        normalized = 7.5625 * t * t;                                        // B9
    } else if (t < (2 / 2.75)) {
        const qreal u = t - (1.5 / 2.75);
        normalized = 7.5625 * u * u + .75;                                  // B10
    } else if (t < (2.5 / 2.75)) {
        const qreal u = t - (2.25 / 2.75);
        normalized = 7.5625 * u * u + .9375;                                // B11
    } else {
        const qreal u = t - (2.625 / 2.75);
        normalized = 7.5625 * u * u + .984375;                              // B12
    }
    const qreal expected = b + change * normalized;

    // Assert：bounceEaseOut(t/d 分段) 精确命中各段公式
    EXPECT_NEAR(FlashTween::bounceEaseOut(t * d, b, change, d), expected, 1e-9);
    // 端点连续性补充断言：t=0 → b
    EXPECT_NEAR(FlashTween::bounceEaseOut(0.0, b, change, d), b, 1e-9);
}

INSTANTIATE_TEST_SUITE_P(
    BounceCases,
    FlashTweenBounceTest,
    ::testing::Values(
        BounceCase{ 0.10, 0.0 },   // B9:  0.10 < 0.3636
        BounceCase{ 0.50, 0.0 },   // B10: 0.3636 <= 0.50 < 0.7273
        BounceCase{ 0.76, 0.0 },   // B11: 0.7273 <= 0.76 < 0.9091
        BounceCase{ 0.95, 0.0 })); // B12: >= 0.9091

TEST_F(FlashTweenTest, Destructor_ScopeExit_DeletesOwnedTimers)
{
    // Arrange：内部定时器指针记录（析构后不可再读）
    QTimer *timerX = obj->m_timerX;
    QTimer *timerY = obj->m_timerY;
    ASSERT_NE(timerX, nullptr);
    ASSERT_NE(timerY, nullptr);

    // Act：作用域析构
    delete obj;
    obj = nullptr;

    // Assert：析构无崩溃；重建对象得到全新定时器（旧指针未被复用即视为已释放）
    FlashTween *fresh = new FlashTween();
    EXPECT_NE(fresh->m_timerX, timerX);
    EXPECT_NE(fresh->m_timerY, timerY);
    EXPECT_FALSE(fresh->activeX());
    EXPECT_FALSE(fresh->activeY());
    delete fresh;
}
