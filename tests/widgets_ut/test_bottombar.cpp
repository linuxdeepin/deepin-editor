// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// ============================================================================
// BottomBar 单元测试（B11 / src/widgets/bottombar.cpp）
//
// 策略：真实 offscreen 构造（真实 DDropdownMenu 编码/高亮菜单、视图模式菜单、
// 行尾格式菜单）。EditWrapper 相关路径（m_pWrapper）由 stub_ext 拦截
// EditWrapper 非虚成员，父对象以真实 QWidget 内存承载（单继承链偏移 0）；
// slotSetTextEditFocus 用真实 Window + 真实 EditWrapper 验证信号回传。
//
// 分支清单（来源：bottombar.cpp）与用例映射：
// - ctor（真实编码/高亮菜单工厂 + 视图模式菜单 + 布局）
//     → Constructor_RealBuild_CreatesChildrenAndDefaults
// - updatePosition → UpdatePosition_SetsRowColumnText
// - updateWordCount（n-1 显示）→ UpdateWordCount_ShowsMinusOne
// - setEncodeName → SetEncodeName_UpdatesEncodeMenuText
// - setPalette（亮/暗两分支 theme 字符串）→ SetPalette_DarkAndLight_AppliedToSelf
// - updateSize → UpdateSize_SetsHeightAndFindFlag
// - setChildEnabled → SetChildEnabled_TogglesMenus
// - setChildrenFocus（带/不带 preOrderWidget）→ SetChildrenFocus_TrueFalse_TogglesTabOrder
// - setScaleLabelText（=12 / >12 / <12 / 下限钳制）
//     → SetScaleLabelText_Param（TEST_P 4 组）
// - setProgress（-1 早退 / 0<n<100 显示 / >=100 完成隐藏）
//     → SetProgress_Negative_Ignored / SetProgress_Mid_ShowsBars /
//       SetProgress_Complete_HidesBars
// - getEncodeMenu / getHighlightMenu → GetMenus_ReturnNonNullDistinct
// - getEndlineFormat 静态（Unix/Windows/Unknow/孤立 \r）
//     → GetEndlineFormatStatic_Param（TEST_P 4 组）
// - getEndlineFormat 成员 → SetEndlineMenuText_SetsFormatAndMenuText
// - initFormatMenu（ctor）+ onFormatMenuTrigged（同值早退 / 切换 / null 早退）
//     → OnFormatMenuTrigged_SwitchesFormatViaAction /
//       OnFormatMenuTrigged_NullAction_EarlyReturn
// - updateSizeMode（ctor）→ UpdateSizeMode_HeightMatchesMode
// - setEndlineMenuText（Unix/Unknow→Unix；Windows）
//     → SetEndlineMenuText_Param（TEST_P）
// - defaultHeight → DefaultHeight_MatchesSizeMode
// - setViewMode（ReadView/LivePreview/Edit/Wysiwyg-default）
//     → SetViewMode_Param（TEST_P）
// - setMarkdownAvailable → SetMarkdownAvailable_TogglesLivePreview
// - eventFilter（ApplicationFontChange / 其它）→ EventFilter_FontChange_UpdatesLabels
// - paintEvent（m_bIsFindOrReplace 两态）→ PaintEvent_BothFindStates_RendersFrame
// - slotSetTextEditFocus（真实 Window pressEsc 转发）→ SlotSetTextEditFocus_EmitsWindowPressEsc
// - 编码切换 lambda（loading 恢复 / 转换失败恢复 / 成功更新）
//     → EncodeMenuChange_Loading_RevertsToPreviousText 等
// - 高亮切换 lambda → HighlightMenuChange_UpdatesTextAndReloads
// ============================================================================

#include <gtest/gtest.h>
#include "stubext.h"

#include <QApplication>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QDir>
#include <QLabel>
#include <QPaintEvent>
#include <QDBusConnection>
#include <QDBusAbstractInterface>
#include <QDBusMessage>

#include "widgets/bottombar.h"
#include "widgets/window.h"
#include "editor/editwrapper.h"
#include "editor/dtextedit.h"
#include "editor/markdown/viewmode.h"
#include "common/settings.h"

class BottomBarTest : public ::testing::Test {
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
        Settings::instance();
        qRegisterMetaType<ViewMode>("ViewMode");
    }

    static void TearDownTestSuite()
    {
        qunsetenv("XDG_CONFIG_HOME");
        qunsetenv("XDG_DATA_HOME");
    }

    void SetUp() override
    {
        installWrapperStubs();
        m_bar = new BottomBar(&m_carrier);
    }

    void TearDown() override
    {
        delete m_bar;
        m_bar = nullptr;
        stub.clear();
    }

    // EditWrapper 非虚成员拦截：m_pWrapper 为 carrier 的 reinterpret 伪指针，
    // 所有被调 EditWrapper 方法在到达前已被替换为记录桩
    void installWrapperStubs()
    {
        stub.set_lamda(&EditWrapper::getFileLoading,
                       [this](EditWrapper *) -> bool { return m_fileLoading; });
        stub.set_lamda(&EditWrapper::reloadFileEncode,
                       [this](EditWrapper *, QByteArray encode) -> bool {
                           ++m_reloadEncodeCalls;
                           m_lastReloadEncode = encode;
                           return m_reloadEncodeResult;
                       });
        stub.set_lamda(&EditWrapper::reloadFileHighlight,
                       [this](EditWrapper *, QString name) {
                           ++m_reloadHighlightCalls;
                           m_lastReloadHighlight = name;
                       });
        stub.set_lamda(&TextEdit::onEndlineFormatChanged,
                       [this](TextEdit *, BottomBar::EndlineFormat from, BottomBar::EndlineFormat to) {
                           ++m_endlineChangeCalls;
                           m_lastEndlineFrom = from;
                           m_lastEndlineTo = to;
                       });
    }

    stub_ext::StubExt stub;
    BottomBar *m_bar = nullptr;
    QWidget m_carrier; // 承载 EditWrapper 伪指针的真实 QObject 内存

    bool m_fileLoading = false;
    bool m_reloadEncodeResult = true;
    int m_reloadEncodeCalls = 0;
    QByteArray m_lastReloadEncode;
    int m_reloadHighlightCalls = 0;
    QString m_lastReloadHighlight;
    int m_endlineChangeCalls = 0;
    BottomBar::EndlineFormat m_lastEndlineFrom = BottomBar::Unknow;
    BottomBar::EndlineFormat m_lastEndlineTo = BottomBar::Unknow;

    static QApplication *s_app;
    static QTemporaryDir *s_configHome;
    static QTemporaryDir *s_dataHome;
    static char s_argv[];
};

QApplication *BottomBarTest::s_app = nullptr;
QTemporaryDir *BottomBarTest::s_configHome = nullptr;
QTemporaryDir *BottomBarTest::s_dataHome = nullptr;
char BottomBarTest::s_argv[] = "test_bottombar";

// ------------------------------------------------------------
// 构造
// ------------------------------------------------------------

TEST_F(BottomBarTest, Constructor_RealBuild_CreatesChildrenAndDefaults)
{
    // Assert: 编码/高亮/视图模式/行尾格式菜单全部就位，初始文本正确
    ASSERT_NE(m_bar->getEncodeMenu(), nullptr);
    ASSERT_NE(m_bar->getHighlightMenu(), nullptr);
    EXPECT_EQ(m_bar->getEncodeMenu()->getCurrentText(), QString("UTF-8"));
    EXPECT_EQ(m_bar->getHighlightMenu()->getCurrentText(), QString("None"));
    EXPECT_EQ(m_bar->m_formatMenu->getCurrentText(), QString("Unix"));
    EXPECT_EQ(m_bar->m_pViewModeMenu->getCurrentText(), QString("Edit Mode"));
    EXPECT_EQ(m_bar->m_pPositionLabel->text(), QString("Row 1  Column 1"));
    EXPECT_EQ(m_bar->m_pCharCountLabel->text(), QString("Characters 0"));
    EXPECT_EQ(m_bar->m_endlineFormat, BottomBar::Unix);
    // 默认非 md：「实时预览」置灰
    EXPECT_FALSE(m_bar->m_actLivePreview->isEnabled());
    EXPECT_TRUE(m_bar->m_actEditView->isChecked());
}

// ------------------------------------------------------------
// 状态更新
// ------------------------------------------------------------

TEST_F(BottomBarTest, UpdatePosition_SetsRowColumnText)
{
    // Act
    m_bar->updatePosition(12, 34);

    // Assert: 位置标签精确更新
    EXPECT_EQ(m_bar->m_pPositionLabel->text(), QString("Row 12  Column 34"));
    m_bar->updatePosition(1, 1);
    EXPECT_EQ(m_bar->m_pPositionLabel->text(), QString("Row 1  Column 1"));
}

TEST_F(BottomBarTest, UpdateWordCount_ShowsMinusOne)
{
    // Act
    m_bar->updateWordCount(10);

    // Assert: 字符计数显示 n-1
    EXPECT_EQ(m_bar->m_pCharCountLabel->text(), QString("Characters 9"));
    m_bar->updateWordCount(1);
    EXPECT_EQ(m_bar->m_pCharCountLabel->text(), QString("Characters 0"));
}

TEST_F(BottomBarTest, SetEncodeName_UpdatesEncodeMenuText)
{
    // Act
    m_bar->setEncodeName(QStringLiteral("GBK"));

    // Assert
    EXPECT_EQ(m_bar->getEncodeMenu()->getCurrentText(), QString("GBK"));
    EXPECT_EQ(m_bar->m_pEncodeMenu, m_bar->getEncodeMenu());
}

TEST_F(BottomBarTest, SetPalette_DarkAndLight_AppliedToSelf)
{
    // Arrange: 暗色窗口背景（lightness < 128 → dark 主题分支）
    QPalette dark;
    dark.setColor(QPalette::Window, QColor(20, 20, 20));

    // Act
    m_bar->setPalette(dark);

    // Assert: 自身 palette 应用 + 各菜单按钮 palette 不再为空
    EXPECT_EQ(m_bar->palette().color(QPalette::Window), QColor(20, 20, 20));

    // Act: 亮色分支
    QPalette light;
    light.setColor(QPalette::Window, QColor(240, 240, 240));
    m_bar->setPalette(light);
    EXPECT_EQ(m_bar->palette().color(QPalette::Window), QColor(240, 240, 240));
    EXPECT_NE(m_bar->getEncodeMenu()->getButton()->palette().color(QPalette::WindowText),
              QColor());
}

TEST_F(BottomBarTest, UpdateSize_SetsHeightAndFindFlag)
{
    // Act
    m_bar->updateSize(48, true);

    // Assert: 高度与查找态标志同时更新
    EXPECT_EQ(m_bar->height(), 48);
    EXPECT_TRUE(m_bar->m_bIsFindOrReplace);

    // Act / Assert: 复位
    m_bar->updateSize(BottomBar::defaultHeight(), false);
    EXPECT_EQ(m_bar->height(), BottomBar::defaultHeight());
    EXPECT_FALSE(m_bar->m_bIsFindOrReplace);
}

TEST_F(BottomBarTest, SetChildEnabled_TogglesMenus)
{
    // Act: 禁用
    m_bar->setChildEnabled(false);

    // Assert: 三个菜单均禁用且不响应菜单请求
    EXPECT_FALSE(m_bar->getEncodeMenu()->isEnabled());
    EXPECT_FALSE(m_bar->getHighlightMenu()->isEnabled());
    EXPECT_FALSE(m_bar->m_formatMenu->isEnabled());
    EXPECT_FALSE(m_bar->getEncodeMenu()->isRequest);

    // Act / Assert: 恢复
    m_bar->setChildEnabled(true);
    EXPECT_TRUE(m_bar->getEncodeMenu()->isEnabled());
    EXPECT_TRUE(m_bar->getHighlightMenu()->isEnabled());
    EXPECT_TRUE(m_bar->m_formatMenu->isEnabled());
    EXPECT_TRUE(m_bar->getEncodeMenu()->isRequest);
}

TEST_F(BottomBarTest, SetChildrenFocus_TrueFalse_TogglesTabOrder)
{
    // Act: 开启焦点链（带前置控件分支）
    QWidget preOrder;
    m_bar->setChildrenFocus(true, &preOrder);

    // Assert: 各菜单工具按钮进入 Tab 焦点链
    EXPECT_EQ(m_bar->getEncodeMenu()->getButton()->focusPolicy(), Qt::StrongFocus);
    EXPECT_EQ(m_bar->m_formatMenu->getButton()->focusPolicy(), Qt::StrongFocus);
    EXPECT_EQ(m_bar->getHighlightMenu()->getButton()->focusPolicy(), Qt::StrongFocus);

    // Act: 无前置控件分支 + 关闭
    m_bar->setChildrenFocus(true);
    EXPECT_EQ(m_bar->getEncodeMenu()->getButton()->focusPolicy(), Qt::StrongFocus);

    m_bar->setChildrenFocus(false);
    EXPECT_EQ(m_bar->getEncodeMenu()->getButton()->focusPolicy(), Qt::NoFocus);
    EXPECT_EQ(m_bar->m_formatMenu->getButton()->focusPolicy(), Qt::NoFocus);
}

TEST_F(BottomBarTest, SetProgress_Negative_Ignored)
{
    // Arrange: 展示底栏（isVisible 需祖先可见）
    m_carrier.show();
    m_bar->show();
    QApplication::processEvents();

    // Act
    m_bar->setProgress(-1);

    // Assert: 早退，进度条保持隐藏、值保持初始（DProgressBar 初始 -1）
    EXPECT_FALSE(m_bar->m_progressBar->isVisible());
    EXPECT_FALSE(m_bar->m_progressLabel->isVisible());
    EXPECT_EQ(m_bar->m_progressBar->value(), -1);
}

TEST_F(BottomBarTest, SetProgress_Mid_ShowsBars)
{
    // Arrange: 父载体 + 底栏一起展示（isVisible 需全祖先可见）
    m_carrier.show();
    m_bar->show();
    QApplication::processEvents();

    // Act
    m_bar->setProgress(50);

    // Assert: 进度条与标签显示且值正确
    EXPECT_TRUE(m_bar->m_progressBar->isVisible());
    EXPECT_TRUE(m_bar->m_progressLabel->isVisible());
    EXPECT_EQ(m_bar->m_progressBar->value(), 50);
}

TEST_F(BottomBarTest, SetProgress_Complete_HidesBars)
{
    // Arrange: 先显示
    m_carrier.show();
    m_bar->show();
    QApplication::processEvents();
    m_bar->setProgress(50);
    ASSERT_TRUE(m_bar->m_progressBar->isVisible());

    // Act: 完成
    m_bar->setProgress(100);

    // Assert: 达到 100 后隐藏，但值保留
    EXPECT_FALSE(m_bar->m_progressBar->isVisible());
    EXPECT_FALSE(m_bar->m_progressLabel->isVisible());
    EXPECT_EQ(m_bar->m_progressBar->value(), 100);
}

TEST_F(BottomBarTest, GetMenus_ReturnNonNullDistinct)
{
    // Assert: 两个菜单均有效且互不相同
    ASSERT_NE(m_bar->getEncodeMenu(), nullptr);
    ASSERT_NE(m_bar->getHighlightMenu(), nullptr);
    EXPECT_NE(m_bar->getEncodeMenu(), m_bar->getHighlightMenu());
}

// ------------------------------------------------------------
// setScaleLabelText 参数化（=12 / >12 / <12 / 下限）
// ------------------------------------------------------------

namespace {
struct ScaleCase {
    qreal fontSize;
    QString expected;
};
} // namespace

class ScaleTextTest : public BottomBarTest,
                      public ::testing::WithParamInterface<ScaleCase> {
};

TEST_P(ScaleTextTest, SetScaleLabelText_EachRange_ShowsExpectedPercent)
{
    // Arrange
    const ScaleCase c = GetParam();

    // Act
    m_bar->setScaleLabelText(c.fontSize);

    // Assert: 缩放标签精确百分比（与实现公式一致：>12 线性放大、<12 线性缩小并下限 10%）
    EXPECT_EQ(m_bar->m_scaleLabel->text(), c.expected);
    EXPECT_TRUE(m_bar->m_scaleLabel->text().endsWith(QStringLiteral("%")));
}

INSTANTIATE_TEST_SUITE_P(ScaleVariants, ScaleTextTest,
                         ::testing::Values(
                             ScaleCase{12.0, QStringLiteral("100%")},
                             ScaleCase{50.0, QStringLiteral("500%")},
                             ScaleCase{20.0, QStringLiteral("184%")},
                             ScaleCase{10.0, QStringLiteral("55%")},
                             ScaleCase{8.0, QStringLiteral("10%")})); // 下限钳制

// ------------------------------------------------------------
// 行尾格式
// ------------------------------------------------------------

namespace {
struct EndlineCase {
    QByteArray text;
    BottomBar::EndlineFormat expected;
};
} // namespace

class EndlineFormatTest : public BottomBarTest,
                          public ::testing::WithParamInterface<EndlineCase> {
};

TEST_P(EndlineFormatTest, GetEndlineFormatStatic_EachInput_ReturnsExpected)
{
    // Act / Assert: 静态判定（首个换行符形态决定）
    EXPECT_EQ(BottomBar::getEndlineFormat(GetParam().text), GetParam().expected);
    EXPECT_EQ(m_endlineChangeCalls, 0); // 纯函数不触发编辑器
}

INSTANTIATE_TEST_SUITE_P(EndlineVariants, EndlineFormatTest,
                         ::testing::Values(
                             EndlineCase{QByteArray("a\nb"), BottomBar::Unix},
                             EndlineCase{QByteArray("a\r\nb"), BottomBar::Windows},
                             EndlineCase{QByteArray("abc"), BottomBar::Unknow},
                             EndlineCase{QByteArray("a\r"), BottomBar::Unknow}, // 孤立 \r（i+1 越界）
                             EndlineCase{QByteArray("\n"), BottomBar::Unix}));

namespace {
struct MenuTextCase {
    BottomBar::EndlineFormat input;
    QString expectedMenuText;
    BottomBar::EndlineFormat expectedFormat;
};
} // namespace

class EndlineMenuTextTest : public BottomBarTest,
                            public ::testing::WithParamInterface<MenuTextCase> {
};

TEST_P(EndlineMenuTextTest, SetEndlineMenuText_EachFormat_SetsMenuAndState)
{
    // Arrange
    const MenuTextCase c = GetParam();

    // Act
    m_bar->setEndlineMenuText(c.input);

    // Assert: 菜单文本与内部格式同步
    EXPECT_EQ(m_bar->m_formatMenu->getCurrentText(), c.expectedMenuText);
    EXPECT_EQ(m_bar->getEndlineFormat(), c.expectedFormat);
}

INSTANTIATE_TEST_SUITE_P(MenuTextVariants, EndlineMenuTextTest,
                         ::testing::Values(
                             MenuTextCase{BottomBar::Unix, QStringLiteral("Unix"), BottomBar::Unix},
                             MenuTextCase{BottomBar::Unknow, QStringLiteral("Unix"), BottomBar::Unix},
                             MenuTextCase{BottomBar::Windows, QStringLiteral("Windows"), BottomBar::Windows}));

TEST_F(BottomBarTest, OnFormatMenuTrigged_SwitchesFormatViaAction)
{
    // Arrange: 初始 Unix
    ASSERT_EQ(m_bar->getEndlineFormat(), BottomBar::Unix);
    QAction *windowsAction = m_bar->findChild<QAction *>("WindowsAction");
    QAction *unixAction = m_bar->findChild<QAction *>("UnixAction");
    ASSERT_NE(windowsAction, nullptr);
    ASSERT_NE(unixAction, nullptr);

    // Act: 触发 Windows（真实 QActionGroup::triggered → onFormatMenuTrigged）
    windowsAction->trigger();

    // Assert: 编辑器收到 Unix→Windows 切换，内部格式与菜单同步
    EXPECT_EQ(m_endlineChangeCalls, 1);
    EXPECT_EQ(m_lastEndlineFrom, BottomBar::Unix);
    EXPECT_EQ(m_lastEndlineTo, BottomBar::Windows);
    EXPECT_EQ(m_bar->getEndlineFormat(), BottomBar::Windows);

    // Act: 再次触发同值 → 早退（不重复通知编辑器）
    windowsAction->trigger();
    EXPECT_EQ(m_endlineChangeCalls, 1);
    EXPECT_EQ(m_bar->getEndlineFormat(), BottomBar::Windows);

    // Act: 切回 Unix
    unixAction->trigger();
    EXPECT_EQ(m_endlineChangeCalls, 2);
    EXPECT_EQ(m_lastEndlineTo, BottomBar::Unix);
    EXPECT_EQ(m_bar->getEndlineFormat(), BottomBar::Unix);
}

TEST_F(BottomBarTest, OnFormatMenuTrigged_NullAction_EarlyReturn)
{
    // Act: null 早退分支
    m_bar->onFormatMenuTrigged(nullptr);

    // Assert: 状态保持（强异常安全）
    EXPECT_EQ(m_endlineChangeCalls, 0);
    EXPECT_EQ(m_bar->getEndlineFormat(), BottomBar::Unix);
    EXPECT_EQ(m_bar->m_formatMenu->getCurrentText(), QString("Unix"));
}

TEST_F(BottomBarTest, UpdateSizeMode_HeightMatchesMode)
{
    // Act（ctor 已调用，直接复核当前模式分支）
    m_bar->updateSizeMode();

    // Assert: 高度与布局模式匹配且为正值
    EXPECT_EQ(m_bar->height(), BottomBar::defaultHeight());
    EXPECT_GT(m_bar->height(), 0);
}

TEST_F(BottomBarTest, DefaultHeight_MatchesSizeMode)
{
    // Act / Assert: 静态高度 = 当前布局模式对应固定值（紧凑 26 / 标准 32）
    EXPECT_EQ(BottomBar::defaultHeight(),
              DGuiApplicationHelper::isCompactMode() ? 26 : 32);
    EXPECT_TRUE(BottomBar::defaultHeight() == 26 || BottomBar::defaultHeight() == 32);
}

// ------------------------------------------------------------
// Markdown 视图模式
// ------------------------------------------------------------

namespace {
struct ViewModeCase {
    ViewMode mode;
    QString expectedText;
};
} // namespace

class ViewModeMenuTest : public BottomBarTest,
                         public ::testing::WithParamInterface<ViewModeCase> {
};

TEST_P(ViewModeMenuTest, SetViewMode_EachMode_SelectsActionAndText)
{
    // Arrange
    const ViewModeCase c = GetParam();

    // Act
    m_bar->setViewMode(c.mode);

    // Assert: 视图模式菜单文本与勾选 action 同步
    EXPECT_EQ(m_bar->m_pViewModeMenu->getCurrentText(), c.expectedText);
    EXPECT_FALSE(m_bar->m_actLivePreview->isEnabled()); // 未开启 markdown 可用性
}

INSTANTIATE_TEST_SUITE_P(ViewModeVariants, ViewModeMenuTest,
                         ::testing::Values(
                             ViewModeCase{ViewMode::Edit, QStringLiteral("Edit Mode")},
                             ViewModeCase{ViewMode::ReadView, QStringLiteral("Read View")},
                             ViewModeCase{ViewMode::LivePreview, QStringLiteral("Live Preview")},
                             ViewModeCase{ViewMode::Wysiwyg, QStringLiteral("Edit Mode")}));

TEST_F(BottomBarTest, SetMarkdownAvailable_TogglesLivePreview)
{
    // Arrange: 默认置灰
    ASSERT_FALSE(m_bar->m_actLivePreview->isEnabled());

    // Act
    m_bar->setMarkdownAvailable(true);

    // Assert: md 文件可用时「实时预览」解禁；再关灰
    EXPECT_TRUE(m_bar->m_actLivePreview->isEnabled());
    m_bar->setMarkdownAvailable(false);
    EXPECT_FALSE(m_bar->m_actLivePreview->isEnabled());
}

TEST_F(BottomBarTest, ViewModeMenuTrigger_EmitsViewModeRequested)
{
    // Arrange: 开启 md 可用性，监听用户请求信号
    m_bar->setMarkdownAvailable(true);
    QSignalSpy spy(m_bar, &BottomBar::viewModeRequested);

    // Act: 用户点选「实时预览」action（经 DMenu::triggered 转发 lambda）
    m_bar->m_actLivePreview->trigger();
    QApplication::processEvents();

    // Assert: 发出 viewModeRequested(LivePreview)
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).value<ViewMode>(), ViewMode::LivePreview);
}

// ------------------------------------------------------------
// eventFilter / paintEvent
// ------------------------------------------------------------

TEST_F(BottomBarTest, EventFilter_FontChange_UpdatesLabels)
{
    // Arrange: 先给标签一个可区分字体
    QFont marker;
    marker.setPixelSize(21);
    m_bar->m_pPositionLabel->setFont(marker);
    ASSERT_EQ(m_bar->m_pPositionLabel->font().pixelSize(), 21);

    // Act: 应用字体变化事件（#if QT>=6 分支）
    QEvent fontEvent(QEvent::ApplicationFontChange);
    bool ret = m_bar->eventFilter(qApp, &fontEvent);

    // Assert: 事件放行且四个标签字体已同步为应用字体
    EXPECT_FALSE(ret);
    EXPECT_EQ(m_bar->m_pPositionLabel->font().pixelSize(), qApp->font().pixelSize());
    EXPECT_EQ(m_bar->m_pCharCountLabel->font().pixelSize(), qApp->font().pixelSize());
    EXPECT_EQ(m_bar->m_scaleLabel->font().pixelSize(), qApp->font().pixelSize());
    EXPECT_EQ(m_bar->m_progressLabel->font().pixelSize(), qApp->font().pixelSize());

    // Act: 无关事件交还基类
    QEvent other(QEvent::MouseMove);
    EXPECT_FALSE(m_bar->eventFilter(qApp, &other));
}

TEST_F(BottomBarTest, PaintEvent_BothFindStates_RendersFrame)
{
    // Arrange: 普通模式
    m_bar->updateSize(32, false);
    m_bar->resize(400, 32);

    // Act: grab 强制同步绘制（非查找态：绘制顶部分隔线）
    const QPixmap normal = m_bar->grab();
    EXPECT_FALSE(normal.isNull());

    // Act: 查找/替换模式（跳过分隔线）
    m_bar->updateSize(40, true);
    const QPixmap finding = m_bar->grab();
    EXPECT_FALSE(finding.isNull());
    EXPECT_EQ(m_bar->m_bIsFindOrReplace, true);
}

// ------------------------------------------------------------
// 编码 / 高亮菜单切换 lambda
// ------------------------------------------------------------

TEST_F(BottomBarTest, EncodeMenuChange_Success_UpdatesMenuText)
{
    // Arrange: 编码菜单顶层 action 均为分组子菜单，经私有 m_menu 精确遍历
    QAction *gbkAction = nullptr;
    const QList<QAction *> tops = m_bar->m_pEncodeMenu->m_menu->actions();
    for (QAction *top : tops) {
        if (!top->menu())
            continue;
        for (QAction *a : top->menu()->actions()) {
            if (a->text() == QString("GB18030")) {
                gbkAction = a;
                break;
            }
        }
    }
    ASSERT_NE(gbkAction, nullptr) << "编码菜单应包含 GB18030 项";
    m_reloadEncodeResult = true;

    // Act: 触发编码切换（currentActionChanged → lambda）
    emit m_bar->getEncodeMenu()->currentActionChanged(gbkAction);

    // Assert: 重载以 GB18030 调用一次且菜单文本更新
    EXPECT_EQ(m_reloadEncodeCalls, 1);
    EXPECT_EQ(m_lastReloadEncode, QByteArray("GB18030"));
    EXPECT_EQ(m_bar->getEncodeMenu()->getCurrentText(), QString("GB18030"));
}

TEST_F(BottomBarTest, EncodeMenuChange_LoadedOrFailed_RevertsToPrevious)
{
    // Arrange: GB18030 action
    QAction *gbkAction = nullptr;
    const QList<QAction *> tops = m_bar->m_pEncodeMenu->m_menu->actions();
    for (QAction *top : tops) {
        if (!top->menu())
            continue;
        for (QAction *a : top->menu()->actions()) {
            if (a->text() == QString("GB18030"))
                gbkAction = a;
        }
    }
    ASSERT_NE(gbkAction, nullptr);

    // Act 1: 文件加载中 → 恢复旧文本
    m_fileLoading = true;
    emit m_bar->getEncodeMenu()->currentActionChanged(gbkAction);
    EXPECT_EQ(m_reloadEncodeCalls, 0); // 加载中不触发转换
    EXPECT_EQ(m_bar->getEncodeMenu()->getCurrentText(), QString("UTF-8"));

    // Act 2: 转换失败 → 同样恢复
    m_fileLoading = false;
    m_reloadEncodeResult = false;
    emit m_bar->getEncodeMenu()->currentActionChanged(gbkAction);
    EXPECT_EQ(m_reloadEncodeCalls, 1);
    EXPECT_EQ(m_bar->getEncodeMenu()->getCurrentText(), QString("UTF-8"));
}

TEST_F(BottomBarTest, HighlightMenuChange_UpdatesTextAndReloads)
{
    // Arrange: 高亮菜单（顶层 action 均为分组子菜单）取第一个可用 action
    QAction *target = nullptr;
    const QList<QAction *> tops = m_bar->m_pHighlightMenu->m_menu->actions();
    for (QAction *top : tops) {
        if (!top->menu())
            continue;
        for (QAction *a : top->menu()->actions()) {
            if (a->text() != QString("None")) {
                target = a;
                break;
            }
        }
    }
    if (target == nullptr)
        GTEST_SKIP() << "系统高亮仓库无可用定义，跳过";
    const QString targetName = target->text();

    // Act
    emit m_bar->getHighlightMenu()->currentActionChanged(target);

    // Assert: 菜单文本更新 + 编辑器收到高亮类型重载（ctor 连接的 lambda）
    EXPECT_EQ(m_bar->getHighlightMenu()->getCurrentText(), targetName);
    EXPECT_EQ(m_reloadHighlightCalls, 1);
    EXPECT_EQ(m_lastReloadHighlight, targetName);
}

// ------------------------------------------------------------
// slotSetTextEditFocus（真实 Window + 真实 EditWrapper）
// ------------------------------------------------------------

TEST_F(BottomBarTest, SlotSetTextEditFocus_EmitsWindowPressEsc)
{
    // Arrange: 真实 Window（DBus 全拦截）+ 真实 EditWrapper 作为父控件
    stub.set_lamda(&QDBusConnection::systemBus,
                   []() -> QDBusConnection { return QDBusConnection(QStringLiteral("ut_no_bus")); });
    stub.set_lamda(&QDBusConnection::sessionBus,
                   []() -> QDBusConnection { return QDBusConnection(QStringLiteral("ut_no_bus")); });
    stub.set_lamda(
        static_cast<QDBusMessage (QDBusAbstractInterface::*)(QDBus::CallMode, const QString &, const QList<QVariant> &)>(
            &QDBusAbstractInterface::callWithArgumentList),
        [](QDBusAbstractInterface *, QDBus::CallMode, const QString &, const QList<QVariant> &) -> QDBusMessage {
            return QDBusMessage();
        });

    Window win;
    EditWrapper *wrapper = new EditWrapper(&win, &win);
    BottomBar bar(wrapper);
    QSignalSpy spy(&win, &Window::pressEsc);

    // Act: 编码菜单失去焦点 → 光标回文本框
    bar.slotSetTextEditFocus();

    // Assert: Window::pressEsc 发射一次（经真实 QObject 信号链）
    EXPECT_EQ(spy.count(), 1);

    // Act: 经信号路径再触发一次（DDropdownMenu::sigSetTextEditFocus 连接）
    emit bar.getEncodeMenu()->sigSetTextEditFocus();
    QApplication::processEvents();
    EXPECT_EQ(spy.count(), 2);
}
