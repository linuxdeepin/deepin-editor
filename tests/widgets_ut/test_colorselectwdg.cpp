// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// ============================================================================
// ColorLabel + ColorSelectWdg 单元测试（B11 / src/widgets/ColorSelectWdg.cpp）
//
// 策略：真实 offscreen 构造（DWidget 族）。paintEvent 经 grab() 触发，
// 鼠标/进出事件经 QApplication::sendEvent 合成派发。
//
// 分支清单（来源：ColorSelectWdg.cpp）与用例映射：
// - ColorLabel::paintEvent：normal/hover/pressed × selected 组合
//     → ColorLabel_Paint_AllStates_CompletesWithoutError（TEST_P 参数化）
// - ColorLabel::enterEvent / leaveEvent → ColorLabel_EnterAndLeave_TogglesHover
// - ColorLabel::mousePressEvent：左键置按下 / 右键忽略
//     → ColorLabel_MousePress_LeftSetsPressedRightIgnored
// - ColorLabel::mouseReleaseEvent：左键且已按下 → 选中 + sigColorClicked；
//     未按下 / 右键 → 无信号
//     → ColorLabel_MouseRelease_EmitsSignalWhenPressed /
//       ColorLabel_MouseRelease_NotPressedEmitsNothing
// - ColorLabel::setColorSelected：同值早退 / 翻转
//     → ColorLabel_SetColorSelected_ToggleAndIdempotent
// - ColorSelectWdg ctor：text 空/非空两套布局
//     → ColorSelectWdg_EmptyText_CompactLayout /
//       ColorSelectWdg_WithText_ButtonAndDefaultColor
// - initWidget（私有，经 ctor）：首个色块默认选中
// - ColorSelectWdg dtor：布局指针回收 → ColorSelectWdg_Destructor_CleansLayouts
// - setTheme：light/dark/未知 → ColorSelectWdg_SetTheme_Param（TEST_P）
// - getDefaultColor → ColorSelectWdg_WithText_ButtonAndDefaultColor
// - eventFilter：m_pLabel 恒为 null（源码从未赋值，记录缺陷）→ object==nullptr
//     且左键 → 发信号返回 true；非左键 → false；object 非 null → 基类
//     → ColorSelectWdg_EventFilter_NullObjectBranches
// - 点击色块互斥（sigColorClicked → 其它 label 取消选中 + 默认色更新）
//     → ColorSelectWdg_LabelClick_ExclusivelySelects
// - 按钮点击 → sigColorSelected(true, defaultColor)
//     → ColorSelectWdg_ButtonClick_EmitsDefaultColorSignal
// ============================================================================

#include <gtest/gtest.h>
#include "stubext.h"

#include <QApplication>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QPointer>
#include <QDir>
#include <QPushButton>

#include "widgets/ColorSelectWdg.h"
#include "common/utils.h"

class ColorSelectWdgTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        s_configHome = new QTemporaryDir();
        s_dataHome = new QTemporaryDir();
        const QString xdgConfig = s_configHome->filePath("xdg-config");
        const QString xdgData = s_dataHome->filePath("xdg-data");
        QDir().mkpath(xdgConfig);
        QDir().mkpath(xdgData);
        qputenv("XDG_CONFIG_HOME", xdgConfig.toUtf8());
        qputenv("XDG_DATA_HOME", xdgData.toUtf8());

        int argc = 1;
        char *argv[] = {s_argv, nullptr};
        s_app = new QApplication(argc, argv);
        QApplication::setOrganizationName(QStringLiteral("deepin"));
        QApplication::setApplicationName(QStringLiteral("deepin-editor"));
    }

    static void TearDownTestSuite()
    {
        qunsetenv("XDG_CONFIG_HOME");
        qunsetenv("XDG_DATA_HOME");
    }

    void SetUp() override
    {
        const QList<QColor> colors = Utils::getHiglightColorList();
        ASSERT_FALSE(colors.isEmpty());
        label = new ColorLabel(colors.first());
        firstColor = colors.first();
    }

    void TearDown() override
    {
        delete label;
        label = nullptr;
        stub.clear();
    }

    // 合成鼠标事件并直接派发给 ColorLabel（Qt6 五参 ctor）
    void sendMouseEvent(QEvent::Type type, Qt::MouseButton button,
                        Qt::MouseButtons buttons)
    {
        const QPointF local(4, 4);
        QMouseEvent ev(type, local, local, button, buttons, Qt::NoModifier);
        QApplication::sendEvent(label, &ev);
    }

    stub_ext::StubExt stub;
    ColorLabel *label = nullptr;
    QColor firstColor;

    static QApplication *s_app;
    static QTemporaryDir *s_configHome;
    static QTemporaryDir *s_dataHome;
    static char s_argv[];
};

QApplication *ColorSelectWdgTest::s_app = nullptr;
QTemporaryDir *ColorSelectWdgTest::s_configHome = nullptr;
QTemporaryDir *ColorSelectWdgTest::s_dataHome = nullptr;
char ColorSelectWdgTest::s_argv[] = "test_colorselectwdg";

// ------------------------------------------------------------
// ColorLabel：状态与事件
// ------------------------------------------------------------

TEST_F(ColorSelectWdgTest, ColorLabel_InitialState_ReturnsCtorColor)
{
    // Assert: 构造色即返回色，未选中、无按下/悬停
    EXPECT_EQ(label->getColor(), firstColor);
    EXPECT_FALSE(label->isSelected());
    EXPECT_FALSE(label->m_bHover);
    EXPECT_FALSE(label->m_bPressed);
}

TEST_F(ColorSelectWdgTest, ColorLabel_SetColorSelected_ToggleAndIdempotent)
{
    // Act: 首次置选中
    label->setColorSelected(true);

    // Assert: 状态翻转
    EXPECT_TRUE(label->isSelected());

    // Act: 同值再设（早退分支，状态不变）
    label->setColorSelected(true);
    EXPECT_TRUE(label->isSelected());

    // Act: 取消选中
    label->setColorSelected(false);
    EXPECT_FALSE(label->isSelected());
}

TEST_F(ColorSelectWdgTest, ColorLabel_EnterAndLeave_TogglesHover)
{
    // Arrange: 合成进入事件
    const QPointF local(2, 2);
    QEnterEvent enter(local, local, local);
    QApplication::sendEvent(label, &enter);

    // Assert: 悬停态置位
    EXPECT_TRUE(label->m_bHover);

    // Act: 离开事件（复位 hover 与 pressed）
    label->m_bPressed = true;
    QEvent leave(QEvent::Leave);
    QApplication::sendEvent(label, &leave);

    // Assert: 两个标志均复位
    EXPECT_FALSE(label->m_bHover);
    EXPECT_FALSE(label->m_bPressed);
}

TEST_F(ColorSelectWdgTest, ColorLabel_MousePress_LeftSetsPressedRightIgnored)
{
    // Act: 左键按下
    sendMouseEvent(QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton);

    // Assert: 按下态置位
    EXPECT_TRUE(label->m_bPressed);

    // Act: 右键按下（不置位）
    sendMouseEvent(QEvent::MouseButtonPress, Qt::RightButton, Qt::RightButton);
    EXPECT_TRUE(label->m_bPressed); // 仍为此前左键置位的状态

    // Arrange: 复位后仅右键 → 不进入按下态
    label->m_bPressed = false;
    sendMouseEvent(QEvent::MouseButtonPress, Qt::RightButton, Qt::RightButton);
    EXPECT_FALSE(label->m_bPressed);
}

TEST_F(ColorSelectWdgTest, ColorLabel_MouseRelease_EmitsSignalWhenPressed)
{
    // Arrange: 左键按下后释放
    QSignalSpy spy(label, &ColorLabel::sigColorClicked);
    sendMouseEvent(QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton);
    ASSERT_TRUE(label->m_bPressed);

    // Act
    sendMouseEvent(QEvent::MouseButtonRelease, Qt::LeftButton, Qt::NoButton);

    // Assert: 选中态置位 + 信号携带 (true, color)
    EXPECT_TRUE(label->isSelected());
    ASSERT_EQ(spy.count(), 1);
    EXPECT_TRUE(spy.at(0).at(0).toBool());
    EXPECT_EQ(spy.at(0).at(1).value<QColor>(), firstColor);
}

TEST_F(ColorSelectWdgTest, ColorLabel_MouseRelease_NotPressedEmitsNothing)
{
    // Arrange: 未经过按下直接释放
    QSignalSpy spy(label, &ColorLabel::sigColorClicked);
    ASSERT_FALSE(label->m_bPressed);

    // Act
    sendMouseEvent(QEvent::MouseButtonRelease, Qt::LeftButton, Qt::NoButton);

    // Assert: 无信号、未选中（强异常安全：状态未变）
    EXPECT_EQ(spy.count(), 0);
    EXPECT_FALSE(label->isSelected());
}

// paintEvent 全状态组合参数化（normal/hover/pressed × selected）
namespace {
struct PaintState {
    bool hover;
    bool pressed;
    bool selected;
};
} // namespace

class ColorLabelPaintTest : public ColorSelectWdgTest,
                            public ::testing::WithParamInterface<PaintState> {
};

TEST_P(ColorLabelPaintTest, Paint_AllStates_CompletesWithoutError)
{
    // Arrange: 注入状态
    const PaintState st = GetParam();
    label->m_bHover = st.hover;
    label->m_bPressed = st.pressed;
    label->m_bSelected = st.selected;
    label->resize(24, 24);

    // Act: grab 强制同步触发 paintEvent（offscreen 真实绘制）
    const QPixmap px = label->grab();

    // Assert: 产出非空图像且状态未被破坏
    EXPECT_FALSE(px.isNull());
    EXPECT_EQ(px.width(), 24);
    EXPECT_EQ(label->isSelected(), st.selected);
}

INSTANTIATE_TEST_SUITE_P(PaintStates, ColorLabelPaintTest,
                         ::testing::Values(
                             PaintState{false, false, false},
                             PaintState{false, false, true},
                             PaintState{true, false, false},
                             PaintState{true, false, true},
                             PaintState{false, true, false},
                             PaintState{false, true, true},
                             PaintState{true, true, true}));

// ------------------------------------------------------------
// ColorSelectWdg：构造 / 主题 / 默认色
// ------------------------------------------------------------

TEST_F(ColorSelectWdgTest, ColorSelectWdg_EmptyText_CompactLayout)
{
    // Act
    ColorSelectWdg w{QString()};

    // Assert: 紧凑高度 35、无按钮、默认色为调色板首色、色块全部创建
    EXPECT_EQ(w.height(), 35);
    EXPECT_EQ(w.findChild<QPushButton *>("PButton"), nullptr);
    EXPECT_EQ(w.getDefaultColor(), firstColor);
    EXPECT_EQ(w.m_colorLabels.size(), Utils::getHiglightColorList().size());
    EXPECT_TRUE(w.m_colorLabels.first()->isSelected());
    EXPECT_EQ(w.m_pMainLayout, nullptr);
    EXPECT_NE(w.m_pHLayout2, nullptr);
}

TEST_F(ColorSelectWdgTest, ColorSelectWdg_WithText_ButtonAndDefaultColor)
{
    // Act
    ColorSelectWdg w(QStringLiteral("Mark"));

    // Assert: 带标题高度 60、按钮存在、主布局为纵向
    EXPECT_EQ(w.height(), 60);
    QPushButton *btn = w.findChild<QPushButton *>("PButton");
    ASSERT_NE(btn, nullptr);
    EXPECT_EQ(btn->text(), QString("Mark"));
    EXPECT_EQ(w.getDefaultColor(), firstColor);
    EXPECT_NE(w.m_pMainLayout, nullptr);
    EXPECT_NE(w.m_pHLayout1, nullptr);
}

namespace {
struct ThemeCase {
    QString theme;
    QString expectedText; // 空串 = 不修改
};
} // namespace

class ColorSelectThemeTest : public ColorSelectWdgTest,
                             public ::testing::WithParamInterface<ThemeCase> {
};

TEST_P(ColorSelectThemeTest, SetTheme_LightDarkUnknown_SetsTextColor)
{
    // Arrange
    ColorSelectWdg w(QStringLiteral("Mark"));
    w.m_textColor = QStringLiteral("#deadbeef");
    const ThemeCase c = GetParam();

    // Act
    w.setTheme(c.theme);

    // Assert: 主题色按分支更新或保持
    if (c.expectedText.isEmpty()) {
        EXPECT_EQ(w.m_textColor, QString("#deadbeef"));
    } else {
        EXPECT_EQ(w.m_textColor, c.expectedText);
    }
    // 状态不受影响（强异常安全）
    EXPECT_EQ(w.getDefaultColor(), firstColor);
}

INSTANTIATE_TEST_SUITE_P(ThemeVariants, ColorSelectThemeTest,
                         ::testing::Values(
                             ThemeCase{QStringLiteral("light"), QStringLiteral("#1f1c1b")},
                             ThemeCase{QStringLiteral("dark"), QStringLiteral("#cfcfc2")},
                             ThemeCase{QStringLiteral("unknown"), QString()}));

TEST_F(ColorSelectWdgTest, ColorSelectWdg_Destructor_CleansLayouts)
{
    // Arrange: 两种布局各建一个（父对象持有）
    QWidget host;
    auto *withText = new ColorSelectWdg(QStringLiteral("Mark"), &host);
    auto *compact = new ColorSelectWdg(QString(), &host);

    // Act: 析构（内部 delete 三个布局指针并置空）
    delete withText;
    delete compact;

    // Assert: 无崩溃即通过无法直接断言，改验证 host 子对象减少
    EXPECT_EQ(host.findChildren<ColorSelectWdg *>().count(), 0);
    EXPECT_TRUE(host.children().isEmpty());
}

// ------------------------------------------------------------
// ColorSelectWdg：交互信号
// ------------------------------------------------------------

TEST_F(ColorSelectWdgTest, ColorSelectWdg_ButtonClick_EmitsDefaultColorSignal)
{
    // Arrange
    ColorSelectWdg w(QStringLiteral("Mark"));
    QPushButton *btn = w.findChild<QPushButton *>("PButton");
    ASSERT_NE(btn, nullptr);
    QSignalSpy spy(&w, &ColorSelectWdg::sigColorSelected);

    // Act: 点击按钮
    btn->click();

    // Assert: 发送 (true, 默认色)
    ASSERT_EQ(spy.count(), 1);
    EXPECT_TRUE(spy.at(0).at(0).toBool());
    EXPECT_EQ(spy.at(0).at(1).value<QColor>(), firstColor);
    EXPECT_EQ(w.getDefaultColor(), firstColor);
}

TEST_F(ColorSelectWdgTest, ColorSelectWdg_LabelClick_ExclusivelySelects)
{
    // Arrange
    ColorSelectWdg w{QString()};
    ASSERT_GE(w.m_colorLabels.size(), 3);
    ColorLabel *first = w.m_colorLabels.at(0);
    ColorLabel *third = w.m_colorLabels.at(2);
    const QColor thirdColor = third->getColor();
    QSignalSpy spy(&w, &ColorSelectWdg::sigColorSelected);
    ASSERT_TRUE(first->isSelected());

    // Act: 点击第三个色块（左键按下 + 释放）
    const QPointF local(4, 4);
    QMouseEvent press(QEvent::MouseButtonPress, local, local,
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(third, &press);
    QMouseEvent release(QEvent::MouseButtonRelease, local, local,
                        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(third, &release);

    // Assert: 第三块选中、首块取消、默认色更新、信号发出
    EXPECT_TRUE(third->isSelected());
    EXPECT_FALSE(first->isSelected());
    EXPECT_EQ(w.getDefaultColor(), thirdColor);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(1).value<QColor>(), thirdColor);
}

// ------------------------------------------------------------
// eventFilter（原 m_pLabel 死分支已随缺陷修复删除，D-048：
// 该成员从未赋值，默认色选择由 m_pButton 的 clicked 信号实现）
// ------------------------------------------------------------

TEST_F(ColorSelectWdgTest, ColorSelectWdg_EventFilter_DelegatesToBase)
{
    // Arrange
    ColorSelectWdg w(QStringLiteral("Mark"));
    QSignalSpy spy(&w, &ColorSelectWdg::sigColorSelected);

    // Act: 任意 object 均透传基类，不再有死分支拦截
    QMouseEvent leftClick(QEvent::MouseButtonPress, QPointF(1, 1), QPointF(1, 1),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    bool retNull = w.eventFilter(nullptr, &leftClick);
    EXPECT_FALSE(retNull);

    QObject stranger;
    bool retStranger = w.eventFilter(&stranger, &leftClick);
    EXPECT_FALSE(retStranger);

    // Assert: 不经按钮点击不发射默认色信号
    EXPECT_EQ(spy.count(), 0);
}

// ============================================================
// PMS 批次 2 回归用例（bug 196825 / 67950）
// PMS: https://pms.uniontech.com/bug-view-196825.html  commit: 306a42de
// PMS: https://pms.uniontech.com/bug-view-67950.html  commit: 9eb4f93c
// 场景：颜色标记图标悬停与按压状态修复（306a42de）：ColorLabel 新增
//       m_bHover/m_bPressed 状态机（enter/leave/press/release），释放时仅在
//       按压有效时选中并发 sigColorClicked；内存释放修复（9eb4f93c）：
//       ~ColorSelectWdg() 显式回收布局，QPointer 托管断言析构释放
// ============================================================

// PMS: https://pms.uniontech.com/bug-view-196825.html  commit: 306a42de
// 场景：完整悬停-按压-释放状态机（306a42de）：enter 置 hover，press 仅置
//       pressed 不立即选中，release 选中并发信号，leave 复位悬停、选中保持
TEST_F(ColorSelectWdgTest, BUG196825_FullHoverPressRelease_StateMachineSequence)
{
    // Arrange
    QSignalSpy spy(label, &ColorLabel::sigColorClicked);
    ASSERT_FALSE(label->m_bHover);
    ASSERT_FALSE(label->m_bPressed);

    // Act 1: enter（QEnterEvent 合成）→ 悬停态置位
    const QPointF local(4, 4);
    QEnterEvent enter(local, local, local);
    QApplication::sendEvent(label, &enter);
    EXPECT_TRUE(label->m_bHover);

    // Act 2: 左键 press → 仅按压态置位，不立即选中（196825 修复语义）
    sendMouseEvent(QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton);
    EXPECT_TRUE(label->m_bPressed);
    EXPECT_FALSE(label->isSelected());
    EXPECT_EQ(spy.count(), 0);

    // Act 3: 左键 release → 选中 + 信号，pressed 复位，hover 保持
    sendMouseEvent(QEvent::MouseButtonRelease, Qt::LeftButton, Qt::NoButton);
    EXPECT_TRUE(label->isSelected());
    EXPECT_FALSE(label->m_bPressed);
    EXPECT_TRUE(label->m_bHover);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_TRUE(spy.at(0).at(0).toBool());
    EXPECT_EQ(spy.at(0).at(1).value<QColor>(), firstColor);

    // Act 4: leave → hover/pressed 复位，选中态保持
    QEvent leave(QEvent::Leave);
    QApplication::sendEvent(label, &leave);
    EXPECT_FALSE(label->m_bHover);
    EXPECT_FALSE(label->m_bPressed);
    EXPECT_TRUE(label->isSelected());
}

// PMS: https://pms.uniontech.com/bug-view-196825.html  commit: 306a42de
// 场景：按压期间移出（leave 复位 pressed）→ 回到图标上释放不再触发选择
TEST_F(ColorSelectWdgTest, BUG196825_LeaveDuringPress_ReleaseEmitsNothing)
{
    // Arrange: 进入并按下
    QSignalSpy spy(label, &ColorLabel::sigColorClicked);
    QEnterEvent enter(QPointF(4, 4), QPointF(4, 4), QPointF(4, 4));
    QApplication::sendEvent(label, &enter);
    sendMouseEvent(QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton);
    ASSERT_TRUE(label->m_bPressed);

    // Act: 按住期间移出图标（leave 复位 hover 与 pressed）
    QEvent leave(QEvent::Leave);
    QApplication::sendEvent(label, &leave);
    ASSERT_FALSE(label->m_bPressed);

    // Act: 释放（pressed 已复位 → mouseReleaseEvent 守卫拦截）
    sendMouseEvent(QEvent::MouseButtonRelease, Qt::LeftButton, Qt::NoButton);

    // Assert: 无信号、未选中（强异常安全：状态未被污染）
    EXPECT_EQ(spy.count(), 0);
    EXPECT_FALSE(label->isSelected());
    EXPECT_FALSE(label->m_bHover);
}

// PMS: https://pms.uniontech.com/bug-view-196825.html  commit: 306a42de
// PMS: https://pms.uniontech.com/bug-view-67950.html  commit: 9eb4f93c
// 场景：程序化选中（setColorSelected，默认色标记路径）后真实绘制：
//       paintEvent 选中圆环分支可渲染，选中态画面与未选中基线不同（
//       非逐点像素断言，仅断言渲染成功且画面随状态变化）
TEST_F(ColorSelectWdgTest, BUG196825_SetColorSelected_PaintRendersSelectedState)
{
    // Arrange
    label->resize(24, 24);

    // Act: 未选中基线绘制 → 置选中 → 再绘制（grab 同步触发 paintEvent）
    const QPixmap base = label->grab();
    label->setColorSelected(true);
    const QPixmap selected = label->grab();

    // Assert: 两态均成功渲染（paintEvent 正常/选中分支无错误），
    //         选中态画面与基线不同（选中圆环已画出）
    ASSERT_FALSE(base.isNull());
    ASSERT_FALSE(selected.isNull());
    EXPECT_TRUE(label->isSelected());
    EXPECT_NE(base.toImage(), selected.toImage());
}

// PMS: https://pms.uniontech.com/bug-view-67950.html  commit: 9eb4f93c
// 场景：内存释放修复（9eb4f93c）：~ColorSelectWdg() 显式 delete
//       m_pMainLayout/m_pHLayout1/m_pHLayout2 并置空；QPointer 托管控件与
//       子色块/按钮，断言两种布局变体析构后全部释放无悬空（Asan 泄漏回归）
TEST_F(ColorSelectWdgTest, BUG67950_ConstructDestroy_QPointerReleased)
{
    // Arrange: QPointer 托管控件与子对象（带标题/紧凑两种变体各一）
    QPointer<ColorSelectWdg> withTextGuard;
    QPointer<ColorSelectWdg> compactGuard;
    QPointer<ColorLabel> childLabel;
    QPointer<QPushButton> childButton;

    {
        ColorSelectWdg *withText = new ColorSelectWdg(QStringLiteral("Mark"));
        withTextGuard = withText;
        ASSERT_FALSE(withTextGuard.isNull());
        QPushButton *btn = withText->findChild<QPushButton *>("PButton");
        ASSERT_NE(btn, nullptr);
        childButton = btn;
        ASSERT_FALSE(withText->m_colorLabels.isEmpty());
        childLabel = withText->m_colorLabels.first();

        ColorSelectWdg *compact = new ColorSelectWdg(QString());
        compactGuard = compact;
        ASSERT_FALSE(compactGuard.isNull());

        // Act: 析构（修复：~ColorSelectWdg 回收三个布局指针并置空）
        delete withText;
        delete compact;
    }

    // Assert: QPointer 全部失效 → 控件、子色块、按钮均已释放，无悬空
    EXPECT_TRUE(withTextGuard.isNull());
    EXPECT_TRUE(compactGuard.isNull());
    EXPECT_TRUE(childLabel.isNull());
    EXPECT_TRUE(childButton.isNull());
}
