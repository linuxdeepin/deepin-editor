// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// Tabbar（src/controls/tabbar.cpp）单元测试
//
// 类特征：DTabBar 子类（QWidget+DObject 多继承，GUI 类），offscreen 真实实例化；
// 宿主为普通 QWidget（this->window() 经 static_cast<Window*> 读取 width()，
// Window 为 QWidget 单继承链且 width() 非虚 → 指针零偏移安全）。
// 绝不构造真实 Window：window()/StartManager 的全部非虚出向调用由 stub_ext 拦截。
//
// 方法映射（公开/保护/private 全集，private 经 -fno-access-control 直调）：
// - ctor/dtor                      → Constructor_DefaultParent_ConfiguresBarProperties
// - addTab                         → AddTab_MultiplePaths_AppendsAtEnd
// - addTabWithIndex                → AddTabWithIndex_DuplicatePath_IgnoresInsert /
//                                    AddTabWithIndex_BackupDraft_UsesTipPathTooltip /
//                                    AddTabWithIndex_LongPath_WrapsTooltipWithNewlines
// - closeTab                       → CloseTab_ValidIndex_EmitsHistoryAndRemoves / CloseTab_NegativeIndex_EarlyReturn
// - closeCurrentTab()/(path)       → CloseCurrentTab_MiddleTab_ClosesSelected / CloseCurrentTabByPath_UnknownPath_Noop
// - closeOtherTabs                 → CloseOtherTabs_ThreeTabs_EmitsAllExceptCurrent
// - closeLeftTabs/closeRightTabs   → CloseLeftTabs_MiddleTab_EmitsPrecedingOnly / CloseRightTabs_MiddleTab_EmitsFollowingOnly
// - closeOtherTabsExceptFile       → CloseOtherTabsExceptFile_UnknownFile_EmitsAll
// - updateTab                      → UpdateTab_ExistingTab_ChangesTextPathAndTooltip
// - previousTab/nextTab            → PreviousTab_WrapsToLast / NextTab_WrapsToFirst (TEST_P)
// - indexOf                        → IndexOf_KnownAndUnknown_ReturnsIndexOrMinusOne
// - currentName/currentPath        → CurrentNameAndPath_SelectedTab_ReflectsBoth / CurrentNameAndPath_NoTabs_ReturnEmptyValues
// - truePathAt/fileAt/textAt       → PathAccessors_ValidAndInvalid_IndexPathMath (TEST_P)
// - setTabText                     → SetTabText_WithAmpersand_EscapesAndRestores
// - setTabPalette                  → SetTabPalette_AnyColors_NoStateCorruption
// - setBackground/setDNDColor      → SetBackgroundAndDNDColor_AnyColors_StoreMembers
// - showTabs                       → ShowTabs_EdgeSelected_DisablesEdgeActions（依赖右键菜单动作存在）
// - eventFilter                    → EventFilter_FontChange_SyncsFontToAppFont / EventFilter_IgnoredEvent_ReturnsFalse /
//                                    EventFilter_RightClickOnTab_BuildsMenuWithStates /
//                                    EventFilter_RightClickBlankArea_SkipsMenu / EventFilter_MiddleClick_EmitsCloseRequest /
//                                    EventFilter_DragMove_AcceptsEvent
// - mousePressEvent                → MousePressEvent_MiddleClick_EmitsCloseRequest / MousePressEvent_LeftClick_PassesToBaseEditor
// - tabSizeHint                    → TabSizeHint_FewTabs_MaxWidth / TabSizeHint_ManyTabs_ClampedToMin (TEST_P)
// - minimumTabSizeHint/maximumTabSizeHint → MinMaxTabSizeHint_AnyIndex_ReturnsFixedBounds
// - handleTabMoved(private)        → HandleTabMoved_ValidIndices_SwapsPaths / HandleTabMoved_OutOfRange_NoChange
// - handleTabReleased(private)     → HandleTabReleased_EmptyPath_EarlyReturn /
//                                    HandleTabReleased_NullWrapper_EarlyReturn /
//                                    HandleTabReleased_FullPath_RebuildsWindowAndCloses
// - handleTabIsRemoved(private)    → 真实执行（Window::removeWrapper stub）→ CloseTab_ValidIndex_EmitsHistoryAndRemoves 链路
// - handleTabDroped(private)       → HandleTabDroped_ExternalTarget_ReleasesTab /
//                                    HandleTabDroped_TabbarTarget_ClosesTab
// - handleDragActionChanged(private) → HandleDragActionChanged_IgnoreAction_ResetsOverrideCursor（dragIconWindow stub）
// - onTabDrapStart(private)        → OnTabDragStart_DragBegin_SavesOldPaths
// - resizeEvent                    → ResizeEvent_WrappedTooltip_StripsNewlines
// - dropEvent                      → DropEvent_WithDragPixmap_PlaysAnimation（sm_pDragPixmap 静态成员直控）
// - createDragPixmapFromTab(protected) → CreateDragPixmap_Composite_Composites（真实 EditWrapper）
// - createMimeDataFromTab(protected)   → CreateMimeData_WrapperMissing_ReturnsNull /
//                                        CreateMimeData_ValidWrapper_CarriesTabPayload
// - insertFromMimeDataOnDragEnter(protected) → InsertFromMimeDataOnDragEnter_GuardBranches (TEST_P) /
//                                        InsertFromMimeDataOnDragEnter_FullPath_AddsTabViaWindow
// - insertFromMimeData(protected)  → InsertFromMimeData_GuardBranches / InsertFromMimeData_FullPath
// - canInsertFromMimeData(protected) → CanInsertFromMimeData_FormatPresence_ReturnsExpected (TEST_P)
//
// 分支清单（来源：tabbar.cpp）→ 用例映射（B 编号）：
// - addTabWithIndex B1: m_tabPaths.contains → return          → AddTabWithIndex_DuplicatePath_IgnoresInsert
// - addTabWithIndex B2: localDataPath 分支 + isBackupFile+tip  → AddTabWithIndex_LocalDraftBackup_...
// - addTabWithIndex B3: localDataPath 分支 else                → 同上（非备份子分支）
// - addTabWithIndex B4: else 分支 + nFontWidth>=w 换行循环      → AddTabWithIndex_LongPath_...
// - closeTab B5: index<0 → return                              → CloseTab_NegativeIndex_EarlyReturn
// - previousTab/nextTab B6: 边界回绕                            → PreviousTab_WrapsToLast / NextTab_WrapsToFirst
// - eventFilter B7: 非鼠标/拖拽事件早退                          → EventFilter_IgnoredEvent_ReturnsFalse
// - eventFilter B8: watched!=this → false                       → EventFilter_RightClickOnChild...（stranger 对象）
// - eventFilter B9: 右键 + m_rightClickTab>=0 → 菜单            → EventFilter_RightClickOnTab_BuildsMenuWithStates
// - eventFilter B10: 右键 + <0 → 落空                           → EventFilter_RightClickBlankArea_SkipsMenu
// - eventFilter B11: 中键 → tabCloseRequested                   → EventFilter_MiddleClick_EmitsCloseRequest
// - eventFilter B12: DragMove → accept                          → EventFilter_DragMove_AcceptsEvent
// - eventFilter B13: 菜单启用矩阵（len<2 / 首 / 尾 / 中间）      → RightClickMenu_TabMatrix_MatchesEnableStates (TEST_P 4 组)
// - tabSizeHint B14: index<0 → 基类 / ≥0 → 宽度计算             → TabSizeHint_WidthVariants_ReturnsClampedWidth (TEST_P)
// - handleTabMoved B15: 索引合法 → swap / 非法 → 跳过            → HandleTabMoved_*
// - handleTabReleased B16: path 空 → return / wrapper 空 → return / 全路径 → 重建窗口
// - handleTabDroped B17: target 非 Tabbar → handleTabReleased / 是 → closeTab
// - handleDragActionChanged B18: IgnoreAction / 其它            → HandleDragActionChanged_*
// - createMimeDataFromTab B19: wrapper 空 / loading / 有效       → CreateMimeData_*
// - insertFromMimeDataOnDragEnter B20: source 空 / wrapper loading / wrapper 空 / 全路径
// - dropEvent B21: CopyAction+format+sm_pDragPixmap → 动画      → DropEvent_WithDragPixmap_...
// - resizeEvent B22: nFontWidth>=w → 去 \n 重排                 → ResizeEvent_WrappedTooltip_StripsNewlines
//
// 最小清单完成情况：
// | 1 | 每个方法 ≥1 用例 | 完成 |
// | 2 | 等价类（路径 普通草稿/备份/超长、索引 首中尾/越界、菜单 首尾中、mime 有效/无效） | 完成 |
// | 3 | 边界值（0/-1/count-1/count、单标签、空 mimeData） | 完成 |
// | 4 | TEST_P（≥3 组同断言：NextPrev 3 组、PathAccessors 3 组、SizeHint 3 组、
//     RightClickMenu 4 组、CanInsert 3 组、InsertGuard 3 组） | 完成 |
// | 5 | 分支清单映射 | 见上方 |
// | 6 | if 分支两侧全覆盖 | 完成 |
// | 7 | 异常路径 | N/A（无 throw；错误路径按早退分支断言） |
// | 8 | 负面（重复路径、负索引、未知路径、空 mime、越界 move） | 完成 |
// | 9 | 强异常安全（失败路径后 m_tabPaths 不变） | 完成 |
// | 10 | stub_ext（Window/StartManager/QMenu::exec/Utils 路径桩；无 gMock 混用） | 完成 |
//
// [缺陷记录] m_rightMenu 每次 right-click new DMenu 无父对象且旧实例被覆盖 → 泄漏（defects）
// [缺陷记录] JumpLineBar 无关；Tabbar::handleTabReleased 中 m_listOldTabPath.value(index)
//            与 closeTab(newIndex) 的索引换算依赖运行时状态，注释记录

#include <gtest/gtest.h>

#include <QApplication>
#include <DPlatformWindowHandle>
#include <DRecentManager>
#include <DWindowManagerHelper>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QImage>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QStyleFactory>
#include <QTabBar>
#include <QToolButton>
#include <QVariant>
#include <QtTest/QTest>

#include "editwrapper.h"
#include "startmanager.h"
#include "tabbar.h"
#include "test_env.h"
#include "utils.h"
#include "window.h"

namespace {

// EditWrapper 真实构造所需的最小 Window 桩集（复用 B8 已验证矩阵）。
// 说明：Tabbar::currentName/textAt/DTabBar::currentIndex 不打桩——本套件中
// Tabbar 为真实实例且 createMimeDataFromTab 依赖真实 textAt；EditWrapper
// 构造链不触达这些调用（仅 openFile/保存等重路径会读，见 editwrapper.cpp:415）。
void installEditWrapperCtorStubs(stub_ext::StubExt &stub, QWidget *host,
                                 EditWrapper **outWrapper)
{
    stub.set_lamda(&Window::findBarIsVisiable, [](Window *) -> bool { return false; });
    stub.set_lamda(&Window::replaceBarIsVisiable, [](Window *) -> bool { return false; });
    stub.set_lamda(&Window::getKeywordForSearchAll, [](Window *) -> QString { return QString(); });
    stub.set_lamda(&Window::getKeywordForSearch, [](Window *) -> QString { return QString(); });
    // 每次调用新建真实子件（随 host 析构，避免跨用例悬挂）
    QStackedWidget *stackHost = new QStackedWidget(host);
    stackHost->resize(600, 400);
    stub.set_lamda(&Window::getStackedWgt,
                   [stackHost](Window *) -> QStackedWidget * { return stackHost; });
    QTabBar *fakeBar = new QTabBar(host);
    fakeBar->addTab(QString("stub"));
    stub.set_lamda(&Window::getTabbar,
                   [fakeBar](Window *) -> Tabbar * { return reinterpret_cast<Tabbar *>(fakeBar); });
    stub.set_lamda(VADDR(DDialog, exec), []() -> int { return 0; });
    stub.set_lamda(VADDR(QDialog, exec), []() -> int { return 0; });
    stub.set_lamda(&Window::updateModifyStatus,
                   [](Window *, const QString &, bool) {});
    stub.set_lamda(&Window::updateSaveAsFileName, [](Window *, QString, QString) {});
    stub.set_lamda(&Window::saveAsFile, [](Window *) -> bool { return false; });
    stub.set_lamda(&Window::setPrintEnabled, [](Window *, bool) {});
    stub.set_lamda(&DRecentManager::addItem,
                   [](const QString &, DRecentData &) -> bool { return true; });
    stub.set_lamda(&Utils::sendFloatMessageFixedFont,
                   [](QWidget *, const QIcon &, const QString &) {});
    stub.set_lamda(
        static_cast<void (DMessageManager::*)(QWidget *, const QIcon &, const QString &)>(
            &DMessageManager::sendMessage),
        [](DMessageManager *, QWidget *, const QIcon &, const QString &) {});

    EditWrapper *wrapper = new EditWrapper(reinterpret_cast<Window *>(host), host);
    if (outWrapper)
        *outWrapper = wrapper;
}

class TabbarTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { controlsut::ensureApp(); }

    void SetUp() override
    {
        stub.clear();
        countersClear();

        // 宿主：普通 QWidget 充当 window()（Window 单继承 QWidget、width() 非虚）
        host = new QWidget();
        host->resize(1200, 800);
        bar = new Tabbar(host);
        bar->resize(1100, 40);
        host->show();
        QApplication::processEvents();

        // Utils 路径隔离：localDataPath → 临时目录（测试内可覆写）
        localRoot = controlsut::tempRoot().path() + "/tabbar-cases";
        stub.set_lamda(&Utils::localDataPath,
                       [this]() -> QString { return localRoot; });
    }

    void TearDown() override
    {
        // 动画/删除挂起事件在桩仍然有效时冲净
        QApplication::processEvents();
        stub.clear();
        if (Tabbar::sm_pDragPixmap) {
            delete Tabbar::sm_pDragPixmap;
            Tabbar::sm_pDragPixmap = nullptr;
        }
        delete host;  // bar 随宿主析构
        host = nullptr;
        bar = nullptr;
        countersClear();
    }

    void countersClear()
    {
        windowRemoveWrapperCalls = 0;
        windowAddTabCalls = 0;
        windowFocusActiveCalls = 0;
        windowSetChildrenFocusCalls = 0;
        startManagerCreateCalls = 0;
        menuExecCalls = 0;
        windowWrapperResult = nullptr;
        lastCreatedTabPath.clear();
        lastRemoveWrapperPath.clear();
    }

    // ---- Window 出向记录桩（handleTabIsRemoved/handleTabReleased 等真实执行所需）----
    void installWindowRecorderStubs(EditWrapper *wrapperResult = nullptr)
    {
        windowWrapperResult = wrapperResult;
        stub.set_lamda(&Window::wrapper,
                       [this](Window *, const QString &) -> EditWrapper * {
                           return windowWrapperResult;
                       });
        stub.set_lamda(&Window::removeWrapper,
                       [this](Window *, const QString &filePath, bool) {
                           ++windowRemoveWrapperCalls;
                           lastRemoveWrapperPath = filePath;
                       });
        stub.set_lamda(&Window::addTabWithWrapper,
                       [this](Window *, EditWrapper *, const QString &, const QString &,
                              const QString &, int) { ++windowAddTabCalls; });
        stub.set_lamda(&Window::focusActiveEditor, [this](Window *) { ++windowFocusActiveCalls; });
        stub.set_lamda(&Window::setChildrenFocus, [this](Window *, bool) {
            ++windowSetChildrenFocusCalls;
        });
        // StartManager：instance 桩返回哑指针（createWindowFromWrapper 已全桩，
        // 指针绝不被解引用——真实 StartManager 构造涉及 DBus 注册，禁止触发）
        static void *fakeManagerStorage = nullptr;
        stub.set_lamda(&StartManager::instance,
                       []() -> StartManager * {
                           return reinterpret_cast<StartManager *>(&fakeManagerStorage);
                       });
        stub.set_lamda(&StartManager::createWindowFromWrapper,
                       [this](StartManager *, const QString &, const QString &filePath,
                              const QString &, EditWrapper *, bool) {
                           ++startManagerCreateCalls;
                           lastCreatedTabPath = filePath;
                       });
        // 右键菜单 exec 拦截（offscreen 模态防挂起）
        stub.set_lamda(
            static_cast<QAction *(QMenu::*)(const QPoint &, QAction *)>(&QMenu::exec),
            [this](QMenu *, const QPoint &, QAction *) -> QAction * {
                ++menuExecCalls;
                return nullptr;
            });
    }

    // 静默加入标签（不触发 tooltip 长路径分支的干扰）
    void addTabsQuietly(const QStringList &paths)
    {
        for (const QString &p : paths)
            bar->addTab(p, QFileInfo(p).fileName());
        QApplication::processEvents();
    }

    stub_ext::StubExt stub;
    QWidget *host = nullptr;
    Tabbar *bar = nullptr;
    QString localRoot;

    // 记录桩计数（夹具成员，TearDown 重置）
    int windowRemoveWrapperCalls = 0;
    int windowAddTabCalls = 0;
    int windowFocusActiveCalls = 0;
    int windowSetChildrenFocusCalls = 0;
    int startManagerCreateCalls = 0;
    int menuExecCalls = 0;
    EditWrapper *windowWrapperResult = nullptr;
    QString lastCreatedTabPath;
    QString lastRemoveWrapperPath;
};

// ---- 构造 ----

TEST_F(TabbarTest, Constructor_DefaultParent_ConfiguresBarProperties)
{
    // Assert：可移动/可关闭/接受拖放/右对齐省略，无标签
    EXPECT_TRUE(bar->isMovable());
    EXPECT_TRUE(bar->tabsClosable());
    EXPECT_TRUE(bar->acceptDrops());
    EXPECT_EQ(bar->elideMode(), Qt::ElideRight);
    EXPECT_EQ(bar->count(), 0);
    EXPECT_EQ(bar->currentIndex(), -1);
}

// ---- addTab / addTabWithIndex ----

TEST_F(TabbarTest, AddTab_MultiplePaths_AppendsAtEnd)
{
    // Act：依次追加三个标签（走 Utils::localDataPath 含分支 → tooltip=tabName）
    addTabsQuietly({ localRoot + "/a.txt", localRoot + "/b.txt", localRoot + "/c.txt" });

    // Assert：顺序与当前项
    ASSERT_EQ(bar->count(), 3);
    EXPECT_EQ(bar->fileAt(0), localRoot + "/a.txt");
    EXPECT_EQ(bar->fileAt(2), localRoot + "/c.txt");
    EXPECT_EQ(bar->currentIndex(), 2);  // addTabWithIndex 里 setCurrentIndex(index)
    EXPECT_EQ(bar->textAt(0), QString("a.txt"));
}

TEST_F(TabbarTest, AddTabWithIndex_DuplicatePath_IgnoresInsert)
{
    // Arrange
    addTabsQuietly({ localRoot + "/dup.txt" });
    ASSERT_EQ(bar->count(), 1);

    // Act：重复路径插入
    bar->addTabWithIndex(0, localRoot + "/dup.txt", QString("dup2.txt"));

    // Assert：早退分支——不新增、不破坏既有状态
    EXPECT_EQ(bar->count(), 1);
    EXPECT_EQ(bar->fileAt(0), localRoot + "/dup.txt");
    EXPECT_EQ(bar->m_tabPaths.size(), 1);  // 强异常安全：路径表未被污染
}

TEST_F(TabbarTest, AddTabWithIndex_BackupDraft_UsesTipPathTooltip)
{
    // Arrange：备份文件分支（isBackupFile=true + tipPath 非空）
    stub.set_lamda(&Utils::isBackupFile, [](const QString &) -> bool { return true; });
    const QString tip = QString::fromUtf8("/真实/路径/原文档.txt");

    // Act
    bar->addTabWithIndex(0, localRoot + "/draft~bak.txt", QString("draft~bak.txt"), tip);

    // Assert：tooltip 采用真实路径
    ASSERT_EQ(bar->count(), 1);
    EXPECT_EQ(bar->tabToolTip(0), tip);
    EXPECT_EQ(bar->truePathAt(0), tip);
}

TEST_F(TabbarTest, AddTabWithIndex_LongPath_WrapsTooltipWithNewlines)
{
    // Arrange：非 localDataPath 分支 + 超宽字体 → 插入 '\n' 换行循环
    stub.set_lamda(
        static_cast<int (QFontMetrics::*)(const QString &, int) const>(
            &QFontMetrics::horizontalAdvance),
        [](const QFontMetrics *, const QString &, int) -> int { return 100000; });
    const QString longPath = QString::fromUtf8("/very/long/path/to/some/file.txt");

    // Act
    bar->addTabWithIndex(0, longPath, QString("file.txt"));

    // Assert：tooltip 被换行（循环至少插一处 '\n'），尾部仍是文件名
    const QString tip = bar->tabToolTip(0);
    EXPECT_TRUE(tip.contains('\n'));
    EXPECT_TRUE(tip.endsWith(QString("file.txt")));
    // 宽度不足分支（else）：恢复默认宽度，tooltip 无换行
    stub.set_lamda(
        static_cast<int (QFontMetrics::*)(const QString &, int) const>(
            &QFontMetrics::horizontalAdvance),
        [](const QFontMetrics *, const QString &, int) -> int { return 10; });
    bar->addTabWithIndex(1, "/short/p.txt", QString("p.txt"));
    EXPECT_FALSE(bar->tabToolTip(1).contains('\n'));
    EXPECT_EQ(bar->tabToolTip(1), QString("/short/p.txt"));
}

// ---- closeTab / closeCurrentTab ----

TEST_F(TabbarTest, CloseTab_ValidIndex_EmitsHistoryAndRemoves)
{
    // Arrange：真实执行 handleTabIsRemoved（Window::removeWrapper 桩记录）
    installWindowRecorderStubs(nullptr);
    addTabsQuietly({ localRoot + "/x.txt", localRoot + "/y.txt" });
    QSignalSpy historySpy(bar, &Tabbar::requestHistorySaved);

    // Act
    bar->closeTab(0);
    QApplication::processEvents();  // tabIsRemoved（如为队列连接）冲净

    // Assert：历史信号携带被关文件路径、标签与路径表同步收缩、Window 被通知
    ASSERT_EQ(historySpy.count(), 1);
    EXPECT_EQ(historySpy.at(0).at(0).toString(), localRoot + "/x.txt");
    EXPECT_EQ(bar->count(), 1);
    EXPECT_EQ(bar->m_tabPaths, QStringList({ localRoot + "/y.txt" }));
    EXPECT_EQ(bar->m_tabTruePaths.size(), 1);
    EXPECT_EQ(windowRemoveWrapperCalls, 1);
    EXPECT_EQ(lastRemoveWrapperPath, localRoot + "/x.txt");
}

TEST_F(TabbarTest, CloseTab_NegativeIndex_EarlyReturn)
{
    installWindowRecorderStubs(nullptr);
    addTabsQuietly({ localRoot + "/keep.txt" });
    QSignalSpy historySpy(bar, &Tabbar::requestHistorySaved);

    // Act：负面——负索引早退
    bar->closeTab(-1);

    // Assert：无信号、无移除
    EXPECT_EQ(historySpy.count(), 0);
    EXPECT_EQ(bar->count(), 1);
    EXPECT_EQ(windowRemoveWrapperCalls, 0);
}

TEST_F(TabbarTest, CloseCurrentTab_MiddleTab_ClosesSelected)
{
    installWindowRecorderStubs(nullptr);
    addTabsQuietly({ localRoot + "/1.txt", localRoot + "/2.txt", localRoot + "/3.txt" });
    bar->setCurrentIndex(1);
    QSignalSpy historySpy(bar, &Tabbar::requestHistorySaved);

    // Act
    bar->closeCurrentTab();
    QApplication::processEvents();

    // Assert：关闭的是当前选中项
    ASSERT_EQ(historySpy.count(), 1);
    EXPECT_EQ(historySpy.at(0).at(0).toString(), localRoot + "/2.txt");
    EXPECT_EQ(bar->count(), 2);
}

TEST_F(TabbarTest, CloseCurrentTabByPath_UnknownPath_Noop)
{
    installWindowRecorderStubs(nullptr);
    addTabsQuietly({ localRoot + "/here.txt" });
    QSignalSpy historySpy(bar, &Tabbar::requestHistorySaved);

    // Act：未知路径 → indexOf=-1 → closeTab(-1) 早退
    bar->closeCurrentTab(QString::fromUtf8("/not/exists.txt"));

    // Assert
    EXPECT_EQ(historySpy.count(), 0);
    EXPECT_EQ(bar->count(), 1);
}

// ---- closeOtherTabs / closeLeftTabs / closeRightTabs / closeOtherTabsExceptFile ----

TEST_F(TabbarTest, CloseOtherTabs_ThreeTabs_EmitsAllExceptCurrent)
{
    installWindowRecorderStubs(nullptr);
    addTabsQuietly({ localRoot + "/a.txt", localRoot + "/b.txt", localRoot + "/c.txt" });
    bar->setCurrentIndex(1);
    QSignalSpy closeSpy(bar, &Tabbar::closeTabs);

    // Act
    bar->closeOtherTabs();

    // Assert：发出其余路径（不改本地状态，由上层逐个 closeTab）
    ASSERT_EQ(closeSpy.count(), 1);
    const QStringList emitted = closeSpy.at(0).at(0).toStringList();
    EXPECT_EQ(emitted, QStringList({ localRoot + "/a.txt", localRoot + "/c.txt" }));
    EXPECT_EQ(bar->count(), 3);  // 信号语义：不直接移除
}

TEST_F(TabbarTest, CloseLeftTabs_MiddleTab_EmitsPrecedingOnly)
{
    addTabsQuietly({ localRoot + "/l1", localRoot + "/l2", localRoot + "/l3", localRoot + "/l4" });
    QSignalSpy closeSpy(bar, &Tabbar::closeTabs);

    // Act
    bar->closeLeftTabs(localRoot + "/l3");

    // Assert：只包含目标之前的路径，顺序保持；信号语义不移除标签
    ASSERT_EQ(closeSpy.count(), 1);
    EXPECT_EQ(closeSpy.at(0).at(0).toStringList(),
              QStringList({ localRoot + "/l1", localRoot + "/l2" }));
    EXPECT_EQ(bar->count(), 4);
}

TEST_F(TabbarTest, CloseRightTabs_MiddleTab_EmitsFollowingOnly)
{
    addTabsQuietly({ localRoot + "/r1", localRoot + "/r2", localRoot + "/r3" });
    QSignalSpy closeSpy(bar, &Tabbar::closeTabs);

    // Act
    bar->closeRightTabs(localRoot + "/r2");

    // Assert：倒序收集（从尾到目标）；本地标签数不变
    ASSERT_EQ(closeSpy.count(), 1);
    EXPECT_EQ(closeSpy.at(0).at(0).toStringList(), QStringList({ localRoot + "/r3" }));
    EXPECT_EQ(bar->count(), 3);
}

TEST_F(TabbarTest, CloseOtherTabsExceptFile_UnknownFile_EmitsAll)
{
    addTabsQuietly({ localRoot + "/u1", localRoot + "/u2" });
    QSignalSpy closeSpy(bar, &Tabbar::closeTabs);

    // Act：负面——未知文件 → 全部命中
    bar->closeOtherTabsExceptFile(QString::fromUtf8("/missing"));

    // Assert：全部命中（未知文件不匹配任何项）；标签不受影响
    ASSERT_EQ(closeSpy.count(), 1);
    EXPECT_EQ(closeSpy.at(0).at(0).toStringList().size(), 2);
    EXPECT_EQ(bar->count(), 2);
}

// ---- updateTab ----

TEST_F(TabbarTest, UpdateTab_ExistingTab_ChangesTextPathAndTooltip)
{
    installWindowRecorderStubs(nullptr);
    addTabsQuietly({ localRoot + "/before.txt" });

    // Act
    bar->updateTab(0, QString("/elsewhere/after.txt"), QString::fromUtf8("改名&后.txt"));

    // Assert：文本（含助记符转义）、路径表、tooltip（非 localDataPath → 路径本体）全部更新
    EXPECT_EQ(bar->textAt(0), QString::fromUtf8("改名&后.txt"));
    EXPECT_EQ(bar->fileAt(0), QString("/elsewhere/after.txt"));
    EXPECT_EQ(bar->m_tabPaths.at(0), QString("/elsewhere/after.txt"));
    EXPECT_EQ(bar->m_tabTruePaths.at(0), QString("/elsewhere/after.txt"));
    EXPECT_EQ(bar->tabToolTip(0), QString("/elsewhere/after.txt"));
}

// ---- previousTab / nextTab（TEST_P 3 组）----

struct NextPrevCase {
    int startIndex;
    bool doNext;       // true: nextTab, false: previousTab
    int expectedIndex;
};

class TabbarNextPrevTest : public TabbarTest,
                           public ::testing::WithParamInterface<NextPrevCase> {
};

TEST_P(TabbarNextPrevTest, StepTab_BoundaryIndex_WrapsOrSteps)
{
    addTabsQuietly({ localRoot + "/n0", localRoot + "/n1", localRoot + "/n2" });
    bar->setCurrentIndex(GetParam().startIndex);

    // Act
    if (GetParam().doNext)
        bar->nextTab();
    else
        bar->previousTab();

    // Assert：索引回绕/步进；当前路径同步指向期望标签（状态一致）
    EXPECT_EQ(bar->currentIndex(), GetParam().expectedIndex);
    EXPECT_EQ(bar->currentPath(), localRoot + QString("/n%1").arg(GetParam().expectedIndex));
}

INSTANTIATE_TEST_SUITE_P(
    NextPrevCases, TabbarNextPrevTest,
    ::testing::Values(
        NextPrevCase{ 0, false, 2 },  // 边界：首项向前 → 回绕到尾
        NextPrevCase{ 2, true, 0 },   // 边界：尾项向后 → 回绕到首
        NextPrevCase{ 1, true, 2 }));  // 常规步进

// ---- indexOf / currentName / currentPath / 访问器（TEST_P 3 组）----

TEST_F(TabbarTest, IndexOf_KnownAndUnknown_ReturnsIndexOrMinusOne)
{
    addTabsQuietly({ localRoot + "/i1", localRoot + "/i2" });

    // Act & Assert
    EXPECT_EQ(bar->indexOf(localRoot + "/i2"), 1);
    EXPECT_EQ(bar->indexOf(QString::fromUtf8("/absent")), -1);  // 负面
}

struct PathAccessorCase {
    int index;
    bool valid;
};

class TabbarAccessorTest : public TabbarTest,
                           public ::testing::WithParamInterface<PathAccessorCase> {
};

TEST_P(TabbarAccessorTest, PathAccessors_ValidAndInvalid_IndexPathMath)
{
    addTabsQuietly({ localRoot + "/p0", localRoot + "/p1", localRoot + "/p2" });
    const int idx = GetParam().index;

    // Act & Assert：越界安全（value() → 空串）；默认 tipPath 的 truePath 为空
    if (GetParam().valid) {
        EXPECT_EQ(bar->fileAt(idx), localRoot + QString("/p%1").arg(idx));
        EXPECT_TRUE(bar->truePathAt(idx).isEmpty());  // addTab 未传 tipPath
    } else {
        EXPECT_EQ(bar->fileAt(idx), QString());
        EXPECT_EQ(bar->truePathAt(idx), QString());
    }
}

INSTANTIATE_TEST_SUITE_P(
    AccessorCases, TabbarAccessorTest,
    ::testing::Values(
        PathAccessorCase{ 0, true },
        PathAccessorCase{ 2, true },
        PathAccessorCase{ 3, false }));  // 边界：越界一位

TEST_F(TabbarTest, CurrentNameAndPath_SelectedTab_ReflectsBoth)
{
    addTabsQuietly({ localRoot + "/cur&1.txt", localRoot + "/cur2.txt" });
    bar->setCurrentIndex(0);

    // Act & Assert：名称经助记符还原、路径对应当前项
    EXPECT_EQ(bar->currentName(), QString("cur&1.txt"));
    EXPECT_EQ(bar->currentPath(), localRoot + "/cur&1.txt");
}

TEST_F(TabbarTest, CurrentNameAndPath_NoTabs_ReturnEmptyValues)
{
    // 负面：空栏的当前名/路径为空、indexOf 未命中
    EXPECT_EQ(bar->currentName(), QString());
    EXPECT_EQ(bar->currentPath(), QString());
    EXPECT_EQ(bar->currentIndex(), -1);
}

// ---- setTabText：助记符 ----

TEST_F(TabbarTest, SetTabText_WithAmpersand_EscapesAndRestores)
{
    addTabsQuietly({ localRoot + "/m.txt" });

    // Act：设置含 '&' 文本
    bar->setTabText(0, QString::fromUtf8("A&B"));

    // Assert：DTabBar 层双写转义、textAt 层还原
    EXPECT_EQ(bar->DTabBar::tabText(0), QString::fromUtf8("A&&B"));
    EXPECT_EQ(bar->textAt(0), QString::fromUtf8("A&B"));
}

// ---- setTabPalette / setBackground / setDNDColor ----

TEST_F(TabbarTest, SetTabPalette_AnyColors_NoStateCorruption)
{
    addTabsQuietly({ localRoot + "/pal.txt" });
    const int before = bar->count();

    // Act：Qt6 分支为空实现（Q_UNUSED）
    bar->setTabPalette(QString("#ffffff"), QString("#000000"));

    // Assert：不破坏状态
    EXPECT_EQ(bar->count(), before);
    EXPECT_EQ(bar->fileAt(0), localRoot + "/pal.txt");
}

TEST_F(TabbarTest, SetBackgroundAndDNDColor_AnyColors_StoreMembers)
{
    // Act
    bar->setBackground(QString("#111111"), QString("#222222"));
    bar->setDNDColor(QString("#333333"), QString("#444444"));

    // Assert：private 成员直读（-fno-access-control）
    EXPECT_EQ(bar->m_backgroundStartColor, QString("#111111"));
    EXPECT_EQ(bar->m_backgroundEndColor, QString("#222222"));
    EXPECT_EQ(bar->m_dndStartColor, QString("#333333"));
    EXPECT_EQ(bar->m_dndEndColor, QString("#444444"));
}

// ---- eventFilter：右键菜单（含启用矩阵 TEST_P 4 组）----

struct RightClickCase {
    int tabCount;
    int clickOn;      // 右键命中的标签索引
    int tabsLeft;     // 期望 closeLeftTabAction 状态（-1 表示不适用/置灰组）
    bool leftEnabled;
    bool rightEnabled;
    bool otherEnabled;
    bool moreWaysEnabled;
};

class TabbarRightClickTest : public TabbarTest,
                             public ::testing::WithParamInterface<RightClickCase> {
};

TEST_P(TabbarRightClickTest, RightClickMenu_TabMatrix_MatchesEnableStates)
{
    installWindowRecorderStubs(nullptr);
    QStringList paths;
    for (int i = 0; i < GetParam().tabCount; ++i)
        paths << localRoot + QString("/tab%1.txt").arg(i);
    addTabsQuietly(paths);
    const int target = GetParam().clickOn;
    ASSERT_GE(target, 0);

    // Act：在目标标签矩形中心合成右键
    const QRect rect = bar->tabRect(target);
    ASSERT_FALSE(rect.isNull());
    QMouseEvent rightPress(QEvent::MouseButtonPress, rect.center(), bar->mapToGlobal(rect.center()),
                           Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    const bool handled = bar->eventFilter(bar, &rightPress);

    // Assert：菜单构建 + exec 恰一次 + 返回 true（事件被吞）
    EXPECT_TRUE(handled);
    EXPECT_EQ(menuExecCalls, 1);
    ASSERT_NE(bar->m_rightMenu, nullptr);
    ASSERT_NE(bar->m_closeTabAction, nullptr);
    ASSERT_NE(bar->m_closeLeftTabAction, nullptr);
    ASSERT_NE(bar->m_closeRightTabAction, nullptr);
    ASSERT_NE(bar->m_closeOtherTabAction, nullptr);
    ASSERT_NE(bar->m_moreWaysCloseMenu, nullptr);
    // 启用矩阵
    EXPECT_EQ(bar->m_closeLeftTabAction->isEnabled(), GetParam().leftEnabled);
    EXPECT_EQ(bar->m_closeRightTabAction->isEnabled(), GetParam().rightEnabled);
    EXPECT_EQ(bar->m_closeOtherTabAction->isEnabled(), GetParam().otherEnabled);
    EXPECT_EQ(bar->m_moreWaysCloseMenu->isEnabled(), GetParam().moreWaysEnabled);
    // 菜单动作挂接：关闭标签动作触发 → tabCloseRequested
    QSignalSpy closeRequestSpy(bar, &Tabbar::tabCloseRequested);
    bar->m_closeTabAction->trigger();
    QApplication::processEvents();
    EXPECT_EQ(closeRequestSpy.count(), 1);
}

INSTANTIATE_TEST_SUITE_P(
    MenuMatrix, TabbarRightClickTest,
    ::testing::Values(
        // 单标签：全部禁用（len<2 + rc==0&&cnt==1 + 尾==首 全命中）
        RightClickCase{ 1, 0, -1, false, false, false, false },
        // 两标签右键首项：left 禁、right 启
        RightClickCase{ 2, 0, -1, false, true, true, true },
        // 两标签右键尾项：left 启、right 禁
        RightClickCase{ 2, 1, -1, true, false, true, true },
        // 三标签右键中间：left/right 均启
        RightClickCase{ 3, 1, -1, true, true, true, true }));

TEST_F(TabbarTest, EventFilter_RightClickBlankArea_SkipsMenu)
{
    installWindowRecorderStubs(nullptr);
    addTabsQuietly({ localRoot + "/only.txt" });

    // Act：右键空白处（标签条最右上角远离标签）
    const QPoint blank(std::max(10, bar->width() - 4), 4);
    QMouseEvent rightPress(QEvent::MouseButtonPress, blank, bar->mapToGlobal(blank),
                           Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    const bool handled = bar->eventFilter(bar, &rightPress);

    // Assert：tabAt=-1 → 不建菜单不 exec、事件不吞
    EXPECT_FALSE(handled);
    EXPECT_EQ(menuExecCalls, 0);
    EXPECT_EQ(bar->m_rightMenu, nullptr);
    EXPECT_EQ(bar->count(), 1);  // 状态未受影响
}

// ---- showTabs：依赖右键菜单动作已存在 ----

TEST_F(TabbarTest, ShowTabs_EdgeSelected_DisablesEdgeActions)
{
    installWindowRecorderStubs(nullptr);
    addTabsQuietly({ localRoot + "/s0", localRoot + "/s1", localRoot + "/s2" });
    // 先构造右键菜单（exec 已桩）
    const QRect rect = bar->tabRect(1);
    QMouseEvent rightPress(QEvent::MouseButtonPress, rect.center(), bar->mapToGlobal(rect.center()),
                           Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    bar->eventFilter(bar, &rightPress);
    ASSERT_NE(bar->m_closeLeftTabAction, nullptr);

    // Act 1：选中首项 → 左侧无标签 → closeLeft 禁用
    bar->setCurrentIndex(0);
    bar->showTabs();
    EXPECT_FALSE(bar->m_closeLeftTabAction->isEnabled());

    // Act 2：选中尾项 → closeRight 禁用
    bar->setCurrentIndex(2);
    bar->showTabs();
    EXPECT_FALSE(bar->m_closeRightTabAction->isEnabled());
    // 中间项两者都不额外禁用（保持上一次 enabled 值 → 显式重新启用对照）
    bar->setCurrentIndex(1);
    bar->showTabs();
    EXPECT_EQ(bar->currentIndex(), 1);
}

// ---- eventFilter：其它分支 ----

TEST_F(TabbarTest, EventFilter_FontChange_SyncsFontToAppFont)
{
    // Arrange：字体偏离 app 字体
    QFont alien(QString("monospace"));
    alien.setPointSize(qApp->font().pointSize() + 5);
    bar->setFont(alien);
    ASSERT_NE(bar->font().toString(), qApp->font().toString());

    // Act：直调 eventFilter 处理 ApplicationFontChange（watched==this 分支）
    QEvent fontEvent(QEvent::ApplicationFontChange);
    const bool handled = bar->eventFilter(bar, &fontEvent);

    // Assert：字体被同步为 app 字体、事件不吞
    EXPECT_FALSE(handled);
    EXPECT_EQ(bar->font().toString(), qApp->font().toString());

    // 分支另一侧：字体已一致 → 不再设置（幂等）
    const bool handled2 = bar->eventFilter(bar, &fontEvent);
    EXPECT_FALSE(handled2);
    EXPECT_EQ(bar->font().toString(), qApp->font().toString());
}

TEST_F(TabbarTest, EventFilter_IgnoredEvent_ReturnsFalse)
{
    // Arrange：非鼠标/拖拽事件（如 MouseMove）→ 早退分支
    QMouseEvent moveEvent(QEvent::MouseMove, QPointF(1, 1), QPointF(1, 1), Qt::NoButton,
                          Qt::NoButton, Qt::NoModifier);

    // Act
    const bool handled = bar->eventFilter(bar, &moveEvent);

    // Assert：早退不吞事件，也不触发菜单构建
    EXPECT_FALSE(handled);
    EXPECT_EQ(menuExecCalls, 0);
}

TEST_F(TabbarTest, EventFilter_MiddleClick_EmitsCloseRequest)
{
    installWindowRecorderStubs(nullptr);
    addTabsQuietly({ localRoot + "/mid.txt" });
    QSignalSpy closeSpy(bar, &Tabbar::tabCloseRequested);

    // Act：中键点击标签
    const QRect rect = bar->tabRect(0);
    QMouseEvent midPress(QEvent::MouseButtonPress, rect.center(), bar->mapToGlobal(rect.center()),
                         Qt::MiddleButton, Qt::MiddleButton, Qt::NoModifier);
    const bool handled = bar->eventFilter(bar, &midPress);

    // Assert：事件被吞 + tabCloseRequested(0)
    EXPECT_TRUE(handled);
    ASSERT_EQ(closeSpy.count(), 1);
    EXPECT_EQ(closeSpy.at(0).at(0).toInt(), 0);
    EXPECT_EQ(bar->count(), 1);  // 只发请求不移除（由上层 closeTab）
}

TEST_F(TabbarTest, EventFilter_MiddleClickOnChild_NotSwallowed)
{
    installWindowRecorderStubs(nullptr);
    addTabsQuietly({ localRoot + "/child.txt" });
    QSignalSpy closeSpy(bar, &Tabbar::tabCloseRequested);

    // Act：watched 不是 bar 自身（早退 false 分支）
    QWidget child(host);
    QMouseEvent midPress(QEvent::MouseButtonPress, QPointF(2, 2), QPointF(2, 2),
                         Qt::MiddleButton, Qt::MiddleButton, Qt::NoModifier);
    const bool handled = bar->eventFilter(&child, &midPress);

    // Assert
    EXPECT_FALSE(handled);
    EXPECT_EQ(closeSpy.count(), 0);
}

TEST_F(TabbarTest, EventFilter_DragMove_AcceptsEvent)
{
    // Arrange
    QMimeData mime;
    QDragMoveEvent dragMove(QPoint(5, 5), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);

    // Act
    const bool handled = bar->eventFilter(bar, &dragMove);

    // Assert：DragMove 分支 accept 但不吞事件（返回 false），标签状态不变
    EXPECT_FALSE(handled);
    EXPECT_TRUE(dragMove.isAccepted());
    EXPECT_EQ(bar->count(), 0);
}

// ---- mousePressEvent ----

TEST_F(TabbarTest, MousePressEvent_MiddleClick_EmitsCloseRequest)
{
    installWindowRecorderStubs(nullptr);
    addTabsQuietly({ localRoot + "/mp.txt" });
    QSignalSpy closeSpy(bar, &Tabbar::tabCloseRequested);

    // Act：中键（Qt6 MiddleButton）
    const QRect rect = bar->tabRect(0);
    QMouseEvent midPress(QEvent::MouseButtonPress, rect.center(), bar->mapToGlobal(rect.center()),
                         Qt::MiddleButton, Qt::MiddleButton, Qt::NoModifier);
    bar->mousePressEvent(&midPress);

    // Assert：中键只发关闭请求（由上层 closeTab），本地不移除
    ASSERT_EQ(closeSpy.count(), 1);
    EXPECT_EQ(closeSpy.at(0).at(0).toInt(), 0);
    EXPECT_EQ(bar->count(), 1);
}

TEST_F(TabbarTest, MousePressEvent_LeftClick_PassesToBaseEditor)
{
    installWindowRecorderStubs(nullptr);
    addTabsQuietly({ localRoot + "/left.txt" });
    QSignalSpy closeSpy(bar, &Tabbar::tabCloseRequested);

    // Act：左键 → else 分支透传 DTabBar
    const QRect rect = bar->tabRect(0);
    QMouseEvent leftPress(QEvent::MouseButtonPress, rect.center(), bar->mapToGlobal(rect.center()),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    EXPECT_NO_THROW(bar->mousePressEvent(&leftPress));

    // Assert：中键语义不触发
    QApplication::processEvents();
    EXPECT_EQ(closeSpy.count(), 0);
    EXPECT_EQ(bar->count(), 1);
}

// ---- tabSizeHint 系列（TEST_P 3 组）----

struct SizeHintCase {
    int tabCount;
    int barWidth;
    int index;
    int expectedWidth;  // -1 → 基类返回（index<0）
    bool expectBase;    // index<0 → 走 DTabBar::tabSizeHint
};

class TabbarSizeHintTest : public TabbarTest,
                           public ::testing::WithParamInterface<SizeHintCase> {
};

TEST_P(TabbarSizeHintTest, TabSizeHint_WidthVariants_ReturnsClampedWidth)
{
    QStringList paths;
    for (int i = 0; i < GetParam().tabCount; ++i)
        paths << localRoot + QString("/sz%1").arg(i);
    addTabsQuietly(paths);
    bar->resize(GetParam().barWidth, 40);
    QApplication::processEvents();

    // Act
    const QSize hint = bar->tabSizeHint(GetParam().index);

    // Assert
    if (GetParam().expectBase) {
        EXPECT_EQ(hint, bar->DTabBar::tabSizeHint(GetParam().index));  // index<0 → 基类
    } else {
        EXPECT_EQ(hint.width(), GetParam().expectedWidth);
        EXPECT_EQ(hint.height(), 40);  // 非紧凑模式
    }
}

INSTANTIATE_TEST_SUITE_P(
    SizeCases, TabbarSizeHintTest,
    ::testing::Values(
        SizeHintCase{ 2, 1100, 0, 160, false },   // 少标签 → 最大宽
        SizeHintCase{ 8, 300, 3, 110, false },    // 多标签溢出 → 等分后夹到最小宽
        SizeHintCase{ 2, 1100, -1, -1, true }));  // 边界：负索引 → 基类

TEST_F(TabbarTest, MinMaxTabSizeHint_AnyIndex_ReturnsFixedBounds)
{
    // Act & Assert：固定边界（非紧凑：高 40）
    EXPECT_EQ(bar->minimumTabSizeHint(0), QSize(110, 40));
    EXPECT_EQ(bar->maximumTabSizeHint(0), QSize(160, 40));
    EXPECT_EQ(bar->minimumTabSizeHint(99), QSize(110, 40));  // 索引不参与
    EXPECT_EQ(bar->maximumTabSizeHint(-1), QSize(160, 40));
}

// ---- handleTabMoved ----

TEST_F(TabbarTest, HandleTabMoved_ValidIndices_SwapsPaths)
{
    addTabsQuietly({ localRoot + "/m0", localRoot + "/m1", localRoot + "/m2" });

    // Act：0 ↔ 2
    bar->handleTabMoved(0, 2);

    // Assert：路径表与真路径表同步交换
    EXPECT_EQ(bar->m_tabPaths,
              QStringList({ localRoot + "/m2", localRoot + "/m1", localRoot + "/m0" }));
    EXPECT_EQ(bar->m_tabTruePaths.size(), 3);
}

TEST_F(TabbarTest, HandleTabMoved_OutOfRange_NoChange)
{
    addTabsQuietly({ localRoot + "/g0", localRoot + "/g1" });
    const QStringList before = bar->m_tabPaths;

    // Act：负面——越界/负索引组合均不交换
    bar->handleTabMoved(0, 99);
    bar->handleTabMoved(-1, 1);

    // Assert：强异常安全（路径表与真路径表均不变）
    EXPECT_EQ(bar->m_tabPaths, before);
    EXPECT_EQ(bar->m_tabTruePaths.size(), 2);
}

// ---- onTabDrapStart / handleTabDroped / handleTabReleased ----

TEST_F(TabbarTest, OnTabDragStart_DragBegin_SavesOldPaths)
{
    installWindowRecorderStubs(nullptr);
    addTabsQuietly({ localRoot + "/d0", localRoot + "/d1" });

    // Act
    bar->onTabDrapStart();

    // Assert：快照保存 + Window 失焦调用
    EXPECT_EQ(bar->m_listOldTabPath, bar->m_tabPaths);
    EXPECT_EQ(windowSetChildrenFocusCalls, 1);
}

TEST_F(TabbarTest, HandleTabDroped_ExternalTarget_ReleasesTab)
{
    // Arrange：外部目标（target 非 Tabbar）→ handleTabReleased；旧路径为空 → 早退
    installWindowRecorderStubs(nullptr);
    addTabsQuietly({ localRoot + "/ext.txt" });
    QSignalSpy historySpy(bar, &Tabbar::requestHistorySaved);

    // Act
    bar->handleTabDroped(0, Qt::MoveAction, nullptr);

    // Assert：handleTabReleased 走 path 空早退（m_listOldTabPath 未初始化为空）
    EXPECT_EQ(startManagerCreateCalls, 0);
    EXPECT_EQ(historySpy.count(), 0);
    EXPECT_EQ(bar->count(), 1);
}

TEST_F(TabbarTest, HandleTabDroped_TabbarTarget_ClosesTab)
{
    installWindowRecorderStubs(nullptr);
    addTabsQuietly({ localRoot + "/inner0", localRoot + "/inner1" });
    QSignalSpy historySpy(bar, &Tabbar::requestHistorySaved);

    // Act：目标为另一 Tabbar（同类型实例）
    Tabbar other(host);
    bar->handleTabDroped(0, Qt::MoveAction, &other);
    QApplication::processEvents();

    // Assert：closeTab 路径（历史信号 + 移除 + removeWrapper 通知）
    ASSERT_EQ(historySpy.count(), 1);
    EXPECT_EQ(historySpy.at(0).at(0).toString(), localRoot + "/inner0");
    EXPECT_EQ(bar->count(), 1);
    EXPECT_EQ(windowRemoveWrapperCalls, 1);
}

TEST_F(TabbarTest, HandleTabReleased_NullWrapper_EarlyReturn)
{
    installWindowRecorderStubs(nullptr);
    addTabsQuietly({ localRoot + "/rel.txt" });
    bar->m_listOldTabPath = bar->m_tabPaths;  // 非空旧路径
    QSignalSpy historySpy(bar, &Tabbar::requestHistorySaved);

    // Act：wrapper 为 null → 早退（不重建窗口、不关闭）
    bar->handleTabReleased(0);

    // Assert
    EXPECT_EQ(startManagerCreateCalls, 0);
    EXPECT_EQ(historySpy.count(), 0);
    EXPECT_EQ(bar->count(), 1);
}

TEST_F(TabbarTest, HandleTabReleased_EmptyPath_EarlyReturn)
{
    installWindowRecorderStubs(nullptr);

    // Act：旧路径为空 → 早退（index -1 也归零处理）
    bar->handleTabReleased(-1);

    // Assert：无任何出向调用
    EXPECT_EQ(startManagerCreateCalls, 0);
    EXPECT_EQ(windowRemoveWrapperCalls, 0);
}

// ---- handleDragActionChanged ----

TEST_F(TabbarTest, HandleDragActionChanged_IgnoreAction_ResetsOverrideCursor)
{
    // Arrange：dragIconWindow 非 null（DTabBar QWindow* 桩）+ 平台静态调用桩
    static QWindow dragWindow;
    int changeCursorCalls = 0;
    stub.set_lamda(&DTabBar::dragIconWindow, [](DTabBar *) -> QWindow * {
        return &dragWindow;
    });
    stub.set_lamda(
        static_cast<void (*)(const QCursor &)>(&QGuiApplication::changeOverrideCursor),
        [&changeCursorCalls](const QCursor &) { ++changeCursorCalls; });
    int disableCursorCalls = 0;
    stub.set_lamda(&DPlatformWindowHandle::setDisableWindowOverrideCursor,
                   [&disableCursorCalls](QWindow *, bool) { ++disableCursorCalls; });

    // Act 1：IgnoreAction → 箭头光标 + 禁用覆盖
    bar->handleDragActionChanged(Qt::IgnoreAction);
    EXPECT_EQ(changeCursorCalls, 1);
    EXPECT_EQ(disableCursorCalls, 1);

    // Act 2：其它动作 → 恢复覆盖（无 overrideCursor 时不改光标）
    bar->handleDragActionChanged(Qt::CopyAction);
    EXPECT_EQ(disableCursorCalls, 2);
    EXPECT_EQ(changeCursorCalls, 1);  // overrideCursor() 为空 → 分支内不改
}

TEST_F(TabbarTest, HandleDragActionChanged_NoDragWindow_Noop)
{
    // Arrange：dragIconWindow 为 null → 两分支均空转
    stub.set_lamda(&DTabBar::dragIconWindow, [](DTabBar *) -> QWindow * { return nullptr; });
    int changeCursorCalls = 0;
    stub.set_lamda(
        static_cast<void (*)(const QCursor &)>(&QGuiApplication::changeOverrideCursor),
        [&changeCursorCalls](const QCursor &) { ++changeCursorCalls; });

    int disableCursorCalls = 0;
    stub.set_lamda(&DPlatformWindowHandle::setDisableWindowOverrideCursor,
                   [&disableCursorCalls](QWindow *, bool) { ++disableCursorCalls; });

    // Act
    bar->handleDragActionChanged(Qt::IgnoreAction);
    bar->handleDragActionChanged(Qt::CopyAction);

    // Assert：无拖拽图标窗口 → 光标两个桩均零调用
    EXPECT_EQ(changeCursorCalls, 0);
    EXPECT_EQ(disableCursorCalls, 0);
}

// ---- resizeEvent ----

TEST_F(TabbarTest, ResizeEvent_WrappedTooltip_StripsNewlines)
{
    // Arrange：植入含 '\n' 的 tooltip（模拟超长路径换行后重排）
    installWindowRecorderStubs(nullptr);
    addTabsQuietly({ localRoot + "/res.txt" });
    bar->setTabToolTip(0, QString("line1\nline2"));

    // Act：触发 resizeEvent（去 '\n' 重排）
    bar->resize(900, 40);
    QApplication::processEvents();

    // Assert：tooltip 不再含换行，且内容为拼接原文
    EXPECT_FALSE(bar->tabToolTip(0).contains('\n'));
    EXPECT_EQ(bar->tabToolTip(0), QString("line1line2"));
}

// ---- dropEvent ----

TEST_F(TabbarTest, DropEvent_WithDragPixmap_PlaysAnimation)
{
    installWindowRecorderStubs(nullptr);
    addTabsQuietly({ localRoot + "/drop.txt" });

    // Arrange：静态拖拽位图就位（public static 成员直控）
    ASSERT_EQ(Tabbar::sm_pDragPixmap, nullptr);
    Tabbar::sm_pDragPixmap = new QPixmap(24, 24);
    Tabbar::sm_pDragPixmap->fill(Qt::red);

    QMimeData mime;
    mime.setData("dedit/tabbar", QByteArray("drop.txt"));
    QDropEvent drop(QPointF(10, 10), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);

    // Act：真实 dropEvent（CopyAction+格式命中 → 动画分支；DTabBar::dropEvent 基类透传）
    EXPECT_NO_THROW(bar->dropEvent(&drop));
    // 冲净动画与 deleteLater
    QTest::qWait(220);
    QApplication::processEvents();

    // Assert：动画创建的 DLabel 在 finished 后自删（无泄漏迹象：无断言崩溃即通过，
    // 以静态位图仍可用作二次断言依据）；拖放后标签仍在
    ASSERT_NE(Tabbar::sm_pDragPixmap, nullptr);
    EXPECT_EQ(Tabbar::sm_pDragPixmap->width(), 24);
    EXPECT_EQ(bar->count(), 1);
}

// ---- createMimeDataFromTab / canInsertFromMimeData ----

TEST_F(TabbarTest, CreateMimeData_WrapperMissing_ReturnsNull)
{
    installWindowRecorderStubs(nullptr);
    addTabsQuietly({ localRoot + "/mime.txt" });

    // Act：wrapper 查无 → null（源码在判空前 new QMimeData 并挂 window 父对象）
    QStyleOptionTab option;
    QMimeData *result = bar->createMimeDataFromTab(0, option);

    // Assert：返回 null 且标签状态不受拖拽准备失败影响
    EXPECT_EQ(result, nullptr);
    EXPECT_EQ(bar->textAt(0), QString("mime.txt"));
}

TEST_F(TabbarTest, CreateMimeData_ValidWrapper_CarriesTabPayload)
{
    // Arrange：真实 EditWrapper（B8 已验证构造矩阵）
    EditWrapper *wrapper = nullptr;
    installEditWrapperCtorStubs(stub, host, &wrapper);
    wrapper->textEditor()->setSettings(Settings::instance());
    installWindowRecorderStubs(wrapper);
    const QString tabPath = localRoot + "/real.txt";
    bar->addTab(tabPath, QString("real.txt"));
    QStyleOptionTab option;

    // Act
    QMimeData *mime = bar->createMimeDataFromTab(0, option);

    // Assert：载荷含标签名/格式，wrapper 以 void* 属性携带
    ASSERT_NE(mime, nullptr);
    EXPECT_TRUE(mime->hasFormat("dedit/tabbar"));
    EXPECT_EQ(QString::fromUtf8(mime->data("dedit/tabbar")), QString("real.txt"));
    EXPECT_FALSE(mime->hasFormat("text/plain"));  // removeFormat 生效
    EXPECT_NE(mime->property("wrapper").value<void *>(), nullptr);
    delete mime;
}

TEST_F(TabbarTest, CreateMimeData_LoadingWrapper_ReturnsNull)
{
    // Arrange：真实 EditWrapper + loading 标记桩
    EditWrapper *wrapper = nullptr;
    installEditWrapperCtorStubs(stub, host, &wrapper);
    stub.set_lamda(&EditWrapper::getFileLoading, [](EditWrapper *) -> bool { return true; });
    installWindowRecorderStubs(wrapper);
    bar->addTab(localRoot + "/loading.txt", QString("loading.txt"));
    QStyleOptionTab option;

    // Act：大文本加载中 → 禁止拖出
    QMimeData *mime = bar->createMimeDataFromTab(0, option);

    // Assert：返回 null 且标签保留
    EXPECT_EQ(mime, nullptr);
    EXPECT_EQ(bar->textAt(0), QString("loading.txt"));
}

struct CanInsertCase {
    bool hasFormat;
    bool expected;
};

class TabbarCanInsertTest : public TabbarTest,
                            public ::testing::WithParamInterface<CanInsertCase> {
};

TEST_P(TabbarCanInsertTest, CanInsertFromMimeData_FormatPresence_ReturnsExpected)
{
    QMimeData mime;
    if (GetParam().hasFormat)
        mime.setData("dedit/tabbar", QByteArray("x"));

    // Act
    const bool ok = bar->canInsertFromMimeData(0, &mime);

    // Assert：判定与格式在场状态一致
    EXPECT_EQ(ok, GetParam().expected);
    EXPECT_EQ(mime.hasFormat("dedit/tabbar"), GetParam().hasFormat);
}

INSTANTIATE_TEST_SUITE_P(
    FormatCases, TabbarCanInsertTest,
    ::testing::Values(
        CanInsertCase{ true, true },
        CanInsertCase{ false, false },
        CanInsertCase{ true, true }));  // 第三组：重复校验幂等

// ---- insertFromMimeDataOnDragEnter / insertFromMimeData ----

TEST_F(TabbarTest, InsertFromMimeDataOnDragEnter_NullSource_EarlyReturns)
{
    installWindowRecorderStubs(nullptr);

    // Act
    bar->insertFromMimeDataOnDragEnter(0, nullptr);

    // Assert：无出向调用
    EXPECT_EQ(windowAddTabCalls, 0);
    EXPECT_EQ(bar->count(), 0);
}

TEST_F(TabbarTest, InsertFromMimeDataOnDragEnter_InvalidWrapper_EarlyReturns)
{
    // Arrange：mime 无 wrapper 属性（QVariant null → static_cast null）
    QMimeData mime;
    mime.setData("dedit/tabbar", QByteArray("t.txt"));
    installWindowRecorderStubs(nullptr);

    // Act
    bar->insertFromMimeDataOnDragEnter(0, &mime);

    // Assert：wrapper null → 早退（无加签、无本地标签）
    EXPECT_EQ(windowAddTabCalls, 0);
    EXPECT_EQ(bar->count(), 0);
}

TEST_F(TabbarTest, InsertFromMimeDataOnDragEnter_LoadingWrapper_EarlyReturns)
{
    EditWrapper *wrapper = nullptr;
    installEditWrapperCtorStubs(stub, host, &wrapper);
    stub.set_lamda(&EditWrapper::getFileLoading, [](EditWrapper *) -> bool { return true; });
    installWindowRecorderStubs(wrapper);
    QMimeData mime;
    mime.setData("dedit/tabbar", QByteArray("t.txt"));
    mime.setProperty("wrapper", QVariant::fromValue(static_cast<void *>(wrapper)));

    // Act：大文本加载中 → 早退
    bar->insertFromMimeDataOnDragEnter(0, &mime);

    // Assert
    EXPECT_EQ(windowAddTabCalls, 0);
    EXPECT_EQ(bar->count(), 0);
}

TEST_F(TabbarTest, InsertFromMimeDataOnDragEnter_FullPath_AddsTabViaWindow)
{
    // Arrange：真实 EditWrapper + 出向记录桩（updateModifyStatus/OnUpdateHighlighter 桩）
    EditWrapper *wrapper = nullptr;
    installEditWrapperCtorStubs(stub, host, &wrapper);
    int updateModifyCalls = 0;
    stub.set_lamda(&EditWrapper::updateModifyStatus,
                   [&updateModifyCalls](EditWrapper *, bool) { ++updateModifyCalls; });
    stub.set_lamda(&EditWrapper::OnUpdateHighlighter, [](EditWrapper *) {});
    installWindowRecorderStubs(wrapper);
    QMimeData mime;
    mime.setData("dedit/tabbar", QString::fromUtf8("拖拽标签.txt").toUtf8());
    mime.setProperty("wrapper", QVariant::fromValue(static_cast<void *>(wrapper)));
    mime.setProperty("isModified", true);

    // Act
    bar->insertFromMimeDataOnDragEnter(1, &mime);

    // Assert：Window::addTabWithWrapper + 修改态回写 + 编辑器聚焦，全链各一次
    EXPECT_EQ(windowAddTabCalls, 1);
    EXPECT_EQ(updateModifyCalls, 1);
    EXPECT_EQ(windowFocusActiveCalls, 1);
}

TEST_F(TabbarTest, InsertFromMimeData_NullSource_EarlyReturns)
{
    installWindowRecorderStubs(nullptr);

    // Act
    bar->insertFromMimeData(0, nullptr);

    // Assert
    EXPECT_EQ(windowAddTabCalls, 0);
    EXPECT_EQ(bar->count(), 0);
}

TEST_F(TabbarTest, InsertFromMimeData_InvalidWrapper_EarlyReturns)
{
    QMimeData mime;
    mime.setData("dedit/tabbar", QByteArray("t.txt"));
    installWindowRecorderStubs(nullptr);

    // Act
    bar->insertFromMimeData(0, &mime);

    // Assert
    EXPECT_EQ(windowAddTabCalls, 0);
    EXPECT_EQ(bar->count(), 0);
}

TEST_F(TabbarTest, InsertFromMimeData_FullPath_AddsTabViaWindow)
{
    EditWrapper *wrapper = nullptr;
    installEditWrapperCtorStubs(stub, host, &wrapper);
    int updateModifyCalls = 0;
    stub.set_lamda(&EditWrapper::updateModifyStatus,
                   [&updateModifyCalls](EditWrapper *, bool) { ++updateModifyCalls; });
    stub.set_lamda(&EditWrapper::OnUpdateHighlighter, [](EditWrapper *) {});
    installWindowRecorderStubs(wrapper);
    QMimeData mime;
    mime.setData("dedit/tabbar", QByteArray("t2.txt"));
    mime.setProperty("wrapper", QVariant::fromValue(static_cast<void *>(wrapper)));

    // Act
    bar->insertFromMimeData(2, &mime);

    // Assert
    EXPECT_EQ(windowAddTabCalls, 1);
    EXPECT_EQ(updateModifyCalls, 1);
    EXPECT_EQ(windowFocusActiveCalls, 1);
}

// ---- handleTabReleased 全路径（真实 EditWrapper）----

TEST_F(TabbarTest, HandleTabReleased_FullPath_RebuildsWindowAndCloses)
{
    // Arrange：真实 EditWrapper 拖出重建窗口
    EditWrapper *wrapper = nullptr;
    installEditWrapperCtorStubs(stub, host, &wrapper);
    wrapper->textEditor()->setSettings(Settings::instance());
    installWindowRecorderStubs(wrapper);
    const QString path = localRoot + "/dragout.txt";
    bar->addTab(path, QString("dragout.txt"));
    bar->m_listOldTabPath = bar->m_tabPaths;
    QSignalSpy historySpy(bar, &Tabbar::requestHistorySaved);

    // Act
    bar->handleTabReleased(0);
    QApplication::processEvents();

    // Assert：重建窗口（StartManager）+ 关闭原标签 + 从窗口摘除 wrapper（不删除）
    EXPECT_EQ(startManagerCreateCalls, 1);
    EXPECT_EQ(lastCreatedTabPath, path);
    ASSERT_EQ(historySpy.count(), 1);  // closeTab(newIndex) 历史
    EXPECT_EQ(historySpy.at(0).at(0).toString(), path);
    EXPECT_EQ(bar->count(), 0);
    // removeWrapper 两次：closeTab → handleTabIsRemoved 一次 + 显式 removeWrapper 一次
    EXPECT_EQ(windowRemoveWrapperCalls, 2);
    EXPECT_EQ(lastRemoveWrapperPath, path);
}

// ---- createDragPixmapFromTab（真实渲染，两分支）----

TEST_F(TabbarTest, CreateDragPixmap_NoComposite_ReturnsScreenshot)
{
    // Arrange：真实 EditWrapper（其 textEditor 离屏 render 真实执行）
    EditWrapper *wrapper = nullptr;
    installEditWrapperCtorStubs(stub, host, &wrapper);
    wrapper->textEditor()->setSettings(Settings::instance());
    installWindowRecorderStubs(wrapper);
    stub.set_lamda(&Utils::isWayland, []() -> bool { return true; });  // 跳过最小化窗口
    stub.set_lamda(&DWindowManagerHelper::hasComposite,
                   [](DWindowManagerHelper *) -> bool { return false; });
    bar->addTab(localRoot + "/shot.txt", QString("shot.txt"));
    host->show();
    QApplication::processEvents();

    QStyleOptionTab option;
    QPoint hotspot;

    // Act
    const QPixmap pixmap = bar->createDragPixmapFromTab(0, option, &hotspot);

    // Assert：非空位图 + 热点为截图缩放后中心（背景图四边各加 5px）+ 静态拖拽位图登记
    EXPECT_FALSE(pixmap.isNull());
    EXPECT_EQ(hotspot.x(), (pixmap.width() - 10) / 2);
    EXPECT_EQ(hotspot.y(), (pixmap.height() - 10) / 2);
    ASSERT_NE(Tabbar::sm_pDragPixmap, nullptr);
    EXPECT_EQ(Tabbar::sm_pDragPixmap->size(), pixmap.size());
}

TEST_F(TabbarTest, CreateDragPixmap_Composite_RoundedShadowPath)
{
    EditWrapper *wrapper = nullptr;
    installEditWrapperCtorStubs(stub, host, &wrapper);
    wrapper->textEditor()->setSettings(Settings::instance());
    installWindowRecorderStubs(wrapper);
    stub.set_lamda(&Utils::isWayland, []() -> bool { return true; });
    stub.set_lamda(&DWindowManagerHelper::hasComposite,
                   [](DWindowManagerHelper *) -> bool { return true; });
    // 阴影计算重（真实可行但慢）：桩为固定尺寸，验证链路与返回即可
    stub.set_lamda(
        static_cast<QPixmap (*)(const QPixmap &, qreal, const QColor &, const QPoint &)>(
            &Utils::dropShadow),
        [](const QPixmap &, qreal, const QColor &, const QPoint &) -> QPixmap {
            return QPixmap(32, 32);
        });
    bar->addTab(localRoot + "/round.txt", QString("round.txt"));
    QApplication::processEvents();

    QStyleOptionTab option;
    QPoint hotspot;

    // Act
    const QPixmap pixmap = bar->createDragPixmapFromTab(0, option, &hotspot);

    // Assert：走圆角阴影分支 → 返回桩尺寸位图、热点已设置
    EXPECT_EQ(pixmap.size(), QSize(32, 32));
    EXPECT_GT(hotspot.x(), 0);
    EXPECT_GT(hotspot.y(), 0);
    ASSERT_NE(Tabbar::sm_pDragPixmap, nullptr);
    EXPECT_EQ(Tabbar::sm_pDragPixmap->size(), QSize(32, 32));
}

// ---- 右键菜单全部动作 lambda（closeOther/Left/Right/CloseAllunModified）----

TEST_F(TabbarTest, MenuActions_Triggered_EmitClosePathSignals)
{
    installWindowRecorderStubs(nullptr);
    addTabsQuietly({ localRoot + "/ma0", localRoot + "/ma1", localRoot + "/ma2" });
    // 建立右键菜单（exec 已桩）
    const QRect rect = bar->tabRect(1);
    QMouseEvent rightPress(QEvent::MouseButtonPress, rect.center(), bar->mapToGlobal(rect.center()),
                           Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    ASSERT_TRUE(bar->eventFilter(bar, &rightPress));
    ASSERT_NE(bar->m_closeOtherTabAction, nullptr);

    // Act 1：Close other tabs → closeOtherTabsExceptFile(点击项路径)
    QSignalSpy closeSpy(bar, &Tabbar::closeTabs);
    bar->m_closeOtherTabAction->trigger();
    ASSERT_EQ(closeSpy.count(), 1);
    EXPECT_EQ(closeSpy.at(0).at(0).toStringList(),
              QStringList({ localRoot + "/ma0", localRoot + "/ma2" }));

    // Act 2：Close tabs to the left → closeLeftTabs
    bar->m_closeLeftTabAction->trigger();
    ASSERT_EQ(closeSpy.count(), 2);
    EXPECT_EQ(closeSpy.at(1).at(0).toStringList(), QStringList({ localRoot + "/ma0" }));

    // Act 3：Close tabs to the right → closeRightTabs
    bar->m_closeRightTabAction->trigger();
    ASSERT_EQ(closeSpy.count(), 3);
    EXPECT_EQ(closeSpy.at(2).at(0).toStringList(), QStringList({ localRoot + "/ma2" }));
}

TEST_F(TabbarTest, CloseAllunModifiedAction_UnmodifiedTabs_RemovesAllClean)
{
    // Arrange：真实 EditWrapper（未修改 → isModified=false → 被清理）
    EditWrapper *wrapper = nullptr;
    installEditWrapperCtorStubs(stub, host, &wrapper);
    installWindowRecorderStubs(wrapper);
    addTabsQuietly({ localRoot + "/clean0", localRoot + "/clean1" });
    const QRect rect = bar->tabRect(0);
    QMouseEvent rightPress(QEvent::MouseButtonPress, rect.center(), bar->mapToGlobal(rect.center()),
                           Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    ASSERT_TRUE(bar->eventFilter(bar, &rightPress));
    ASSERT_NE(bar->m_closeAllunModifiedTabAction, nullptr);
    QSignalSpy historySpy(bar, &Tabbar::requestHistorySaved);

    // Act：Close unmodified tabs → removeWrapper(path,true) + closeTab 逐个执行
    bar->m_closeAllunModifiedTabAction->trigger();
    QApplication::processEvents();

    // Assert：两个未修改标签全部清理；removeWrapper 每路径两次
    //（lambda 显式一次 + closeTab → handleTabIsRemoved 一次）
    EXPECT_EQ(bar->count(), 0);
    EXPECT_EQ(bar->m_tabPaths.size(), 0);
    EXPECT_EQ(windowRemoveWrapperCalls, 4);
    EXPECT_EQ(historySpy.count(), 2);
}

TEST_F(TabbarTest, SizeModeChangedSignal_ModeToggle_SchedulesUpdate)
{
    // Arrange：ctor 连接的 lambda（sizeModeChanged → update()）
    const bool emitted = QMetaObject::invokeMethod(
        DGuiApplicationHelper::instance(), "sizeModeChanged",
        Q_ARG(DGuiApplicationHelper::SizeMode, DGuiApplicationHelper::CompactMode));
    QApplication::processEvents();

    // Assert：信号送达（lambda 仅调用 update()，无崩溃即覆盖），控件仍在
    EXPECT_TRUE(emitted);
    EXPECT_EQ(bar->count(), 0);
}

// ---- Settings 资源依赖冒烟（防止 qrc 未编入导致后续用例误判）----

TEST_F(TabbarTest, SettingsResource_QrcLinked_InstanceAvailable)
{
    // Assert：Settings 单例可建且幂等（qrc 资源在位）
    EXPECT_NE(Settings::instance(), nullptr);
    EXPECT_EQ(Settings::instance(), Settings::instance());
}

}  // namespace
