// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// ============================================================================
// DDropdownMenu 单元测试（B11 / src/widgets/ddropdownmenu.cpp）
//
// 策略：真实 offscreen 构造（qrc 资源编入目标，SVG 图标可用），
// 模态 QMenu::exec 由 stub_ext 拦截，其余全部真实执行。
//
// 分支清单（来源：ddropdownmenu.cpp）与用例映射：
// - ctor：图标/字体/布局/快捷键连接
//     → Constructor_RealBuild_CreatesButtonAndDefaultText
// - setFontEx → SetFontEx_NewFont_AppliedToButton
// - setCurrentAction：null 早退 / 有效 action 勾选+设文本
//     → SetCurrentAction_NullAction_KeepsState / SetCurrentAction_ValidAction_ChecksAndSetsText
// - setCurrentTextOnly：空菜单 / 嵌套菜单勾选
//     → SetCurrentTextOnly_EmptyMenu_SetsText / SetCurrentTextOnly_NestedMenu_ChecksMatchedAction
// - setCheckedExclusive：null / 有子菜单递归 / 无子菜单文本相等与不等
//     → SetCheckedExclusive_NullAction_EarlyReturn
//     → SetCheckedExclusive_NestedMenu_RecursesIntoSubmenus（经 setCurrentTextOnly 递归分支）
// - slotRequestMenu：request=true 清焦 / exec + HoverLeave + 信号
//     → SlotRequestMenu_EmitsFocusSignalAfterExec
// - setText（经 setCurrentTextOnly）
// - setMenu：非空设名 / null；deleteMenu：空/非空 → SetMenu_* / DeleteMenu_*
// - setMenuActionGroup / deleteMenuActionGroup → SetMenuActionGroup_*/DeleteMenuActionGroup_*
// - setTheme（亮/暗图标重建）→ SetTheme_LightAndDark_RebuildsArrowPixmap
// - setChildrenFocus true/false → SetChildrenFocus_*
// - setRequestMenu → SetRequestMenu_SetsInternalFlag
// - getButton / getCurrentText → Constructor_RealBuild_* / GetCurrentText_*
// - createEncodeMenu（静态）→ CreateEncodeMenu_ContainsUtf8Default
// - createHighLightMenu（静态）→ CreateHighLightMenu_DefaultNoneWithActionGroup
// - createIcon（经 setText）/ setSvgColor+SetSVGBackColor（经鼠标按下高亮分支）
//     → EventFilter_MousePressLeft_MarksPressedAndReturnsTrue（按下 → createIcon → setSvgColor）
//     → SetSvgColor_RealSvg_ReturnsColoredPixmap（直接调用私有函数）
// - OnFontChangedSlot（经 ApplicationFontChange 事件）
//     → EventFilter_ApplicationFontChange_RefreshesFont
// - eventFilter：工具按钮 KeyPress(Return/Space/其他)、鼠标左/右按下、左释放（可用/禁用）
//     → EventFilter_* 系列
//
// 环境隔离：XDG 重定向临时目录（SetUpTestSuite/TearDownTestSuite qputenv/qunsetenv 配平）；
// QMenu::exec 拦截避免模态阻塞；无真实 IO / 网络 / 子进程。
// ============================================================================

#include <gtest/gtest.h>
#include "stubext.h"

#include <QApplication>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QHoverEvent>
#include <QActionGroup>
#include <QFont>
#include <QDir>
#include <DGuiApplicationHelper>

using Dtk::Gui::DGuiApplicationHelper;

#include "widgets/ddropdownmenu.h"

class DDropdownMenuTest : public ::testing::Test {
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
        obj = new DDropdownMenu();
    }

    void TearDown() override
    {
        // 析构链：deleteMenuActionGroup + deleteMenu（真实执行）
        delete obj;
        obj = nullptr;
        stub.clear();
    }

    // 构造"顶层 action 均带子菜单"的菜单（与真实编码菜单同构）
    QMenu *buildNestedMenu(QWidget *parent)
    {
        QMenu *menu = new QMenu(parent);
        QMenu *sub1 = new QMenu("Group1", menu);
        a1 = sub1->addAction("A1");
        a2 = sub1->addAction("A2");
        menu->addMenu(sub1);
        QMenu *sub2 = new QMenu("Group2", menu);
        b1 = sub2->addAction("B1");
        b2 = sub2->addAction("B2");
        menu->addMenu(sub2);
        return menu;
    }

    stub_ext::StubExt stub;
    DDropdownMenu *obj = nullptr;
    QAction *a1 = nullptr;
    QAction *a2 = nullptr;
    QAction *b1 = nullptr;
    QAction *b2 = nullptr;

    int menuExecCalls = 0;

    static QApplication *s_app;
    static QTemporaryDir *s_configHome;
    static QTemporaryDir *s_dataHome;
    static char s_argv[];
};

QApplication *DDropdownMenuTest::s_app = nullptr;
QTemporaryDir *DDropdownMenuTest::s_configHome = nullptr;
QTemporaryDir *DDropdownMenuTest::s_dataHome = nullptr;
char DDropdownMenuTest::s_argv[] = "test_ddropdownmenu";

// ------------------------------------------------------------
// 构造 / 基础 getter
// ------------------------------------------------------------

TEST_F(DDropdownMenuTest, Constructor_RealBuild_CreatesButtonAndDefaultText)
{
    // Assert: 按钮子控件就绪 + 默认文本 UTF-8 + 箭头图标已渲染（qrc 资源）
    ASSERT_NE(obj->getButton(), nullptr);
    EXPECT_EQ(obj->getButton()->parent(), obj);
    EXPECT_EQ(obj->getCurrentText(), QString("UTF-8"));
    EXPECT_FALSE(obj->m_arrowPixmap.isNull());
    EXPECT_EQ(obj->getButton()->focusPolicy(), Qt::StrongFocus);
}

TEST_F(DDropdownMenuTest, GetCurrentText_AfterSetTextOnly_ReflectsNewValue)
{
    // Act
    obj->setCurrentTextOnly(QStringLiteral("GBK"));

    // Assert
    EXPECT_EQ(obj->getCurrentText(), QString("GBK"));
    EXPECT_NE(obj->getCurrentText(), QString("UTF-8"));
}

// ------------------------------------------------------------
// setFontEx
// ------------------------------------------------------------

TEST_F(DDropdownMenuTest, SetFontEx_NewFont_AppliedToButton)
{
    // Arrange
    QFont font;
    font.setPixelSize(21);
    font.setFamily(QStringLiteral("monospace"));

    // Act
    obj->setFontEx(font);

    // Assert: 按钮字体生效 + 内部记录字体一致
    EXPECT_EQ(obj->getButton()->font().pixelSize(), 21);
    EXPECT_EQ(obj->m_font.pixelSize(), 21);
    EXPECT_EQ(obj->m_font.family(), QString("monospace"));
}

// ------------------------------------------------------------
// setMenu / deleteMenu
// ------------------------------------------------------------

TEST_F(DDropdownMenuTest, SetMenu_RealMenu_SetsObjectNames)
{
    // Arrange
    QMenu *menu = buildNestedMenu(nullptr);

    // Act
    obj->setMenu(menu);

    // Assert: 菜单替换成功且设置对象名/无障碍名
    EXPECT_EQ(menu->objectName(), QString("DropdownMenu"));
    EXPECT_EQ(menu->accessibleName(), QString("DropdownMenu"));
    EXPECT_EQ(obj->m_menu, menu);
}

TEST_F(DDropdownMenuTest, SetMenu_NullMenu_ClearsMenu)
{
    // Arrange: 先装一个真实菜单
    QMenu *menu = buildNestedMenu(nullptr);
    obj->setMenu(menu);

    // Act: 置空（内部 deleteMenu 分支执行，旧菜单被析构）
    obj->setMenu(nullptr);

    // Assert: 指针置空且菜单不再作为子对象存在
    EXPECT_EQ(obj->m_menu, nullptr);
    EXPECT_EQ(obj->findChildren<QMenu *>().count(), 0);
}

TEST_F(DDropdownMenuTest, DeleteMenu_WithMenu_ReleasesAndNulls)
{
    // Arrange
    QMenu *menu = buildNestedMenu(nullptr);
    obj->setMenu(menu);

    // Act
    obj->deleteMenu();

    // Assert
    EXPECT_EQ(obj->m_menu, nullptr);
    // 二次删除（空分支）不应崩溃
    obj->deleteMenu();
    EXPECT_EQ(obj->m_menu, nullptr);
}

// ------------------------------------------------------------
// setMenuActionGroup / deleteMenuActionGroup
// ------------------------------------------------------------

TEST_F(DDropdownMenuTest, SetMenuActionGroup_RealGroup_SetsObjectName)
{
    // Arrange
    QActionGroup *group = new QActionGroup(nullptr);

    // Act
    obj->setMenuActionGroup(group);

    // Assert
    EXPECT_EQ(obj->m_actionGroup, group);
    EXPECT_EQ(group->objectName(), QString("ActionGroup"));
}

TEST_F(DDropdownMenuTest, DeleteMenuActionGroup_WithGroup_ReleasesAndNulls)
{
    // Arrange
    QActionGroup *group = new QActionGroup(nullptr);
    obj->setMenuActionGroup(group);
    QPointer<QActionGroup> watch(group);

    // Act
    obj->deleteMenuActionGroup();

    // Assert: 组被删除且指针置空；空二次删除不崩溃
    EXPECT_TRUE(watch.isNull());
    EXPECT_EQ(obj->m_actionGroup, nullptr);
    obj->deleteMenuActionGroup();
    EXPECT_EQ(obj->m_actionGroup, nullptr);
}

// ------------------------------------------------------------
// setTheme
// ------------------------------------------------------------

TEST_F(DDropdownMenuTest, SetTheme_LightAndDark_RebuildsArrowPixmap)
{
    // Arrange: 记录初始 pixmap 序列号
    const qint64 oldSerial = obj->m_arrowPixmap.cacheKey();

    // Act
    obj->setTheme(QStringLiteral("light"));

    // Assert: 亮色箭头重建（资源 :/images/dropdown_arrow_light.svg 存在）
    EXPECT_FALSE(obj->m_arrowPixmap.isNull());
    const qint64 lightSerial = obj->m_arrowPixmap.cacheKey();

    // Act: 暗色
    obj->setTheme(QStringLiteral("dark"));

    // Assert: 暗色箭头再次重建且与亮色不同
    EXPECT_FALSE(obj->m_arrowPixmap.isNull());
    EXPECT_NE(obj->m_arrowPixmap.cacheKey(), lightSerial);
    EXPECT_NE(oldSerial, lightSerial);
}

// ------------------------------------------------------------
// setChildrenFocus / setRequestMenu
// ------------------------------------------------------------

TEST_F(DDropdownMenuTest, SetChildrenFocus_TrueAndFalse_TogglesFocusPolicy)
{
    // Act / Assert: true → StrongFocus
    obj->setChildrenFocus(true);
    EXPECT_EQ(obj->getButton()->focusPolicy(), Qt::StrongFocus);

    // Act / Assert: false → NoFocus
    obj->setChildrenFocus(false);
    EXPECT_EQ(obj->getButton()->focusPolicy(), Qt::NoFocus);
    EXPECT_FALSE(obj->getButton()->hasFocus());
}

TEST_F(DDropdownMenuTest, SetRequestMenu_TrueAndFalse_SetsInternalFlag)
{
    // Act
    obj->setRequestMenu(true);

    // Assert: 内部标志翻转（后续 slotRequestMenu 据此清焦）
    EXPECT_TRUE(obj->isRequest);

    // Act / Assert: 复位
    obj->setRequestMenu(false);
    EXPECT_FALSE(obj->isRequest);
}

// ------------------------------------------------------------
// setCurrentAction / setCurrentTextOnly / setCheckedExclusive
// ------------------------------------------------------------

TEST_F(DDropdownMenuTest, SetCurrentAction_NullAction_KeepsState)
{
    // Arrange
    obj->setCurrentTextOnly(QStringLiteral("UTF-8"));

    // Act: null 早退分支
    obj->setCurrentAction(nullptr);

    // Assert: 文本与内部菜单保持不变
    EXPECT_EQ(obj->getCurrentText(), QString("UTF-8"));
    EXPECT_FALSE(obj->m_bPressed);
}

TEST_F(DDropdownMenuTest, SetCurrentAction_ValidAction_ChecksAndSetsText)
{
    // Arrange: 嵌套菜单（与真实编码菜单同构：顶层 action 均带子菜单）
    QMenu *menu = buildNestedMenu(nullptr);
    obj->setMenu(menu);
    a1->setCheckable(true);
    a2->setCheckable(true);
    b1->setCheckable(true);
    b2->setCheckable(true);
    a2->setChecked(false);

    // Act
    obj->setCurrentAction(a2);

    // Assert: 目标勾选、文本更新
    EXPECT_TRUE(a2->isChecked());
    EXPECT_EQ(obj->getCurrentText(), QString("A2"));
    // 切到 B1 后 A2 取消勾选
    obj->setCurrentAction(b1);
    EXPECT_FALSE(a2->isChecked());
    EXPECT_TRUE(b1->isChecked());
    EXPECT_EQ(obj->getCurrentText(), QString("B1"));
}

TEST_F(DDropdownMenuTest, SetCurrentTextOnly_EmptyMenu_SetsText)
{
    // Act: 默认构造的 m_menu 无 action（空循环）
    obj->setCurrentTextOnly(QStringLiteral("UTF-16"));

    // Assert: 文本仍被设置
    EXPECT_EQ(obj->getCurrentText(), QString("UTF-16"));
    EXPECT_EQ(obj->getButton()->isEnabled(), true);
}

TEST_F(DDropdownMenuTest, SetCurrentTextOnly_NestedMenu_ChecksMatchedAction)
{
    // Arrange
    QMenu *menu = buildNestedMenu(nullptr);
    obj->setMenu(menu);

    // Act: 选中嵌套子菜单中的 B1
    obj->setCurrentTextOnly(QStringLiteral("B1"));

    // Assert: B1 勾选，其余 action 取消勾选且不可勾选
    EXPECT_TRUE(b1->isCheckable());
    EXPECT_TRUE(b1->isChecked());
    EXPECT_FALSE(a1->isCheckable());
    EXPECT_FALSE(a1->isChecked());
    EXPECT_FALSE(b2->isCheckable());
    EXPECT_EQ(obj->getCurrentText(), QString("B1"));
}

TEST_F(DDropdownMenuTest, SetCheckedExclusive_NullAction_EarlyReturn)
{
    // Arrange
    const QString before = obj->getCurrentText();

    // Act: null 早退（B1 分支）
    obj->setCheckedExclusive(nullptr, QStringLiteral("whatever"));

    // Assert: 状态未受影响
    EXPECT_EQ(obj->getCurrentText(), before);
    EXPECT_EQ(obj->getCurrentText(), QString("UTF-8"));
}

TEST_F(DDropdownMenuTest, SetCheckedExclusive_NoSubMenu_MatchesByNameDirectly)
{
    // Arrange: 平铺菜单（action 无子菜单 → else 分支，含同名/异名两侧）
    QMenu *menu = new QMenu(nullptr);
    QAction *plain1 = menu->addAction("P1");
    QAction *plain2 = menu->addAction("P2");
    obj->setMenu(menu);

    // Act: 匹配 P2
    obj->setCurrentTextOnly(QStringLiteral("P2"));

    // Assert: P2 勾选，P1 不勾选（异名分支）
    EXPECT_TRUE(plain2->isChecked());
    EXPECT_FALSE(plain1->isChecked());

    // Act: 再匹配 P1（覆盖同名分支 + 状态切换）
    obj->setCurrentTextOnly(QStringLiteral("P1"));
    EXPECT_TRUE(plain1->isChecked());
    EXPECT_FALSE(plain2->isChecked());
}

// ------------------------------------------------------------
// slotRequestMenu（经 requestContextMenu 信号触发）
// ------------------------------------------------------------

TEST_F(DDropdownMenuTest, SlotRequestMenu_EmitsFocusSignalAfterExec)
{
    // Arrange: 拦截 QMenu::exec（DMenu 为 QMenu typedef；三重载均非虚，static_cast 定点）
    stub.set_lamda(static_cast<QAction *(QMenu::*)()>(&QMenu::exec),
                   [this](QMenu *) -> QAction * {
                       ++menuExecCalls;
                       return nullptr;
                   });
    QSignalSpy focusSpy(obj, &DDropdownMenu::sigSetTextEditFocus);

    // Act: 发射 requestContextMenu(true)（清焦分支）
    emit obj->requestContextMenu(true);

    // Assert: exec 执行一次并随后发出回焦信号
    EXPECT_EQ(menuExecCalls, 1);
    EXPECT_EQ(focusSpy.count(), 1);

    // Act: request=false 分支（不清焦）
    emit obj->requestContextMenu(false);
    EXPECT_EQ(menuExecCalls, 2);
    EXPECT_EQ(focusSpy.count(), 2);
}

// ------------------------------------------------------------
// 静态工厂
// ------------------------------------------------------------

TEST_F(DDropdownMenuTest, CreateEncodeMenu_ContainsUtf8Default)
{
    // Act
    DDropdownMenu *menu = DDropdownMenu::createEncodeMenu();

    // Assert: 默认文本 UTF-8，UTF-8 action 存在且预选中
    ASSERT_NE(menu, nullptr);
    EXPECT_EQ(menu->getCurrentText(), QString("UTF-8"));
    ASSERT_NE(menu->m_pActUtf8, nullptr);
    EXPECT_TRUE(menu->m_pActUtf8->isChecked());
    EXPECT_EQ(menu->m_pActUtf8->objectName(), QString("PActUtf8"));
    delete menu;
}

TEST_F(DDropdownMenuTest, CreateHighLightMenu_DefaultNoneWithActionGroup)
{
    // Act
    DDropdownMenu *menu = DDropdownMenu::createHighLightMenu();

    // Assert: 默认 None，带互斥 action 组
    ASSERT_NE(menu, nullptr);
    EXPECT_EQ(menu->getCurrentText(), QString("None"));
    ASSERT_NE(menu->m_actionGroup, nullptr);
    EXPECT_TRUE(menu->m_actionGroup->isExclusive());
    EXPECT_FALSE(menu->m_menu->actions().isEmpty());
    delete menu;
}

// ------------------------------------------------------------
// 静态工厂：菜单 triggered 转发 lambda
// ------------------------------------------------------------

TEST_F(DDropdownMenuTest, CreateEncodeMenu_TriggeredAction_EmitsCurrentActionChanged)
{
    // Arrange
    DDropdownMenu *menu = DDropdownMenu::createEncodeMenu();
    QSignalSpy spy(menu, &DDropdownMenu::currentActionChanged);

    // Act: 触发同文本（UTF-8）action → 不发信号（防抖分支）
    emit menu->m_menu->triggered(menu->m_pActUtf8);
    EXPECT_EQ(spy.count(), 0);

    // Act: 触发同组其它 action → 发 currentActionChanged
    QAction *other = nullptr;
    const QList<QAction *> tops = menu->m_menu->actions();
    for (QAction *top : tops) {
        if (!top->menu())
            continue;
        for (QAction *a : top->menu()->actions()) {
            if (a->text() != QString("UTF-8")) {
                other = a;
                break;
            }
        }
    }
    ASSERT_NE(other, nullptr);
    emit menu->m_menu->triggered(other);

    // Assert
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).value<QAction *>(), other);
    delete menu;
}

TEST_F(DDropdownMenuTest, CreateHighLightMenu_NoneTriggered_EmitsCurrentActionChanged)
{
    // Arrange: None 是 m_menu 首个 action
    DDropdownMenu *menu = DDropdownMenu::createHighLightMenu();
    QAction *noneAction = menu->m_menu->actions().first();
    QSignalSpy spy(menu, &DDropdownMenu::currentActionChanged);

    // Act: None 勾选触发（checked=true 分支）
    emit noneAction->triggered(true);

    // Assert
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).value<QAction *>(), noneAction);

    // Act: 取消勾选（checked=false 分支，不发信号）
    QSignalSpy spy2(menu, &DDropdownMenu::currentActionChanged);
    emit noneAction->triggered(false);
    EXPECT_EQ(spy2.count(), 0);
    delete menu;
}

TEST_F(DDropdownMenuTest, CreateHighLightMenu_ActionGroupTriggered_BothBranches)
{
    // Arrange: 互斥组中的真实高亮定义 action
    DDropdownMenu *menu = DDropdownMenu::createHighLightMenu();
    QActionGroup *group = menu->m_actionGroup;
    ASSERT_NE(group, nullptr);
    QSignalSpy spy(menu, &DDropdownMenu::currentActionChanged);

    // Act: 触发组内 action（有效定义 + 文本不同 → 发信号）
    QAction *valid = nullptr;
    for (QAction *a : group->actions()) {
        if (a->text() != QString("None")) {
            valid = a;
            break;
        }
    }
    if (valid == nullptr)
        GTEST_SKIP() << "系统高亮仓库无可用定义";
    emit group->triggered(valid);

    // Assert
    ASSERT_EQ(spy.count(), 1);

    // Act: 注入无效定义名 action → 回退 None 文本（无效分支）
    QAction *invalid = group->addAction(QStringLiteral("NotARealDefinition"));
    emit group->triggered(invalid);
    EXPECT_EQ(menu->getCurrentText(), QString("None"));
    delete menu;
}

TEST_F(DDropdownMenuTest, Constructor_SizeModeChanged_AdjustsButtonHeight)
{
    // Arrange: 记录当前模式并切换（ctor 连接的 lambda 触发）
    const auto origMode = DGuiApplicationHelper::instance()->sizeMode();
    const auto flipped = (origMode == DGuiApplicationHelper::CompactMode)
                             ? DGuiApplicationHelper::NormalMode
                             : DGuiApplicationHelper::CompactMode;
    const int expectH = (flipped == DGuiApplicationHelper::CompactMode) ? 20 : 28;

    // Act
    DGuiApplicationHelper::instance()->setSizeMode(flipped);

    // Assert: 工具按钮高度随布局模式联动
    EXPECT_EQ(obj->getButton()->height(), expectH);

    // Act / Assert: 还原后高度回到原值（联动可逆）
    const int origH = (origMode == DGuiApplicationHelper::CompactMode) ? 20 : 28;
    DGuiApplicationHelper::instance()->setSizeMode(origMode);
    EXPECT_EQ(obj->getButton()->height(), origH);

}

// ------------------------------------------------------------
// eventFilter：键盘 / 鼠标 / 字体变化
// ------------------------------------------------------------

TEST_F(DDropdownMenuTest, EventFilter_KeyReturnAndSpace_TriggersContextMenu)
{
    // Arrange: 键盘触发会拉起 slotRequestMenu → QMenu::exec，先拦截防模态阻塞
    stub.set_lamda(static_cast<QAction *(QMenu::*)()>(&QMenu::exec),
                   [](QMenu *) -> QAction * { return nullptr; });
    QSignalSpy menuSpy(obj, &DDropdownMenu::requestContextMenu);

    // Act: Enter 键
    QKeyEvent enterEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    bool enterRet = obj->eventFilter(obj->getButton(), &enterEvent);

    // Assert: 拦截并请求菜单（false = 键盘触发）
    EXPECT_TRUE(enterRet);
    ASSERT_EQ(menuSpy.count(), 1);
    EXPECT_FALSE(menuSpy.at(0).at(0).toBool());

    // Act: Space 键
    QKeyEvent spaceEvent(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier);
    EXPECT_TRUE(obj->eventFilter(obj->getButton(), &spaceEvent));
    EXPECT_EQ(menuSpy.count(), 2);

    // Act: 其他键不处理
    QKeyEvent otherEvent(QEvent::KeyPress, Qt::Key_F5, Qt::NoModifier);
    EXPECT_FALSE(obj->eventFilter(obj->getButton(), &otherEvent));
    EXPECT_EQ(menuSpy.count(), 2);
}

TEST_F(DDropdownMenuTest, EventFilter_MousePressLeft_MarksPressedAndReturnsTrue)
{
    // Arrange
    const QPointF local(2, 2);
    QMouseEvent pressLeft(QEvent::MouseButtonPress, local, local,
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    // Act
    bool ret = obj->eventFilter(obj->getButton(), &pressLeft);

    // Assert: 左键按下被拦截且置按下标志（联动 createIcon → setSvgColor 高亮分支）
    EXPECT_TRUE(ret);
    EXPECT_TRUE(obj->m_bPressed);

    // Act: 右键按下同样拦截（不改变按下态）
    QMouseEvent pressRight(QEvent::MouseButtonPress, local, local,
                           Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    EXPECT_TRUE(obj->eventFilter(obj->getButton(), &pressRight));
    EXPECT_TRUE(obj->m_bPressed);
}

TEST_F(DDropdownMenuTest, EventFilter_MouseReleaseLeft_RequestsMenuWhenEnabled)
{
    // Arrange: 处于按下态 + 菜单信号监听
    const QPointF local(2, 2);
    QMouseEvent pressLeft(QEvent::MouseButtonPress, local, local,
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    obj->eventFilter(obj->getButton(), &pressLeft);
    QSignalSpy menuSpy(obj, &DDropdownMenu::requestContextMenu);
    stub.set_lamda(static_cast<QAction *(QMenu::*)()>(&QMenu::exec),
                   [this](QMenu *) -> QAction * {
                       ++menuExecCalls;
                       return nullptr;
                   });

    // Act: 左键释放
    QMouseEvent release(QEvent::MouseButtonRelease, local, local,
                        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    bool ret = obj->eventFilter(obj->getButton(), &release);

    // Assert: 释放被拦截、按下态复位、菜单请求（true = 鼠标触发）+ exec 被拉起
    EXPECT_TRUE(ret);
    EXPECT_FALSE(obj->m_bPressed);
    ASSERT_EQ(menuSpy.count(), 1);
    EXPECT_TRUE(menuSpy.at(0).at(0).toBool());
    EXPECT_EQ(menuExecCalls, 1);

    // Act: 禁用后释放不再请求菜单（isEnabled 分支）
    QMouseEvent pressAgain(QEvent::MouseButtonPress, local, local,
                           Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    obj->eventFilter(obj->getButton(), &pressAgain);
    obj->setEnabled(false);
    QSignalSpy menuSpy2(obj, &DDropdownMenu::requestContextMenu);
    QMouseEvent release2(QEvent::MouseButtonRelease, local, local,
                         Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    EXPECT_TRUE(obj->eventFilter(obj->getButton(), &release2));
    EXPECT_EQ(menuSpy2.count(), 0);
    obj->setEnabled(true);
}

TEST_F(DDropdownMenuTest, EventFilter_OtherObjectOrType_PassesThrough)
{
    // Arrange: 非工具按钮对象 / 非相关事件类型
    QWidget stranger;
    QMouseEvent strangerPress(QEvent::MouseButtonPress, QPointF(1, 1), QPointF(1, 1),
                              Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    // Act
    bool ret1 = obj->eventFilter(&stranger, &strangerPress);
    QEvent unrelated(QEvent::MouseMove);
    bool ret2 = obj->eventFilter(obj->getButton(), &unrelated);

    // Assert: 交还基类处理（QFrame::eventFilter 默认 false）
    EXPECT_FALSE(ret1);
    EXPECT_FALSE(ret2);
}

TEST_F(DDropdownMenuTest, EventFilter_ApplicationFontChange_RefreshesFont)
{
    // Arrange: 先设置一个可区分的字体
    QFont marker;
    marker.setPixelSize(21);
    obj->setFontEx(marker);
    EXPECT_EQ(obj->m_font.pixelSize(), 21);

    // Act: 派发应用字体变化事件（Qt6 分支 → OnFontChangedSlot）
    QEvent fontEvent(QEvent::ApplicationFontChange);
    bool ret = obj->eventFilter(obj->getButton(), &fontEvent);

    // Assert: 事件放行且内部字体已同步为 qApp 字体
    EXPECT_FALSE(ret);
    EXPECT_EQ(obj->m_font.pixelSize(), qApp->font().pixelSize());
    EXPECT_NE(obj->m_font.pixelSize(), 21);
}

// ------------------------------------------------------------
// 私有：setSvgColor / SetSVGBackColor（经公有路径 + 直接调用）
// ------------------------------------------------------------

TEST_F(DDropdownMenuTest, SetSvgColor_RealSvgResource_ReturnsColoredPixmap)
{
    // Act: 直接驱动（生产路径由按下态 createIcon 触发）
    const QPixmap pixmap = obj->setSvgColor(QStringLiteral("#ff0000"));

    // Assert: 基于 qrc 内 arrow_dark.svg 着色成功
    EXPECT_FALSE(pixmap.isNull());
    EXPECT_EQ(pixmap.width(), 8);
    EXPECT_EQ(pixmap.height(), 5);
}

TEST_F(DDropdownMenuTest, SetSVGBackColor_ColorGroup_AttributeReplaced)
{
    // Arrange: 构造含 <g id="color" fill="#000000"> 的 SVG 文档
    QDomDocument doc;
    doc.setContent(QStringLiteral(
        "<svg><g id=\"color\" fill=\"#000000\"><rect/></g><g id=\"other\"/></svg>"));
    QDomElement elem = doc.documentElement();

    // Act: 递归替换 fill 属性
    obj->SetSVGBackColor(elem, QStringLiteral("fill"), QStringLiteral("#123456"));

    // Assert: 目标 g 的 fill 已替换，其它节点不受影响
    bool found = false;
    QDomNodeList children = elem.elementsByTagName(QStringLiteral("g"));
    for (int i = 0; i < children.count(); ++i) {
        QDomElement g = children.at(i).toElement();
        if (g.attribute("id") == QString("color")) {
            EXPECT_EQ(g.attribute("fill"), QString("#123456"));
            found = true;
        }
    }
    EXPECT_TRUE(found);
}
