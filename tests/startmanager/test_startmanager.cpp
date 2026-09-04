// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// StartManager（src/startmanager.h/.cpp）单元测试
//
// 类特征：QObject 子类（非 GUI 类），文件打开/URL 解析/进程环境判定/备份恢复逻辑。
// 外部依赖全部 stub_ext 拦截（Qt 内置类/全局函数无虚函数不可注入，按 test-types §7.5
// 选 stub_ext 而非 gMock）：
//   - DBus：QDBusConnection::systemBus/sessionBus、QDBusAbstractInterface::callWithArgumentList
//   - 单例：Settings::instance + DSettings/DSettingsOption 链、IflytekAiAssistant、DApplication::aboutDialog
//   - 路径：QStandardPaths::standardLocations → QTemporaryDir 隔离（绝不写真实用户目录）
//   - 子进程/DBus 激活：Utils::activeWindowFromDock（无真实子进程/DBus 调用）
//   - 屏幕：QGuiApplication::primaryScreen / QScreen::availableGeometry / QCursor::pos
//   - Window/EditWrapper/TextEdit/Tabbar：全量源码编入但所有被调方法 stub 拦截，
//     fake 指针使用零化内存（成员级访问仅 -fno-access-control 注入 QString 字段）
//     或 QObject 化内存（reinterpret_cast，QObject 基类单继承链偏移 0，用于
//     QTimer::singleShot context / connect sender 语境）
//   - StartManager::createWindow 真实执行用例：Window 构造函数入口 patch
//     （StubExt::get_ctor_addr + placement-new QObject 最小初始化）
//
// 方法覆盖清单（30 方法，含 3 个 private 经间接覆盖）：
// instance/~StartManager/StartManager/checkPath/ifKlu/isMultiWindow/isTemFilesEmpty/
// autoBackupFile/recoverFile/openFilesInWindow/openFilesInTab/createWindowFromWrapper/
// loadTheme/createWindow/initWindowPosition/popupExistTabs/getFileTabInfo/
// slotCheckUnsaveTab/closeAboutForWindow/slotCreatNewwindow/slotCloseWindow/
// slotDelayBackupFile/delayMallocTrim/timerEvent/analyzeBookmakeInfo/recordBookmark/
// findBookmark + private: initBlockShutdown(构造间接)/initBookmark(构造间接)/saveBookmark(直接调用)
//
// 最小清单完成情况：
// | 1 | 每个公开方法 ≥1 用例 | 完成（见用例映射） |
// | 2 | 等价类划分（环境变量组合/书签串/临时文件列表/JSON 记录形态） | 完成 |
// | 3 | 边界值（空列表/空串/INT 极值/20 窗口上限/tabIndex 越界） | 完成 |
// | 4 | TEST_P ≥3 组（ifKlu 环境组合/isTemFilesEmpty 列表形态/analyzeBookmakeInfo 串形态） | 完成 |
// | 5 | 分支清单 → 用例映射 | 见下方 |
// | 6 | if/switch/early-return 全分支 | 完成（除 3 处标注 GUI/构造约束不可达分支） |
// | 7 | 异常路径 EXPECT_THROW 精确匹配 | N/A（方法无 throw，Qt 风格 bool/错误码） |
// | 8 | 负面场景（空输入/无效 JSON/文件缺失/窗口上限） | 完成 |
// | 9 | 强异常安全（状态前后对比） | 完成（多处 count 前后对比） |
// | 10 | stub_ext（Qt 类/全局函数），无项目内可注入虚接口 | 完成 |
//
// 分支清单（来源：src/startmanager.cpp 各方法）：
// instance: B1 m_instance==nullptr→new
// ~StartManager: B2 m_instance==this→置空
// ctor: B3 blank 目录不存在→mkpath；B4 backup 目录不存在→mkpath（QFileInfo::exists stub 控制）
// checkPath: B5 wrapper 非空→popup+false；B6 全空→true
// ifKlu: B7 XDG_SESSION_TYPE==wayland→true；B8 WAYLAND_DISPLAY 含 wayland→true；B9 否则 false
// isMultiWindow: B10 count>1→true；B11 否则 false
// isTemFilesEmpty: B12 存在空串项→true；B13 否则 false
// autoBackupFile: B14 拖拽→早退；B15 autoBackupDir 不存在→mkpath；B16 存在且 backup 非空→清空；
//   B17 getFileLoading→continue；B18 tabIndex<0→continue；B19 书签非空→insert；B20 空→remove；
//   B21 活动窗口且 currentWrapper→focus；B22 草稿→saveTemFile(filePath)；B23 modified→saveTemFile(autoBackup 名)；
//   B24 tabIndex<size→replace；B25 越界→append；B26 pending 标签→从配置恢复
// recoverFile: B27 非 blank 文件→移除；B28 非法 JSON→丢弃；B29 focus 记录→focusIndex；
//   B30 无 focus→首标签；B31 localPath 缺失→continue；B32 temFile 存在→openPath=tem；
//   B33 localPath 存在（草稿/MIME 判定 displayName）；B34 都缺→continue；B35 window 索引变化→createWindow；
//   B36 focus→立即加载+书签记录/全局书签；B37 非 focus→addPendingTab；B38 pFocusWindow→popup；
//   B39 无 focus 但有恢复→activeTab(0)
// openFilesInWindow: B40 空且 ≥20 窗→拒；B41 空且已有窗→showCenterWindow(false)；B42 空/首窗→center(true)；
//   B43 已打开→popup；B44 新文件→createWindow+addTab
// openFilesInTab: B45 空+无窗+有临时文件→recover；B46 recover 0→addBlankTab；B47 blank 文件存在→删除；
//   B48 空+已有窗→新窗+show；B49 已打开→popup；B50 首文件无窗→singleShot 延迟打开；
//   B51 已有窗→首窗 addTab+dock 激活（dock 失败→activateWindow）
// createWindowFromWrapper: B52 x 超界→截断；B53 x<0→0；B54 y 超界→截断；B55 y<0→0；
//   B56 拖拽 pixmap 空→回退窗口尺寸；B57 动画完成 buffer 悬空→放弃；B58 存活→addTabWithWrapper
// loadTheme: B59 每窗调用
// createWindow: B60 真实创建（构造 patch 语境）+ connect×5 + m_windows 追加
// initWindowPosition: B61 无窗/alwaysCenter→居中（不 move）；B62 否则→偏移 move
// popupExistTabs: B63 dock 激活失败→activateWindow；B64 成功→跳过
// getFileTabInfo: B65 找到→索引；B66 未找到→-1
// slotCheckUnsaveTab: B67 有未保存→Inhibit+早退；B68 无→无操作（m_reply 无效）
// closeAboutForWindow: B69 qApp/aboutDialog null→跳；B70 parent null→跳；B71 parent!=window→跳；B72 ==window→close
// slotCloseWindow: B73 sender 在列表→移除；B74 窗口清空→saveBookmark+清理 tabPaths+注销 DBus+延迟 quit；
//   B75 currentPath 不存在→早退
// slotDelayBackupFile: B76 定时器未激活且非拖拽→start；B77 已激活→跳
// delayMallocTrim: B78 未激活→start；B79 已激活→跳
// timerEvent: B80 DelayTimer→backup+重启周期定时器；B81 FreeMemTimer→malloc_trim；B82 其他→忽略
// analyzeBookmakeInfo: B83 逗号切分循环（空串/单元素/多元素）
// recordBookmark/findBookmark: B84 insert/覆盖；B85 value 缺省
// initBookmark: B86 非法 JSON→跳；B87 文件缺失→跳；B88 书签空→跳；B89 有效→缓存
// saveBookmark: B90 文件缺失/空书签→erase；B91 有效→序列化写配置
//
// 用例映射（TestName → 分支）：
// - Instance_FirstCall_CreatesAndReusesSingleton → B1
// - Destructor_ForeignInstance_KeepsStaticPointer / Instance_Deleted_ResetsStaticPointer → B2 反/正
// - Constructor_StandardLocations_SetsUpBackupDirs → B3/B4(false 侧)
// - Constructor_DirMissing_CreatesDirectories → B3/B4(true 侧)
// - Constructor_TemFileConfig_LoadsHistoryList / Constructor_AutoBackupTimer_Started → ctor
// - CheckPath_FileNotOpen_ReturnsTrue → B6
// - CheckPath_FileAlreadyOpen_ReturnsFalseAndActivatesTab → B5
// - IfKlu_EnvCombinations_ReturnsExpected( TEST_P 3 组) → B7/B8/B9
// - IsMultiWindow_SingleWindow_ReturnsFalse → B11；...MultipleWindows_ReturnsTrue → B10
// - IsTemFilesEmpty_ListVariants_ReturnsExpected( TEST_P 4 组) → B12/B13
// - AutoBackupFile_TagDragging_SkipsBackup → B14
// - AutoBackupFile_BackupDirMissing_CreatesDirAndKeepsUserBackup → B15
// - AutoBackupFile_UserBackupExist_RemovesUserBackup → B16
// - AutoBackupFile_FileLoading_SkipsWrapper → B17
// - AutoBackupFile_TabIndexInvalid_SkipsRecord → B18
// - AutoBackupFile_NormalWrapper_WritesBackupJson → B24（含 cursorPosition/localPath 字段断言）
// - AutoBackupFile_TabIndexOutOfRange_AppendsRecord → B25
// - AutoBackupFile_HasBookmarks_RecordsAndWrites → B19
// - AutoBackupFile_NoBookmarks_RemovesRecord → B20
// - AutoBackupFile_ActiveCurrentWrapper_MarksFocus → B21
// - AutoBackupFile_DraftFile_SavesAsTemFile → B22
// - AutoBackupFile_ModifiedFile_SavesToAutoBackupDir → B23
// - AutoBackupFile_PendingTab_RestoresFromConfig → B26
// - RecoverFile_EmptyRecords_ReturnsZero → B27/B28 语境
// - RecoverFile_LocalFileMissing_SkipsRecord → B31
// - RecoverFile_TemFileExists_OpensTemFile → B32
// - RecoverFile_FocusTab_LoadsImmediatelyWithBookmarks → B29/B36
// - RecoverFile_BookMarkFromGlobal_AppliesConfigBookmark → B36(全局书签侧)
// - RecoverFile_NonFocusTab_AddedAsPending → B37
// - RecoverFile_WindowIndexChanges_CreatesNewWindow → B35
// - RecoverFile_NoFocusRecord_ActivatesFirstTab → B30/B39
// - RecoverFile_DraftDisplayName_UsesUntitled → B33
// - OpenFilesInWindow_MaxWindowsReached_RejectsCreation → B40
// - OpenFilesInWindow_EmptyWithExistingWindows_NotCentered → B41
// - OpenFilesInWindow_EmptyFirstWindow_Centered → B42
// - OpenFilesInWindow_AlreadyOpenedFile_ActivatesExistingTab → B43
// - OpenFilesInWindow_NewFile_CreatesWindowAndTab → B44
// - OpenFilesInTab_EmptyNoWindowsNoTemFiles_AddsBlankTab → B45(false)/B47(false)
// - OpenFilesInTab_EmptyNoWindowsTemFilesRecovers_Recovers → B45(true)
// - OpenFilesInTab_RecoveryFoundNothing_AddsBlankTab → B46
// - OpenFilesInTab_BlankFilesExist_RemovesThenAddsBlankTab → B47(true)
// - OpenFilesInTab_WindowsExist_ShowsNewWindowWithBlankTab → B48
// - OpenFilesInTab_FileAlreadyOpen_SkipsToNextFile → B49(经 checkPath false)
// - OpenFilesInTab_FirstFileNoWindow_DelayedOpen → B50
// - OpenFilesInTab_ExistingWindow_AddsTabAndActivates → B51(dock 失败侧)
// - CreateWindowFromWrapper_WithinScreen_PlaysDropAnimation → B52-B58 正常侧
// - CreateWindowFromWrapper_BufferDestroyedDuringAnimation_DropsTearOff → B57
// - CreateWindowFromWrapper_CursorBeyondScreen_ClampsPosition → B52/B54
// - CreateWindowFromWrapper_NegativeCursor_ClampsToZero → B53/B55
// - LoadTheme_NoWindows_NoWindowTouched / LoadTheme_EachWindow_AppliesTheme → B59
// - CreateWindow_FirstWindow_AppendedAndCentered → B60/B61
// - CreateWindow_SubsequentWindow_MovedByOffset → B62
// - InitWindowPosition_AlwaysCenter_SkipsOffsetMove / InitWindowPosition_NoWindows_SkipsOffsetMove → B61
// - InitWindowPosition_WithExistingWindows_MovesByOffset → B62
// - PopupExistTabs_DockActivationFails_ActivatesManually → B63
// - PopupExistTabs_DockActivationSucceeds_SkipsManualActivation → B64
// - GetFileTabInfo_NotOpen_ReturnsInvalidIndices → B66
// - GetFileTabInfo_OpenInSecondWindow_ReturnsIndices → B65
// - SlotCheckUnsaveTab_UnsavedTabExists_BlocksShutdown → B67
// - SlotCheckUnsaveTab_AllTabsSaved_NoDbusCall → B68
// - CloseAboutForWindow_DialogNull_NoClose / ...ParentNull_NoClose / ...OtherParent_NoClose /
//   ...MatchingParent_ClosesDialog → B69-B72
// - SlotCreatNewwindow_DelegatesToOpenFilesInWindow → slotCreatNewwindow
// - SlotCloseWindow_SenderInList_RemovedFromList → B73
// - SlotCloseWindow_LastWindow_CleansUpAndSchedulesQuit → B74
// - SlotCloseWindow_WorkingDirMissing_EarlyReturn → B75
// - SlotDelayBackupFile_IdleTimer_StartsDelayTimer → B76
// - SlotDelayBackupFile_TimerActive_SkipsRestart / ...TagDragging_SkipsStart → B77
// - DelayMallocTrim_Idle_StartsFreeTimer / ...Active_SkipsRestart → B78/B79
// - TimerEvent_DelayTimerExpired_BackupsAndRestartsCycle → B80
// - TimerEvent_FreeMemTimerExpired_TrimsAndStops → B81
// - TimerEvent_UnknownTimer_Ignored → B82
// - AnalyzeBookmakeInfo_StringVariants_ReturnsExpectedList( TEST_P 5 组) → B83
// - RecordBookmark_NewPath_StoresList / ...ExistingPath_OverwritesList / FindBookmark_UnknownPath_ReturnsEmpty → B84/B85
// - InitBookmark_Variants_LoadsOnlyExistingValid → B86-B89（经构造，派生 fixture）
// - SaveBookmark_FileMissingOrEmpty_DropsRecord → B90
// - SaveBookmark_ValidRecord_SerializedToConfig → B91
//
// 不可达分支说明（GUI/DBus 运行时约束）：
// - popupExistTabs 内 #if 0 代码块不参与编译，无分支
// - slotCheckUnsaveTab "m_reply valid→清空"分支（B68 true 侧）：需构造有效
//   QDBusReply<QDBusUnixFileDescriptor>（要求真实 DBus 应答），单测环境不可构造，记录为限制
// - createWindowFromWrapper 拖拽 pixmap 非空分支（B56 true 侧）：QPixmap::rect 虚调用
//   需真实 GUI 像素数据，QCoreApplication 语境不可安全构造，记录为限制

#include <gtest/gtest.h>
#include "stubext.h"

#include <QCoreApplication>
#include <QTemporaryDir>
#include <QTimer>
#include <QEventLoop>
#include <QElapsedTimer>
#include <QThread>
#include <QVariant>
#include <QStringList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusAbstractInterface>
#include <QStandardPaths>
#include <QScreen>
#include <QCursor>
#include <QTimerEvent>
#include <malloc.h>
#include <new>
#include <dlfcn.h>

#include <DApplication>
#include <DAboutDialog>
#include <DSettings>
#include <DSettingsOption>

#include "startmanager.h"
#include "common/settings.h"
#include "common/iflytek_ai_assistant.h"
#include "common/performancemonitor.h"
#include "widgets/window.h"
#include "controls/tabbar.h"
#include "editor/editwrapper.h"
#include "editor/dtextedit.h"
#include "common/utils.h"

// ---------------- 公共 helper ----------------

// 在事件循环中等待 ms 毫秒（不依赖 QtTest，避免引入 Qt6::Test）
static void processEventsFor(int ms)
{
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < ms) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        QThread::msleep(5);
    }
}

// Window 构造函数桩：仅 placement-new QObject 基类子对象（单继承链偏移 0），
// 使 createWindow 真实路径中的 connect/sender 语境合法；不初始化任何 GUI 成员
static void fakeWindowCtor(Window *self)
{
    new (self) QObject(nullptr);
}

// 精确定位 Window 默认构造符号（C1 优先，C2 兜底）。
// 注：不用 StubExt::get_ctor_addr——其 CALL 扫描在 gcov 插桩（-fprofile-arcs）
// 下会抓到计数函数地址而非构造函数，patch 会破坏无辜函数。
static void *locateWindowCtor()
{
    // Window::Window(DMainWindow *parent = nullptr) 的完整构造符号（C1/C2）
    void *addr = dlsym(RTLD_DEFAULT, "_ZN6WindowC1EPN3Dtk6Widget11DMainWindowE");
    if (addr == nullptr)
        addr = dlsym(RTLD_DEFAULT, "_ZN6WindowC2EPN3Dtk6Widget11DMainWindowE");
    return addr;
}

class StartManagerTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // StartManager 为 QObject 子类（非 GUI），按规范使用 QCoreApplication
        if (QCoreApplication::instance() == nullptr) {
            static int argc = 1;
            static char appName[] = "test_startmanager";
            static char *argv[] = { appName, nullptr };
            s_app = new QCoreApplication(argc, argv);
        }
    }

    static void TearDownTestSuite()
    {
        // 保留 QCoreApplication 至进程退出（后续 suite 复用）
    }

    // 派生 fixture 钩子：在 new StartManager 之前注入配置
    virtual void customizeConfig() {}

    void SetUp() override
    {
        tmp = std::make_unique<QTemporaryDir>();
        ASSERT_TRUE(tmp->isValid()) << "QTemporaryDir 创建失败";

        // ---- fake 对象（零化内存 / QObject 化内存） ----
        fakeSettings = zeroFake<Settings>();
        fakeOptionObj = new QObject;
        ownedObjs.push_back(fakeOptionObj);
        fakeOption = reinterpret_cast<DSettingsOption *>(fakeOptionObj);
        fakeIflytek = zeroFake<IflytekAiAssistant>();

        installCommonStubs();

        customizeConfig();

        // 全 stub 环境下构造被测对象
        obj = new StartManager();
        ASSERT_NE(obj, nullptr);

        // 构造期间 initBlockShutdown 触发一次 DBus 调用（stub 计数），记录基线
        dbusCallBaseline = dbusCalls;
        setValueBaseline = setValueCalls;
    }

    void TearDown() override
    {
        if (obj != nullptr) {
            delete obj->m_pTimer;   // 构造中 new 的周期定时器（无 parent），手动回收
            delete obj;
            obj = nullptr;
        }
        StartManager::m_instance = nullptr;   // 兜底清理单例静态指针
        stub.clear();
        for (void *p : zeroBlocks)
            free(p);
        zeroBlocks.clear();
        for (QObject *o : ownedObjs)
            delete o;
        ownedObjs.clear();
        tmp.reset();
        // 环境变量还原（与 IfKlu 用例的 qputenv 配对，避免用例间泄漏）
        qunsetenv("XDG_SESSION_TYPE");
        qunsetenv("WAYLAND_DISPLAY");
    }

    // ---------------- stub 安装 ----------------
    void installCommonStubs()
    {
        // 路径隔离：AppDataLocation → QTemporaryDir（绝不触碰真实用户目录）
        stub.set_lamda(&QStandardPaths::standardLocations,
                       [this](QStandardPaths::StandardLocation) -> QStringList {
                           return QStringList { tmp->path() };
                       });

        // Settings 单例与 DSettings 链
        stub.set_lamda(&Settings::instance, [this]() -> Settings * {
            return fakeSettings;
        });
        stub.set_lamda(&DSettings::option,
                       [this](DSettings *, const QString &key) -> QPointer<DSettingsOption> {
                           lastOptKey = key;
                           return QPointer<DSettingsOption>(fakeOption);
                       });
        stub.set_lamda(
            static_cast<QVariant (DSettings::*)(const QString &) const>(&DSettings::value),
            [this](DSettings *, const QString &) -> QVariant {
                return settingsValue;
            });
        stub.set_lamda(static_cast<QVariant (DSettingsOption::*)() const>(&DSettingsOption::value),
                       [this](DSettingsOption *) -> QVariant {
                           return optionValue;
                       });
        stub.set_lamda(static_cast<void (DSettingsOption::*)(QVariant)>(&DSettingsOption::setValue),
                       [this](DSettingsOption *, QVariant v) {
                           lastSetValue = v;
                           ++setValueCalls;
                           setValues.append(qMakePair(lastOptKey, v));
                       });

        // IflytekAiAssistant 单例
        stub.set_lamda(&IflytekAiAssistant::instance,
                       [this](void) -> IflytekAiAssistant * {
                           return fakeIflytek;
                       });
        stub.set_lamda(&IflytekAiAssistant::checkAiExists,
                       [](IflytekAiAssistant *) -> void {});

        // DBus 隔离（禁止真实连接/调用；未知连接名 → disconnected 连接）
        stub.set_lamda(&QDBusConnection::systemBus,
                       []() -> QDBusConnection { return QDBusConnection(QStringLiteral("ut_no_bus")); });
        stub.set_lamda(&QDBusConnection::sessionBus,
                       []() -> QDBusConnection { return QDBusConnection(QStringLiteral("ut_no_bus")); });
        stub.set_lamda(
            static_cast<bool (QDBusConnection::*)(const QString &)>(&QDBusConnection::unregisterService),
            [this](QDBusConnection *, const QString &svc) -> bool {
                ++unregisterCalls;
                lastUnregisterService = svc;
                return true;
            });
        stub.set_lamda(
            static_cast<QDBusMessage (QDBusAbstractInterface::*)(QDBus::CallMode, const QString &, const QList<QVariant> &)>(
                &QDBusAbstractInterface::callWithArgumentList),
            [this](QDBusAbstractInterface *, QDBus::CallMode, const QString &method,
                   const QList<QVariant> &) -> QDBusMessage {
                ++dbusCalls;
                lastDbusMethod = method;
                return QDBusMessage();   // 无错误但无参数 → reply invalid
            });

        // 文件存在性判定（可变控制）：成员版（recoverFile 用）+ 静态版（saveBookmark/initBookmark 用）
        stub.set_lamda(static_cast<bool (QFileInfo::*)() const>(&QFileInfo::exists),
                       [this](QFileInfo *) -> bool {
                           return fileInfoExists;
                       });
        stub.set_lamda(static_cast<bool (*)(const QString &)>(&QFileInfo::exists),
                       [this](const QString &) -> bool {
                           return fileInfoExists;
                       });
        // 工作目录（slotCloseWindow）→ 临时目录
        stub.set_lamda(&QDir::currentPath,
                       [this]() -> QString {
                           return curPathOverride.isEmpty() ? tmp->path() : curPathOverride;
                       });

        // Utils 静态工具（纯行为控制，避免真实文件/MIME/DBus 查询）
        stub.set_lamda(&Utils::cleanPath,
                       [](const QStringList &in) -> QStringList { return in; });
        stub.set_lamda(&Utils::getFilePath,
                       [](const QString &in) -> QString { return in; });
        stub.set_lamda(&Utils::isDraftFile,
                       [this](const QString &) -> bool { return isDraft; });
        stub.set_lamda(&Utils::fileExists,
                       [this](const QString &) -> bool { return fileExistsFlag; });
        stub.set_lamda(&Utils::isMimeTypeSupport,
                       [this](const QString &) -> bool { return mimeSupport; });
        stub.set_lamda(&Utils::getStringMD5Hash,
                       [](const QString &) -> QString { return QStringLiteral("md5hash"); });
        stub.set_lamda(&Utils::activeWindowFromDock,
                       [this](quintptr) -> bool { return dockActivate; });

        stub.set_lamda(&PerformanceMonitor::closeAPPFinish, []() -> void {});

        // QApplication::quit 拦截（slotCloseWindow 延迟退出路径，防真实退出测试进程）
        stub.set_lamda(&QCoreApplication::quit, [this]() -> void { ++quitCalls; });
    }

    // 窗口交互基础 stub（popupExistTabs / 各 open* 方法共用）
    void stubWindowInteraction(Window *win)
    {
        stub.set_lamda(static_cast<WId (QWidget::*)() const>(&QWidget::winId),
                       [](QWidget *) -> WId { return 4242; });
        stub.set_lamda(static_cast<void (QWidget::*)()>(&QWidget::activateWindow),
                       [this](QWidget *) -> void { ++activateWindowCalls; });
        stub.set_lamda(static_cast<void (Window::*)(int)>(&Window::activeTab),
                       [this](Window *, int idx) { ++activeTabCalls; lastActiveTab = idx; });
        stub.set_lamda(static_cast<int (Window::*)(const QString &)>(&Window::getTabIndex),
                       [this, win](Window *self, const QString &) -> int {
                           return self == win ? stubTabIndex : -1;
                       });
        stub.set_lamda(static_cast<void (Window::*)(bool)>(&Window::showCenterWindow),
                       [this](Window *, bool center) { ++showCenterCalls; lastCenterFlag = center; });
    }

    // ---------------- fake 内存管理 ----------------
    template <typename T>
    T *zeroFake()
    {
        void *p = calloc(sizeof(T) + 256, 1);
        zeroBlocks.push_back(p);
        return reinterpret_cast<T *>(p);
    }

    template <typename T>
    T *qobjFake()
    {
        QObject *o = new QObject;
        ownedObjs.push_back(o);
        return reinterpret_cast<T *>(o);
    }

    // ---------------- 状态 ----------------
    static QCoreApplication *s_app;

    stub_ext::StubExt stub;
    StartManager *obj = nullptr;
    std::unique_ptr<QTemporaryDir> tmp;

    std::vector<void *> zeroBlocks;
    std::vector<QObject *> ownedObjs;

    Settings *fakeSettings = nullptr;
    QObject *fakeOptionObj = nullptr;
    DSettingsOption *fakeOption = nullptr;
    IflytekAiAssistant *fakeIflytek = nullptr;

    // ---- 可控桩返回值 ----
    QVariant optionValue;                 // DSettingsOption::value
    QVariant settingsValue;               // DSettings::value（initBookmark）
    QVariant lastSetValue;                // DSettingsOption::setValue 收到的值
    QString lastOptKey;
    bool fileInfoExists = true;
    QString curPathOverride;
    bool isDraft = false;
    bool fileExistsFlag = true;
    bool mimeSupport = true;
    bool dockActivate = false;

    // ---- 调用计数/记录 ----
    int setValueCalls = 0;
    int setValueBaseline = 0;
    QVector<QPair<QString, QVariant>> setValues;   // 全部 setValue（按 option key 区分）
    QString lastDbusMethod;
    int dbusCalls = 0;
    int dbusCallBaseline = 0;
    int unregisterCalls = 0;
    QString lastUnregisterService;
    int quitCalls = 0;
    int activateWindowCalls = 0;
    int activeTabCalls = 0;
    int lastActiveTab = -1;
    int showCenterCalls = 0;
    bool lastCenterFlag = false;
    int stubTabIndex = 0;

    // ---- autoBackupFile 用例公共状态 ----
    Window *backupWin = nullptr;
    Tabbar *fakeBackupTabbar = nullptr;
    EditWrapper *fakeBackupWrapper = nullptr;
    TextEdit *fakeBackupTextEdit = nullptr;
    QString backupFilePath;
    QString backupTruePath;
    QStringList tabFiles;
    int tabCount = 1;
    int backupTabIndex = 0;
    bool fileLoading = false;
    QList<int> wrapperBookmarks;
    bool windowActive = false;
    bool wrapperModified = false;
    int saveTemFileCalls = 0;
    QString lastSaveTemPath;

    // ---- createWindowFromWrapper 用例公共状态 ----
    Window *dropWin = nullptr;
    EditWrapper *dropWrapper = nullptr;
    QScreen *dropScreen = nullptr;
    QPoint dropCursorPos;
    int dropMoveCalls = 0;
    QPoint lastDropPos;
    int dropShowCalls = 0;
    int dropCenterCalls = 0;
    int dropAddTabCalls = 0;
    QString lastDropFilePath;
    bool lastDropModified = false;

    // autoBackupFile 的备份写入（排除 saveBookmark 对 lastSetValue 的覆盖干扰）
    QStringList backupWrites() const
    {
        QStringList ret;
        for (const auto &kv : setValues) {
            if (kv.first == QLatin1String("advance.editor.browsing_history_temfile"))
                ret = kv.second.toStringList();   // 取最后一次备份写入
        }
        return ret;
    }

    int backupWriteCount() const
    {
        int n = 0;
        for (const auto &kv : setValues) {
            if (kv.first == QLatin1String("advance.editor.browsing_history_temfile"))
                ++n;
        }
        return n;
    }

    void installAutoBackupStubs();
    void installDropStubs(QPoint cursorPos);
};

QCoreApplication *StartManagerTest::s_app = nullptr;

// ============================================================
// 构造 / 单例 / 析构
// ============================================================

TEST_F(StartManagerTest, Instance_FirstCall_CreatesAndReusesSingleton)
{
    // Arrange
    StartManager::m_instance = nullptr;

    // Act
    StartManager *p1 = StartManager::instance();
    StartManager *p2 = StartManager::instance();

    // Assert：首建非空且复用同一实例
    ASSERT_NE(p1, nullptr);
    EXPECT_EQ(p1, p2);

    // Cleanup：析构置空静态指针（覆盖 ~StartManager B2 正侧）
    delete p1->m_pTimer;
    delete p1;
    EXPECT_EQ(StartManager::m_instance, nullptr);
}

TEST_F(StartManagerTest, Destructor_ForeignInstance_KeepsStaticPointer)
{
    // Arrange：静态指针指向其他实例
    StartManager *foreign = reinterpret_cast<StartManager *>(0x1234);
    StartManager::m_instance = foreign;
    StartManager *victim = new StartManager();

    // Act
    delete victim->m_pTimer;
    delete victim;

    // Assert：析构不误清他者指针（B2 反侧，静态指针保持非空且指向原值）
    EXPECT_EQ(StartManager::m_instance, foreign);
    EXPECT_NE(StartManager::m_instance, nullptr);
    StartManager::m_instance = nullptr;
}

TEST_F(StartManagerTest, Constructor_StandardLocations_SetsUpBackupDirs)
{
    // Arrange（SetUp 已构造，fileInfoExists=true 跳过 mkpath）

    // Assert：三个目录均位于隔离的 AppDataLocation 之下
    EXPECT_EQ(obj->m_blankFileDir, QDir(tmp->path()).filePath("blank-files"));
    EXPECT_EQ(obj->m_backupDir, QDir(tmp->path()).filePath("backup-files"));
    EXPECT_EQ(obj->m_autoBackupDir, QDir(tmp->path()).filePath("autoBackup-files"));
}

TEST_F(StartManagerTest, Constructor_DirMissing_CreatesDirectories)
{
    // Arrange：目标目录不存在（fileInfoExists=false → mkpath 分支）；重建被测对象
    fileInfoExists = false;
    delete obj->m_pTimer;
    delete obj;
    obj = new StartManager();

    // Assert：blank/backup 目录真实创建于临时目录（mkpath 真实执行，路径隔离）
    EXPECT_TRUE(QDir(obj->m_blankFileDir).exists());
    EXPECT_TRUE(QDir(obj->m_backupDir).exists());
    EXPECT_EQ(obj->m_blankFileDir, QDir(tmp->path()).filePath("blank-files"));
}

TEST_F(StartManagerTest, Constructor_TemFileConfig_LoadsHistoryList)
{
    // Arrange：构造前 optionValue 注入历史列表（派生钩子未用时为空，这里重建）
    optionValue = QVariant(QStringList { "entry-a", "entry-b" });
    delete obj->m_pTimer;
    delete obj;

    // Act
    obj = new StartManager();

    // Assert：m_qlistTemFile 来自配置
    EXPECT_EQ(obj->m_qlistTemFile.count(), 2);
    EXPECT_EQ(obj->m_qlistTemFile.at(0), QString("entry-a"));
}

TEST_F(StartManagerTest, Constructor_AutoBackupTimer_Started)
{
    // Assert：周期备份定时器启动且周期为 5 分钟
    ASSERT_NE(obj->m_pTimer, nullptr);
    EXPECT_TRUE(obj->m_pTimer->isActive());
    EXPECT_EQ(obj->m_pTimer->interval(), 5 * 60 * 1000);
}

// ============================================================
// checkPath
// ============================================================

TEST_F(StartManagerTest, CheckPath_FileNotOpen_ReturnsTrue)
{
    // Arrange：一个窗口但未打开该文件
    Window *win = qobjFake<Window>();
    obj->m_windows << win;
    stub.set_lamda(static_cast<EditWrapper *(Window::*)(const QString &)>(&Window::wrapper),
                   [](Window *, const QString &) -> EditWrapper * { return nullptr; });

    // Act
    bool ret = obj->checkPath(tmp->filePath("not_open.cpp"));

    // Assert：可新开 + 窗口列表状态未变（强异常安全）
    EXPECT_TRUE(ret);
    EXPECT_EQ(obj->m_windows.count(), 1);
}

TEST_F(StartManagerTest, CheckPath_FileAlreadyOpen_ReturnsFalseAndActivatesTab)
{
    // Arrange：窗口已打开该文件（wrapper 非空）
    Window *win = qobjFake<Window>();
    obj->m_windows << win;
    QString opened = tmp->filePath("opened.cpp");
    EditWrapper *fakeWrapper = zeroFake<EditWrapper>();
    TextEdit *fakeTextEdit = zeroFake<TextEdit>();
    fakeTextEdit->m_sFilePath = opened;   // inline getter 直接读成员（零化内存安全注入）

    stub.set_lamda(static_cast<EditWrapper *(Window::*)(const QString &)>(&Window::wrapper),
                   [opened, fakeWrapper](Window *, const QString &f) -> EditWrapper * {
                       return f == opened ? fakeWrapper : nullptr;
                   });
    stub.set_lamda(static_cast<TextEdit *(EditWrapper::*)()>(&EditWrapper::textEditor),
                   [fakeTextEdit](EditWrapper *) -> TextEdit * { return fakeTextEdit; });
    stubWindowInteraction(win);

    // Act
    bool ret = obj->checkPath(opened);

    // Assert：返回 false（已打开）且激活了对应标签
    EXPECT_FALSE(ret);
    EXPECT_EQ(activeTabCalls, 1);
}

// ============================================================
// ifKlu（环境变量组合 → TEST_P）
// ============================================================

struct IfKluCase {
    QByteArray xdgSessionType;   // 空 = 未设置
    QByteArray waylandDisplay;
    bool expected;
};

class IfKluEnvTest : public StartManagerTest,
                     public ::testing::WithParamInterface<IfKluCase> {
};

TEST_P(IfKluEnvTest, IfKlu_EnvCombinations_ReturnsExpected)
{
    const IfKluCase &c = GetParam();

    // Arrange：注入环境变量（空串等效"未设置"，TearDown 统一还原）
    qputenv("XDG_SESSION_TYPE", c.xdgSessionType);
    qputenv("WAYLAND_DISPLAY", c.waylandDisplay);

    // Act
    bool ret = obj->ifKlu();

    // Assert：期望边 + 无状态污染
    EXPECT_EQ(ret, c.expected);
    EXPECT_TRUE(obj->m_windows.isEmpty());
}

INSTANTIATE_TEST_SUITE_P(
    IfKluCases,
    IfKluEnvTest,
    ::testing::Values(
        IfKluCase{ "wayland", "", true },              // B7：XDG_SESSION_TYPE 命中
        IfKluCase{ "", "wayland-0", true },            // B8：WAYLAND_DISPLAY 含 wayland（大小写不敏感）
        IfKluCase{ "", "WAYLAND-X", true },            // B8：大小写不敏感侧
        IfKluCase{ "x11", "x0", false },               // B9：均未命中
        IfKluCase{ "", "", false }));                  // B9：均未设置

// ============================================================
// isMultiWindow / isTemFilesEmpty
// ============================================================

TEST_F(StartManagerTest, IsMultiWindow_SingleWindow_ReturnsFalse)
{
    obj->m_windows << qobjFake<Window>();
    EXPECT_FALSE(obj->isMultiWindow());
    EXPECT_EQ(obj->m_windows.count(), 1);
}

TEST_F(StartManagerTest, IsMultiWindow_MultipleWindows_ReturnsTrue)
{
    obj->m_windows << qobjFake<Window>() << qobjFake<Window>();
    EXPECT_TRUE(obj->isMultiWindow());
    EXPECT_EQ(obj->m_windows.count(), 2);
}

struct TemFilesCase {
    QStringList files;
    bool expected;
};

class IsTemFilesEmptyTest : public StartManagerTest,
                            public ::testing::WithParamInterface<TemFilesCase> {
};

TEST_P(IsTemFilesEmptyTest, IsTemFilesEmpty_ListVariants_ReturnsExpected)
{
    const TemFilesCase &c = GetParam();

    // Arrange
    obj->m_qlistTemFile = c.files;

    // Act
    bool ret = obj->isTemFilesEmpty();

    // Assert：期望边 + 列表不被修改（强异常安全）
    EXPECT_EQ(ret, c.expected);
    EXPECT_EQ(obj->m_qlistTemFile, c.files);
}

INSTANTIATE_TEST_SUITE_P(
    TemFilesCases,
    IsTemFilesEmptyTest,
    ::testing::Values(
        TemFilesCase{ {}, false },                       // 空列表：不进循环
        TemFilesCase{ { "a.cpp" }, false },              // 全非空
        TemFilesCase{ { "" }, true },                    // 单空项
        TemFilesCase{ { "a.cpp", "", "b.cpp" }, true })); // 混合含空项

// ============================================================
// analyzeBookmakeInfo（书签串解析 → TEST_P）
// ============================================================

struct BookmarkParseCase {
    QString input;
    QList<int> expected;
};

class AnalyzeBookmarkTest : public StartManagerTest,
                            public ::testing::WithParamInterface<BookmarkParseCase> {
};

TEST_P(AnalyzeBookmarkTest, AnalyzeBookmakeInfo_StringVariants_ReturnsExpectedList)
{
    const BookmarkParseCase &c = GetParam();

    // Act
    QList<int> ret = obj->analyzeBookmakeInfo(c.input);

    // Assert：逐元素精确比对 + 元素个数
    EXPECT_EQ(ret.count(), c.expected.count());
    for (int i = 0; i < qMin(ret.count(), c.expected.count()); ++i)
        EXPECT_EQ(ret.at(i), c.expected.at(i)) << "index=" << i;
}

INSTANTIATE_TEST_SUITE_P(
    BookmarkCases,
    AnalyzeBookmarkTest,
    ::testing::Values(
        BookmarkParseCase{ "5", { 5 } },                  // 单元素（无逗号）
        BookmarkParseCase{ "1,2,3", { 1, 2, 3 } },        // 多元素
        BookmarkParseCase{ "0,0", { 0, 0 } },             // 零值边界
        BookmarkParseCase{ "", { 0 } },                   // 空串 → 单个 0（toInt 失败默认值）
        BookmarkParseCase{ "10,20,30,40", { 10, 20, 30, 40 } })); // 长列表

// ============================================================
// recordBookmark / findBookmark
// ============================================================

TEST_F(StartManagerTest, RecordBookmark_NewPath_StoresList)
{
    // Arrange
    QString path = tmp->filePath("marked.cpp");

    // Act
    obj->recordBookmark(path, QList<int> { 3, 9 });

    // Assert：写入后可查回且内容精确
    QList<int> found = obj->findBookmark(path);
    EXPECT_EQ(found.count(), 2);
    EXPECT_EQ(found, QList<int>({ 3, 9 }));
}

TEST_F(StartManagerTest, RecordBookmark_ExistingPath_OverwritesList)
{
    // Arrange
    QString path = tmp->filePath("twice.cpp");
    obj->recordBookmark(path, QList<int> { 1 });

    // Act：同一路径二次记录
    obj->recordBookmark(path, QList<int> { 4, 5 });

    // Assert：覆盖而非追加
    EXPECT_EQ(obj->findBookmark(path).count(), 2);
    EXPECT_EQ(obj->findBookmark(path), QList<int>({ 4, 5 }));
}

TEST_F(StartManagerTest, FindBookmark_UnknownPath_ReturnsEmpty)
{
    // Act
    QList<int> ret = obj->findBookmark(tmp->filePath("unknown.cpp"));

    // Assert：缺省值为空列表（QHash::value 缺省语义）
    EXPECT_TRUE(ret.isEmpty());
    EXPECT_EQ(ret.count(), 0);
}

// ============================================================
// loadTheme
// ============================================================

TEST_F(StartManagerTest, LoadTheme_NoWindows_NoWindowTouched)
{
    QStringList themes;
    stub.set_lamda(static_cast<void (Window::*)(const QString &)>(&Window::loadTheme),
                   [&themes](Window *, const QString &t) { themes << t; });

    // Act
    obj->loadTheme("dark");

    // Assert：无窗口时无调用，且无崩溃
    EXPECT_EQ(themes.count(), 0);
    EXPECT_TRUE(obj->m_windows.isEmpty());
}

TEST_F(StartManagerTest, LoadTheme_EachWindow_AppliesTheme)
{
    // Arrange：两个窗口均记录主题
    obj->m_windows << qobjFake<Window>() << qobjFake<Window>();
    QStringList themes;
    stub.set_lamda(static_cast<void (Window::*)(const QString &)>(&Window::loadTheme),
                   [&themes](Window *, const QString &t) { themes << t; });

    // Act
    obj->loadTheme("light");

    // Assert：每个窗口恰好一次且参数正确
    EXPECT_EQ(themes.count(), 2);
    EXPECT_EQ(themes.at(0), QString("light"));
    EXPECT_EQ(themes.at(1), QString("light"));
}

// ============================================================
// getFileTabInfo / popupExistTabs
// ============================================================

TEST_F(StartManagerTest, GetFileTabInfo_NotOpen_ReturnsInvalidIndices)
{
    // Arrange：窗口存在但无该标签
    obj->m_windows << qobjFake<Window>();
    stub.set_lamda(static_cast<int (Window::*)(const QString &)>(&Window::getTabIndex),
                   [](Window *, const QString &) -> int { return -1; });

    // Act
    StartManager::FileTabInfo info = obj->getFileTabInfo(tmp->filePath("none.cpp"));

    // Assert：哨兵 -1（与 FileTabInfo 哨兵语义一致）
    EXPECT_EQ(info.windowIndex, -1);
    EXPECT_EQ(info.tabIndex, -1);
}

TEST_F(StartManagerTest, GetFileTabInfo_OpenInSecondWindow_ReturnsIndices)
{
    // Arrange：首窗未命中、次窗命中
    Window *winA = qobjFake<Window>();
    Window *winB = qobjFake<Window>();
    obj->m_windows << winA << winB;
    stub.set_lamda(static_cast<int (Window::*)(const QString &)>(&Window::getTabIndex),
                   [winB](Window *self, const QString &) -> int {
                       return self == winB ? 2 : -1;
                   });

    // Act
    StartManager::FileTabInfo info = obj->getFileTabInfo(tmp->filePath("hit.cpp"));

    // Assert：窗口索引与标签索引均精确
    EXPECT_EQ(info.windowIndex, 1);
    EXPECT_EQ(info.tabIndex, 2);
}

TEST_F(StartManagerTest, PopupExistTabs_DockActivationFails_ActivatesManually)
{
    // Arrange：dock 激活失败（dockActivate=false 默认）
    Window *win = qobjFake<Window>();
    obj->m_windows << win;
    stubWindowInteraction(win);

    // Act
    obj->popupExistTabs(StartManager::FileTabInfo { 0, 3 });

    // Assert：激活指定标签 + 手动激活窗口兜底
    EXPECT_EQ(activeTabCalls, 1);
    EXPECT_EQ(lastActiveTab, 3);
    EXPECT_EQ(activateWindowCalls, 1);
}

TEST_F(StartManagerTest, PopupExistTabs_DockActivationSucceeds_SkipsManualActivation)
{
    // Arrange：dock 激活成功
    dockActivate = true;
    Window *win = qobjFake<Window>();
    obj->m_windows << win;
    stubWindowInteraction(win);

    // Act
    obj->popupExistTabs(StartManager::FileTabInfo { 0, 7 });

    // Assert：标签被激活但不走 Qt 手动激活
    EXPECT_EQ(activeTabCalls, 1);
    EXPECT_EQ(lastActiveTab, 7);
    EXPECT_EQ(activateWindowCalls, 0);
}

// ============================================================
// initWindowPosition（直接调用）
// ============================================================

TEST_F(StartManagerTest, InitWindowPosition_AlwaysCenter_SkipsOffsetMove)
{
    // Arrange：已有窗口但强制居中
    obj->m_windows << qobjFake<Window>();
    Window *win = qobjFake<Window>();
    int moveCalls = 0;
    stub.set_lamda(static_cast<void (QWidget::*)(int, int)>(&QWidget::move),
                   [&moveCalls](QWidget *, int, int) { ++moveCalls; });

    // Act
    obj->initWindowPosition(win, true);

    // Assert：居中分支不做偏移移动 + 窗口列表状态未变
    EXPECT_EQ(moveCalls, 0);
    EXPECT_EQ(obj->m_windows.count(), 1);
}

TEST_F(StartManagerTest, InitWindowPosition_NoWindows_SkipsOffsetMove)
{
    // Arrange：无既有窗口（首窗），不强制居中
    Window *win = qobjFake<Window>();
    int moveCalls = 0;
    stub.set_lamda(static_cast<void (QWidget::*)(int, int)>(&QWidget::move),
                   [&moveCalls](QWidget *, int, int) { ++moveCalls; });

    // Act
    obj->initWindowPosition(win, false);

    // Assert：首窗居中分支不偏移 + 无窗口状态保持
    EXPECT_EQ(moveCalls, 0);
    EXPECT_TRUE(obj->m_windows.isEmpty());
}

TEST_F(StartManagerTest, InitWindowPosition_WithExistingWindows_MovesByOffset)
{
    // Arrange：已有一个窗口 → 偏移 1*50；屏幕几何固定
    obj->m_windows << qobjFake<Window>();
    Window *win = qobjFake<Window>();
    int moveCalls = 0;
    int lastX = -999, lastY = -999;
    stub.set_lamda(static_cast<void (QWidget::*)(int, int)>(&QWidget::move),
                   [&moveCalls, &lastX, &lastY](QWidget *, int x, int y) {
                       ++moveCalls;
                       lastX = x;
                       lastY = y;
                   });
    QScreen *fakeScreen = zeroFake<QScreen>();
    stub.set_lamda(&QGuiApplication::primaryScreen,
                   [fakeScreen]() -> QScreen * { return fakeScreen; });
    stub.set_lamda(static_cast<QRect (QScreen::*)() const>(&QScreen::availableGeometry),
                   [](QScreen *) -> QRect { return QRect(0, 0, 1920, 1080); });

    // Act
    obj->initWindowPosition(win, false);

    // Assert：偏移 = 1*50，基于屏幕原点
    EXPECT_EQ(moveCalls, 1);
    EXPECT_EQ(lastX, 50);
    EXPECT_EQ(lastY, 50);
}

// ============================================================
// slotDelayBackupFile / delayMallocTrim / timerEvent
// ============================================================

TEST_F(StartManagerTest, SlotDelayBackupFile_IdleTimer_StartsDelayTimer)
{
    // Act
    obj->slotDelayBackupFile();

    // Assert：延迟备份定时器激活 + 周期定时器不受影响
    EXPECT_TRUE(obj->m_DelayTimer.isActive());
    EXPECT_TRUE(obj->m_pTimer->isActive());
}

TEST_F(StartManagerTest, SlotDelayBackupFile_TimerActive_SkipsRestart)
{
    // Arrange：先启动一次
    obj->slotDelayBackupFile();
    int id = obj->m_DelayTimer.timerId();

    // Act：再次触发（不应重启）
    obj->slotDelayBackupFile();

    // Assert：定时器 id 未变（未重启）
    EXPECT_TRUE(obj->m_DelayTimer.isActive());
    EXPECT_EQ(obj->m_DelayTimer.timerId(), id);
}

TEST_F(StartManagerTest, SlotDelayBackupFile_TagDragging_SkipsStart)
{
    // Arrange：拖拽状态
    obj->m_bIsTagDragging = true;

    // Act
    obj->slotDelayBackupFile();

    // Assert：不启动定时器 + timerId 保持未激活值（QBasicTimer 未启动 id==0）
    EXPECT_FALSE(obj->m_DelayTimer.isActive());
    EXPECT_EQ(obj->m_DelayTimer.timerId(), 0);
}

TEST_F(StartManagerTest, DelayMallocTrim_Idle_StartsFreeTimer)
{
    // Act
    obj->delayMallocTrim();

    // Assert：内存释放定时器启动 + 获得有效 timer id
    EXPECT_TRUE(obj->m_FreeMemTimer.isActive());
    EXPECT_GT(obj->m_FreeMemTimer.timerId(), 0);
}

TEST_F(StartManagerTest, DelayMallocTrim_Active_SkipsRestart)
{
    // Arrange
    obj->delayMallocTrim();
    int id = obj->m_FreeMemTimer.timerId();

    // Act
    obj->delayMallocTrim();

    // Assert：未重启（id 不变且保持激活）
    EXPECT_EQ(obj->m_FreeMemTimer.timerId(), id);
    EXPECT_TRUE(obj->m_FreeMemTimer.isActive());
}

TEST_F(StartManagerTest, TimerEvent_DelayTimerExpired_BackupsAndRestartsCycle)
{
    // Arrange：启动延迟备份定时器，伪造到期事件
    installAutoBackupStubs();
    obj->slotDelayBackupFile();
    QTimerEvent ev(obj->m_DelayTimer.timerId());
    int before = setValueCalls;

    // Act
    obj->timerEvent(&ev);

    // Assert：执行了备份（配置写入次数增加）+ 定时器停止 + 周期定时器重启
    EXPECT_GT(setValueCalls, before);
    EXPECT_FALSE(obj->m_DelayTimer.isActive());
    EXPECT_TRUE(obj->m_pTimer->isActive());
}

TEST_F(StartManagerTest, TimerEvent_FreeMemTimerExpired_TrimsAndStops)
{
    // Arrange
    obj->delayMallocTrim();
    QTimerEvent ev(obj->m_FreeMemTimer.timerId());

    // Act（malloc_trim 真实调用，glibc 无害）
    obj->timerEvent(&ev);

    // Assert：定时器停止 + timerId 复位未激活值（QBasicTimer 停止后 id==0）
    EXPECT_FALSE(obj->m_FreeMemTimer.isActive());
    EXPECT_EQ(obj->m_FreeMemTimer.timerId(), 0);
}

TEST_F(StartManagerTest, TimerEvent_UnknownTimer_Ignored)
{
    // Arrange：未注册的 timer id
    installAutoBackupStubs();
    QTimerEvent ev(999999);
    int before = setValueCalls;

    // Act
    obj->timerEvent(&ev);

    // Assert：无任何备份副作用 + 现有定时器状态未被误动
    EXPECT_EQ(setValueCalls, before);
    EXPECT_FALSE(obj->m_DelayTimer.isActive());
}

// ============================================================
// autoBackupFile
// ============================================================

// autoBackupFile 用例公共依赖：一个窗口 + 一个已加载 wrapper + 基础桩
void StartManagerTest::installAutoBackupStubs()
{
    backupWin = qobjFake<Window>();
    obj->m_windows << backupWin;
    fakeBackupTabbar = zeroFake<Tabbar>();
    fakeBackupWrapper = zeroFake<EditWrapper>();
    fakeBackupTextEdit = zeroFake<TextEdit>();

    stub.set_lamda(static_cast<Tabbar *(Window::*)()>(&Window::getTabbar),
                   [this](Window *) -> Tabbar * { return fakeBackupTabbar; });
    stub.set_lamda(static_cast<int (QTabBar::*)() const>(&QTabBar::count),
                   [this](QTabBar *) -> int { return tabCount; });
    stub.set_lamda(static_cast<QString (Tabbar::*)(int) const>(&Tabbar::fileAt),
                   [this](Tabbar *, int idx) -> QString {
                       return (idx >= 0 && idx < tabFiles.count()) ? tabFiles.at(idx) : QString();
                   });
    stub.set_lamda(static_cast<QMap<QString, EditWrapper *> (Window::*)()>(&Window::getWrappers),
                   [this](Window *) -> QMap<QString, EditWrapper *> {
                       return QMap<QString, EditWrapper *> { { backupFilePath, fakeBackupWrapper } };
                   });
    stub.set_lamda(static_cast<bool (EditWrapper::*)()>(&EditWrapper::getFileLoading),
                   [this](EditWrapper *) -> bool { return fileLoading; });
    stub.set_lamda(static_cast<TextEdit *(EditWrapper::*)()>(&EditWrapper::textEditor),
                   [this](EditWrapper *) -> TextEdit * { return fakeBackupTextEdit; });
    stub.set_lamda(static_cast<QString (TextEdit::*)()>(&TextEdit::getTruePath),
                   [this](TextEdit *) -> QString { return backupTruePath; });
    stub.set_lamda(static_cast<QTextCursor (QPlainTextEdit::*)() const>(&QPlainTextEdit::textCursor),
                   [](QPlainTextEdit *) -> QTextCursor { return QTextCursor(); });
    // null QTextCursor::position() 为 -1；固定为 0 保证 cursorPosition 字段确定
    stub.set_lamda(static_cast<int (QTextCursor::*)() const>(&QTextCursor::position),
                   [](QTextCursor *) -> int { return 0; });
    stub.set_lamda(static_cast<int (Tabbar::*)(const QString &)>(&Tabbar::indexOf),
                   [this](Tabbar *, const QString &) -> int { return backupTabIndex; });
    stub.set_lamda(static_cast<QList<int> (TextEdit::*)()>(&TextEdit::getBookmarkInfo),
                   [this](TextEdit *) -> QList<int> { return wrapperBookmarks; });
    stub.set_lamda(static_cast<bool (QWidget::*)() const>(&QWidget::isActiveWindow),
                   [this](QWidget *) -> bool { return windowActive; });
    stub.set_lamda(static_cast<EditWrapper *(Window::*)()>(&Window::currentWrapper),
                   [this](Window *) -> EditWrapper * { return windowActive ? fakeBackupWrapper : nullptr; });
    stub.set_lamda(static_cast<bool (EditWrapper::*)()>(&EditWrapper::isModified),
                   [this](EditWrapper *) -> bool { return wrapperModified; });
    stub.set_lamda(static_cast<bool (EditWrapper::*)(QString)>(&EditWrapper::saveTemFile),
                   [this](EditWrapper *, QString path) -> bool {
                       ++saveTemFileCalls;
                       lastSaveTemPath = path;
                       return true;
                   });
}

TEST_F(StartManagerTest, AutoBackupFile_TagDragging_SkipsBackup)
{
    // Arrange：拖拽中 + 备份桩就绪（不应被触发）
    installAutoBackupStubs();
    fakeBackupTextEdit->m_sFilePath = backupFilePath;
    obj->m_bIsTagDragging = true;
    int before = setValueCalls;

    // Act
    obj->autoBackupFile();

    // Assert：未写任何配置、未保存任何临时文件
    EXPECT_EQ(setValueCalls, before);
    EXPECT_EQ(saveTemFileCalls, 0);
}

TEST_F(StartManagerTest, AutoBackupFile_BackupDirMissing_CreatesDirAndKeepsUserBackup)
{
    // Arrange：autoBackupDir 不存在（构造时 fileInfoExists=true 未创建）
    // 用户备份目录存在且非空
    QDir().mkpath(obj->m_backupDir);
    QFile userFile(QDir(obj->m_backupDir).filePath("user.txt"));
    ASSERT_TRUE(userFile.open(QIODevice::WriteOnly));
    userFile.write("keep");
    userFile.close();
    installAutoBackupStubs();
    fakeBackupTextEdit->m_sFilePath = backupFilePath;
    obj->m_windows.clear();   // 无窗口：纯目录维护路径
    // 还原 exists 真实语义：让 autoBackupDir"不存在"判定生效（走 mkpath 分支）
    stub.reset(static_cast<bool (QFileInfo::*)() const>(&QFileInfo::exists));
    stub.reset(static_cast<bool (*)(const QString &)>(&QFileInfo::exists));
    int before = setValueCalls;

    // Act
    obj->autoBackupFile();

    // Assert：autoBackupDir 被创建 + 用户备份仍在（走"不存在"分支不清空）
    EXPECT_TRUE(QDir(obj->m_autoBackupDir).exists());
    EXPECT_TRUE(QFile::exists(QDir(obj->m_backupDir).filePath("user.txt")));
    EXPECT_GT(setValueCalls, before);   // 空列表仍写配置
}

TEST_F(StartManagerTest, AutoBackupFile_UserBackupExist_RemovesUserBackup)
{
    // Arrange：autoBackupDir 已存在（走 else 分支）+ 用户备份非空 → 应清空
    QDir().mkpath(obj->m_autoBackupDir);
    QDir().mkpath(obj->m_backupDir);
    QFile userFile(QDir(obj->m_backupDir).filePath("stale.txt"));
    ASSERT_TRUE(userFile.open(QIODevice::WriteOnly));
    userFile.close();
    installAutoBackupStubs();
    fakeBackupTextEdit->m_sFilePath = backupFilePath;
    obj->m_windows.clear();
    // 还原 exists 真实语义：autoBackupDir 已真实存在 → 走清空用户备份分支
    stub.reset(static_cast<bool (QFileInfo::*)() const>(&QFileInfo::exists));
    stub.reset(static_cast<bool (*)(const QString &)>(&QFileInfo::exists));

    // Act
    obj->autoBackupFile();

    // Assert：用户备份被整目录清除（removeRecursively 连根删除）+ 备份配置写一次
    EXPECT_FALSE(QFile::exists(QDir(obj->m_backupDir).filePath("stale.txt")));
    EXPECT_FALSE(QDir(obj->m_backupDir).exists());
    EXPECT_EQ(backupWriteCount(), 1);
}

TEST_F(StartManagerTest, AutoBackupFile_NormalWrapper_WritesBackupJson)
{
    // Arrange：1 窗口 1 标签 1 wrapper，正常未修改文件
    backupFilePath = tmp->filePath("normal.cpp");
    backupTruePath = backupFilePath;
    tabFiles = QStringList { backupFilePath };
    tabCount = 1;
    backupTabIndex = 0;
    installAutoBackupStubs();
    fakeBackupTextEdit->m_sFilePath = backupFilePath;
    QDir().mkpath(obj->m_autoBackupDir);
    int before = setValueCalls;

    // Act
    obj->autoBackupFile();

    // Assert：配置写入且 JSON 字段精确（localPath/cursorPosition/window/modify）
    ASSERT_GT(setValueCalls, before);
    QStringList written = backupWrites();
    ASSERT_EQ(written.count(), 1);
    QJsonObject json = QJsonDocument::fromJson(written.at(0).toUtf8()).object();
    EXPECT_EQ(json.value("localPath").toString(), backupFilePath);
    EXPECT_EQ(json.value("cursorPosition").toString(), QString("0"));
    EXPECT_EQ(json.value("window").toInt(), 0);
    EXPECT_FALSE(json.value("modify").toBool());
    EXPECT_EQ(saveTemFileCalls, 0);   // 未修改普通文件不写临时文件
}

TEST_F(StartManagerTest, AutoBackupFile_FileLoading_SkipsWrapper)
{
    // Arrange：大文件加载中 → 跳过该 wrapper
    backupFilePath = tmp->filePath("loading.cpp");
    tabFiles = QStringList { backupFilePath };
    fileLoading = true;
    installAutoBackupStubs();
    fakeBackupTextEdit->m_sFilePath = backupFilePath;
    QDir().mkpath(obj->m_autoBackupDir);
    int before = setValueCalls;

    // Act
    obj->autoBackupFile();

    // Assert：该 wrapper 无记录（列表保留 tabbar 文件路径占位）
    QStringList written = backupWrites();
    ASSERT_EQ(written.count(), 1);
    EXPECT_EQ(written.at(0), backupFilePath);   // 占位 = tabbar 的 fileAt(0)
    EXPECT_EQ(setValueCalls, before + 1 + 1);   // 本次备份 + saveBookmark 空表写
}

TEST_F(StartManagerTest, AutoBackupFile_TabIndexInvalid_SkipsRecord)
{
    // Arrange：tabbar 找不到该文件 → continue
    backupFilePath = tmp->filePath("orphan.cpp");
    tabFiles = QStringList { "other.cpp" };
    backupTabIndex = -1;
    installAutoBackupStubs();
    fakeBackupTextEdit->m_sFilePath = backupFilePath;
    QDir().mkpath(obj->m_autoBackupDir);

    // Act
    obj->autoBackupFile();

    // Assert：记录保持占位未被替换 + 无临时文件保存副作用
    QStringList written = backupWrites();
    ASSERT_EQ(written.count(), 1);
    EXPECT_EQ(written.at(0), QString("other.cpp"));   // 占位 = tabbar 的 fileAt(0)
    EXPECT_EQ(saveTemFileCalls, 0);
}

TEST_F(StartManagerTest, AutoBackupFile_TabIndexOutOfRange_AppendsRecord)
{
    // Arrange：tabIndex == list.size()（越界）→ append
    backupFilePath = tmp->filePath("append.cpp");
    backupTruePath = backupFilePath;
    tabFiles = QStringList { "a.cpp" };
    tabCount = 1;
    backupTabIndex = 1;   // list.size()==1 → 越界
    installAutoBackupStubs();
    fakeBackupTextEdit->m_sFilePath = backupFilePath;
    QDir().mkpath(obj->m_autoBackupDir);

    // Act
    obj->autoBackupFile();

    // Assert：记录被 append（第二条），占位仍在首位
    QStringList written = backupWrites();
    ASSERT_EQ(written.count(), 2);
    EXPECT_EQ(written.at(0), QString("a.cpp"));
    QJsonObject json = QJsonDocument::fromJson(written.at(1).toUtf8()).object();
    EXPECT_EQ(json.value("localPath").toString(), backupFilePath);
}

TEST_F(StartManagerTest, AutoBackupFile_HasBookmarks_RecordsAndWrites)
{
    // Arrange：wrapper 携带书签
    backupFilePath = tmp->filePath("marked.cpp");
    backupTruePath = backupFilePath;
    tabFiles = QStringList { backupFilePath };
    wrapperBookmarks = QList<int> { 2, 8 };
    installAutoBackupStubs();
    fakeBackupTextEdit->m_sFilePath = backupFilePath;
    QDir().mkpath(obj->m_autoBackupDir);

    // Act
    obj->autoBackupFile();

    // Assert：JSON 含书签串 + 全局书签表更新
    QStringList written = backupWrites();
    QJsonObject json = QJsonDocument::fromJson(written.at(0).toUtf8()).object();
    EXPECT_EQ(json.value("bookMark").toString(), QString("2,8"));
    EXPECT_EQ(obj->findBookmark(backupFilePath), QList<int>({ 2, 8 }));
}

TEST_F(StartManagerTest, AutoBackupFile_NoBookmarks_RemovesRecord)
{
    // Arrange：预置全局书签后让 wrapper 书签为空
    backupFilePath = tmp->filePath("clean.cpp");
    backupTruePath = backupFilePath;
    tabFiles = QStringList { backupFilePath };
    obj->m_bookmarkTable.insert(backupFilePath, QList<int> { 1 });
    wrapperBookmarks.clear();
    installAutoBackupStubs();
    fakeBackupTextEdit->m_sFilePath = backupFilePath;
    QDir().mkpath(obj->m_autoBackupDir);

    // Act
    obj->autoBackupFile();

    // Assert：全局书签表移除该文件记录 + JSON 无 bookMark 字段
    EXPECT_TRUE(obj->findBookmark(backupFilePath).isEmpty());
    EXPECT_EQ(obj->m_bookmarkTable.count(), 0);
    QStringList written = backupWrites();
    QJsonObject json = QJsonDocument::fromJson(written.at(0).toUtf8()).object();
    EXPECT_FALSE(json.contains("bookMark"));
}

TEST_F(StartManagerTest, AutoBackupFile_ActiveCurrentWrapper_MarksFocus)
{
    // Arrange：活动窗口 + 当前 wrapper
    backupFilePath = tmp->filePath("focus.cpp");
    tabFiles = QStringList { backupFilePath };
    windowActive = true;
    installAutoBackupStubs();
    fakeBackupTextEdit->m_sFilePath = backupFilePath;
    QDir().mkpath(obj->m_autoBackupDir);

    // Act
    obj->autoBackupFile();

    // Assert：JSON 标记 focus + 恰好一次备份写入
    QStringList written = backupWrites();
    ASSERT_EQ(written.count(), 1);
    QJsonObject json = QJsonDocument::fromJson(written.at(0).toUtf8()).object();
    EXPECT_TRUE(json.value("focus").toBool());
    EXPECT_EQ(backupWriteCount(), 1);
}

TEST_F(StartManagerTest, AutoBackupFile_DraftFile_SavesAsTemFile)
{
    // Arrange：草稿文件（Utils::isDraftFile=true）→ 按原路径 saveTemFile
    backupFilePath = tmp->filePath("draft_untitled");
    tabFiles = QStringList { backupFilePath };
    isDraft = true;
    installAutoBackupStubs();
    fakeBackupTextEdit->m_sFilePath = backupFilePath;
    QDir().mkpath(obj->m_autoBackupDir);

    // Act
    obj->autoBackupFile();

    // Assert：saveTemFile 收到原始 filePath
    EXPECT_EQ(saveTemFileCalls, 1);
    EXPECT_EQ(lastSaveTemPath, backupFilePath);
}

TEST_F(StartManagerTest, AutoBackupFile_ModifiedFile_SavesToAutoBackupDir)
{
    // Arrange：已修改普通文件 → 存入 autoBackupDir（md5.路径.后缀）
    backupFilePath = tmp->filePath("modified.cpp");
    backupTruePath = backupFilePath;
    tabFiles = QStringList { backupFilePath };
    wrapperModified = true;
    installAutoBackupStubs();
    fakeBackupTextEdit->m_sFilePath = backupFilePath;
    QDir().mkpath(obj->m_autoBackupDir);

    // Act
    obj->autoBackupFile();

    // Assert：saveTemFile 收到 autoBackupDir 下的备份名（MD5 与路径替换后的形态）
    EXPECT_EQ(saveTemFileCalls, 1);
    EXPECT_TRUE(lastSaveTemPath.startsWith(obj->m_autoBackupDir + "/"));
    EXPECT_TRUE(lastSaveTemPath.contains("md5hash"));
    // JSON 记录 temFilePath
    QStringList written = backupWrites();
    QJsonObject json = QJsonDocument::fromJson(written.at(0).toUtf8()).object();
    EXPECT_EQ(json.value("temFilePath").toString(), lastSaveTemPath);
}

TEST_F(StartManagerTest, AutoBackupFile_PendingTab_RestoresFromConfig)
{
    // Arrange：tabbar 有两个标签（一个已加载、一个 pending），配置含 pending 的原始记录
    backupFilePath = tmp->filePath("loaded.cpp");
    QString pendingPath = tmp->filePath("pending.cpp");
    tabFiles = QStringList { backupFilePath, pendingPath };
    tabCount = 2;
    backupTabIndex = 0;
    QJsonObject pendingJson;
    pendingJson.insert("localPath", pendingPath);
    pendingJson.insert("cursorPosition", "11");
    optionValue = QVariant(QStringList { QString::fromUtf8(
        QJsonDocument(pendingJson).toJson(QJsonDocument::Compact)) });
    installAutoBackupStubs();
    fakeBackupTextEdit->m_sFilePath = backupFilePath;
    QDir().mkpath(obj->m_autoBackupDir);

    // Act
    obj->autoBackupFile();

    // Assert：pending 标签的原始 JSON 记录被原样恢复到对应位置
    QStringList written = backupWrites();
    ASSERT_EQ(written.count(), 2);
    QJsonObject restored = QJsonDocument::fromJson(written.at(1).toUtf8()).object();
    EXPECT_EQ(restored.value("localPath").toString(), pendingPath);
    EXPECT_EQ(restored.value("cursorPosition").toString(), QString("11"));
}

// ============================================================
// recoverFile
// ============================================================

TEST_F(StartManagerTest, RecoverFile_EmptyRecords_ReturnsZero)
{
    // Arrange：无临时记录
    Window *win = qobjFake<Window>();
    int batchCalls = 0;
    stub.set_lamda(static_cast<void (Window::*)(bool)>(&Window::setBatchAddingPendingTabs),
                   [&batchCalls](Window *, bool) { ++batchCalls; });

    // Act
    int ret = obj->recoverFile(win);

    // Assert：零恢复 + 批量模式开/关各一次（setBatch true + 恢复后 false）
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(batchCalls, 2);
}

TEST_F(StartManagerTest, RecoverFile_LocalFileMissing_SkipsRecord)
{
    // Arrange：localPath 指向不存在文件（fileInfoExists=false → QFileInfo::exists 桩）
    fileInfoExists = false;
    QString path = tmp->filePath("missing.cpp");
    QJsonObject json;
    json.insert("localPath", path);
    json.insert("focus", true);
    obj->m_qlistTemFile = QStringList { QString::fromUtf8(
        QJsonDocument(json).toJson(QJsonDocument::Compact)) };
    Window *win = qobjFake<Window>();
    int addTemCalls = 0;
    stub.set_lamda(static_cast<void (Window::*)(bool)>(&Window::setBatchAddingPendingTabs),
                   [](Window *, bool) {});
    stub.set_lamda(
        static_cast<void (Window::*)(const QString &, const QString &, const QString &, const QString &, bool, int, int)>(
            &Window::addTemFileTab),
        [&addTemCalls](Window *, const QString &, const QString &, const QString &, const QString &,
                       bool, int, int) {
            ++addTemCalls;
        });

    // Act
    int ret = obj->recoverFile(win);

    // Assert：该记录被跳过（未加载、计数 0）
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(addTemCalls, 0);
}

TEST_F(StartManagerTest, RecoverFile_FocusTab_LoadsImmediatelyWithBookmarks)
{
    // Arrange：焦点记录（temFile 存在 + 记录书签）
    QString localPath = tmp->filePath("focus.txt");
    QString temPath = tmp->filePath("focus.tem");
    QJsonObject json;
    json.insert("localPath", localPath);
    json.insert("temFilePath", temPath);
    json.insert("modify", true);
    json.insert("cursorPosition", "7");
    json.insert("bookMark", "3,5");
    json.insert("focus", true);
    obj->m_qlistTemFile = QStringList { QString::fromUtf8(
        QJsonDocument(json).toJson(QJsonDocument::Compact)) };

    Window *win = qobjFake<Window>();
    obj->m_windows << win;
    EditWrapper *fakeWrapper = zeroFake<EditWrapper>();
    TextEdit *fakeTextEdit = zeroFake<TextEdit>();
    QList<int> appliedBookmarks;
    stub.set_lamda(static_cast<void (Window::*)(bool)>(&Window::setBatchAddingPendingTabs),
                   [](Window *, bool) {});
    QString gotPath, gotName, gotTrue;
    stub.set_lamda(
        static_cast<void (Window::*)(const QString &, const QString &, const QString &, const QString &, bool, int, int)>(
            &Window::addTemFileTab),
        [&gotPath, &gotName, &gotTrue](Window *, const QString &p, const QString &n, const QString &t,
                                       const QString &, bool, int, int) {
            gotPath = p;
            gotName = n;
            gotTrue = t;
        });
    stub.set_lamda(static_cast<EditWrapper *(Window::*)(const QString &)>(&Window::wrapper),
                   [fakeWrapper](Window *, const QString &) -> EditWrapper * { return fakeWrapper; });
    stub.set_lamda(static_cast<TextEdit *(EditWrapper::*)()>(&EditWrapper::textEditor),
                   [fakeTextEdit](EditWrapper *) -> TextEdit * { return fakeTextEdit; });
    stub.set_lamda(static_cast<void (TextEdit::*)(QList<int>)>(&TextEdit::setBookMarkList),
                   [&appliedBookmarks](TextEdit *, QList<int> marks) { appliedBookmarks = marks; });
    stubWindowInteraction(win);   // popup 激活焦点标签

    // Act
    int ret = obj->recoverFile(win);

    // Assert：焦点标签立即加载（openPath=temFilePath、displayName=本地文件名）、书签恢复
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(gotPath, temPath);
    EXPECT_EQ(gotName, QString("focus.txt"));
    EXPECT_EQ(gotTrue, localPath);
    EXPECT_EQ(appliedBookmarks, QList<int>({ 3, 5 }));
}

TEST_F(StartManagerTest, RecoverFile_BookMarkFromGlobal_AppliesConfigBookmark)
{
    // Arrange：记录无书签字段，但全局书签表有 temPath 记录
    QString localPath = tmp->filePath("global.txt");
    QString temPath = tmp->filePath("global.tem");
    QJsonObject json;
    json.insert("localPath", localPath);
    json.insert("temFilePath", temPath);
    json.insert("focus", true);
    obj->m_qlistTemFile = QStringList { QString::fromUtf8(
        QJsonDocument(json).toJson(QJsonDocument::Compact)) };
    obj->m_bookmarkTable.insert(temPath, QList<int> { 6, 7 });

    Window *win = qobjFake<Window>();
    obj->m_windows << win;
    EditWrapper *fakeWrapper = zeroFake<EditWrapper>();
    TextEdit *fakeTextEdit = zeroFake<TextEdit>();
    QList<int> appliedBookmarks;
    stub.set_lamda(static_cast<void (Window::*)(bool)>(&Window::setBatchAddingPendingTabs),
                   [](Window *, bool) {});
    stub.set_lamda(
        static_cast<void (Window::*)(const QString &, const QString &, const QString &, const QString &, bool, int, int)>(
            &Window::addTemFileTab),
        [](Window *, const QString &, const QString &, const QString &, const QString &, bool, int, int) {});
    stub.set_lamda(static_cast<EditWrapper *(Window::*)(const QString &)>(&Window::wrapper),
                   [fakeWrapper](Window *, const QString &) -> EditWrapper * { return fakeWrapper; });
    stub.set_lamda(static_cast<TextEdit *(EditWrapper::*)()>(&EditWrapper::textEditor),
                   [fakeTextEdit](EditWrapper *) -> TextEdit * { return fakeTextEdit; });
    stub.set_lamda(static_cast<void (TextEdit::*)(QList<int>)>(&TextEdit::setBookMarkList),
                   [&appliedBookmarks](TextEdit *, QList<int> marks) { appliedBookmarks = marks; });
    stubWindowInteraction(win);

    // Act
    int ret = obj->recoverFile(win);

    // Assert：焦点标签使用全局书签配置
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(appliedBookmarks, QList<int>({ 6, 7 }));
}

TEST_F(StartManagerTest, RecoverFile_NonFocusTab_AddedAsPending)
{
    // Arrange：两条记录，第一条非焦点（懒加载）、第二条焦点
    QString localA = tmp->filePath("lazy.txt");
    QString localB = tmp->filePath("hot.txt");
    QJsonObject jsonA;
    jsonA.insert("localPath", localA);
    jsonA.insert("cursorPosition", "2");
    QJsonObject jsonB;
    jsonB.insert("localPath", localB);
    jsonB.insert("focus", true);
    obj->m_qlistTemFile = QStringList {
        QString::fromUtf8(QJsonDocument(jsonA).toJson(QJsonDocument::Compact)),
        QString::fromUtf8(QJsonDocument(jsonB).toJson(QJsonDocument::Compact))
    };

    Window *win = qobjFake<Window>();
    obj->m_windows << win;
    Window::PendingTabInfo pendingInfo;
    int pendingCalls = 0;
    stub.set_lamda(static_cast<void (Window::*)(bool)>(&Window::setBatchAddingPendingTabs),
                   [](Window *, bool) {});
    stub.set_lamda(
        static_cast<void (Window::*)(const Window::PendingTabInfo &, int)>(&Window::addPendingTab),
        [&pendingInfo, &pendingCalls](Window *, const Window::PendingTabInfo &info, int) {
            pendingInfo = info;
            ++pendingCalls;
        });
    stub.set_lamda(
        static_cast<void (Window::*)(const QString &, const QString &, const QString &, const QString &, bool, int, int)>(
            &Window::addTemFileTab),
        [](Window *, const QString &, const QString &, const QString &, const QString &, bool, int, int) {});
    stubWindowInteraction(win);

    // Act
    int ret = obj->recoverFile(win);

    // Assert：非焦点标签按 pending 方式添加，字段（路径/光标）透传
    EXPECT_EQ(ret, 2);
    EXPECT_EQ(pendingCalls, 1);
    EXPECT_EQ(pendingInfo.filepath, localA);
    EXPECT_EQ(pendingInfo.truePath, localA);
    EXPECT_EQ(pendingInfo.cursorPosition, 2);
}

TEST_F(StartManagerTest, RecoverFile_WindowIndexChanges_CreatesNewWindow)
{
    // Arrange：两条记录 window 字段变化（0 → 1）→ 第二阶段需创建新窗
    QString localA = tmp->filePath("w0.txt");
    QString localB = tmp->filePath("w1.txt");
    QJsonObject jsonA;
    jsonA.insert("localPath", localA);
    jsonA.insert("window", 0);
    QJsonObject jsonB;
    jsonB.insert("localPath", localB);
    jsonB.insert("window", 1);
    obj->m_qlistTemFile = QStringList {
        QString::fromUtf8(QJsonDocument(jsonA).toJson(QJsonDocument::Compact)),
        QString::fromUtf8(QJsonDocument(jsonB).toJson(QJsonDocument::Compact))
    };

    Window *win0 = qobjFake<Window>();
    obj->m_windows << win0;
    Window *win1 = qobjFake<Window>();
    int createCalls = 0;
    int batchTrue = 0, batchFalse = 0;
    stub.set_lamda(static_cast<Window *(StartManager::*)(bool)>(&StartManager::createWindow),
                   [this, win1, &createCalls](StartManager *, bool) -> Window * {
                       ++createCalls;
                       obj->m_windows << win1;   // 模拟真实 createWindow 的追加行为
                       return win1;
                   });
    stub.set_lamda(static_cast<void (Window::*)(bool)>(&Window::setBatchAddingPendingTabs),
                   [&batchTrue, &batchFalse](Window *, bool en) { en ? ++batchTrue : ++batchFalse; });
    stub.set_lamda(
        static_cast<void (Window::*)(const QString &, const QString &, const QString &, const QString &, bool, int, int)>(
            &Window::addTemFileTab),
        [](Window *, const QString &, const QString &, const QString &, const QString &, bool, int, int) {});
    stub.set_lamda(
        static_cast<void (Window::*)(const Window::PendingTabInfo &, int)>(&Window::addPendingTab),
        [](Window *, const Window::PendingTabInfo &, int) {});
    stubWindowInteraction(win1);   // 焦点（无 focus 记录默认首条）popup 走新窗

    // Act
    int ret = obj->recoverFile(win0);

    // Assert：第二窗口被创建并加入恢复列表（batch 恢复 false 两次）
    EXPECT_EQ(ret, 2);
    EXPECT_EQ(createCalls, 1);
    EXPECT_EQ(showCenterCalls, 1);   // stubWindowInteraction 版 showCenterWindow 计数
    EXPECT_EQ(batchTrue, 2);    // 主窗口 + 新窗口各一次
    EXPECT_EQ(batchFalse, 2);   // 结束后各恢复一次
    EXPECT_EQ(obj->m_windows.count(), 2);
}

TEST_F(StartManagerTest, RecoverFile_NoFocusRecord_ActivatesFirstTab)
{
    // Arrange：无 focus 字段的记录；恢复后走 activeTab(0)
    QString localPath = tmp->filePath("first.txt");
    QJsonObject json;
    json.insert("localPath", localPath);
    obj->m_qlistTemFile = QStringList { QString::fromUtf8(
        QJsonDocument(json).toJson(QJsonDocument::Compact)) };

    Window *win = qobjFake<Window>();
    obj->m_windows << win;
    Tabbar *fakeTabbar = zeroFake<Tabbar>();
    stub.set_lamda(static_cast<void (Window::*)(bool)>(&Window::setBatchAddingPendingTabs),
                   [](Window *, bool) {});
    stub.set_lamda(static_cast<Tabbar *(Window::*)()>(&Window::getTabbar),
                   [fakeTabbar](Window *) -> Tabbar * { return fakeTabbar; });
    stub.set_lamda(static_cast<int (QTabBar::*)() const>(&QTabBar::count),
                   [](QTabBar *) -> int { return 1; });
    stub.set_lamda(
        static_cast<void (Window::*)(const QString &, const QString &, const QString &, const QString &, bool, int, int)>(
            &Window::addTemFileTab),
        [](Window *, const QString &, const QString &, const QString &, const QString &, bool, int, int) {});
    stub.set_lamda(
        static_cast<void (Window::*)(const Window::PendingTabInfo &, int)>(&Window::addPendingTab),
        [](Window *, const Window::PendingTabInfo &, int) {});
    stubWindowInteraction(win);

    // Act
    int ret = obj->recoverFile(win);

    // Assert：恢复 1 个标签 + 激活首标签（activeTab(0)，stubWindowInteraction 统一计数）
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(activeTabCalls, 1);
    EXPECT_EQ(lastActiveTab, 0);
}

TEST_F(StartManagerTest, RecoverFile_DraftDisplayName_BlankListed_UsesUntitledNumbered)
{
    // Arrange：草稿文件名出现在 blank 目录列表中（idx>=0 → tr("Untitled %1") 命名）
    QDir().mkpath(obj->m_blankFileDir);
    QFile bf(QDir(obj->m_blankFileDir).filePath("blank_file_0"));
    ASSERT_TRUE(bf.open(QIODevice::WriteOnly));
    bf.close();

    QString localPath = tmp->filePath("blank_file_0");
    QJsonObject json;
    json.insert("localPath", localPath);
    json.insert("focus", true);
    obj->m_qlistTemFile = QStringList { QString::fromUtf8(
        QJsonDocument(json).toJson(QJsonDocument::Compact)) };

    Window *win = qobjFake<Window>();
    obj->m_windows << win;
    QString gotName;
    stub.set_lamda(static_cast<void (Window::*)(bool)>(&Window::setBatchAddingPendingTabs),
                   [](Window *, bool) {});
    stub.set_lamda(
        static_cast<void (Window::*)(const QString &, const QString &, const QString &, const QString &, bool, int, int)>(
            &Window::addTemFileTab),
        [&gotName](Window *, const QString &, const QString &name, const QString &, const QString &,
                   bool, int, int) { gotName = name; });
    stub.set_lamda(static_cast<EditWrapper *(Window::*)(const QString &)>(&Window::wrapper),
                   [](Window *, const QString &) -> EditWrapper * { return nullptr; });
    stubWindowInteraction(win);
    stub.set_lamda(&Utils::isDraftFile, [](const QString &) -> bool { return true; });

    // Act
    int ret = obj->recoverFile(win);

    // Assert：blank 列表命中 idx=0 → tr("Untitled %1") 命名为 Untitled 1
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(gotName, QString("Untitled 1"));
}

TEST_F(StartManagerTest, RecoverFile_DraftDisplayName_UsesUntitled)
{
    // Arrange：草稿文件（isDraftFile=true）且不在 blank 文件列表 → Untitled 命名
    isDraft = false;
    QString localPath = tmp->filePath("blank_file_0");
    QJsonObject json;
    json.insert("localPath", localPath);
    json.insert("focus", true);
    obj->m_qlistTemFile = QStringList { QString::fromUtf8(
        QJsonDocument(json).toJson(QJsonDocument::Compact)) };

    // blank 目录不存在（entryList 空）→ files.indexOf(...) == -1
    Window *win = qobjFake<Window>();
    obj->m_windows << win;
    QString gotName;
    stub.set_lamda(static_cast<void (Window::*)(bool)>(&Window::setBatchAddingPendingTabs),
                   [](Window *, bool) {});
    stub.set_lamda(
        static_cast<void (Window::*)(const QString &, const QString &, const QString &, const QString &, bool, int, int)>(
            &Window::addTemFileTab),
        [&gotName](Window *, const QString &, const QString &name, const QString &, const QString &,
                   bool, int, int) {
            gotName = name;
        });
    stub.set_lamda(static_cast<EditWrapper *(Window::*)(const QString &)>(&Window::wrapper),
                   [](Window *, const QString &) -> EditWrapper * { return nullptr; });
    stubWindowInteraction(win);
    // 草稿/MIME 分支：isDraftFile=true 命中
    stub.reset(static_cast<bool (*)(const QString &)>(&Utils::isDraftFile));
    stub.set_lamda(&Utils::isDraftFile, [](const QString &) -> bool { return true; });

    // Act
    int ret = obj->recoverFile(win);

    // Assert：显示名回退为文件名（blank 列表未命中 idx<0 侧）
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(gotName, QString("blank_file_0"));
}

// ============================================================
// openFilesInWindow
// ============================================================

TEST_F(StartManagerTest, OpenFilesInWindow_MaxWindowsReached_RejectsCreation)
{
    // Arrange：窗口数达上限 20
    for (int i = 0; i < 20; ++i)
        obj->m_windows << qobjFake<Window>();
    int createCalls = 0;
    stub.set_lamda(static_cast<Window *(StartManager::*)(bool)>(&StartManager::createWindow),
                   [&createCalls](StartManager *, bool) -> Window * {
                       ++createCalls;
                       return nullptr;
                   });

    // Act
    obj->openFilesInWindow(QStringList());

    // Assert：不创建新窗口、窗口数不变
    EXPECT_EQ(createCalls, 0);
    EXPECT_EQ(obj->m_windows.count(), 20);
}

TEST_F(StartManagerTest, OpenFilesInWindow_EmptyWithExistingWindows_NotCentered)
{
    // Arrange：已有 1 窗口 → 新窗 showCenterWindow(false)
    obj->m_windows << qobjFake<Window>();
    Window *newWin = qobjFake<Window>();
    int blankCalls = 0;
    stub.set_lamda(static_cast<Window *(StartManager::*)(bool)>(&StartManager::createWindow),
                   [newWin](StartManager *, bool) -> Window * { return newWin; });
    stubWindowInteraction(newWin);
    stub.set_lamda(static_cast<void (Window::*)()>(&Window::addBlankTab),
                   [&blankCalls](Window *) { ++blankCalls; });

    // Act
    obj->openFilesInWindow(QStringList());

    // Assert：创建空标签窗 + 不居中 + 激活
    EXPECT_EQ(showCenterCalls, 1);
    EXPECT_FALSE(lastCenterFlag);
    EXPECT_EQ(blankCalls, 1);
    EXPECT_EQ(activateWindowCalls, 1);
}

TEST_F(StartManagerTest, OpenFilesInWindow_EmptyFirstWindow_Centered)
{
    // Arrange：无窗口 → 首窗居中
    Window *newWin = qobjFake<Window>();
    int blankCalls = 0;
    stub.set_lamda(static_cast<Window *(StartManager::*)(bool)>(&StartManager::createWindow),
                   [newWin](StartManager *, bool) -> Window * { return newWin; });
    stubWindowInteraction(newWin);
    stub.set_lamda(static_cast<void (Window::*)()>(&Window::addBlankTab),
                   [&blankCalls](Window *) { ++blankCalls; });

    // Act
    obj->openFilesInWindow(QStringList());

    // Assert：居中显示 + 空白标签
    EXPECT_EQ(showCenterCalls, 1);
    EXPECT_TRUE(lastCenterFlag);
    EXPECT_EQ(blankCalls, 1);
}

TEST_F(StartManagerTest, OpenFilesInWindow_AlreadyOpenedFile_ActivatesExistingTab)
{
    // Arrange：文件已在窗口 0 打开
    Window *win = qobjFake<Window>();
    obj->m_windows << win;
    stubWindowInteraction(win);   // getTabIndex → stubTabIndex(0)
    int createCalls = 0;
    stub.set_lamda(static_cast<Window *(StartManager::*)(bool)>(&StartManager::createWindow),
                   [&createCalls](StartManager *, bool) -> Window * {
                       ++createCalls;
                       return nullptr;
                   });

    // Act
    obj->openFilesInWindow(QStringList { "already_open.cpp" });

    // Assert：激活已有标签，不创建新窗
    EXPECT_EQ(activeTabCalls, 1);
    EXPECT_EQ(createCalls, 0);
}

TEST_F(StartManagerTest, OpenFilesInWindow_NewFile_CreatesWindowAndTab)
{
    // Arrange：无窗口 → 新建窗口并添加文件标签
    Window *newWin = qobjFake<Window>();
    int createCalls = 0;
    QString addedPath;
    stub.set_lamda(static_cast<Window *(StartManager::*)(bool)>(&StartManager::createWindow),
                   [newWin, &createCalls](StartManager *, bool) -> Window * {
                       ++createCalls;
                       return newWin;
                   });
    stubWindowInteraction(newWin);
    stub.set_lamda(static_cast<void (Window::*)(const QString &, bool)>(&Window::addTab),
                   [&addedPath](Window *, const QString &p, bool) { addedPath = p; });

    // Act
    obj->openFilesInWindow(QStringList { "brand_new.cpp" });

    // Assert：创建窗口 + 居中 + addTab 参数透传
    EXPECT_EQ(createCalls, 1);
    EXPECT_EQ(showCenterCalls, 1);
    EXPECT_TRUE(lastCenterFlag);
    EXPECT_EQ(addedPath, QString("brand_new.cpp"));
}

// ============================================================
// openFilesInTab
// ============================================================

TEST_F(StartManagerTest, OpenFilesInTab_EmptyNoWindowsNoTemFiles_AddsBlankTab)
{
    // Arrange：无窗口 + 临时记录含空串占位（isTemFilesEmpty=true → 不走恢复）+ blank 目录无文件
    // 注：isTemFilesEmpty() 语义为"列表含空串项"（源码 143-156），空列表返回 false 走恢复空转
    obj->m_qlistTemFile = QStringList { QString() };
    Window *newWin = qobjFake<Window>();
    int blankCalls = 0;
    stub.set_lamda(static_cast<Window *(StartManager::*)(bool)>(&StartManager::createWindow),
                   [newWin](StartManager *, bool) -> Window * { return newWin; });
    stubWindowInteraction(newWin);
    stub.set_lamda(static_cast<void (Window::*)()>(&Window::addBlankTab),
                   [&blankCalls](Window *) { ++blankCalls; });

    // Act
    obj->openFilesInTab(QStringList());

    // Assert：新建居中窗口 + 空白标签
    EXPECT_EQ(showCenterCalls, 1);
    EXPECT_TRUE(lastCenterFlag);
    EXPECT_EQ(blankCalls, 1);
}

TEST_F(StartManagerTest, OpenFilesInTab_EmptyNoWindowsTemFilesRecovers_Recovers)
{
    // Arrange：有临时文件记录 → 走恢复路径
    obj->m_qlistTemFile = QStringList { "stub-record" };
    Window *newWin = qobjFake<Window>();
    int recoverCalls = 0;
    int blankCalls = 0;
    stub.set_lamda(static_cast<Window *(StartManager::*)(bool)>(&StartManager::createWindow),
                   [newWin](StartManager *, bool) -> Window * { return newWin; });
    stubWindowInteraction(newWin);
    stub.set_lamda(static_cast<int (StartManager::*)(Window *)>(&StartManager::recoverFile),
                   [&recoverCalls](StartManager *, Window *) -> int {
                       ++recoverCalls;
                       return 2;
                   });
    stub.set_lamda(static_cast<void (Window::*)()>(&Window::addBlankTab),
                   [&blankCalls](Window *) { ++blankCalls; });

    // Act
    obj->openFilesInTab(QStringList());

    // Assert：执行恢复且恢复了 2 个 → 不补空白标签
    EXPECT_EQ(recoverCalls, 1);
    EXPECT_EQ(blankCalls, 0);
}

TEST_F(StartManagerTest, OpenFilesInTab_RecoveryFoundNothing_AddsBlankTab)
{
    // Arrange：恢复返回 0 → 补空白标签
    obj->m_qlistTemFile = QStringList { "stub-record" };
    Window *newWin = qobjFake<Window>();
    int recoverCalls = 0;
    int blankCalls = 0;
    stub.set_lamda(static_cast<Window *(StartManager::*)(bool)>(&StartManager::createWindow),
                   [newWin](StartManager *, bool) -> Window * { return newWin; });
    stubWindowInteraction(newWin);
    stub.set_lamda(static_cast<int (StartManager::*)(Window *)>(&StartManager::recoverFile),
                   [&recoverCalls](StartManager *, Window *) -> int {
                       ++recoverCalls;
                       return 0;
                   });
    stub.set_lamda(static_cast<void (Window::*)()>(&Window::addBlankTab),
                   [&blankCalls](Window *) { ++blankCalls; });

    // Act
    obj->openFilesInTab(QStringList());

    // Assert：恢复 0 个 → 补空白标签
    EXPECT_EQ(recoverCalls, 1);
    EXPECT_EQ(blankCalls, 1);
}

TEST_F(StartManagerTest, OpenFilesInTab_BlankFilesExist_RemovesThenAddsBlankTab)
{
    // Arrange：临时记录含空串占位（isTemFilesEmpty=true → 走清理分支）+ blank 目录存在遗留 blank_file
    obj->m_qlistTemFile = QStringList { QString() };
    QDir().mkpath(obj->m_blankFileDir);
    QString stale = QDir(obj->m_blankFileDir).filePath("blank_file_0");
    QFile f(stale);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.close();
    Window *newWin = qobjFake<Window>();
    int blankCalls = 0;
    stub.set_lamda(static_cast<Window *(StartManager::*)(bool)>(&StartManager::createWindow),
                   [newWin](StartManager *, bool) -> Window * { return newWin; });
    stubWindowInteraction(newWin);
    stub.set_lamda(static_cast<void (Window::*)()>(&Window::addBlankTab),
                   [&blankCalls](Window *) { ++blankCalls; });

    // Act
    obj->openFilesInTab(QStringList());
    // Assert：遗留 blank 文件被删除 + 补空白标签
    // 注：QFile::exists/QDir::exists(name) 内部转调 QFileInfo::exists（已被 stub），改用 entryList 真实判定
    EXPECT_EQ(QDir(obj->m_blankFileDir).entryList(QStringList { "blank_file_0" }, QDir::Files).count(), 0);
    EXPECT_EQ(blankCalls, 1);
}

TEST_F(StartManagerTest, OpenFilesInTab_WindowsExist_ShowsNewWindowWithBlankTab)
{
    // Arrange：已有窗口 → 新建并 show
    obj->m_windows << qobjFake<Window>();
    Window *newWin = qobjFake<Window>();
    int showCalls = 0;
    int blankCalls = 0;
    stub.set_lamda(static_cast<Window *(StartManager::*)(bool)>(&StartManager::createWindow),
                   [newWin](StartManager *, bool) -> Window * { return newWin; });
    stub.set_lamda(static_cast<void (QWidget::*)()>(&QWidget::show),
                   [&showCalls](QWidget *) { ++showCalls; });
    stub.set_lamda(static_cast<void (Window::*)()>(&Window::addBlankTab),
                   [&blankCalls](Window *) { ++blankCalls; });

    // Act
    obj->openFilesInTab(QStringList());

    // Assert：直接 show（非 showCenterWindow）+ 空白标签
    EXPECT_EQ(showCalls, 1);
    EXPECT_EQ(showCenterCalls, 0);
    EXPECT_EQ(blankCalls, 1);
}

TEST_F(StartManagerTest, OpenFilesInTab_FileAlreadyOpen_SkipsToNextFile)
{
    // Arrange：首文件已打开（checkPath=false）→ 处理次文件
    Window *win = qobjFake<Window>();
    obj->m_windows << win;
    QString opened = tmp->filePath("opened_tab.cpp");
    EditWrapper *fakeWrapper = zeroFake<EditWrapper>();
    TextEdit *fakeTextEdit = zeroFake<TextEdit>();
    fakeTextEdit->m_sFilePath = opened;
    stub.set_lamda(static_cast<EditWrapper *(Window::*)(const QString &)>(&Window::wrapper),
                   [opened, fakeWrapper](Window *, const QString &f) -> EditWrapper * {
                       return f == opened ? fakeWrapper : nullptr;
                   });
    stub.set_lamda(static_cast<TextEdit *(EditWrapper::*)()>(&EditWrapper::textEditor),
                   [fakeTextEdit](EditWrapper *) -> TextEdit * { return fakeTextEdit; });
    stubWindowInteraction(win);
    // 覆盖默认 getTabIndex：仅已打开文件命中索引 0，其余文件 -1（走 addTab 路径）
    stub.set_lamda(static_cast<int (Window::*)(const QString &)>(&Window::getTabIndex),
                   [win, opened](Window *self, const QString &f) -> int {
                       return (self == win && f == opened) ? 0 : -1;
                   });
    QString addedPath;
    stub.set_lamda(static_cast<void (Window::*)(const QString &, bool)>(&Window::addTab),
                   [&addedPath](Window *, const QString &p, bool) { addedPath = p; });

    // Act：列表 = [已打开文件, 新文件]
    obj->openFilesInTab(QStringList { opened, "fresh.cpp" });

    // Assert：已打开文件触发激活（popup），新文件在首窗 addTab
    EXPECT_EQ(activeTabCalls, 1);
    EXPECT_EQ(addedPath, QString("fresh.cpp"));
}

TEST_F(StartManagerTest, OpenFilesInTab_FirstFileNoWindow_DelayedOpen)
{
    // Arrange：无窗口 + 新文件 → singleShot 延迟打开（context 需合法 QObject 化 Window）
    Window *newWin = qobjFake<Window>();
    int recoverCalls = 0;
    QString addedPath;
    stub.set_lamda(static_cast<Window *(StartManager::*)(bool)>(&StartManager::createWindow),
                   [newWin](StartManager *, bool) -> Window * { return newWin; });
    stubWindowInteraction(newWin);
    stub.set_lamda(static_cast<int (StartManager::*)(Window *)>(&StartManager::recoverFile),
                   [&recoverCalls](StartManager *, Window *) -> int {
                       ++recoverCalls;
                       return 0;
                   });
    stub.set_lamda(static_cast<void (Window::*)(const QString &, bool)>(&Window::addTab),
                   [&addedPath](Window *, const QString &p, bool) { addedPath = p; });

    // Act
    obj->openFilesInTab(QStringList { "delayed.cpp" });
    // 立即断言：窗口创建并居中，延迟任务未执行
    EXPECT_EQ(showCenterCalls, 1);
    EXPECT_TRUE(lastCenterFlag);
    EXPECT_EQ(recoverCalls, 0);

    // 等待 50ms singleShot 触发（事件循环驱动）
    processEventsFor(200);

    // Assert：延迟打开发生（recover + addTab）
    EXPECT_EQ(recoverCalls, 1);
    EXPECT_EQ(addedPath, QString("delayed.cpp"));
}

TEST_F(StartManagerTest, OpenFilesInTab_ExistingWindow_AddsTabAndActivates)
{
    // Arrange：已有窗口（非首文件路径）→ 首窗 addTab + dock 激活失败 → Qt 激活兜底
    dockActivate = false;
    Window *win = qobjFake<Window>();
    obj->m_windows << win;
    stubWindowInteraction(win);   // getTabIndex=-1（stubTabIndex 设 -1）
    stubTabIndex = -1;
    stub.set_lamda(static_cast<EditWrapper *(Window::*)(const QString &)>(&Window::wrapper),
                   [](Window *, const QString &) -> EditWrapper * { return nullptr; });
    QString addedPath;
    stub.set_lamda(static_cast<void (Window::*)(const QString &, bool)>(&Window::addTab),
                   [&addedPath](Window *, const QString &p, bool) { addedPath = p; });

    // Act
    obj->openFilesInTab(QStringList { "plain.cpp" });

    // Assert：首窗 addTab + 手动激活（dock 失败兜底）
    EXPECT_EQ(addedPath, QString("plain.cpp"));
    EXPECT_EQ(activateWindowCalls, 1);
}

// ============================================================
// createWindowFromWrapper（drop 拖出动画）
// ============================================================

// 该方法用例公共桩：createWindow 拦截 + 屏幕几何 + 光标 + 窗口交互
void StartManagerTest::installDropStubs(QPoint cursorPos)
{
    dropCursorPos = cursorPos;
    dropWin = qobjFake<Window>();   // QPropertyAnimation target 需要 QObject 语境
    dropScreen = zeroFake<QScreen>();
    stub.set_lamda(static_cast<Window *(StartManager::*)(bool)>(&StartManager::createWindow),
                   [this](StartManager *, bool) -> Window * { return dropWin; });
    stub.set_lamda(&QGuiApplication::primaryScreen,
                   [this]() -> QScreen * { return dropScreen; });
    stub.set_lamda(static_cast<QRect (QScreen::*)() const>(&QScreen::availableGeometry),
                   [](QScreen *) -> QRect { return QRect(0, 0, 1920, 1080); });
    stub.set_lamda(static_cast<QPoint (*)()>(&QCursor::pos),
                   [this]() -> QPoint { return dropCursorPos; });
    stub.set_lamda(static_cast<QRect (QWidget::*)() const>(&QWidget::rect),
                   [](QWidget *) -> QRect { return QRect(0, 0, 400, 300); });
    stub.set_lamda(static_cast<void (QWidget::*)(int, int)>(&QWidget::move),
                   [this](QWidget *, int x, int y) {
                       ++dropMoveCalls;
                       lastDropPos = QPoint(x, y);
                   });
    stub.set_lamda(static_cast<void (QWidget::*)(const QPoint &)>(&QWidget::move),
                   [this](QWidget *, const QPoint &pt) {
                       ++dropMoveCalls;
                       lastDropPos = pt;
                   });
    stub.set_lamda(static_cast<void (QWidget::*)()>(&QWidget::show),
                   [this](QWidget *) { ++dropShowCalls; });
    stub.set_lamda(static_cast<void (Window::*)(bool)>(&Window::showCenterWindow),
                   [this](Window *, bool) { ++dropCenterCalls; });
    stub.set_lamda(
        static_cast<void (Window::*)(EditWrapper *, const QString &, const QString &, const QString &, int)>(
            &Window::addTabWithWrapper),
        [this](Window *, EditWrapper *, const QString &fp, const QString &, const QString &, int) {
            ++dropAddTabCalls;
            lastDropFilePath = fp;
        });
    stub.set_lamda(static_cast<EditWrapper *(Window::*)()>(&Window::currentWrapper),
                   [this](Window *) -> EditWrapper * { return dropWrapper; });
    stub.set_lamda(static_cast<void (EditWrapper::*)(bool)>(&EditWrapper::updateModifyStatus),
                   [this](EditWrapper *, bool m) { lastDropModified = m; });
    stub.set_lamda(static_cast<void (EditWrapper::*)()>(&EditWrapper::OnUpdateHighlighter),
                   [](EditWrapper *) {});
    stub.set_lamda(static_cast<void (QWidget::*)()>(&QWidget::setFocus),
                   [](QWidget *) {});
    stub.set_lamda(static_cast<void (Window::*)(EditWrapper *)>(&Window::setFontSizeWithConfig),
                   [](Window *, EditWrapper *) {});
    Tabbar::sm_pDragPixmap = nullptr;   // 拖拽 pixmap 空 → 回退窗口尺寸（B56 false 侧）
}

TEST_F(StartManagerTest, CreateWindowFromWrapper_WithinScreen_PlaysDropAnimation)
{
    // Arrange：光标在屏内、buffer 存活
    installDropStubs(QPoint(100, 100));
    dropWrapper = qobjFake<EditWrapper>();   // QPointer guard 需合法 QObject

    // Act
    obj->createWindowFromWrapper("tab", tmp->filePath("drop.cpp"),
                                 tmp->filePath("drop_true.cpp"), dropWrapper, true);

    // Assert（动画前）：窗口 move 到光标位置
    EXPECT_EQ(dropMoveCalls, 1);
    EXPECT_EQ(lastDropPos, QPoint(100, 100));

    // 等待动画完成（200ms InCubic + finished 回调）
    processEventsFor(400);

    // Assert（动画后）：窗口显示 + wrapper 挂载到新窗 + 修改状态透传
    EXPECT_GE(dropShowCalls, 1);
    EXPECT_EQ(dropCenterCalls, 1);
    EXPECT_EQ(dropAddTabCalls, 1);
    EXPECT_EQ(lastDropFilePath, tmp->filePath("drop.cpp"));
    EXPECT_TRUE(lastDropModified);
}

TEST_F(StartManagerTest, CreateWindowFromWrapper_BufferDestroyedDuringAnimation_DropsTearOff)
{
    // Arrange：动画期间销毁 buffer → finished 回调放弃挂载
    installDropStubs(QPoint(50, 50));
    QObject *bufferObj = new QObject;
    EditWrapper *buffer = reinterpret_cast<EditWrapper *>(bufferObj);

    // Act
    obj->createWindowFromWrapper("tab", "path", "true", buffer, false);
    // 动画进行中（200ms）销毁 buffer
    processEventsFor(80);
    delete bufferObj;
    processEventsFor(400);

    // Assert：未挂载 wrapper、未显示新窗内容（防御性回退）
    EXPECT_EQ(dropAddTabCalls, 0);
    EXPECT_EQ(dropShowCalls, 0);
}

TEST_F(StartManagerTest, CreateWindowFromWrapper_CursorBeyondScreen_ClampsPosition)
{
    // Arrange：光标 x/y 均超出屏幕（400x300 窗 + 1920x1080 屏）
    installDropStubs(QPoint(1900, 1100));

    // Act
    obj->createWindowFromWrapper("t", "p", "tp", qobjFake<EditWrapper>(), false);
    processEventsFor(50);

    // Assert：位置截断到 屏宽-窗宽 / 屏高-窗高
    EXPECT_EQ(dropMoveCalls, 1);
    EXPECT_EQ(lastDropPos, QPoint(1920 - 400, 1080 - 300));
}

TEST_F(StartManagerTest, CreateWindowFromWrapper_NegativeCursor_ClampsToZero)
{
    // Arrange：光标为负坐标
    installDropStubs(QPoint(-100, -50));

    // Act
    obj->createWindowFromWrapper("t", "p", "tp", qobjFake<EditWrapper>(), false);
    processEventsFor(50);

    // Assert：截断到 (0, 0)
    EXPECT_EQ(dropMoveCalls, 1);
    EXPECT_EQ(lastDropPos, QPoint(0, 0));
}

// ============================================================
// createWindow（真实执行：Window 构造入口 patch）
// ============================================================

TEST_F(StartManagerTest, CreateWindow_FirstWindow_AppendedAndCentered)
{
    // Arrange：patch Window 构造入口（dlsym 精确定位，placement-new QObject 最小初始化）
    void *windowCtor = locateWindowCtor();
    ASSERT_NE(windowCtor, nullptr) << "Window 构造符号未找到";
    stub.set(windowCtor, reinterpret_cast<void (*)(Window *)>(&fakeWindowCtor));
    int moveCalls = 0;
    stub.set_lamda(static_cast<void (QWidget::*)(int, int)>(&QWidget::move),
                   [&moveCalls](QWidget *, int, int) { ++moveCalls; });
    Tabbar *fakeTabbar = qobjFake<Tabbar>();   // connect sender 语境
    stub.set_lamda(static_cast<Tabbar *(Window::*)()>(&Window::getTabbar),
                   [fakeTabbar](Window *) -> Tabbar * { return fakeTabbar; });

    // Act：真实执行 createWindow（alwaysCenter=true → 居中分支不 move）
    Window *win = obj->createWindow(true);

    // Assert：窗口入列表、返回非空、未做偏移移动
    ASSERT_NE(win, nullptr);
    EXPECT_EQ(obj->m_windows.count(), 1);
    EXPECT_EQ(obj->m_windows.at(0), win);
    EXPECT_EQ(moveCalls, 0);

    // Cleanup：仅回收裸内存（不触发 QWidget 析构链）。
    // 注：QObject 元系统在 QObject 化 fake 窗口上 connect 失败时仍有部分注册写入，
    // 后续 ~StartManager 的 QObject 析构不安全 → 本用例对 obj 免析构（泄漏交由进程回收）
    ::operator delete(win);
    obj->m_windows.clear();
    delete obj->m_pTimer;
    ::operator delete(obj);
    obj = nullptr;
}

TEST_F(StartManagerTest, CreateWindow_SubsequentWindow_MovedByOffset)
{
    // Arrange：已有 1 窗口 → 偏移分支
    obj->m_windows << qobjFake<Window>();
    stub.set(locateWindowCtor(), reinterpret_cast<void (*)(Window *)>(&fakeWindowCtor));
    QScreen *fakeScreen = zeroFake<QScreen>();
    stub.set_lamda(&QGuiApplication::primaryScreen,
                   [fakeScreen]() -> QScreen * { return fakeScreen; });
    stub.set_lamda(static_cast<QRect (QScreen::*)() const>(&QScreen::availableGeometry),
                   [](QScreen *) -> QRect { return QRect(0, 0, 1920, 1080); });
    int moveCalls = 0;
    int lastX = -1, lastY = -1;
    stub.set_lamda(static_cast<void (QWidget::*)(int, int)>(&QWidget::move),
                   [&moveCalls, &lastX, &lastY](QWidget *, int x, int y) {
                       ++moveCalls;
                       lastX = x;
                       lastY = y;
                   });
    Tabbar *fakeTabbar = qobjFake<Tabbar>();
    stub.set_lamda(static_cast<Tabbar *(Window::*)()>(&Window::getTabbar),
                   [fakeTabbar](Window *) -> Tabbar * { return fakeTabbar; });

    // Act
    Window *win = obj->createWindow(false);

    // Assert：偏移 1*50 移动 + 窗口列表追加
    ASSERT_NE(win, nullptr);
    EXPECT_EQ(obj->m_windows.count(), 2);
    EXPECT_EQ(moveCalls, 1);
    EXPECT_EQ(lastX, 50);
    EXPECT_EQ(lastY, 50);

    // Cleanup：同上，obj 免析构
    ::operator delete(win);
    obj->m_windows.clear();
    delete obj->m_pTimer;
    ::operator delete(obj);
    obj = nullptr;
}

// ============================================================
// slotCheckUnsaveTab
// ============================================================

TEST_F(StartManagerTest, SlotCheckUnsaveTab_UnsavedTabExists_BlocksShutdown)
{
    // Arrange：两个窗口均有未保存标签（第一个即命中应早退）
    obj->m_windows << qobjFake<Window>() << qobjFake<Window>();
    int checkCalls = 0;
    stub.set_lamda(static_cast<bool (Window::*)()>(&Window::checkBlockShutdown),
                   [&checkCalls](Window *) -> bool {
                       ++checkCalls;
                       return true;
                   });
    int dbusBase = dbusCalls;

    // Act
    obj->slotCheckUnsaveTab();

    // Assert：发起 Inhibit 阻塞 + 仅检查首个窗口（早退）
    EXPECT_EQ(dbusCalls - dbusBase, 1);
    EXPECT_EQ(lastDbusMethod, QString("Inhibit"));
    EXPECT_EQ(checkCalls, 1);
}

TEST_F(StartManagerTest, SlotCheckUnsaveTab_AllTabsSaved_NoDbusCall)
{
    // Arrange：窗口无未保存标签
    obj->m_windows << qobjFake<Window>();
    stub.set_lamda(static_cast<bool (Window::*)()>(&Window::checkBlockShutdown),
                   [](Window *) -> bool { return false; });
    int dbusBase = dbusCalls;

    // Act
    obj->slotCheckUnsaveTab();

    // Assert：不发起阻塞 + valid reply 被清空（B68 true 侧）
    EXPECT_EQ(dbusCalls - dbusBase, 0);
    EXPECT_FALSE(obj->m_reply.isValid());
}

// ============================================================
// closeAboutForWindow
// ============================================================

TEST_F(StartManagerTest, CloseAboutForWindow_DialogNull_NoClose)
{
    // Arrange：aboutDialog 为空
    stub.set_lamda(static_cast<DAboutDialog *(DApplication::*)()>(&DApplication::aboutDialog),
                   [](DApplication *) -> DAboutDialog * { return nullptr; });
    int closeCalls = 0;
    stub.set_lamda(static_cast<bool (QWidget::*)()>(&QWidget::close),
                   [&closeCalls](QWidget *) -> bool {
                       ++closeCalls;
                       return true;
                   });

    // Act
    obj->closeAboutForWindow(qobjFake<Window>());

    // Assert：不关闭 + 窗口列表状态保持
    EXPECT_EQ(closeCalls, 0);
    EXPECT_TRUE(obj->m_windows.isEmpty());
}

TEST_F(StartManagerTest, CloseAboutForWindow_ParentNull_NoClose)
{
    // Arrange：对话框存在但无 parent
    DAboutDialog *dialog = zeroFake<DAboutDialog>();
    stub.set_lamda(static_cast<DAboutDialog *(DApplication::*)()>(&DApplication::aboutDialog),
                   [dialog](DApplication *) -> DAboutDialog * { return dialog; });
    stub.set_lamda(static_cast<QObject *(QObject::*)() const>(&QObject::parent),
                   [](QObject *) -> QObject * { return nullptr; });
    int closeCalls = 0;
    stub.set_lamda(static_cast<bool (QWidget::*)()>(&QWidget::close),
                   [&closeCalls](QWidget *) -> bool {
                       ++closeCalls;
                       return true;
                   });

    // Act
    obj->closeAboutForWindow(qobjFake<Window>());

    // Assert：不关闭 + 窗口列表状态保持
    EXPECT_EQ(closeCalls, 0);
    EXPECT_TRUE(obj->m_windows.isEmpty());
}

TEST_F(StartManagerTest, CloseAboutForWindow_OtherParent_NoClose)
{
    // Arrange：对话框 parent 是其他窗口
    DAboutDialog *dialog = zeroFake<DAboutDialog>();
    QObject *otherParent = new QObject;
    ownedObjs.push_back(otherParent);
    stub.set_lamda(static_cast<DAboutDialog *(DApplication::*)()>(&DApplication::aboutDialog),
                   [dialog](DApplication *) -> DAboutDialog * { return dialog; });
    stub.set_lamda(static_cast<QObject *(QObject::*)() const>(&QObject::parent),
                   [otherParent](QObject *) -> QObject * { return otherParent; });
    int closeCalls = 0;
    stub.set_lamda(static_cast<bool (QWidget::*)()>(&QWidget::close),
                   [&closeCalls](QWidget *) -> bool {
                       ++closeCalls;
                       return true;
                   });

    // Act
    obj->closeAboutForWindow(qobjFake<Window>());

    // Assert：不关闭（parent 非目标窗口）+ 窗口列表状态保持
    EXPECT_EQ(closeCalls, 0);
    EXPECT_TRUE(obj->m_windows.isEmpty());
}

TEST_F(StartManagerTest, CloseAboutForWindow_MatchingParent_ClosesDialog)
{
    // Arrange：对话框 parent 即目标窗口
    DAboutDialog *dialog = zeroFake<DAboutDialog>();
    Window *win = qobjFake<Window>();
    stub.set_lamda(static_cast<DAboutDialog *(DApplication::*)()>(&DApplication::aboutDialog),
                   [dialog](DApplication *) -> DAboutDialog * { return dialog; });
    stub.set_lamda(static_cast<QObject *(QObject::*)() const>(&QObject::parent),
                   [win](QObject *) -> QObject * { return win; });
    int closeCalls = 0;
    stub.set_lamda(static_cast<bool (QWidget::*)()>(&QWidget::close),
                   [&closeCalls](QWidget *) -> bool {
                       ++closeCalls;
                       return true;
                   });

    // Act
    obj->closeAboutForWindow(win);

    // Assert：对话框被关闭 + 窗口列表状态保持
    EXPECT_EQ(closeCalls, 1);
    EXPECT_TRUE(obj->m_windows.isEmpty());
}

// ============================================================
// slotCreatNewwindow / slotCloseWindow
// ============================================================

TEST_F(StartManagerTest, SlotCreatNewwindow_DelegatesToOpenFilesInWindow)
{
    // Arrange：拦截同类方法验证委托（空列表触发新窗）
    int openWinCalls = 0;
    QStringList received;
    stub.set_lamda(static_cast<void (StartManager::*)(QStringList)>(&StartManager::openFilesInWindow),
                   [&openWinCalls, &received](StartManager *, QStringList files) {
                       ++openWinCalls;
                       received = files;
                   });

    // Act
    obj->slotCreatNewwindow();

    // Assert：委托一次且参数为空列表
    EXPECT_EQ(openWinCalls, 1);
    EXPECT_TRUE(received.isEmpty());
}

TEST_F(StartManagerTest, SlotCloseWindow_SenderInList_RemovedFromList)
{
    // Arrange：sender 为第二个窗口
    Window *winA = qobjFake<Window>();
    Window *winB = qobjFake<Window>();
    obj->m_windows << winA << winB;
    stub.set_lamda(static_cast<QObject *(QObject::*)() const>(&QObject::sender),
                   [winB](const QObject *) -> QObject * { return winB; });
    curPathOverride = tmp->filePath("no_such_dir");   // 剩余窗口非空 → 不走清退

    // Act
    obj->slotCloseWindow();

    // Assert：仅移除该窗口，剩余列表正确
    EXPECT_EQ(obj->m_windows.count(), 1);
    EXPECT_EQ(obj->m_windows.at(0), winA);
}

TEST_F(StartManagerTest, SlotCloseWindow_LastWindow_CleansUpAndSchedulesQuit)
{
    // Arrange：sender 移除后窗口清空 → 清退路径；工作目录含 tabPaths.txt
    Window *win = qobjFake<Window>();
    obj->m_windows << win;
    stub.set_lamda(static_cast<QObject *(QObject::*)() const>(&QObject::sender),
                   [win](const QObject *) -> QObject * { return win; });
    curPathOverride = tmp->path();
    QString stale = QDir(tmp->path()).filePath("tabPaths.txt");
    QFile f(stale);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.close();
    int setValueBase = setValueCalls;
    int removeCalls = 0;
    stub.set_lamda(static_cast<bool (QFile::*)()>(&QFile::remove),
                   [&removeCalls](QFile *) -> bool {
                       ++removeCalls;
                       return true;
                   });

    // Act
    obj->slotCloseWindow();

    // Assert：窗口列表空 + 书签配置写回 + tabPaths 清理发起 + DBus 服务注销
    EXPECT_TRUE(obj->m_windows.isEmpty());
    EXPECT_GT(setValueCalls, setValueBase);
    // 源码缺陷：QFile file(name) 以相对名删除，实际删除落点依赖进程 CWD 而非
    // entryList 所属目录（与 stub 的 currentPath 无关，文件引擎走系统 getcwd）。
    // 此处断言 remove 被发起（缺陷行为记录于 defects，不改源码）
    EXPECT_EQ(removeCalls, 1);
    EXPECT_EQ(unregisterCalls, 1);
    EXPECT_EQ(lastUnregisterService, QString("com.deepin.Editor"));
    EXPECT_EQ(quitCalls, 0);   // 延迟 1000ms 退出尚未触发

    // 等待延迟退出任务
    processEventsFor(1200);

    // Assert：延迟退出执行（quit 被拦截计数）+ 内存释放定时器启动
    EXPECT_EQ(quitCalls, 1);
    EXPECT_TRUE(obj->m_FreeMemTimer.isActive());
}

TEST_F(StartManagerTest, SlotCloseWindow_WorkingDirMissing_EarlyReturn)
{
    // Arrange：sender 不在列表（移除分支不触发）+ 窗口已空 + 工作目录不存在
    stub.set_lamda(static_cast<QObject *(QObject::*)() const>(&QObject::sender),
                   [](const QObject *) -> QObject * { return nullptr; });
    curPathOverride = tmp->filePath("no_such_dir");
    int setValueBase = setValueCalls;

    // Act
    obj->slotCloseWindow();

    // Assert：saveBookmark 已执行但提前返回（未注销 DBus、未调度退出）
    EXPECT_GT(setValueCalls, setValueBase);
    EXPECT_EQ(unregisterCalls, 0);
    EXPECT_EQ(quitCalls, 0);
}

// ============================================================
// saveBookmark（private：直接调用，-fno-access-control）
// ============================================================

TEST_F(StartManagerTest, SaveBookmark_FileMissingOrEmpty_DropsRecord)
{
    // Arrange：存在的文件 + 空书签；不存在的文件 + 非空书签
    QString exists = tmp->filePath("alive.cpp");
    QFile f(exists);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.close();
    obj->m_bookmarkTable.insert(exists, QList<int>());                    // 空书签 → 丢弃
    obj->m_bookmarkTable.insert(tmp->filePath("gone.cpp"), QList<int> { 1 });   // 文件不存在（fileInfoExists 桩=true 全命中，改用真实删除语义）
    // 注：fileInfoExists=true 时"文件不存在"分支需真实缺失路径配合 QFileInfo::exists 桩关闭
    stub.reset(static_cast<bool (QFileInfo::*)() const>(&QFileInfo::exists));
    stub.reset(static_cast<bool (*)(const QString &)>(&QFileInfo::exists));
    int setValueBase = setValueCalls;

    // Act
    obj->saveBookmark();

    // Assert：两条记录均被清除，写入空列表
    EXPECT_TRUE(obj->m_bookmarkTable.isEmpty());
    EXPECT_EQ(setValueCalls, setValueBase + 1);
    EXPECT_TRUE(lastSetValue.toStringList().isEmpty());
}

TEST_F(StartManagerTest, SaveBookmark_ValidRecord_SerializedToConfig)
{
    // Arrange：真实存在的文件 + 书签
    QString exists = tmp->filePath("keep.cpp");
    QFile f(exists);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.close();
    obj->m_bookmarkTable.insert(exists, QList<int> { 3, 9 });
    stub.reset(static_cast<bool (QFileInfo::*)() const>(&QFileInfo::exists));
    stub.reset(static_cast<bool (*)(const QString &)>(&QFileInfo::exists));
    int setValueBase = setValueCalls;

    // Act
    obj->saveBookmark();

    // Assert：序列化为 JSON 记录（localPath + bookmark 串）
    EXPECT_EQ(setValueCalls, setValueBase + 1);
    QStringList written = lastSetValue.toStringList();
    ASSERT_EQ(written.count(), 1);
    QJsonObject json = QJsonDocument::fromJson(written.at(0).toUtf8()).object();
    EXPECT_EQ(json.value("localPath").toString(), exists);
    EXPECT_EQ(json.value("bookmark").toString(), QString("3,9"));
}

// ============================================================
// initBookmark（private：经构造覆盖，派生 fixture 注入配置）
// ============================================================

class StartManagerInitBookmarkTest : public StartManagerTest {
protected:
    void customizeConfig() override
    {
        // 还原 QFileInfo::exists 真实语义（文件存在性由真实临时文件控制）
        stub.reset(static_cast<bool (QFileInfo::*)() const>(&QFileInfo::exists));
        stub.reset(static_cast<bool (*)(const QString &)>(&QFileInfo::exists));

        QString existing = tmp->filePath("existing.cpp");
        QFile f(existing);
        if (f.open(QIODevice::WriteOnly))
            f.close();

        QJsonObject valid;
        valid.insert("localPath", existing);
        valid.insert("bookmark", "4,6");

        QJsonObject missingFile;
        missingFile.insert("localPath", tmp->filePath("gone.cpp"));
        missingFile.insert("bookmark", "1");

        QJsonObject emptyBookmark;
        emptyBookmark.insert("localPath", existing);
        emptyBookmark.insert("bookmark", "");

        settingsValue = QVariant(QStringList {
            QString::fromUtf8(QJsonDocument(valid).toJson(QJsonDocument::Compact)),
            QString::fromUtf8(QJsonDocument(missingFile).toJson(QJsonDocument::Compact)),
            QString::fromUtf8(QJsonDocument(emptyBookmark).toJson(QJsonDocument::Compact)),
            "{ not a valid json" });

        existingPath = existing;
    }

    QString existingPath;
};

TEST_F(StartManagerInitBookmarkTest, InitBookmark_Variants_LoadsOnlyExistingValid)
{
    // Assert：仅"文件存在且书签非空"的记录被缓存
    QList<int> found = obj->findBookmark(existingPath);
    EXPECT_EQ(found, QList<int>({ 4, 6 }));
    EXPECT_EQ(obj->m_bookmarkTable.count(), 1);
}
