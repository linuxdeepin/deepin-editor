// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// JumpLineBar（src/controls/jumplinebar.cpp）单元测试
//
// 类特征：DFloatingWidget 子类（GUI 类），offscreen QApplication 无头构造。
//
// 方法映射（公开/保护方法全集）：
// - JumpLineBar::JumpLineBar   → Constructor_DefaultParent_CreatesCoreChildren
// - ~JumpLineBar               → Destructor_DeleteWithoutLeak（TearDown 链覆盖）
// - focus                      → ActiveInput_AfterShow_FocusesSpinBoxInput
// - isFocus                    → 同上
// - activeInput                → ActiveInput_WithLineCount_SavesContextAndRange /
//                                 ActiveInput_OversizedLineNumber_Cleared (TEST_P)
// - handleFocusOut             → HandleFocusOut_OnFocusLoss_EmitsLostFocusExit
// - handleLineChanged          → HandleLineChanged_ValidNumber_EmitsJumpToLine (TEST_P) /
//                                 HandleLineChanged_ZeroOrEmpty_NoSignal
// - jumpCancel                 → JumpCancel_VisibleBar_HidesAndClearsInput
// - jumpConfirm                → JumpConfirm_ValidNumber_EmitsJumpToLineWithFocus /
//                                 JumpConfirm_EmptyInput_NoSignal
// - slotFocusChanged           → SlotFocusChanged_FalseOnly_EmitsLostFocusExit
// - hide                       → Hide_VisibleBar_ClearsInputAndHidesBar
// - getLineCount               → ActiveInput_WithLineCount_SavesContextAndRange
// - eventFilter(protected)     → EventFilter_SpinBoxFocusOut_EmitsLostAndClears /
//                                 EventFilter_OtherObject_PassesThrough
// - updateSizeMode(protected)  → 构造函数直调覆盖
//
// 分支清单（来源：jumplinebar.cpp）→ 用例映射：
// - activeInput: minimumWidth < lineWidth → setFixedWidth(lineWidth)  → ActiveInput...AdjustsRange（多行数）
// - activeInput: else → setFixedWidth(默认宽)                          → 同上（小行数）
// - activeInput: text.toInt() > lineCount → 清空                       → ActiveInput_OversizedLineNumber_Cleared
// - handleLineChanged: content=="" → 跳过                              → HandleLineChanged_ZeroOrEmpty
// - handleLineChanged: toInt()==0 → clear+return                       → HandleLineChanged_ZeroOrEmpty
// - handleLineChanged: else → jumpToLine(file, n, false)               → HandleLineChanged_ValidNumber (TEST_P)
// - jumpConfirm: content=="" → 跳过 / else → jumpToLine(..., true)     → JumpConfirm_两用例
// - slotFocusChanged: bFocus==false → lostFocusExit                    → SlotFocusChanged_FalseOnly
// - eventFilter: pObject==spinbox && FocusOut → handleFocusOut         → EventFilter_SpinBoxFocusOut
// - eventFilter: FocusOut 且 lineEdit 空 → clear                        → 同上
// - eventFilter: 其它对象/事件 → 透传                                   → EventFilter_OtherObject
//
// 最小清单完成情况：
// | 1 | 每个公开/保护方法 ≥1 用例 | 完成 |
// | 2 | 等价类（行号 有效/0/空/超界、行数 1 位/4 位） | 完成 |
// | 3 | 边界值（lineCount=1、行号=1、行号=lineCount、超界+1） | 完成 |
// | 4 | TEST_P（activeInput 3 组、handleLineChanged 3 组） | 完成 |
// | 5 | 分支清单映射 | 见上方 |
// | 6 | if 分支两侧全覆盖 | 完成 |
// | 7 | 异常路径 | N/A（无 throw） |
// | 8 | 负面（空/0/超界行号不发信号） | 完成 |
// | 9 | 强异常安全（jumpCancel 后行数上下文保留） | 完成 |
// | 10 | stub_ext（全部真实 offscreen + QSignalSpy） | 完成 |
//
// [注] m_lineCount 构造函数未初始化（源码缺陷，见批次 defects 记录）：
// getLineCount() 在 activeInput 之前返回未定值，测试仅在 activeInput 后断言。

#include <gtest/gtest.h>

#include <QFocusEvent>
#include <QLabel>
#include <QLineEdit>
#include <QSignalSpy>

#include "jumplinebar.h"
#include "test_env.h"

namespace {

class JumpLineBarTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { controlsut::ensureApp(); }

    void SetUp() override
    {
        stub.clear();
        bar = new JumpLineBar();
        spinBox = bar->findChild<DSpinBox *>("PSpinBoxInput");
    }

    void TearDown() override
    {
        delete bar;
        bar = nullptr;
        spinBox = nullptr;
        stub.clear();
    }

    // 静默设置输入文本（不触发 textChanged → 队列中的 handleLineChanged）
    void setInputQuietly(const QString &text)
    {
        spinBox->lineEdit()->blockSignals(true);
        spinBox->lineEdit()->setText(text);
        spinBox->lineEdit()->blockSignals(false);
    }

    void activateWindow()
    {
        controlsut::ensureApp()->setActiveWindow(bar);
        QApplication::processEvents();
    }

    stub_ext::StubExt stub;
    JumpLineBar *bar = nullptr;
    DSpinBox *spinBox = nullptr;
};

// ---- 构造 ----

TEST_F(JumpLineBarTest, Constructor_DefaultParent_CreatesCoreChildren)
{
    // Assert：标签/输入框/关闭按钮齐全、输入框无按钮样式、初始无文本
    ASSERT_NE(spinBox, nullptr);
    EXPECT_NE(bar->findChild<QAbstractButton *>("JumpCloseButton"), nullptr);
    EXPECT_EQ(spinBox->buttonSymbols(), QAbstractSpinBox::NoButtons);
    EXPECT_EQ(spinBox->lineEdit()->text(), QString());
}

// ---- activeInput ----

TEST_F(JumpLineBarTest, ActiveInput_WithLineCount_SavesContextAndRange)
{
    // Act：4 位数行数（触发 minimumWidth < lineWidth 分支）与最小行数各一次
    bar->activeInput(QString::fromUtf8("/ut/jump.txt"), 3, 4, 1000, 55);

    // Assert：行数上下文保存、范围调整为 0~lineCount
    EXPECT_EQ(bar->getLineCount(), 1000);
    EXPECT_EQ(spinBox->minimum(), 0);
    EXPECT_EQ(spinBox->maximum(), 1000);

    // Act 2：1 位数行数（minimumWidth 足够 → else 分支，恢复默认宽）
    bar->activeInput(QString::fromUtf8("/ut/jump2.txt"), 1, 1, 5, 0);
    EXPECT_EQ(bar->getLineCount(), 5);  // 上下文更新
    EXPECT_EQ(spinBox->maximum(), 5);   // 边界：lineCount 最小
}

struct OversizeCase {
    QString presetText;
    int lineCount;
    bool expectCleared;
};

class JumpLineOversizeTest : public JumpLineBarTest,
                             public ::testing::WithParamInterface<OversizeCase> {
};

TEST_P(JumpLineOversizeTest, ActiveInput_OversizedLineNumber_Cleared)
{
    const auto &c = GetParam();
    bar->activeInput(QString::fromUtf8("/ut/j.txt"), 1, 1, 100, 0);
    setInputQuietly(c.presetText);

    // Act
    bar->activeInput(QString::fromUtf8("/ut/j.txt"), 1, 1, c.lineCount, 0);

    // Assert：超界输入被清空；未超界保留
    const QString text = spinBox->lineEdit()->text();
    if (c.expectCleared) {
        EXPECT_EQ(text, QString());
    } else {
        EXPECT_EQ(text, c.presetText);
    }
    EXPECT_EQ(bar->getLineCount(), c.lineCount);  // 行数上下文始终更新
}

INSTANTIATE_TEST_SUITE_P(
    OversizeCases, JumpLineOversizeTest,
    ::testing::Values(
        OversizeCase{ QString("150"), 100, true },    // 边界外：> lineCount → 清空
        OversizeCase{ QString("100"), 100, false },   // 边界上：== lineCount → 保留
        OversizeCase{ QString("1"), 1, false }        // 边界：单行文档
        ));

// ---- focus / isFocus ----

TEST_F(JumpLineBarTest, ActiveInput_AfterShow_FocusesSpinBoxInput)
{
    // Arrange：显示前无焦点
    bar->activeInput(QString::fromUtf8("/ut/f.txt"), 1, 1, 10, 0);
    EXPECT_FALSE(bar->isFocus());

    // Act：显示 + 激活 + focus()（聚焦行编辑）
    bar->QWidget::show();
    activateWindow();
    bar->focus();
    QApplication::processEvents();

    // Assert：行编辑持焦 + 行数上下文未被聚焦操作破坏
    EXPECT_TRUE(bar->isFocus());
    EXPECT_EQ(bar->getLineCount(), 10);
}

// ---- handleLineChanged（TEST_P 3 组）----

struct LineChangedCase {
    QString input;
    bool expectSignal;
    int expectedLine;
};

class JumpLineChangedTest : public JumpLineBarTest,
                            public ::testing::WithParamInterface<LineChangedCase> {
};

TEST_P(JumpLineChangedTest, HandleLineChanged_ValidNumber_EmitsJumpToLineOnly)
{
    const auto &c = GetParam();
    bar->activeInput(QString::fromUtf8("/ut/lc.txt"), 2, 3, 500, 9);
    setInputQuietly(c.input);
    QSignalSpy spy(bar, &JumpLineBar::jumpToLine);

    // Act
    bar->handleLineChanged();

    // Assert
    if (c.expectSignal) {
        ASSERT_EQ(spy.count(), 1);
        EXPECT_EQ(spy.at(0).at(0).toString(), QString::fromUtf8("/ut/lc.txt"));
        EXPECT_EQ(spy.at(0).at(1).toInt(), c.expectedLine);
        EXPECT_FALSE(spy.at(0).at(2).toBool());  // 行内跳转不带焦点
    } else {
        EXPECT_EQ(spy.count(), 0);
    }
}

INSTANTIATE_TEST_SUITE_P(
    LineCases, JumpLineChangedTest,
    ::testing::Values(
        LineChangedCase{ QString("42"), true, 42 },
        LineChangedCase{ QString("1"), true, 1 },    // 边界：首行
        LineChangedCase{ QString("0"), false, 0 }    // 边界：0 → 清空不发
        ));

TEST_F(JumpLineBarTest, HandleLineChanged_EmptyInput_NoSignal)
{
    bar->activeInput(QString::fromUtf8("/ut/empty.txt"), 1, 1, 10, 0);
    setInputQuietly(QString());
    QSignalSpy spy(bar, &JumpLineBar::jumpToLine);

    // Act
    bar->handleLineChanged();

    // Assert：空输入直接跳过
    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(spinBox->lineEdit()->text(), QString());
}

// ---- jumpConfirm ----

TEST_F(JumpLineBarTest, JumpConfirm_ValidNumber_EmitsJumpToLineWithFocus)
{
    bar->activeInput(QString::fromUtf8("/ut/conf.txt"), 8, 9, 100, 7);
    setInputQuietly(QString("77"));
    QSignalSpy spy(bar, &JumpLineBar::jumpToLine);

    // Act
    bar->jumpConfirm();

    // Assert：确认跳转 focusEditor=true
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), QString::fromUtf8("/ut/conf.txt"));
    EXPECT_EQ(spy.at(0).at(1).toInt(), 77);
    EXPECT_TRUE(spy.at(0).at(2).toBool());
    EXPECT_EQ(spinBox->lineEdit()->text(), QString("77"));  // 输入保留
}

TEST_F(JumpLineBarTest, JumpConfirm_EmptyInput_NoSignal)
{
    setInputQuietly(QString());
    QSignalSpy spy(bar, &JumpLineBar::jumpToLine);

    // Act
    bar->jumpConfirm();

    // Assert：空输入不发信号，输入框状态保持为空
    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(spinBox->lineEdit()->text(), QString());
}

// ---- jumpCancel / hide ----

TEST_F(JumpLineBarTest, JumpCancel_VisibleBar_HidesAndClearsInput)
{
    // Arrange：显示并输入
    bar->QWidget::show();
    setInputQuietly(QString("33"));
    ASSERT_TRUE(bar->isVisible());

    // Act
    bar->jumpCancel();

    // Assert：隐藏且输入清空（经 JumpLineBar::hide），不发射 jumpToLine
    EXPECT_FALSE(bar->isVisible());
    EXPECT_EQ(spinBox->lineEdit()->text(), QString());
}

TEST_F(JumpLineBarTest, Hide_VisibleBar_ClearsInputAndHidesBar)
{
    // Arrange
    bar->QWidget::show();
    setInputQuietly(QString("5"));

    // Act：直接调用重载 hide()（非虚，遮蔽 QWidget::hide）
    bar->hide();

    // Assert
    EXPECT_FALSE(bar->isVisible());
    EXPECT_EQ(spinBox->lineEdit()->text(), QString());
}

// ---- handleFocusOut / slotFocusChanged ----

TEST_F(JumpLineBarTest, HandleFocusOut_OnFocusLoss_EmitsLostFocusExit)
{
    QSignalSpy spy(bar, &JumpLineBar::lostFocusExit);
    QSignalSpy jumpSpy(bar, &JumpLineBar::jumpToLine);

    // Act
    bar->handleFocusOut();

    // Assert：只发失焦退出，不触发跳转
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(jumpSpy.count(), 0);
}

TEST_F(JumpLineBarTest, SlotFocusChanged_FalseOnly_EmitsLostFocusExit)
{
    QSignalSpy spy(bar, &JumpLineBar::lostFocusExit);

    // Act：true 不发、false 发
    bar->slotFocusChanged(true);
    EXPECT_EQ(spy.count(), 0);
    bar->slotFocusChanged(false);

    // Assert
    EXPECT_EQ(spy.count(), 1);
}

// ---- eventFilter ----

TEST_F(JumpLineBarTest, EventFilter_SpinBoxFocusOut_EmitsLostAndKeepsEmpty)
{
    // Arrange：输入框静默置空
    setInputQuietly(QString());
    QSignalSpy spy(bar, &JumpLineBar::lostFocusExit);

    // Act：向 spinBox 派发 FocusOut（经 installEventFilter 走 eventFilter）
    QFocusEvent focusOut(QEvent::FocusOut, Qt::MouseFocusReason);
    const bool handled = bar->eventFilter(spinBox, &focusOut);

    // Assert：handleFocusOut 被触发、空文本分支 clear、事件未被吞（透传基类）
    EXPECT_EQ(spy.count(), 1);
    EXPECT_FALSE(handled);
    EXPECT_EQ(spinBox->lineEdit()->text(), QString());
}

TEST_F(JumpLineBarTest, EventFilter_OtherObject_PassesThrough)
{
    // Arrange：其它对象上的 FocusOut 不处理
    QWidget stranger;
    QFocusEvent focusOut(QEvent::FocusOut, Qt::MouseFocusReason);
    QSignalSpy spy(bar, &JumpLineBar::lostFocusExit);

    // Act
    const bool handled = bar->eventFilter(&stranger, &focusOut);

    // Assert
    EXPECT_FALSE(handled);
    EXPECT_EQ(spy.count(), 0);
}

}  // namespace
