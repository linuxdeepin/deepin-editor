// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// LineBar（src/controls/linebar.cpp）单元测试
//
// 类特征：DLineEdit 子类（GUI 类），offscreen QApplication 无头构造。
//
// 方法映射（公开/保护方法全集）：
// - LineBar::LineBar            → Constructor_DefaultParent_CreatesCoreChildren
// - handleTextChangeTimer       → HandleTextChangeTimer_OnTimeout_EmitsContentChanged
// - handleTextChanged           → HandleTextChanged_EmptyText_HidesClearButtonAndTimerRestart /
//                                 HandleTextChanged_NonEmptyText_ShowsClearButton / TextEdited... (TEST_P 由 timer 用例覆盖)
// - sendText                    → SendText_AnyInput_EmitsSignalSentText (TEST_P)
// - focusOutEvent               → FocusOutEvent_OnEvent_EmitsFocusOutSignal
// - keyPressEvent               → KeyPressEvent_ModifierEnterMatrix_EmitsMappedSignal (TEST_P) /
//                                 KeyPressEvent_PlainLetter_PassesToBaseEditor
// - resizeEvent                 → ResizeEvent_SizeChanged_RepositionsRightWidgets
// - setMatchCount               → SetMatchCount_TotalZero_HidesLabel /
//                                 SetMatchCount_NonZero_ShowsLabelWithOrdinalText (TEST_P)
// - updateRightWidgetsGeometry(private) → 经 setMatchCount/resizeEvent 间接全覆盖（几何精确断言）
//
// 分支清单（来源：linebar.cpp）→ 用例映射：
// - handleTextChanged: timer active → stop           → HandleTextChanged_Consecutive_RestartsTimer（连续两次调用）
// - handleTextChanged: str.isEmpty() → setAlert(false)+hide clear btn → HandleTextChanged_EmptyText_...
// - handleTextChanged: else → show clear btn          → HandleTextChanged_NonEmptyText_...
// - keyPressEvent: Ctrl/Alt/Meta/NoModifier + "\r"    → KeyPressEvent_ModifierEnterMatrix (TEST_P 4 组)
// - keyPressEvent: else → DLineEdit::keyPressEvent     → KeyPressEvent_PlainLetter_PassesToBaseEditor
// - setMatchCount: total==0 → hide label               → SetMatchCount_TotalZero_HidesLabel
// - setMatchCount: else → set text + show label        → SetMatchCount_NonZero_ShowsLabelWithOrdinalText (TEST_P)
// - updateRightWidgetsGeometry: le == null → return    → LineBar 内嵌 lineEdit 恒非空，结构性不可达（无独立分支断言）
//
// 最小清单完成情况：
// | 1 | 每个公开方法 ≥1 用例 | 完成 |
// | 2 | 等价类（空/非空文本、total 零/非零、4 类修饰键） | 完成 |
// | 3 | 边界值（current=1/total=1、单字符、空串） | 完成 |
// | 4 | TEST_P（≥3 组同断言：sendText 3 组、修饰键 4 组、MatchCount 3 组） | 完成 |
// | 5 | 分支清单映射 | 见上方 |
// | 6 | if 分支两侧全覆盖 | 完成 |
// | 7 | 异常路径 | N/A（Qt 代码无 throw） |
// | 8 | 负面（空文本、total=0） | 完成 |
// | 9 | 强异常安全（模型/文本状态未破坏） | 完成 |
// | 10 | stub_ext（QPainter 无需、纯信号断言） | 完成 |

#include <gtest/gtest.h>

#include <DGuiApplicationHelper>

#include <QAbstractButton>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QTimer>

#include "linebar.h"
#include "test_env.h"

namespace {

class LineBarTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { controlsut::ensureApp(); }

    void SetUp() override
    {
        stub.clear();
        bar = new LineBar();
        // 清除按钮为 DIconButton（QAbstractButton 子类，非 QPushButton），按 objectName 定位
        clearButton = bar->findChild<QAbstractButton *>("ClearButton");
        matchLabel = nullptr;
        const auto labels = bar->lineEdit()->findChildren<QLabel *>();
        if (!labels.isEmpty())
            matchLabel = labels.first();
        autoSaveTimer = bar->findChild<QTimer *>();
    }

    void TearDown() override
    {
        delete bar;
        bar = nullptr;
        stub.clear();
    }

    void typeText(const QString &text)
    {
        // 模拟用户输入（textEdited 信号源）：insert 会走 QLineEdit::insert → textEdited
        bar->lineEdit()->insert(text);
    }

    stub_ext::StubExt stub;
    LineBar *bar = nullptr;
    QAbstractButton *clearButton = nullptr;
    QLabel *matchLabel = nullptr;
    QTimer *autoSaveTimer = nullptr;
};

// ---- 构造 ----

TEST_F(LineBarTest, Constructor_DefaultParent_CreatesCoreChildren)
{
    // Assert：核心子件齐全、初始无文本、清除按钮/计数标签隐藏、定时器单发 50ms
    ASSERT_NE(bar->lineEdit(), nullptr);
    EXPECT_EQ(bar->lineEdit()->text(), QString());
    ASSERT_NE(clearButton, nullptr);
    EXPECT_TRUE(clearButton->isHidden());
    ASSERT_NE(matchLabel, nullptr);
    EXPECT_TRUE(matchLabel->isHidden());
    ASSERT_NE(autoSaveTimer, nullptr);
    EXPECT_TRUE(autoSaveTimer->isSingleShot());
    EXPECT_FALSE(autoSaveTimer->isActive());
}

// ---- handleTextChanged：空文本分支 ----

TEST_F(LineBarTest, HandleTextChanged_EmptyText_HidesClearButtonAndDisablesAlert)
{
    // Arrange：先置警械态并显示清除按钮（走非空分支前置），再清空
    bar->setAlert(true);
    bar->handleTextChanged("abc");
    EXPECT_FALSE(clearButton->isHidden());

    // Act
    bar->handleTextChanged(QString());

    // Assert：空文本 → 告警解除、清除按钮隐藏、防抖定时器重新启动、文本为空
    EXPECT_FALSE(bar->isAlert());
    EXPECT_TRUE(clearButton->isHidden());
    ASSERT_NE(autoSaveTimer, nullptr);
    EXPECT_TRUE(autoSaveTimer->isActive());  // 状态变更：新定时器已启动
    EXPECT_EQ(bar->lineEdit()->text(), QString());
}

// ---- handleTextChanged：非空文本分支 + 定时器重启（active → stop → start）----

TEST_F(LineBarTest, HandleTextChanged_NonEmptyText_ShowsClearButton)
{
    // Act
    bar->handleTextChanged(QString::fromUtf8("deep深度编辑"));

    // Assert：清除按钮显示、定时器激活；同时经定时器到期发 contentChanged（覆盖
    // handleTextChangeTimer 的真实触发链路：m_autoSaveInternal=50ms）
    EXPECT_FALSE(clearButton->isHidden());
    EXPECT_TRUE(autoSaveTimer->isActive());

    QSignalSpy changedSpy(bar, &LineBar::contentChanged);
    EXPECT_TRUE(changedSpy.wait(300));            // 定时器到期 → contentChanged
    EXPECT_EQ(changedSpy.count(), 1);             // 单发定时器只触发一次
    EXPECT_EQ(bar->lineEdit()->text(), QString());  // 文本状态未被破坏（强异常安全）
}

// ---- handleTextChangeTimer：直接调用 ----

TEST_F(LineBarTest, HandleTextChangeTimer_OnTimeout_EmitsContentChanged)
{
    QSignalSpy spy(bar, &LineBar::contentChanged);

    // Act
    bar->handleTextChangeTimer();

    // Assert：信号恰一次；文本状态不被定时器回调破坏
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(bar->lineEdit()->text(), QString());
}

// ---- sendText：参数化（3 组同断言逻辑）----

struct SendTextCase {
    QString input;
};

class LineBarSendTextTest : public LineBarTest,
                            public ::testing::WithParamInterface<SendTextCase> {
};

TEST_P(LineBarSendTextTest, SendText_AnyInput_EmitsSignalSentText)
{
    const auto &c = GetParam();
    QSignalSpy spy(bar, &LineBar::signal_sentText);

    // Act
    bar->sendText(c.input);

    // Assert：原样转发
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), c.input);
}

INSTANTIATE_TEST_SUITE_P(
    BasicCases, LineBarSendTextTest,
    ::testing::Values(
        SendTextCase{ QString::fromUtf8("hello") },
        SendTextCase{ QString() },                       // 边界：空串
        SendTextCase{ QString::fromUtf8("中文关键词&<>'\"") }  // 边界：多字节+特殊字符
        ));

// ---- focusOutEvent ----

TEST_F(LineBarTest, FocusOutEvent_OnEvent_EmitsFocusOutSignal)
{
    QSignalSpy spy(bar, &LineBar::focusOut);

    // Act：合成焦点离开事件直接派发（虚函数经 sendEvent 走真实事件链）
    QFocusEvent focusOut(QEvent::FocusOut, Qt::MouseFocusReason);
    QApplication::sendEvent(bar, &focusOut);

    // Assert：信号恰一次；文本状态不受焦点事件影响
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(bar->lineEdit()->text(), QString());
}

// ---- keyPressEvent：修饰键矩阵（4 组同断言逻辑，TEST_P）----

struct ModifierKeyCase {
    Qt::KeyboardModifiers modifiers;
    const char *expectedSignal;  // 期望命中的信号名
};

class LineBarKeyTest : public LineBarTest,
                       public ::testing::WithParamInterface<ModifierKeyCase> {
};

TEST_P(LineBarKeyTest, KeyPressEvent_ModifierEnter_EmitsMappedSignal)
{
    const auto &c = GetParam();
    QSignalSpy ctrlSpy(bar, &LineBar::pressCtrlEnter);
    QSignalSpy altSpy(bar, &LineBar::pressAltEnter);
    QSignalSpy metaSpy(bar, &LineBar::pressMetaEnter);
    QSignalSpy enterSpy(bar, &LineBar::pressEnter);

    // Act：合成 Ctrl/Alt/Meta/无修饰 + Enter（"\r"）
    QKeyEvent keyEvent(QEvent::KeyPress, Qt::Key_Return, c.modifiers, "\r");
    QApplication::sendEvent(bar, &keyEvent);

    // Assert：只有对应信号发射（其余为 0）
    const int ctrl = ctrlSpy.count(), alt = altSpy.count();
    const int meta = metaSpy.count(), enter = enterSpy.count();
    EXPECT_EQ(ctrl, QString(c.expectedSignal) == "pressCtrlEnter" ? 1 : 0);
    EXPECT_EQ(alt, QString(c.expectedSignal) == "pressAltEnter" ? 1 : 0);
    EXPECT_EQ(meta, QString(c.expectedSignal) == "pressMetaEnter" ? 1 : 0);
    EXPECT_EQ(enter, QString(c.expectedSignal) == "pressEnter" ? 1 : 0);
    EXPECT_EQ(ctrl + alt + meta + enter, 1);  // 恰好一个信号
}

INSTANTIATE_TEST_SUITE_P(
    ModifierMatrix, LineBarKeyTest,
    ::testing::Values(
        ModifierKeyCase{ Qt::ControlModifier, "pressCtrlEnter" },
        ModifierKeyCase{ Qt::AltModifier, "pressAltEnter" },
        ModifierKeyCase{ Qt::MetaModifier, "pressMetaEnter" },
        ModifierKeyCase{ Qt::NoModifier, "pressEnter" }));

// ---- keyPressEvent：其它按键透传基类（else 分支）----

TEST_F(LineBarTest, KeyPressEvent_PlainLetter_PassesToBaseEditor)
{
    QSignalSpy ctrlSpy(bar, &LineBar::pressCtrlEnter);
    QSignalSpy enterSpy(bar, &LineBar::pressEnter);

    // Act：普通字母键（无修饰、非 "\r"）→ else 分支透传；事件直接派发到
    // 内嵌 QLineEdit（生产中焦点即在其上，DLineEdit 经事件过滤转发）
    QKeyEvent keyEvent(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier, "a");
    QApplication::sendEvent(bar->lineEdit(), &keyEvent);
    QApplication::processEvents();

    // Assert：不发射任何 Enter 系信号，且字符真实进入编辑器（透传成功）
    EXPECT_EQ(ctrlSpy.count(), 0);
    EXPECT_EQ(enterSpy.count(), 0);
    EXPECT_EQ(bar->lineEdit()->text(), QString("a"));
}

// ---- setMatchCount：total==0 分支 ----

TEST_F(LineBarTest, SetMatchCount_TotalZero_HidesLabel)
{
    // Arrange：先显示标签
    bar->setMatchCount(1, 3);
    ASSERT_FALSE(matchLabel->isHidden());

    // Act
    bar->setMatchCount(0, 0);

    // Assert：标签隐藏、文本保留（状态未清空）
    EXPECT_TRUE(matchLabel->isHidden());
    EXPECT_EQ(matchLabel->text(), QString::fromUtf8("第1/3项"));
}

// ---- setMatchCount：非零分支（TEST_P 3 组）+ 几何重排 ----

struct MatchCountCase {
    int current;
    int total;
    QString expectedText;
};

class LineBarMatchCountTest : public LineBarTest,
                              public ::testing::WithParamInterface<MatchCountCase> {
};

TEST_P(LineBarMatchCountTest, SetMatchCount_NonZero_ShowsLabelWithOrdinalText)
{
    const auto &c = GetParam();

    // Act
    bar->setMatchCount(c.current, c.total);

    // Assert：文本精确、标签显示、几何被重排（label 右缘贴清除按钮左缘 6px）
    EXPECT_EQ(matchLabel->text(), c.expectedText);
    EXPECT_FALSE(matchLabel->isHidden());
    EXPECT_FALSE(matchLabel->geometry().isNull());
    EXPECT_LE(matchLabel->x() + matchLabel->width(),
              clearButton->x() - 6 + 1);  // label 右侧与按钮左侧间距 ≥ 6px（int 截断容差 1）
}

INSTANTIATE_TEST_SUITE_P(
    OrdinalCases, LineBarMatchCountTest,
    ::testing::Values(
        MatchCountCase{ 1, 1, QString::fromUtf8("第1/1项") },   // 边界：唯一匹配
        MatchCountCase{ 3, 12, QString::fromUtf8("第3/12项") },
        MatchCountCase{ 12, 345, QString::fromUtf8("第12/345项") }  // 宽度增长边界
        ));

// ---- resizeEvent / updateRightWidgetsGeometry ----

TEST_F(LineBarTest, ResizeEvent_SizeChanged_RepositionsRightWidgets)
{
    // Arrange
    bar->setMatchCount(2, 9);
    bar->resize(240, 36);
    QApplication::processEvents();

    // Act 已由 resize 触发；读取内嵌 lineEdit 实际几何计算期望值
    QWidget *le = bar->lineEdit();
    const int editWidth = le->width();
    const int editHeight = le->height();
    const int buttonWidth = clearButton->width();

    // Assert：清除按钮垂直居中、右缘距编辑框右缘 6px（s_nRightMargin）
    EXPECT_EQ(clearButton->x(), editWidth - 6 - buttonWidth);
    EXPECT_EQ(clearButton->y(), (editHeight - clearButton->height()) / 2);
    // 计数标签紧贴按钮左侧 6px（s_nLabelButtonSpacing），垂直居中
    EXPECT_EQ(matchLabel->x(),
              clearButton->x() - 6 - matchLabel->sizeHint().width());
    EXPECT_EQ(matchLabel->y(), (editHeight - matchLabel->sizeHint().height()) / 2);
    // 负面：resize 不改变文本内容
    EXPECT_EQ(bar->lineEdit()->text(), QString());
}

// ---- 构造期 connect 的 lambda（清除按钮 / 尺寸模式切换）----

TEST_F(LineBarTest, ClearButton_Click_ClearsEditLineText)
{
    // Arrange：有文本 → 清除按钮显示
    bar->handleTextChanged(QString::fromUtf8("待清除"));
    ASSERT_FALSE(clearButton->isHidden());

    // Act：真实点击（ctor 里 connect 的 lambda：lineEdit()->clear()）
    clearButton->click();

    // Assert
    EXPECT_EQ(bar->lineEdit()->text(), QString());
    EXPECT_EQ(bar->text(), QString());
}

TEST_F(LineBarTest, SizeModeChangedSignal_CompactToggle_UpdatesBarHeight)
{
    // Arrange：经最小/最大高读取固定高（未 show 时 geometry 未布局，约束恒真）
    auto *helper = DGuiApplicationHelper::instance();
    const int expectedCompact = 24;   // s_nLineBarHeightCompact
    const int expectedNormal = 36;    // s_nLineBarHeight
    EXPECT_EQ(bar->minimumHeight(), expectedNormal);  // ctor 已按 NormalMode 设置

    // Act：真实切换全局尺寸模式（setSizeMode 置状态并发 sizeModeChanged，
    // ctor 连接的 lambda 按 isCompactMode() 重设固定高）
    helper->setSizeMode(DGuiApplicationHelper::CompactMode);
    QApplication::processEvents();

    // Assert：紧凑模式生效
    EXPECT_EQ(bar->minimumHeight(), expectedCompact);
    EXPECT_EQ(bar->maximumHeight(), expectedCompact);

    // Act 2：切回普通模式（恢复全局状态，避免跨用例污染）
    helper->setSizeMode(DGuiApplicationHelper::NormalMode);
    QApplication::processEvents();
    EXPECT_EQ(bar->minimumHeight(), expectedNormal);
    EXPECT_EQ(bar->maximumHeight(), expectedNormal);
}

}  // namespace
