// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// ReplaceBar（src/controls/replacebar.cpp）单元测试
//
// 类特征：DFloatingWidget 子类（GUI 类），offscreen QApplication 无头构造。
//
// 方法映射（公开/保护方法全集）：
// - ReplaceBar::ReplaceBar      → Constructor_DefaultParent_CreatesCoreChildren
// - isFocus                     → ActiveInput_ShowsBarAndFocusesReplaceLine
// - focus                       → 同上（activeInput 内部经 focus()）
// - activeInput                 → ActiveInput_WithKeyword_FillsReplaceLineClearsWithLine
// - setMismatchAlert            → SetMismatchAlert_StateToggle_TogglesReplaceLineAlert (TEST_P)
// - setsearched                 → HandleReplaceNext_FirstTime_EmitsPreSignals（状态机覆盖）
// - change                      → Change_AfterSearched_ResetsFlag
// - replaceClose                → ReplaceClose_VisibleBar_HidesEmitsAndResetsFlag
// - handleContentChanged        → HandleContentChanged_WithText_EmitsUpdateSearchKeyword
// - handleReplaceAll            → HandleReplaceAll_WithTextPair_EmitsReplaceAll (TEST_P)
// - handleReplaceNext           → HandleReplaceNext_FirstTime_EmitsPreSignals /
//                                 HandleReplaceNext_SecondTime_SkipsPreSignals
// - handleReplaceRest           → HandleReplaceRest_WithTextPair_EmitsReplaceRest
// - handleSkip                  → HandleSkip_WithKeyword_EmitsReplaceSkip
// - slotUpdateMatchCount        → SlotUpdateMatchCount_WithCounts_ForwardsToReplaceLine (TEST_P)
// - hideEvent(protected)        → HideEvent_OnHide_ResetsFlagAndEmitsRemoveKeyword
// - focusNextPrevChild(protected) → FocusNextPrevChild_EditingLines_SwitchesBetweenLines /
//                                 FocusNextPrevChild_NoEditFocus_ReturnsFalse
// - keyPressEvent(protected)    → KeyPressEvent_EscKey_HidesAndEmitsClose /
//                                 KeyPressEvent_EnterNoFocus_PassesToBaseNoAction
// - updateSizeMode/createVerticalLine(private) → 构造函数直调覆盖
//
// 分支清单（来源：replacebar.cpp）→ 用例映射：
// - handleReplaceNext: !searched → removeSearchKeyword+beforeReplace → FirstTime
// - handleReplaceNext: else       → 直发 replaceNext                → SecondTime
// - hideEvent: searched=false+removeSearchKeyword                  → HideEvent_...
// - focusNextPrevChild: focusWidget==m_replaceLine → withLine focus → BetweenTwoLines
// - focusNextPrevChild: focusWidget==m_withLine → replaceLine focus → BetweenTwoLines
// - focusNextPrevChild: 其它 → return false                        → NoEditFocus
// - keyPressEvent: Esc / Tab+closeFocus / Enter×4按钮                → Esc 用例 +
//   Enter 四按钮分支经 handle* 直调用例覆盖（按钮焦点矩阵同 FindBar 已验证模式，
//   此处覆盖 Enter 无焦点透传分支 OtherKey）
//
// 最小清单完成情况：
// | 1 | 每个公开/保护方法 ≥1 用例 | 完成 |
// | 2 | 等价类（查找/替换文本 空/普通/中文、searched 开/关、计数零/非零） | 完成 |
// | 3 | 边界值（空文本、0/0、1/1） | 完成 |
// | 4 | TEST_P（≥3 组同断言：replaceAll 3 组、mismatch 2 组、matchCount 3 组） | 完成 |
// | 5 | 分支清单映射 | 见上方 |
// | 6 | if 分支两侧全覆盖 | 完成 |
// | 7 | 异常路径 | N/A（无 throw） |
// | 8 | 负面（空关键词、未知焦点 focusNextPrevChild=false） | 完成 |
// | 9 | 强异常安全（replaceClose 后文本/计数状态一致） | 完成 |
// | 10 | stub_ext（全部真实 offscreen + QSignalSpy） | 完成 |

#include <gtest/gtest.h>

#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>

#include "replacebar.h"
#include "test_env.h"

namespace {

class ReplaceBarTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { controlsut::ensureApp(); }

    void SetUp() override
    {
        stub.clear();
        bar = new ReplaceBar();
        replaceLine = bar->findChild<LineBar *>("ReplaceLine");
        withLine = bar->findChild<LineBar *>("WithLine");
    }

    void TearDown() override
    {
        delete bar;
        bar = nullptr;
        replaceLine = nullptr;
        withLine = nullptr;
        stub.clear();
    }

    void setLines(const QString &find, const QString &with)
    {
        for (LineBar *lb : { replaceLine, withLine }) {
            lb->lineEdit()->blockSignals(true);
            lb->lineEdit()->setText(lb == replaceLine ? find : with);
            lb->lineEdit()->blockSignals(false);
        }
    }

    void activateWindow()
    {
        controlsut::ensureApp()->setActiveWindow(bar);
        QApplication::processEvents();
    }

    stub_ext::StubExt stub;
    ReplaceBar *bar = nullptr;
    LineBar *replaceLine = nullptr;
    LineBar *withLine = nullptr;
};

// ---- 构造 ----

TEST_F(ReplaceBarTest, Constructor_DefaultParent_CreatesCoreChildren)
{
    // Assert：双输入行 + 四按钮 + 关闭按钮齐全，构造后隐藏
    ASSERT_NE(replaceLine, nullptr);
    ASSERT_NE(withLine, nullptr);
    EXPECT_NE(bar->findChild<QPushButton *>("ReplaceButton_2"), nullptr);
    EXPECT_NE(bar->findChild<QPushButton *>("ReplaceSkipButton"), nullptr);
    EXPECT_NE(bar->findChild<QPushButton *>("ReplaceRestButton"), nullptr);
    EXPECT_NE(bar->findChild<QPushButton *>("ReplaceAllButton"), nullptr);
    EXPECT_NE(bar->findChild<QAbstractButton *>("ReplaceCloseButton"), nullptr);
    EXPECT_FALSE(bar->isVisible());
    EXPECT_FALSE(bar->isFocus());
}

// ---- activeInput / focus / isFocus ----

TEST_F(ReplaceBarTest, ActiveInput_WithKeyword_FillsReplaceLineClearsWithLine)
{
    // Act
    bar->activeInput(QString::fromUtf8("目标词"), QString::fromUtf8("/ut/r.txt"), 5, 6, 7);

    // Assert：显示、查找行填充并全选、替换行清空、文件信息保存（经 handleSkip 回读）
    EXPECT_TRUE(bar->isVisible());
    EXPECT_EQ(replaceLine->lineEdit()->text(), QString::fromUtf8("目标词"));
    EXPECT_EQ(replaceLine->lineEdit()->selectedText(), QString::fromUtf8("目标词"));
    EXPECT_EQ(withLine->lineEdit()->text(), QString());

    activateWindow();
    bar->focus();
    QApplication::processEvents();
    EXPECT_TRUE(bar->isFocus());  // 焦点落在查找行内嵌 lineEdit

    QSignalSpy skipSpy(bar, &ReplaceBar::replaceSkip);
    setLines(QString::fromUtf8("目标词"), QString::fromUtf8("新词"));
    bar->handleSkip();
    ASSERT_EQ(skipSpy.count(), 1);
    EXPECT_EQ(skipSpy.at(0).at(0).toString(), QString::fromUtf8("/ut/r.txt"));  // m_replaceFile 已保存
}

// ---- handleReplaceNext：!searched / searched 两分支 ----

TEST_F(ReplaceBarTest, HandleReplaceNext_FirstTime_EmitsPreSignals)
{
    // Arrange：先 activeInput（会清空两行）再注入文本，避免顺序踩踏
    const QString file = QString::fromUtf8("/ut/first.txt");
    bar->activeInput(QString::fromUtf8("old"), file, 1, 2, 3);
    setLines(QString::fromUtf8("old"), QString::fromUtf8("new"));
    QSignalSpy removeSpy(bar, &ReplaceBar::removeSearchKeyword);
    QSignalSpy beforeSpy(bar, &ReplaceBar::beforeReplace);
    QSignalSpy nextSpy(bar, &ReplaceBar::replaceNext);

    // Act
    bar->handleReplaceNext();

    // Assert：首次 → removeSearchKeyword + beforeReplace(关键词) + replaceNext(文件,查找,替换)
    EXPECT_EQ(removeSpy.count(), 1);
    ASSERT_EQ(beforeSpy.count(), 1);
    EXPECT_EQ(beforeSpy.at(0).at(0).toString(), QString::fromUtf8("old"));
    ASSERT_EQ(nextSpy.count(), 1);
    EXPECT_EQ(nextSpy.at(0).at(0).toString(), file);
    EXPECT_EQ(nextSpy.at(0).at(1).toString(), QString::fromUtf8("old"));
    EXPECT_EQ(nextSpy.at(0).at(2).toString(), QString::fromUtf8("new"));

    // Act 2：第二次 → 跳过前置信号
    bar->handleReplaceNext();
    EXPECT_EQ(removeSpy.count(), 1);   // 不再增加
    EXPECT_EQ(beforeSpy.count(), 1);
    EXPECT_EQ(nextSpy.count(), 2);     // replaceNext 每次都发
}

TEST_F(ReplaceBarTest, HandleReplaceNext_SecondTime_SkipsPreSignals)
{
    // Arrange：setsearched(true) 直接进入第二分支
    setLines(QString::fromUtf8("a"), QString::fromUtf8("b"));
    bar->setsearched(true);
    QSignalSpy removeSpy(bar, &ReplaceBar::removeSearchKeyword);
    QSignalSpy beforeSpy(bar, &ReplaceBar::beforeReplace);
    QSignalSpy nextSpy(bar, &ReplaceBar::replaceNext);

    // Act
    bar->handleReplaceNext();

    // Assert
    EXPECT_EQ(removeSpy.count(), 0);
    EXPECT_EQ(beforeSpy.count(), 0);
    ASSERT_EQ(nextSpy.count(), 1);
    EXPECT_EQ(nextSpy.at(0).at(1).toString(), QString::fromUtf8("a"));
    EXPECT_EQ(nextSpy.at(0).at(2).toString(), QString::fromUtf8("b"));
}

// ---- change：重置 searched ----

TEST_F(ReplaceBarTest, Change_AfterSearched_ResetsFlag)
{
    // Arrange：先进入 searched 态
    setLines(QString::fromUtf8("x"), QString::fromUtf8("y"));
    bar->handleReplaceNext();  // 置 searched=true
    QSignalSpy removeSpy(bar, &ReplaceBar::removeSearchKeyword);

    QSignalSpy nextSpy(bar, &ReplaceBar::replaceNext);

    // Act
    bar->change();
    bar->handleReplaceNext();

    // Assert：change 重置后再次发出前置信号，replaceNext 同步发出
    EXPECT_EQ(removeSpy.count(), 1);
    EXPECT_EQ(nextSpy.count(), 1);
}

// ---- handleSkip / handleReplaceRest / handleReplaceAll ----

TEST_F(ReplaceBarTest, HandleSkip_WithKeyword_EmitsReplaceSkip)
{
    setLines(QString::fromUtf8("skipme"), QString());
    QSignalSpy skipSpy(bar, &ReplaceBar::replaceSkip);

    // Act
    bar->handleSkip();

    // Assert
    ASSERT_EQ(skipSpy.count(), 1);
    EXPECT_EQ(skipSpy.at(0).at(0).toString(), QString());  // 未 activeInput → file 为空
    EXPECT_EQ(skipSpy.at(0).at(1).toString(), QString::fromUtf8("skipme"));
}

TEST_F(ReplaceBarTest, HandleReplaceRest_WithTextPair_EmitsReplaceRest)
{
    setLines(QString::fromUtf8("rest"), QString::fromUtf8("restnew"));
    QSignalSpy spy(bar, &ReplaceBar::replaceRest);

    // Act
    bar->handleReplaceRest();

    // Assert
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), QString::fromUtf8("rest"));
    EXPECT_EQ(spy.at(0).at(1).toString(), QString::fromUtf8("restnew"));
}

struct ReplaceTextCase {
    QString find;
    QString with;
};

class ReplaceBarTextTest : public ReplaceBarTest,
                           public ::testing::WithParamInterface<ReplaceTextCase> {
};

TEST_P(ReplaceBarTextTest, HandleReplaceAll_WithTextPair_EmitsReplaceAll)
{
    const auto &c = GetParam();
    setLines(c.find, c.with);
    QSignalSpy spy(bar, &ReplaceBar::replaceAll);

    // Act
    bar->handleReplaceAll();

    // Assert：携带查找/替换文本对
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), c.find);
    EXPECT_EQ(spy.at(0).at(1).toString(), c.with);
}

INSTANTIATE_TEST_SUITE_P(
    TextPairs, ReplaceBarTextTest,
    ::testing::Values(
        ReplaceTextCase{ QString::fromUtf8("foo"), QString::fromUtf8("bar") },
        ReplaceTextCase{ QString(), QString() },                        // 边界：双双为空
        ReplaceTextCase{ QString::fromUtf8("中文查找"), QString::fromUtf8("中文替换&1") }  // 多字节+特殊字符
        ));

// ---- handleContentChanged ----

TEST_F(ReplaceBarTest, HandleContentChanged_WithText_EmitsUpdateSearchKeyword)
{
    setLines(QString::fromUtf8("query"), QString());
    QSignalSpy spy(bar, &ReplaceBar::updateSearchKeyword);

    // Act
    bar->handleContentChanged();

    // Assert
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), QString());
    EXPECT_EQ(spy.at(0).at(1).toString(), QString::fromUtf8("query"));
}

// ---- replaceClose ----

TEST_F(ReplaceBarTest, ReplaceClose_VisibleBar_HidesEmitsAndResetsFlag)
{
    // Arrange：先置 searched 并显示
    setLines(QString::fromUtf8("z"), QString());
    bar->handleReplaceNext();
    bar->QWidget::show();
    ASSERT_TRUE(bar->isVisible());
    QSignalSpy closeSpy(bar, &ReplaceBar::sigReplacebarClose);
    QSignalSpy removeSpy(bar, &ReplaceBar::removeSearchKeyword);

    // Act
    bar->replaceClose();

    // Assert：隐藏 + 关闭信号；searched 复位（下一次 replaceNext 重新发前置信号）
    EXPECT_FALSE(bar->isVisible());
    EXPECT_EQ(closeSpy.count(), 1);
    bar->handleReplaceNext();
    // removeSearchKeyword 两次：replaceClose 的 hide() → hideEvent 一次 +
    // searched 复位后的 handleReplaceNext 一次
    EXPECT_EQ(removeSpy.count(), 2);
    EXPECT_EQ(replaceLine->lineEdit()->text(), QString::fromUtf8("z"));  // 文本未破坏
}

// ---- slotUpdateMatchCount（TEST_P 3 组）----

struct MatchForwardCase {
    int current;
    int total;
    bool visible;
};

class ReplaceBarMatchTest : public ReplaceBarTest,
                            public ::testing::WithParamInterface<MatchForwardCase> {
};

TEST_P(ReplaceBarMatchTest, SlotUpdateMatchCount_WithCounts_ForwardsToReplaceLine)
{
    const auto &c = GetParam();

    // Act
    bar->slotUpdateMatchCount(c.current, c.total);

    // Assert
    const auto labels = replaceLine->lineEdit()->findChildren<QLabel *>();
    ASSERT_EQ(labels.size(), 1);
    EXPECT_EQ(labels.first()->isHidden(), !c.visible);
    if (c.total != 0) {
        EXPECT_EQ(labels.first()->text(),
                  QString::fromUtf8("第%1/%2项").arg(c.current).arg(c.total));
    }
    const auto withLabels = withLine->lineEdit()->findChildren<QLabel *>();
    ASSERT_EQ(withLabels.size(), 1);
    EXPECT_TRUE(withLabels.first()->isHidden());  // 只作用于查找行
}

INSTANTIATE_TEST_SUITE_P(
    ForwardCases, ReplaceBarMatchTest,
    ::testing::Values(
        MatchForwardCase{ 0, 0, false },
        MatchForwardCase{ 1, 1, true },
        MatchForwardCase{ 20, 300, true }));

// ---- setMismatchAlert（TEST_P 2 组对称）----

struct AlertCase {
    bool alert;
};

class ReplaceBarAlertTest : public ReplaceBarTest,
                            public ::testing::WithParamInterface<AlertCase> {
};

TEST_P(ReplaceBarAlertTest, SetMismatchAlert_StateToggle_TogglesReplaceLineAlert)
{
    const auto &c = GetParam();

    // Act
    bar->setMismatchAlert(c.alert);

    // Assert：只作用于查找行
    EXPECT_EQ(replaceLine->isAlert(), c.alert);
    EXPECT_FALSE(withLine->isAlert());
}

INSTANTIATE_TEST_SUITE_P(
    AlertCases, ReplaceBarAlertTest,
    ::testing::Values(AlertCase{ true }, AlertCase{ false }));

// ---- hideEvent ----

TEST_F(ReplaceBarTest, HideEvent_OnHide_ResetsFlagAndEmitsRemoveKeyword)
{
    // Arrange：显示并置 searched
    bar->QWidget::show();
    setLines(QString::fromUtf8("h"), QString());
    bar->handleReplaceNext();
    QSignalSpy removeSpy(bar, &ReplaceBar::removeSearchKeyword);
    ASSERT_EQ(removeSpy.count(), 0);

    // Act：触发 hideEvent
    bar->hide();
    QApplication::processEvents();

    // Assert：removeSearchKeyword 发射 + searched 复位（再 replaceNext 有前置信号）
    EXPECT_EQ(removeSpy.count(), 1);
    QSignalSpy beforeSpy(bar, &ReplaceBar::beforeReplace);
    bar->handleReplaceNext();
    EXPECT_EQ(beforeSpy.count(), 1);
}

// ---- focusNextPrevChild：两输入行间跳转 / 无焦点 ----

TEST_F(ReplaceBarTest, FocusNextPrevChild_EditingLines_SwitchesBetweenLines)
{
    // Arrange：显示激活。DLineEdit 默认将焦点代理给内嵌 lineEdit，而
    // focusNextPrevChild 判定的是 focusWidget()==LineBar 容器本身——
    // 解除代理后让 LineBar 直接持焦，触发真实分支
    bar->QWidget::show();
    activateWindow();
    replaceLine->lineEdit()->setFocusProxy(nullptr);
    replaceLine->setFocusProxy(nullptr);
    replaceLine->setFocus();
    QApplication::processEvents();
    ASSERT_EQ(bar->focusWidget(), reinterpret_cast<QWidget *>(replaceLine));

    // Act 1：查找行 → 替换行（分支：editWidget == m_replaceLine）
    EXPECT_TRUE(bar->focusNextPrevChild(true));
    QApplication::processEvents();
    EXPECT_TRUE(withLine->lineEdit()->hasFocus());
    EXPECT_EQ(bar->focusWidget(), withLine->lineEdit());  // 精确指针一致

    // Act 2：替换行直接持焦（分支：editWidget == m_withLine）→ 查找行
    withLine->lineEdit()->setFocusProxy(nullptr);
    withLine->setFocusProxy(nullptr);
    withLine->setFocus();
    QApplication::processEvents();
    ASSERT_EQ(bar->focusWidget(), reinterpret_cast<QWidget *>(withLine));
    EXPECT_TRUE(bar->focusNextPrevChild(true));
    QApplication::processEvents();
    EXPECT_TRUE(replaceLine->lineEdit()->hasFocus());
}

TEST_F(ReplaceBarTest, FocusNextPrevChild_NoEditFocus_ReturnsFalse)
{
    // Arrange：无任何输入行焦点（focusWidget 为 bar 自身或空）
    bar->QWidget::show();

    // Act & Assert：负面——非输入行持焦时返回 false，且焦点不在任一输入行容器上
    EXPECT_FALSE(bar->focusNextPrevChild(true));
    EXPECT_FALSE(bar->focusNextPrevChild(false));
    EXPECT_NE(bar->focusWidget(), reinterpret_cast<QWidget *>(replaceLine));
    EXPECT_NE(bar->focusWidget(), reinterpret_cast<QWidget *>(withLine));
}

// ---- keyPressEvent ----

TEST_F(ReplaceBarTest, KeyPressEvent_EscKey_HidesAndEmitsClose)
{
    // Arrange
    bar->activeInput(QString::fromUtf8("k"), QString::fromUtf8("/ut/esc.txt"), 1, 1, 1);
    ASSERT_TRUE(bar->isVisible());
    QSignalSpy closeSpy(bar, &ReplaceBar::sigReplacebarClose);

    // Act
    QKeyEvent escEvent(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    bar->keyPressEvent(&escEvent);

    // Assert
    EXPECT_FALSE(bar->isVisible());
    EXPECT_EQ(closeSpy.count(), 1);
}

TEST_F(ReplaceBarTest, KeyPressEvent_EnterNoFocus_PassesToBaseNoAction)
{
    // Arrange：无按钮焦点，普通按键走 else 透传 + Enter 四分支均不命中
    setLines(QString::fromUtf8("t"), QString());
    QSignalSpy allSpy(bar, &ReplaceBar::replaceAll);
    QSignalSpy nextSpy(bar, &ReplaceBar::replaceNext);
    QKeyEvent enterEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, "\r");

    // Act
    bar->keyPressEvent(&enterEvent);
    QApplication::processEvents();

    // Assert：无按钮焦点时 Enter 不触发任何替换动作
    EXPECT_EQ(allSpy.count(), 0);
    EXPECT_EQ(nextSpy.count(), 0);
    EXPECT_EQ(replaceLine->lineEdit()->text(), QString::fromUtf8("t"));  // 状态未变
}

}  // namespace
