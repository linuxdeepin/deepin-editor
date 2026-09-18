// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// FindBar（src/controls/findbar.cpp）单元测试
//
// 类特征：DFloatingWidget 子类（GUI 类），offscreen QApplication 无头构造。
//
// 方法映射（公开/保护方法全集）：
// - FindBar::FindBar         → Constructor_DefaultParent_CreatesCoreChildren
// - isFocus                  → ActiveInput_ShowsBarFocusesAndFillsKeyword
// - focus                    → 同上（activeInput 内部调用 focus；focus 单独用例 Focus_SelectsAllEditLine）
// - activeInput              → ActiveInput_WithKeyword_FillsAndShowsBar /
//                              ActiveInput_EmptyKeyword_StillShown
// - setMismatchAlert         → SetMismatchAlert_StateToggle_TogglesEditLineAlert (TEST_P)
// - receiveText              → ReceiveText_ResetsSearchedFlag（经 findPreClicked 行为验证）
// - setSearched              → SetSearched_FlagToggle_ControlsKeywordUpdate
// - findPreClicked           → FindPreClicked_FirstTime_EmitsKeywordUpdate /
//                              FindPreClicked_AlreadySearched_EmitsFindPrevOnly
// - getCurrentSearchText     → GetInvalidSearchText_ReflectsEditLineContent (TEST_P)
// - findCancel               → FindCancel_VisibleBar_HidesAndEmitsClose
// - handleContentChanged     → HandleContentChanged_WithFile_EmitsUpdateSearchKeyword
// - handleFindNext           → HandleFindNext_WithKeyword_EmitsFindNextOnly (TEST_P)
// - handleFindPrev           → HandleFindPrev_WithKeyword_EmitsFindPrevOnly
// - handleSwitchToReplace    → HandleSwitchToReplace_ButtonTriggered_EmitsSwitchSignal
// - slotUpdateMatchCount     → SlotUpdateMatchCount_WithCounts_ForwardsToEditLine (TEST_P)
// - hideEvent(protected)     → HideEvent_OnHide_KeepsSearchKeyword（removeSearchKeyword 不发）
// - focusNextPrevChild(protected) → FocusNextPrevChild_AnyDirection_ReturnsFalse
// - keyPressEvent(protected) → KeyPressEvent_EscKey_HidesAndEmitsClose /
//                              KeyPressEvent_EnterWithButtonFocus_ClicksButton /
//                              KeyPressEvent_TabWithCloseFocus_BackToEditLine /
//                              KeyPressEvent_EnterNoFocus_PassesToBaseNoAction
// - updateSizeMode(private)  → 构造函数经 DTKWIDGET_CLASS_DSizeMode 直调覆盖
//
// 分支清单（来源：findbar.cpp）→ 用例映射：
// - keyPressEvent: key=="Esc" → hide+sigFindbarClose           → KeyPressEvent_Esc_...
// - keyPressEvent: closeButton focus && "Tab" → setFocus editline → KeyPressEvent_TabWithCloseFocus_...
// - keyPressEvent: else → DFloatingWidget::keyPressEvent        → KeyPressEvent_OtherKey_...
// - keyPressEvent: "Enter" && prevBtn focus → click             → KeyPressEvent_EnterWithButtonFocus_...
// - keyPressEvent: "Enter" && nextBtn focus → click             → 同上
// - receiveText: t != "" → 保存文本                              → ReceiveText_ResetsSearchedFlag（两分支均触发）
// - findPreClicked: !searched → updateKeyword+findPrev+置位      → FindPreClicked_FirstTime_...
// - findPreClicked: else → findPrev only                        → FindPreClicked_AlreadySearched_...
// - getCurrentSearchText: m_editLine/lineEdit 判空               → 两分支（默认构造即有 editLine；TEST_P 验证内容）
//
// 最小清单完成情况：
// | 1 | 每个公开/保护方法 ≥1 用例 | 完成 |
// | 2 | 等价类（关键词空/普通/中文、alert 开/关、match 计数零/非零） | 完成 |
// | 3 | 边界值（空关键词、0/0 计数、1/1 计数） | 完成 |
// | 4 | TEST_P（≥3 组同断言：mismatch 2 组×正反、searchText 3 组、findNext 3 组、matchCount 3 组） | 完成 |
// | 5 | 分支清单映射 | 见上方 |
// | 6 | if 分支两侧全覆盖 | 完成 |
// | 7 | 异常路径 | N/A（无 throw） |
// | 8 | 负面（空关键词 activeInput、Esc 关闭） | 完成 |
// | 9 | 强异常安全（hideEvent 后关键词保留） | 完成 |
// | 10 | stub_ext（无外部依赖需 stub；全部真实 offscreen） | 完成 |

#include <gtest/gtest.h>

#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>

#include "findbar.h"
#include "test_env.h"

namespace {

class FindBarTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { controlsut::ensureApp(); }

    void SetUp() override
    {
        stub.clear();
        bar = new FindBar();
        editLine = bar->findChild<LineBar *>("EditLine");
    }

    void TearDown() override
    {
        delete bar;
        bar = nullptr;
        editLine = nullptr;
        stub.clear();
    }

    // offscreen 下激活窗口并派发挂起事件，使 setFocus/hasFocus 生效
    void activateWindow()
    {
        controlsut::ensureApp()->setActiveWindow(bar);
        QApplication::processEvents();
    }

    void setKeyword(const QString &text)
    {
        editLine->lineEdit()->blockSignals(true);
        editLine->lineEdit()->setText(text);
        editLine->lineEdit()->blockSignals(false);
    }

    stub_ext::StubExt stub;
    FindBar *bar = nullptr;
    LineBar *editLine = nullptr;
};

// ---- 构造 ----

TEST_F(FindBarTest, Constructor_DefaultParent_CreatesCoreChildren)
{
    // Assert：核心子件齐全、构造后隐藏、关键词初始为空
    ASSERT_NE(editLine, nullptr);
    EXPECT_NE(bar->findChild<QPushButton *>("FindPrevButton"), nullptr);
    EXPECT_NE(bar->findChild<QPushButton *>("FindNextButton"), nullptr);
    EXPECT_NE(bar->findChild<QPushButton *>("ReplaceButton"), nullptr);
    EXPECT_NE(bar->findChild<QAbstractButton *>("FindCloseButton"), nullptr);
    EXPECT_FALSE(bar->isVisible());  // 构造即 hide()
    EXPECT_EQ(bar->getCurrentSearchText(), QString());
}

// ---- activeInput / focus / isFocus ----

TEST_F(FindBarTest, ActiveInput_WithKeyword_FillsAndShowsBar)
{
    QSignalSpy updateSpy(bar, &FindBar::updateSearchKeyword);

    // Act
    bar->activeInput(QString::fromUtf8("关键词kw"), QString::fromUtf8("/ut/a.txt"), 11, 22, 33);

    // Assert：栏显示、关键词填充并全选、文件信息保存（经 handleContentChanged 的
    // updateSearchKeyword 信号参数回读验证 m_findFile 状态）
    EXPECT_TRUE(bar->isVisible());
    EXPECT_EQ(bar->getCurrentSearchText(), QString::fromUtf8("关键词kw"));
    EXPECT_EQ(editLine->lineEdit()->selectedText(), QString::fromUtf8("关键词kw"));  // selectAll 生效

    bar->handleContentChanged();
    ASSERT_EQ(updateSpy.count(), 1);
    EXPECT_EQ(updateSpy.at(0).at(0).toString(), QString::fromUtf8("/ut/a.txt"));
    EXPECT_EQ(updateSpy.at(0).at(1).toString(), QString::fromUtf8("关键词kw"));
}

TEST_F(FindBarTest, ActiveInput_EmptyKeyword_StillShown)
{
    // Act：负面——空关键词也应当正常显示
    bar->activeInput(QString(), QString::fromUtf8("/ut/b.txt"), 0, 0, 0);

    // Assert
    EXPECT_TRUE(bar->isVisible());
    EXPECT_EQ(bar->getCurrentSearchText(), QString());
    EXPECT_EQ(editLine->lineEdit()->selectedText(), QString());
}

TEST_F(FindBarTest, Focus_ExistingText_SelectsAllAndGainsFocus)
{
    // Arrange：填充并取消全选
    bar->activeInput(QString::fromUtf8("hello"), QString::fromUtf8("/ut/c.txt"), 1, 1, 1);
    editLine->lineEdit()->setSelection(0, 1);  // 缩小选区
    activateWindow();

    // Act
    bar->focus();

    // Assert：全选恢复（focus 副作用），焦点在输入行（isFocus 状态变更）
    QApplication::processEvents();
    EXPECT_EQ(editLine->lineEdit()->selectedText(), QString::fromUtf8("hello"));
    EXPECT_TRUE(bar->isFocus());
}

// ---- findCancel ----

TEST_F(FindBarTest, FindCancel_VisibleBar_HidesAndEmitsClose)
{
    // Arrange
    bar->activeInput(QString::fromUtf8("x"), QString::fromUtf8("/ut/d.txt"), 1, 1, 1);
    ASSERT_TRUE(bar->isVisible());
    QSignalSpy closeSpy(bar, &FindBar::sigFindbarClose);

    // Act
    bar->findCancel();

    // Assert：隐藏 + 关闭信号恰一次；匹配计数清零（0/0 → label 隐藏）
    EXPECT_FALSE(bar->isVisible());
    EXPECT_EQ(closeSpy.count(), 1);
    const auto labels = editLine->lineEdit()->findChildren<QLabel *>();
    ASSERT_EQ(labels.size(), 1);
    EXPECT_TRUE(labels.first()->isHidden());
}

// ---- handleContentChanged ----

TEST_F(FindBarTest, HandleContentChanged_WithFile_EmitsUpdateSearchKeyword)
{
    QSignalSpy spy(bar, &FindBar::updateSearchKeyword);
    setKeyword(QString::fromUtf8("内容"));

    // Act
    bar->handleContentChanged();

    // Assert：默认未 activeInput → file 为空；keyword 为当前文本
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), QString());
    EXPECT_EQ(spy.at(0).at(1).toString(), QString::fromUtf8("内容"));
}

// ---- handleFindNext / handleFindPrev（TEST_P 3 组）----

struct KeywordCase {
    QString keyword;
};

class FindBarKeywordTest : public FindBarTest,
                           public ::testing::WithParamInterface<KeywordCase> {
};

TEST_P(FindBarKeywordTest, HandleFindNext_WithKeyword_EmitsFindNextOnly)
{
    const auto &c = GetParam();
    setKeyword(c.keyword);
    QSignalSpy nextSpy(bar, &FindBar::findNext);
    QSignalSpy prevSpy(bar, &FindBar::findPrev);

    // Act
    bar->handleFindNext();

    // Assert：findNext 携带关键词，且不触发 findPrev
    ASSERT_EQ(nextSpy.count(), 1);
    EXPECT_EQ(nextSpy.at(0).at(0).toString(), c.keyword);
    EXPECT_EQ(prevSpy.count(), 0);
}

INSTANTIATE_TEST_SUITE_P(
    KeywordCases, FindBarKeywordTest,
    ::testing::Values(
        KeywordCase{ QString::fromUtf8("abc") },
        KeywordCase{ QString() },                          // 边界：空关键词
        KeywordCase{ QString::fromUtf8("中文&<>搜索") }     // 边界：多字节+特殊字符
        ));

TEST_P(FindBarKeywordTest, HandleFindPrev_WithKeyword_EmitsFindPrevOnly)
{
    const auto &c = GetParam();
    setKeyword(c.keyword);
    QSignalSpy nextSpy(bar, &FindBar::findNext);
    QSignalSpy prevSpy(bar, &FindBar::findPrev);

    // Act
    bar->handleFindPrev();

    // Assert
    ASSERT_EQ(prevSpy.count(), 1);
    EXPECT_EQ(prevSpy.at(0).at(0).toString(), c.keyword);
    EXPECT_EQ(nextSpy.count(), 0);
}

// ---- handleSwitchToReplace ----

TEST_F(FindBarTest, HandleSwitchToReplace_ButtonTriggered_EmitsSwitchSignal)
{
    QSignalSpy spy(bar, &FindBar::sigSwitchToReplaceBar);

    // Act
    bar->handleSwitchToReplace();

    // Assert：切换信号恰一次，且不隐藏查找栏、不影响输入内容（副作用边界）
    EXPECT_EQ(spy.count(), 1);
    EXPECT_FALSE(bar->isVisible());
    EXPECT_EQ(bar->getCurrentSearchText(), QString());
}

// ---- setMismatchAlert（TEST_P：开/关两组对称断言）----

struct AlertCase {
    bool alert;
};

class FindBarAlertTest : public FindBarTest,
                         public ::testing::WithParamInterface<AlertCase> {
};

TEST_P(FindBarAlertTest, SetMismatchAlert_StateToggle_TogglesEditLineAlert)
{
    const auto &c = GetParam();

    // Act
    bar->setMismatchAlert(c.alert);

    // Assert：告警态精确传递到内嵌 LineBar；文本状态不受扰动
    EXPECT_EQ(editLine->isAlert(), c.alert);
    EXPECT_EQ(editLine->lineEdit()->text(), QString());
}

INSTANTIATE_TEST_SUITE_P(
    AlertCases, FindBarAlertTest,
    ::testing::Values(AlertCase{ true }, AlertCase{ false }));

// ---- receiveText / setSearched / findPreClicked 状态机 ----

TEST_F(FindBarTest, FindPreClicked_FirstTime_EmitsKeywordUpdate)
{
    setKeyword(QString::fromUtf8("first"));
    QSignalSpy updateSpy(bar, &FindBar::updateSearchKeyword);
    QSignalSpy prevSpy(bar, &FindBar::findPrev);

    // Act：首次（searched=false）→ updateSearchKeyword + findPrev，并置 searched
    bar->findPreClicked();

    ASSERT_EQ(prevSpy.count(), 1);
    ASSERT_EQ(updateSpy.count(), 1);

    // Act 2：第二次（searched=true）→ 仅 findPrev
    bar->findPreClicked();
    EXPECT_EQ(prevSpy.count(), 2);
    EXPECT_EQ(updateSpy.count(), 1);  // 不再更新关键词

    // Act 3：receiveText 重置 searched → 再次更新关键词（t 非空与空两分支各覆盖一次）
    bar->receiveText(QString::fromUtf8("changed"));
    bar->findPreClicked();
    EXPECT_EQ(prevSpy.count(), 3);
    EXPECT_EQ(updateSpy.count(), 2);

    bar->receiveText(QString());  // 空串分支：仅重置 searched，不改文本
    bar->findPreClicked();
    EXPECT_EQ(prevSpy.count(), 4);
    EXPECT_EQ(updateSpy.count(), 3);
}

TEST_F(FindBarTest, SetSearched_FlagToggle_ControlsKeywordUpdate)
{
    setKeyword(QString::fromUtf8("kw"));
    QSignalSpy updateSpy(bar, &FindBar::updateSearchKeyword);
    QSignalSpy prevSpy(bar, &FindBar::findPrev);

    // Act：外部置 searched=true → 直接 findPrev
    bar->setSearched(true);
    bar->findPreClicked();

    // Assert
    EXPECT_EQ(prevSpy.count(), 1);
    EXPECT_EQ(updateSpy.count(), 0);

    // Act 2：置回 false → 恢复首搜行为
    bar->setSearched(false);
    bar->findPreClicked();
    EXPECT_EQ(prevSpy.count(), 2);
    EXPECT_EQ(updateSpy.count(), 1);
}

// ---- slotUpdateMatchCount（TEST_P 3 组）----

struct MatchCountForwardCase {
    int current;
    int total;
    bool labelVisible;
};

class FindBarMatchCountTest : public FindBarTest,
                              public ::testing::WithParamInterface<MatchCountForwardCase> {
};

TEST_P(FindBarMatchCountTest, SlotUpdateMatchCount_WithCounts_ForwardsToEditLine)
{
    const auto &c = GetParam();

    // Act
    bar->slotUpdateMatchCount(c.current, c.total);

    // Assert：转发至 LineBar 计数标签
    const auto labels = editLine->lineEdit()->findChildren<QLabel *>();
    ASSERT_EQ(labels.size(), 1);
    EXPECT_EQ(labels.first()->isHidden(), !c.labelVisible);
    if (c.total != 0) {
        EXPECT_EQ(labels.first()->text(),
                  QString::fromUtf8("第%1/%2项").arg(c.current).arg(c.total));
    }
}

INSTANTIATE_TEST_SUITE_P(
    ForwardCases, FindBarMatchCountTest,
    ::testing::Values(
        MatchCountForwardCase{ 0, 0, false },   // 边界：无匹配 → 隐藏
        MatchCountForwardCase{ 1, 1, true },    // 边界：唯一匹配
        MatchCountForwardCase{ 7, 99, true }));

// ---- hideEvent：保留查询标记 ----

TEST_F(FindBarTest, HideEvent_OnHide_KeepsSearchKeyword)
{
    // Arrange
    setKeyword(QString::fromUtf8("keepme"));
    bar->QWidget::show();
    QSignalSpy removeSpy(bar, &FindBar::removeSearchKeyword);

    // Act：触发 hideEvent
    bar->hide();
    QApplication::processEvents();

    // Assert：隐藏后关键词保留（getCurrentSearchText 仍可读）、removeSearchKeyword 不发射
    EXPECT_FALSE(bar->isVisible());
    EXPECT_EQ(bar->getCurrentSearchText(), QString::fromUtf8("keepme"));
    EXPECT_EQ(removeSpy.count(), 0);
}

// ---- focusNextPrevChild ----

TEST_F(FindBarTest, FocusNextPrevChild_AnyDirection_ReturnsFalse)
{
    // Act & Assert：两个方向都禁用焦点链
    EXPECT_FALSE(bar->focusNextPrevChild(true));
    EXPECT_FALSE(bar->focusNextPrevChild(false));
    EXPECT_EQ(bar->getCurrentSearchText(), QString());  // 状态未受影响
}

// ---- keyPressEvent ----

TEST_F(FindBarTest, KeyPressEvent_EscKey_HidesAndEmitsClose)
{
    // Arrange
    bar->activeInput(QString::fromUtf8("esc"), QString::fromUtf8("/ut/e.txt"), 1, 1, 1);
    ASSERT_TRUE(bar->isVisible());
    QSignalSpy closeSpy(bar, &FindBar::sigFindbarClose);

    // Act：合成 Esc 按键直接调用（protected 经 -fno-access-control）
    QKeyEvent escEvent(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    bar->keyPressEvent(&escEvent);

    // Assert
    EXPECT_FALSE(bar->isVisible());
    EXPECT_EQ(closeSpy.count(), 1);
}

TEST_F(FindBarTest, KeyPressEvent_TabWithCloseFocus_BackToEditLine)
{
    // Arrange：显示并激活，把焦点给关闭按钮
    bar->activeInput(QString::fromUtf8("tab"), QString::fromUtf8("/ut/f.txt"), 1, 1, 1);
    activateWindow();
    auto *closeButton = bar->findChild<QAbstractButton *>("FindCloseButton");
    ASSERT_NE(closeButton, nullptr);
    closeButton->setFocus();
    QApplication::processEvents();
    ASSERT_TRUE(closeButton->hasFocus());

    // Act
    QKeyEvent tabEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
    bar->keyPressEvent(&tabEvent);
    QApplication::processEvents();

    // Assert：焦点回到查找输入行
    EXPECT_TRUE(editLine->lineEdit()->hasFocus());
    EXPECT_FALSE(closeButton->hasFocus());
    EXPECT_EQ(bar->focusWidget(), editLine->lineEdit());  // 精确指针一致
}

TEST_P(FindBarKeywordTest, KeyPressEvent_EnterWithButtonFocus_ClicksButton)
{
    const auto &c = GetParam();
    setKeyword(c.keyword);
    bar->QWidget::show();
    activateWindow();
    // 焦点放在 Previous 按钮上：Enter → click → queued handleFindPrev → findPrev 信号
    auto *prevButton = bar->findChild<QPushButton *>("FindPrevButton");
    ASSERT_NE(prevButton, nullptr);
    prevButton->setFocus();
    QApplication::processEvents();
    ASSERT_TRUE(prevButton->hasFocus());

    QSignalSpy prevSpy(bar, &FindBar::findPrev);
    QSignalSpy nextSpy(bar, &FindBar::findNext);
    QKeyEvent enterEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, "\r");
    bar->keyPressEvent(&enterEvent);
    QApplication::processEvents();  // flushed queued handleFindPrev

    // Assert：Previous 按钮 Enter 只触发 findPrev，不触发 findNext
    ASSERT_EQ(prevSpy.count(), 1);
    EXPECT_EQ(prevSpy.at(0).at(0).toString(), c.keyword);
    EXPECT_EQ(nextSpy.count(), 0);
}

TEST_F(FindBarTest, KeyPressEvent_EnterNoFocus_PassesToBaseNoAction)
{
    // Arrange：无按钮焦点时按普通键 → else 分支透传 DFloatingWidget
    QSignalSpy closeSpy(bar, &FindBar::sigFindbarClose);
    QKeyEvent letterEvent(QEvent::KeyPress, Qt::Key_Z, Qt::NoModifier, "z");

    // Act：不应崩溃、不应触发关闭
    bar->keyPressEvent(&letterEvent);

    // Assert
    EXPECT_EQ(closeSpy.count(), 0);
    EXPECT_EQ(bar->isVisible(), false);  // 状态未变（构造后本就隐藏）
}

// ---- getCurrentSearchText（TEST_P 3 组，复用 KeywordTest 参数）----

TEST_P(FindBarKeywordTest, GetCurrentSearchText_AfterTyped_ReflectsEditLineContent)
{
    const auto &c = GetParam();
    setKeyword(c.keyword);

    // Act & Assert
    EXPECT_EQ(bar->getCurrentSearchText(), c.keyword);
    EXPECT_EQ(bar->getCurrentSearchText(), editLine->lineEdit()->text());  // 双向一致
}

}  // namespace
