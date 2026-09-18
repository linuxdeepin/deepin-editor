// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// ThemePanel（src/thememodule/themepanel.cpp）单元测试
//
// 类特征：QWidget 子类（GUI 类），offscreen QApplication 无头构造。
// Window 依赖处理：themepanel.cpp 以相对路径包含真实 ../widgets/window.h（无法用 -I 遮蔽），
// 符号经 startmanager_ut_src 全量源码库提供；测试绝不真实实例化 Window——popup/hide
// 仅经 m_window->geometry()（QWidget 单继承链上的非虚方法）读取父窗口几何，
// 故父对象传普通 QWidget（DMainWindow→QWidget 单继承偏移 0，静态下转不改变地址）。
//
// 分支清单 → 用例映射：
// - ThemePanel::ThemePanel → 各用例 SetUp 构造（含 connect 的 lambda 于 setSelectionTheme 用例覆盖）
// - setBackground: lightness < 128（暗色 → 深色描边） / >= 128（亮色 → 浅色描边） → SetBackground_ParamLightness_SelectsFrameColor(TEST_P 3 组)
// - popup → Popup_ShownView_AnimatesGeometryIntoView（含 valueChanged lambda 经动画真实触发）
// - hide → Hide_AnimatesGeometryOutAndHidesPanel（finished → QWidget::hide 真实收尾）
// - setFrameColor → SetFrameColor_ForwardsColorsToModel
// - setSelectionTheme → SetSelectionTheme_KnownPath_UpdatesCurrentIndex（构造内 lambda 覆盖）
// - paintEvent → PaintEvent_TwoFillPathCalls_BackgroundAndSeparator（fillPath 计数 + 颜色断言）
//
// 最小清单完成情况：
// | 1 | 每个公开方法 ≥1 用例 | 完成（ctor/dtor/setBackground/popup/hide/setFrameColor/setSelectionTheme；paintEvent 为 protected 覆盖经直接调用） |
// | 2 | 等价类划分（亮度两侧/路径已知未知） | 完成 |
// | 3 | 边界值（lightness 恰为 128 走亮色分支） | 完成（TEST_P） |
// | 4 | TEST_P ≥3 组（背景色 3 参数） | 完成 |
// | 5 | 分支清单映射 | 见上方 |
// | 6 | if/early-return 全分支 | 完成 |
// | 7 | 异常路径 EXPECT_THROW | N/A（Qt 风格无 throw） |
// | 8 | 负面场景（未知路径不崩溃且索引不变） | 完成 |
// | 9 | 强异常安全（popup 后模型/行数不变） | 完成 |
// | 10 | stub_ext（QPainter::fillPath 计数；主题数据源隔离） | 完成 |
//
// 动画策略：QPropertyAnimation 真实运行（offscreen 定时器驱动），QEventLoop+QTimer
// 等待 250ms 动画完成，几何终值即断言依据；不 stub start。

#include <gtest/gtest.h>

#include <QColor>
#include <QEventLoop>
#include <QMargins>
#include <QModelIndex>
#include <QPainter>
#include <QPainterPath>
#include <QRect>
#include <QSignalSpy>
#include <QTimer>
#include <QVariant>
#include <QVBoxLayout>

#include "test_env.h"
#include "themelistmodel.h"
#include "themelistview.h"
#include "themepanel.h"

namespace {

// 等待动画（250ms）完成并处理收尾事件（finished → hide/deleteLater）
inline void waitForAnimation(int msec = 420)
{
    QEventLoop loop;
    QTimer::singleShot(msec, &loop, &QEventLoop::quit);
    loop.exec();
}

class ThemePanelTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { thememoduleEnsureApp(); }

    void SetUp() override
    {
        stub.clear();
        installThemeSourceStubs(stub, {
            { QStringLiteral("/ut-fake-themes/dark.theme"), makeThemeMap("Dark", "#000000") },
            { QStringLiteral("/ut-fake-themes/bright.theme"), makeThemeMap("Bright", "#FFFFFF") },
        });
        // 父窗口：800x600，替代 Window（仅经 geometry() 被读取，不实例化真实主窗口）
        parent = new QWidget();
        parent->setGeometry(0, 0, 800, 600);
        panel = new ThemePanel(parent);  // 内部构造 ThemeListView/ThemeListModel
    }

    void TearDown() override
    {
        // 确保动画对象随面板释放（finished 已 deleteLater 或作为子对象清理）
        delete parent;  // 级联释放 panel → 覆盖 ~ThemePanel
        parent = nullptr;
        panel = nullptr;
        stub.clear();
    }

    stub_ext::StubExt stub;
    QWidget *parent = nullptr;
    ThemePanel *panel = nullptr;
};

// ---- 构造 ----

TEST_F(ThemePanelTest, Constructor_BuildsViewHierarchyAndStaysHidden)
{
    // Arrange/Act：构造已完成（SetUp）
    const ThemeListView *view = panel->m_themeView;        // 经 -fno-access-control
    const ThemeListModel *model = panel->m_themeModel;

    // Assert：视图/模型就绪、模型已装载主题、面板初始隐藏、固定宽 250
    ASSERT_NE(view, nullptr);
    ASSERT_NE(model, nullptr);
    EXPECT_EQ(view->accessibleName(), QStringLiteral("ThemeListView"));
    EXPECT_EQ(view->objectName(), QStringLiteral("ThemeView"));
    EXPECT_EQ(model->rowCount(QModelIndex()), 2);
    EXPECT_EQ(view->model(), model);  // 视图已绑定模型
    EXPECT_FALSE(panel->isVisible());  // 构造即隐藏
    EXPECT_EQ(panel->minimumWidth(), 250);
    EXPECT_EQ(panel->maximumWidth(), 250);  // setFixedWidth(250)

    // 布局：内容边距与间距均为 0
    const auto *layout = qobject_cast<const QVBoxLayout *>(panel->layout());
    ASSERT_NE(layout, nullptr);    EXPECT_EQ(layout->contentsMargins(), QMargins(0, 0, 0, 0));
    EXPECT_EQ(layout->spacing(), 0);
}

// ---- setBackground：亮度分支参数化 ----

struct BackgroundCase {
    QString backgroundHex;
    QString expectedFrameName;
    QString caseName;
};

class ThemePanelBackgroundTest : public ThemePanelTest,
                                 public ::testing::WithParamInterface<BackgroundCase> {
};

TEST_P(ThemePanelBackgroundTest, SetBackground_ParamLightness_SelectsFrameColor)
{
    // Arrange
    const QColor expectedFrame(GetParam().expectedFrameName);

    // Act
    panel->setBackground(GetParam().backgroundHex);

    // Assert：背景色按入参落盘；描边色按亮度分支选取（暗色→#FFFFFF，亮色→#000000）
    EXPECT_EQ(panel->m_backgroundColor.name(), QColor(GetParam().backgroundHex).name());
    EXPECT_EQ(panel->m_frameColor.name(), expectedFrame.name());
}

INSTANTIATE_TEST_SUITE_P(
    LightnessEquivalenceClasses, ThemePanelBackgroundTest,
    ::testing::Values(
        // lightness=0 <128 → 暗色分支取 m_frameDarkColor(#FFFFFF)
        (BackgroundCase{ QStringLiteral("#000000"), QStringLiteral("#FFFFFF"), "darkBackground" }),
        // lightness=255 >=128 → 亮色分支取 m_frameLightColor(#000000)
        (BackgroundCase{ QStringLiteral("#FFFFFF"), QStringLiteral("#000000"), "lightBackground" }),
        // 边界：lightness=128（#808080）不小于 128 → 亮色分支
        (BackgroundCase{ QStringLiteral("#808080"), QStringLiteral("#000000"), "boundaryLightness128" })),
    [](const ::testing::TestParamInfo<BackgroundCase> &info) { return info.param.caseName.toStdString(); });

// ---- popup ----

TEST_F(ThemePanelTest, Popup_ShownView_AnimatesGeometryIntoView)
{
    // Arrange：父窗口与面板就位（面板初始几何 x=800 完全在窗口外）
    parent->show();
    panel->setGeometry(800, 30, 250, 100);
    ASSERT_FALSE(panel->isVisible());

    // Act：弹出入场（show/raise + 动画 x: 800 → 800-250）
    panel->popup();
    EXPECT_TRUE(panel->isVisible());  // popup 即显示（断言 1：副作用）

    waitForAnimation();  // 等 250ms 动画跑完（期间 valueChanged lambda 被真实触发）

    // Assert：动画终值 x=550、几何其余分量保持；模型行数不受影响（强异常安全）
    EXPECT_EQ(panel->x(), 550);
    EXPECT_EQ(panel->y(), 30);
    EXPECT_EQ(panel->width(), 250);
    EXPECT_EQ(panel->m_themeModel->rowCount(QModelIndex()), 2);
}

// ---- hide ----

TEST_F(ThemePanelTest, Hide_AnimatesGeometryOutAndHidesPanel)
{
    // Arrange：先显示并置于收起前位置 x=550
    parent->show();
    panel->setGeometry(550, 30, 250, 100);
    panel->show();
    ASSERT_TRUE(panel->isVisible());

    // Act：收起（动画 x: 800-250=550 → 800，完成后 QWidget::hide）
    panel->hide();
    waitForAnimation();

    // Assert：动画终值 x=800；finished 链路真实执行到 QWidget::hide → 面板隐藏
    EXPECT_EQ(panel->x(), 800);
    EXPECT_EQ(panel->width(), 250);
    EXPECT_FALSE(panel->isVisible());
}

// ---- setFrameColor ----

TEST_F(ThemePanelTest, SetFrameColor_ForwardsColorsToModel)
{
    // Arrange
    const QModelIndex idx = panel->m_themeModel->index(0, 0);

    // Act
    panel->setFrameColor("#123456", "#654321");

    // Assert：转发到模型（经模型 data() 精确回读）
    EXPECT_EQ(panel->m_themeModel->data(idx, ThemeListModel::FrameSelectedColor).toString(),
              QStringLiteral("#123456"));
    EXPECT_EQ(panel->m_themeModel->data(idx, ThemeListModel::FrameNormalColor).toString(),
              QStringLiteral("#654321"));
}

// ---- setSelectionTheme ----

TEST_F(ThemePanelTest, SetSelectionTheme_KnownPath_UpdatesCurrentIndex)
{
    // Arrange：监听模型的 requestCurrentIndex（构造内 connect 的 lambda 将其转到视图）
    QSignalSpy spy(panel->m_themeModel, &ThemeListModel::requestCurrentIndex);
    ASSERT_TRUE(spy.isValid());

    // Act：选中已知主题路径（dark 排序后行 0）
    panel->setSelectionTheme(QStringLiteral("/ut-fake-themes/dark.theme"));
    QApplication::processEvents();  // 让 connect 的 lambda（同步直连也即时生效）执行

    // Assert：模型发射定位索引；视图当前索引被 lambda 更新为行 0
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(panel->m_themeView->currentIndex().row(), 0);
    EXPECT_EQ(panel->m_themeView->currentIndex().data(ThemeListModel::ThemePath).toString(),
              QStringLiteral("/ut-fake-themes/dark.theme"));
}

TEST_F(ThemePanelTest, SetSelectionTheme_UnknownPath_KeepsCurrentIndex)
{
    // Arrange：预选行 1 作为基线
    panel->m_themeView->setCurrentIndex(panel->m_themeModel->index(1, 0));
    ASSERT_EQ(panel->m_themeView->currentIndex().row(), 1);

    // Act：未知路径（模型循环不命中，无信号）
    panel->setSelectionTheme(QStringLiteral("/ut-fake-themes/absent.theme"));
    QApplication::processEvents();

    // Assert：当前索引保持不变（负面场景 + 强异常安全）
    EXPECT_EQ(panel->m_themeView->currentIndex().row(), 1);
    EXPECT_EQ(panel->m_themeModel->rowCount(QModelIndex()), 2);
}

// ---- paintEvent ----

TEST_F(ThemePanelTest, PaintEvent_TwoFillPathCalls_BackgroundAndSeparator)
{
    // Arrange：先定背景（背景 #123456 → 亮度 38 <128 → 描边取 #FFFFFF），
    // 拦截 QPainter::fillPath 记录落笔颜色
    panel->setBackground("#123456");
    const QColor expectedBackground(QStringLiteral("#123456"));
    const QColor expectedFrame(QStringLiteral("#FFFFFF"));
    QStringList filledColors;
    stub.set_lamda(&QPainter::fillPath,
                   [&filledColors](QPainter *, const QPainterPath &, const QBrush &brush) {
                       filledColors << brush.color().name();
                   });
    QPaintEvent event(QRect(0, 0, 250, 100));

    // Act：直接调用 protected 覆盖（经 -fno-access-control）
    panel->paintEvent(&event);

    // Assert：恰好两次填充——先 0.9 透明度背景色，后 0.1 透明度描边色（分隔线）
    ASSERT_EQ(filledColors.size(), 2);
    EXPECT_EQ(filledColors.at(0), expectedBackground.name());
    EXPECT_EQ(filledColors.at(1), expectedFrame.name());
}

}  // namespace
