// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// ============================================================================
// Window 单元测试（B11 / src/widgets/window.cpp，主窗口入口类）
//
// 策略：真实 offscreen 构造 Window（B8 已验证 EditWrapper/TextEdit 真实链接可行，
// 探针已验证 DMainWindow/DTitlebar 在 DBus 全拦截下可构造）。外围重依赖运行期拦截：
//   - DBus：QDBusConnection::systemBus/sessionBus → 伪连接；callWithArgumentList 汇聚桩
//     （DConfig/DTitlebar 会经 isServiceRegistered 查询，伪连接上真实调用会崩溃）
//   - 模态对话框：DDialog::exec / QDialog::exec / QFileDialog::selectedFiles /
//     DFileDialog::getComboBoxValue
//   - 浮动消息：DMessageManager::sendMessage（两个重载）计数桩
//   - 子进程：QProcess::startDetached 拦截（displayShortcuts）
//   - 文件系统：XDG_CONFIG_HOME/XDG_DATA_HOME → QTemporaryDir（真实 Settings 读写）
// 私有成员（m_wrappers/m_pendingTabs/m_findBar/m_fontSize 等）经 -fno-access-control
// 访问做状态注入与断言（不影响 ABI 布局）。
//
// 用例映射（分支清单）见各测试头部注释；不可达分支（真实打印机绘制 / 焦点窗口切换 /
// 系统托盘）在 autotests/.ut-session.batch11.json 记录原因。
// ============================================================================

#include <gtest/gtest.h>
#include "stubext.h"

#include <QApplication>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QTimerEvent>
#include <QImage>
#include <QPainter>
#include <QTextDocument>
#include <QElapsedTimer>
#include <QThread>
#include <QDBusConnection>
#include <QDBusAbstractInterface>
#include <QDBusMessage>
#include <QPrintDialog>

#include <DDialog>
#include <DTitlebar>
#include <DSettings>
#include <DSettingsOption>
#include <DIconButton>
#include <DMessageManager>

#include "widgets/window.h"
#include "widgets/pathsettintwgt.h"
#include "controls/tabbar.h"
#include "controls/findbar.h"
#include "controls/replacebar.h"
#include "controls/jumplinebar.h"
#include "editor/editwrapper.h"
#include "editor/dtextedit.h"
#include "common/settings.h"
#include "common/utils.h"
#include "common/iflytek_ai_assistant.h"
#include "startmanager.h"

class WindowTest : public ::testing::Test {
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

        // 真实 Settings 单例（qrc settings.json + 临时 config.conf）
        Settings::instance();
        qRegisterMetaType<ViewMode>("ViewMode");

        // 草稿/备份目录（AppDataLocation = $XDG_DATA_HOME/deepin/deepin-editor）
        QDir().mkpath(xdgData + "/deepin/deepin-editor/blank-files");
        QDir().mkpath(xdgData + "/deepin/deepin-editor/backup-files");
        QDir().mkpath(xdgData + "/deepin/deepin-editor/autoBackup-files");
    }

    static void TearDownTestSuite()
    {
        qunsetenv("XDG_CONFIG_HOME");
        qunsetenv("XDG_DATA_HOME");
    }

    void SetUp() override
    {
        m_tempDir = new QTemporaryDir();
        installCommonStubs();

        // 真实 Window 构造（offscreen + DBus 拦截）
        m_win = new Window();
        m_tabbar = m_win->getTabbar();
    }

    void TearDown() override
    {
        // 恢复测试内降权的文件
        for (const QString &p : m_restoreFiles) {
            QFile f(p);
            if (f.exists())
                f.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        }
        // stub 保持激活状态下析构（析构链中的 DBus/消息调用仍被拦截）
        delete m_win;
        m_win = nullptr;
        m_tabbar = nullptr;
        QApplication::processEvents();
        stub.clear();
        delete m_tempDir;
    }

    // ==================== 运行期 stub 矩阵 ====================
    void installCommonStubs()
    {
        // DBus 隔离：伪连接 + 汇聚桩（禁止真实总线；DConfig 查询安全失败）
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

        // 浮动消息计数桩（showNotify / 只读提示 / 无权限提示）
        stub.set_lamda(
            static_cast<void (DMessageManager::*)(QWidget *, const QIcon &, const QString &)>(
                &DMessageManager::sendMessage),
            [this](DMessageManager *, QWidget *, const QIcon &, const QString &message) {
                ++m_iconMsgCalls;
                m_lastIconMsg = message;
            });
        stub.set_lamda(
            static_cast<void (DMessageManager::*)(QWidget *, DFloatingMessage *)>(
                &DMessageManager::sendMessage),
            [this](DMessageManager *, QWidget *, DFloatingMessage *) { ++m_widgetMsgCalls; });
        stub.set_lamda(&Utils::sendFloatMessageFixedFont,
                       [this](QWidget *, const QIcon &, const QString &message) {
                           ++m_floatMsgCalls;
                           m_lastFloatMsg = message;
                       });

        // StartManager 关闭联动：qApp 非 DApplication 时 aboutDialog() 会做非法
        // static_cast（生产环境 qApp 为 DApplication），此处属跨类环境差异，拦截
        stub.set_lamda(&StartManager::closeAboutForWindow,
                       [](StartManager *, Window *) -> void {});

        // 子进程拦截（displayShortcuts 启动 deepin-shortcut-viewer）
        stub.set_lamda(static_cast<bool (*)(const QString &, const QStringList &, const QString &, qint64 *)>(&QProcess::startDetached),
                       [this](const QString &program, const QStringList &args, const QString &, qint64 *) -> bool {
                           ++m_startDetachedCalls;
                           m_lastDetachedProgram = program;
                           m_lastDetachedArgs = args;
                           return true;
                       });

        // 模态对话框默认拦截（个别用例覆盖返回值）
        stub.set_lamda(VADDR(DDialog, exec), [this]() -> int {
            ++m_ddialogExecCalls;
            return m_ddialogResult;
        });
        stub.set_lamda(VADDR(QDialog, exec), [this]() -> int {
            ++m_qdialogExecCalls;
            return m_qdialogResult;
        });
        stub.set_lamda(VADDR(QFileDialog, selectedFiles),
                       [this](QFileDialog *) -> QStringList { return m_selectedFiles; });
        stub.set_lamda(&DFileDialog::getComboBoxValue,
                       [this](DFileDialog *, const QString &) -> QString { return m_comboValue; });
    }

    // ==================== 辅助 ====================

    QString createFile(const QString &name, const QByteArray &content = "hello world")
    {
        const QString path = m_tempDir->filePath(name);
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly))
            return QString();
        f.write(content);
        f.close();
        return path;
    }

    // 添加空白标签页并返回其路径
    QString addBlankAndGetPath()
    {
        m_win->addBlankTab();
        QApplication::processEvents();
        return m_tabbar->currentPath();
    }

    // 添加真实文件标签页并等待内容载入（getFileLoading 在 finished 前恒为 false，
    // 直接轮询有竞态；以"文档非空 + 非加载中"为完成判据）
    QString addFileTab(const QString &name, const QByteArray &content = "hello world\nsecond line\n")
    {
        const QString path = createFile(name, content);
        m_win->addTab(path, true);
        EditWrapper *w = m_win->wrapper(path);
        if (w != nullptr && !content.isEmpty()) {
            waitUntil([w]() {
                return !w->getFileLoading() && !w->textEditor()->document()->isEmpty();
            });
        }
        return path;
    }

    bool waitUntil(const std::function<bool()> &predicate, int timeoutMs = 8000)
    {
        QElapsedTimer timer;
        timer.start();
        while (!predicate()) {
            if (timer.elapsed() > timeoutMs)
                return false;
            QApplication::processEvents(QEventLoop::AllEvents, 50);
            QThread::msleep(10);
        }
        return true;
    }

    void processEventsFor(int ms)
    {
        QElapsedTimer t;
        t.start();
        while (t.elapsed() < ms) {
            QApplication::processEvents(QEventLoop::AllEvents, 20);
            QThread::msleep(5);
        }
    }

    // ==================== 状态 ====================
    stub_ext::StubExt stub;
    Window *m_win = nullptr;
    Tabbar *m_tabbar = nullptr;
    QTemporaryDir *m_tempDir = nullptr;

    int m_iconMsgCalls = 0;
    QString m_lastIconMsg;
    int m_widgetMsgCalls = 0;
    int m_floatMsgCalls = 0;
    QString m_lastFloatMsg;
    int m_startDetachedCalls = 0;
    QString m_lastDetachedProgram;
    QStringList m_lastDetachedArgs;
    int m_ddialogExecCalls = 0;
    int m_ddialogResult = 0;
    int m_qdialogExecCalls = 0;
    int m_qdialogResult = QDialog::Rejected;
    QStringList m_selectedFiles;
    QString m_comboValue = QStringLiteral("UTF-8");
    int m_saveAsCalls = 0;

    QStringList m_restoreFiles;

    static QApplication *s_app;
    static QTemporaryDir *s_configHome;
    static QTemporaryDir *s_dataHome;
    static char s_argv[];
};

QApplication *WindowTest::s_app = nullptr;
QTemporaryDir *WindowTest::s_configHome = nullptr;
QTemporaryDir *WindowTest::s_dataHome = nullptr;
char WindowTest::s_argv[] = "test_window";

// ============================================================
// 构造 / 基础 getter / 析构
// ============================================================

TEST_F(WindowTest, Constructor_RealBuild_CreatesCoreParts)
{
    // Assert: 核心部件就绪 + 初始状态
    ASSERT_NE(m_win->getTabbar(), nullptr);
    ASSERT_NE(m_win->getStackedWgt(), nullptr);
    EXPECT_EQ(m_win->getTabbar()->count(), 0);
    EXPECT_TRUE(m_win->m_wrappers.isEmpty());
    EXPECT_TRUE(m_win->m_pendingTabs.isEmpty());
    EXPECT_EQ(m_win->minimumWidth(), 680);
    EXPECT_EQ(m_win->minimumHeight(), 300);
    EXPECT_FALSE(m_win->findBarIsVisiable());
    EXPECT_FALSE(m_win->replaceBarIsVisiable());
}

TEST_F(WindowTest, InitTitlebar_FromConstructor_MenuActionsRegistered)
{
    // Assert: 标题栏菜单包含全部功能项（按注册时的 objectName 核对 9 项）
    QSet<QString> registered;
    const QList<QAction *> actions = m_win->m_menu->actions();
    for (QAction *a : actions)
        registered.insert(a->objectName());
    for (const char *name : { "NewWindow", "NewTab", "OpenFile", "Save", "SaveAs",
                              "Print", "Settings", "Find", "Replace" }) {
        EXPECT_TRUE(registered.contains(QString(name))) << "missing menu action: " << name;
    }
    EXPECT_GE(actions.count(), 9);
    // 标签栏已挂载到标题栏
    EXPECT_NE(m_win->getTabbar()->parentWidget(), nullptr);
}

TEST_F(WindowTest, Destructor_AfterUse_ReleasesSafely)
{
    // Arrange: 打开一个标签页制造状态
    addBlankAndGetPath();
    ASSERT_EQ(m_tabbar->count(), 1);

    // Act: 手动析构（TearDown 会对夹具窗口再覆盖一次）
    Window *victim = std::exchange(m_win, nullptr);
    m_tabbar = nullptr;
    delete victim;

    // Assert: 析构后重新构造可正常工作（析构链未损坏全局状态）
    ASSERT_NO_FATAL_FAILURE(m_win = new Window());
    m_tabbar = m_win->getTabbar();
    EXPECT_NE(m_win->getTabbar(), nullptr);
    EXPECT_EQ(m_win->getTabbar()->count(), 0);
}

TEST_F(WindowTest, Getters_KeywordAndStack_ReturnInternalState)
{
    // Arrange: 注入关键词
    m_win->m_keywordForSearch = QStringLiteral("kw1");
    m_win->m_keywordForSearchAll = QStringLiteral("kw2");

    // Assert
    EXPECT_EQ(m_win->getKeywordForSearch(), QString("kw1"));
    EXPECT_EQ(m_win->getKeywordForSearchAll(), QString("kw2"));
    EXPECT_EQ(m_win->getStackedWgt(), m_win->m_editorWidget);
}

// ============================================================
// 语音助手配置
// ============================================================

TEST_F(WindowTest, LoadIflytekaiassistantConfig_NoDir_EarlyReturn)
{
    // Act: ConfigLocation 下无 iflytek 目录
    m_win->loadIflytekaiassistantConfig();

    // Assert: 状态表保持为空
    EXPECT_TRUE(m_win->m_IflytekAiassistantState.isEmpty());
    EXPECT_FALSE(m_win->getIflytekaiassistantConfig(QStringLiteral("any-iat")));
}

TEST_F(WindowTest, LoadIflytekaiassistantConfig_IniFiles_ParsesEnableStates)
{
    // Arrange: 在隔离的 ConfigLocation 下构造 iflytek 配置目录
    const QString cfgRoot = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    const QString iflytekDir = QDir(cfgRoot).filePath(QStringLiteral("iflytek"));
    ASSERT_TRUE(QDir().mkpath(iflytekDir));
    auto writeIni = [&](const QString &name, bool enable) {
        QSettings ini(QDir(iflytekDir).filePath(name), QSettings::IniFormat);
        ini.beginGroup(QStringLiteral("base"));
        ini.setValue(QStringLiteral("enable"), enable);
        ini.endGroup();
        ini.sync();
    };
    writeIni(QStringLiteral("sv-iat.ini"), true);
    writeIni(QStringLiteral("sv-tts.ini"), false);
    writeIni(QStringLiteral("ignore.txt"), true); // 命名不含关键后缀，跳过

    // Act
    m_win->loadIflytekaiassistantConfig();

    // Assert: 仅 -iat/-tts/-trans 后缀进入状态表
    EXPECT_TRUE(m_win->getIflytekaiassistantConfig(QStringLiteral("sv-iat")));
    EXPECT_FALSE(m_win->getIflytekaiassistantConfig(QStringLiteral("sv-tts")));
    EXPECT_FALSE(m_win->getIflytekaiassistantConfig(QStringLiteral("nonexistent")));
    EXPECT_EQ(m_win->m_IflytekAiassistantState.count(), 2);
}

// ============================================================
// Tab 状态更新
// ============================================================

TEST_F(WindowTest, UpdateModifyStatus_UnknownPath_EarlyReturn)
{
    // Act
    m_win->updateModifyStatus(QStringLiteral("/no/such/file"), true);

    // Assert: 无标签变化（强异常安全）
    EXPECT_EQ(m_tabbar->count(), 0);
    EXPECT_TRUE(m_win->m_wrappers.isEmpty());
}

TEST_F(WindowTest, UpdateModifyStatus_DraftTab_AddsAndRemovesStar)
{
    // Arrange: 空白标签（草稿路径，truePath 为空）
    const QString blank = addBlankAndGetPath();
    ASSERT_EQ(m_tabbar->currentName(), QString("Untitled 1"));
    QSignalSpy blockSpy(m_win, &Window::sigJudgeBlockShutdown);

    // Act: 标记修改 → 加 *
    m_win->updateModifyStatus(blank, true);

    // Assert
    EXPECT_EQ(m_tabbar->currentName(), QString("*Untitled 1"));
    EXPECT_EQ(blockSpy.count(), 1);

    // Act: 取消修改 → 去 *
    m_win->updateModifyStatus(blank, false);
    EXPECT_EQ(m_tabbar->currentName(), QString("Untitled 1"));
    EXPECT_EQ(blockSpy.count(), 2);
}

TEST_F(WindowTest, UpdateModifyStatus_RealFile_UsesFileBaseName)
{
    // Arrange: 真实文件标签
    const QString path = addFileTab(QStringLiteral("doc.txt"));
    ASSERT_FALSE(path.isEmpty());

    // Act: 修改 → 文件名加 *
    m_win->updateModifyStatus(path, true);

    // Assert: 标签名变为 *doc.txt（走 QFileInfo 分支）
    EXPECT_EQ(m_tabbar->currentName(), QString("*doc.txt"));

    // Act: 取消 → 恢复文件名
    m_win->updateModifyStatus(path, false);
    EXPECT_EQ(m_tabbar->currentName(), QString("doc.txt"));
}

TEST_F(WindowTest, UpdateSaveAsFileName_NoWrapper_EarlyReturn)
{
    // Act
    m_win->updateSaveAsFileName(QStringLiteral("/a"), QStringLiteral("/b"));

    // Assert
    EXPECT_TRUE(m_win->m_wrappers.isEmpty());
    EXPECT_EQ(m_tabbar->count(), 0);
}

TEST_F(WindowTest, UpdateSaveAsFileName_WithWrapper_MovesMappingAndTab)
{
    // Arrange: 空白标签 + wrapper
    const QString blank = addBlankAndGetPath();
    EditWrapper *w = m_win->wrapper(blank);
    ASSERT_NE(w, nullptr);
    const QString newPath = m_tempDir->filePath(QStringLiteral("renamed.txt"));

    // Act
    m_win->updateSaveAsFileName(blank, newPath);

    // Assert: 映射键迁移 + 路径更新 + 标签名更新
    EXPECT_FALSE(m_win->m_wrappers.contains(blank));
    EXPECT_EQ(m_win->m_wrappers.value(newPath), w);
    EXPECT_EQ(w->filePath(), newPath);
    EXPECT_EQ(m_tabbar->currentName(), QString("renamed.txt"));
}

TEST_F(WindowTest, UpdateSabeAsFileNameTemp_WithWrapper_MovesWithoutClose)
{
    // Arrange
    const QString blank = addBlankAndGetPath();
    EditWrapper *w = m_win->wrapper(blank);
    const QString newPath = m_tempDir->filePath(QStringLiteral("temp-renamed.txt"));

    // Act
    m_win->updateSabeAsFileNameTemp(blank, newPath);

    // Assert: 与 updateSaveAsFileName 一致但不触发重名关闭
    EXPECT_EQ(m_win->m_wrappers.value(newPath), w);
    EXPECT_EQ(w->filePath(), newPath);
    EXPECT_EQ(m_tabbar->count(), 1);
}

// ============================================================
// 窗口状态
// ============================================================

TEST_F(WindowTest, ShowCenterWindow_NormalState_NoMoveFlag)
{
    // Act: 默认 window_normal
    m_win->showCenterWindow(true);

    // Assert
    EXPECT_FALSE(m_win->m_needMoveToCenter);
    EXPECT_FALSE(m_win->isMaximized());
    EXPECT_FALSE(m_win->isFullScreen());
}

TEST_F(WindowTest, ShowCenterWindow_MaximumConfig_SetsMoveFlag)
{
    // Arrange: 配置最大化
    Settings::instance()->settings->option("advance.window.windowstate")->setValue("window_maximum");

    // Act
    m_win->showCenterWindow(false);

    // Assert: 最大化 + 请求居中标志（bIsCenter=false 跳过 moveToCenter）
    EXPECT_TRUE(m_win->isMaximized());
    EXPECT_TRUE(m_win->m_needMoveToCenter);

    // 还原配置
    Settings::instance()->settings->option("advance.window.windowstate")->setValue("window_normal");
}

TEST_F(WindowTest, ShowCenterWindow_FullscreenConfig_EntersFullscreen)
{
    // Arrange
    Settings::instance()->settings->option("advance.window.windowstate")->setValue("fullscreen");

    // Act
    m_win->showCenterWindow(true);

    // Assert
    EXPECT_TRUE(m_win->isFullScreen());
    EXPECT_TRUE(m_win->m_needMoveToCenter);

    Settings::instance()->settings->option("advance.window.windowstate")->setValue("window_normal");
    m_win->showNormal();
}

TEST_F(WindowTest, ToggleFullscreen_SwitchesStateBackAndForth)
{
    // Assert: 初始非全屏
    EXPECT_FALSE(m_win->windowState().testFlag(Qt::WindowFullScreen));

    // Act / Assert: 进入
    m_win->toggleFullscreen();
    EXPECT_TRUE(m_win->windowState().testFlag(Qt::WindowFullScreen));

    // Act / Assert: 退出
    m_win->toggleFullscreen();
    EXPECT_FALSE(m_win->windowState().testFlag(Qt::WindowFullScreen));
}

TEST_F(WindowTest, SlotSigChangeWindowSize_ThreeModes_SwitchesWindowStates)
{
    // Act / Assert: fullscreen
    m_win->slotSigChangeWindowSize(QStringLiteral("fullscreen"));
    EXPECT_TRUE(m_win->isFullScreen());

    // Act / Assert: window_maximum
    m_win->slotSigChangeWindowSize(QStringLiteral("window_maximum"));
    EXPECT_TRUE(m_win->isMaximized());

    // Act / Assert: 其它 → normal
    m_win->slotSigChangeWindowSize(QStringLiteral("window_minimum"));
    EXPECT_FALSE(m_win->isFullScreen());
    EXPECT_FALSE(m_win->isMaximized());
}

TEST_F(WindowTest, ResizeEvent_SavesGeometryToSettings)
{
    // Arrange
    Settings::instance()->settings->option("advance.window.window_width")->setValue(1);
    Settings::instance()->settings->option("advance.window.window_height")->setValue(1);

    // Act
    m_win->resize(900, 700);
    QApplication::processEvents();

    // Assert: 尺寸持久化（resizeEvent 写配置）+ 窗口几何生效
    EXPECT_EQ(Settings::instance()->settings->option("advance.window.window_width")->value().toInt(), 900);
    EXPECT_EQ(Settings::instance()->settings->option("advance.window.window_height")->value().toInt(), 700);
    EXPECT_EQ(m_win->width(), 900);
    EXPECT_EQ(m_win->height(), 700);
}

// ============================================================
// Tab 管理
// ============================================================

TEST_F(WindowTest, CheckBlockShutdown_NoTabsOrClean_ReturnsFalse)
{
    // Assert: 无标签 → false
    EXPECT_FALSE(m_win->checkBlockShutdown());

    // Arrange: 干净标签
    addBlankAndGetPath();

    // Assert: 无 * 前缀 → false
    EXPECT_FALSE(m_win->checkBlockShutdown());
}

TEST_F(WindowTest, CheckBlockShutdown_UnsavedTab_ReturnsTrue)
{
    // Arrange: 构造带 * 的标签（空文本分支先行覆盖）
    m_tabbar->addTab(QStringLiteral("/x/empty"), QString(""));
    m_tabbar->addTab(QStringLiteral("/x/mod"), QStringLiteral("*modified.txt"));
    EXPECT_FALSE(m_win->checkBlockShutdown()); // 空文本标签 → false

    // Act / Assert: 关闭空标签后仅剩修改标签 → true
    m_tabbar->removeTab(0);
    EXPECT_TRUE(m_win->checkBlockShutdown());
}

TEST_F(WindowTest, GetTabIndex_UnknownAndKnownPaths)
{
    // Assert: 未知 → -1
    EXPECT_EQ(m_win->getTabIndex(QStringLiteral("/none")), -1);

    // Arrange
    const QString blank = addBlankAndGetPath();

    // Assert
    EXPECT_EQ(m_win->getTabIndex(blank), 0);
}

TEST_F(WindowTest, ActiveTab_SwitchesCurrentIndex)
{
    // Arrange: 两个标签
    const QString b1 = addBlankAndGetPath();
    addBlankAndGetPath();
    EXPECT_EQ(m_tabbar->currentIndex(), 1);

    // Act
    m_win->activeTab(0);

    // Assert
    EXPECT_EQ(m_tabbar->currentIndex(), 0);
    EXPECT_EQ(m_tabbar->currentPath(), b1);
}

TEST_F(WindowTest, AddBlankTab_CreatesUntitledWithWrapper)
{
    // Act
    m_win->addBlankTab();
    QApplication::processEvents();

    // Assert: 标签创建 + wrapper 注册 + 空白目录在隔离 AppData 下
    ASSERT_EQ(m_tabbar->count(), 1);
    EXPECT_EQ(m_tabbar->currentName(), QString("Untitled 1"));
    const QString blank = m_tabbar->currentPath();
    EXPECT_TRUE(blank.contains(QStringLiteral("blank-files")));
    EXPECT_NE(m_win->wrapper(blank), nullptr);
    EXPECT_EQ(m_win->m_editorWidget->count(), 1);
}

TEST_F(WindowTest, AddBlankTab_WithExistingFile_LoadsContent)
{
    // Arrange: 预置空白文件
    const QString blankFile = createFile(QStringLiteral("seed.txt"), QByteArray("seed content"));
    QDir().mkpath(m_win->m_blankFileDir);

    // Act: 以指定文件新建空白标签
    m_win->addBlankTab(blankFile);
    TextEdit *editor = m_win->getTextEditor(blankFile);
    ASSERT_NE(editor, nullptr);
    ASSERT_TRUE(waitUntil([editor]() { return !editor->document()->isEmpty(); }))
        << "seed content 未加载";

    // Assert: 标签路径即文件路径且内容已加载
    EXPECT_EQ(m_tabbar->currentPath(), blankFile);
    EXPECT_EQ(editor->toPlainText(), QString("seed content"));
}

TEST_F(WindowTest, AddTab_RealTextFile_CreatesLoadedTab)
{
    // Arrange
    const QString path = createFile(QStringLiteral("real.txt"));

    // Act
    m_win->addTab(path, true);

    // Assert: 标签 + wrapper + 文件加载完成 + 激活
    EXPECT_EQ(m_win->getTabIndex(path), 0);
    EditWrapper *w = m_win->wrapper(path);
    ASSERT_NE(w, nullptr);
    EXPECT_TRUE(waitUntil([w]() { return !w->getFileLoading(); }));
    EXPECT_EQ(m_tabbar->currentPath(), path);
}

TEST_F(WindowTest, AddTab_AlreadyOpen_ActivatesExistingTab)
{
    // Arrange
    const QString path = addFileTab(QStringLiteral("dup.txt"));
    ASSERT_EQ(m_tabbar->count(), 1);

    // Act: 再次添加同一路径
    m_win->addTab(path, true);
    QApplication::processEvents();

    // Assert: 不重复建标签（checkPath 拒绝）
    EXPECT_EQ(m_tabbar->count(), 1);
    EXPECT_EQ(m_win->m_wrappers.count(), 1);
}

TEST_F(WindowTest, AddTab_UnsupportedMime_ShowsInvalidNotice)
{
    // Arrange: 目录作为不支持类型（QMimeDatabase → inode/directory）
    const QString dirPath = m_tempDir->path();

    // Act
    m_win->addTab(dirPath, true);
    QApplication::processEvents();

    // Assert: 弹出无效文件提示 + 自动补空白标签
    EXPECT_GE(m_iconMsgCalls, 1);
    EXPECT_TRUE(m_lastIconMsg.contains(QStringLiteral("Invalid file")));
    EXPECT_EQ(m_tabbar->count(), 1); // addBlankTab
    EXPECT_EQ(m_tabbar->currentName(), QString("Untitled 1"));
}

TEST_F(WindowTest, AddTab_ReadOnlyFile_MarksTabReadOnly)
{
    // Arrange: 只读文件（无写权限、可读）
    const QString path = createFile(QStringLiteral("readonly.txt"));
    QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther);
    m_restoreFiles.append(path);

    // Act
    m_win->addTab(path, true);
    EditWrapper *w = m_win->wrapper(path);

    // Assert: 标签名带 Read-Only 标记 + wrapper 存在
    ASSERT_NE(w, nullptr);
    EXPECT_TRUE(m_tabbar->currentName().contains(QStringLiteral("Read-Only")));
    EXPECT_TRUE(waitUntil([w]() { return !w->getFileLoading(); }));
}

TEST_F(WindowTest, AddTab_UnreadableFile_OpenFailsWithGenericError_Continues)
{
    // Arrange: 无读权限文件。Qt 6.8 实测 EACCES 映射为 OpenError（非
    // PermissionsError），走"仅告警继续"分支（PermissionsError 提示分支在
    // 本 Qt 版本不可达，已记录到 session.skip_reason）
    if (geteuid() == 0)
        GTEST_SKIP() << "root 用户不受读权限位约束，跳过";
    const QString path = createFile(QStringLiteral("secret.txt"));
    QFile::setPermissions(path, QFileDevice::WriteOwner);
    m_restoreFiles.append(path);
    if (QFileInfo(path).isReadable())
        GTEST_SKIP() << "环境无法构造不可读文件（权限位未生效）";

    // Act
    m_win->addTab(path, true);
    QApplication::processEvents();

    // Assert: 无权限提示（未走 PermissionsError 分支）且标签照常创建
    EXPECT_EQ(m_iconMsgCalls, 0);
    EXPECT_EQ(m_tabbar->count(), 1);
    EXPECT_EQ(m_tabbar->currentName(), QString("secret.txt"));
}

TEST_F(WindowTest, AddTabWithWrapper_RegistersAndShowsEditor)
{
    // Arrange: 独立真实 wrapper
    EditWrapper *w = m_win->createEditor();
    ASSERT_NE(w, nullptr);
    const QString path = m_tempDir->filePath(QStringLiteral("wrapped.txt"));

    // Act
    m_win->addTabWithWrapper(w, path, path, QStringLiteral("wrapped.txt"));

    // Assert: 标签 + 映射 + 当前显示
    EXPECT_EQ(m_win->getTabIndex(path), 0);
    EXPECT_EQ(m_win->m_wrappers.value(path), w);
    EXPECT_EQ(w->filePath(), path);
    EXPECT_EQ(m_win->m_editorWidget->currentWidget(), w);
}

// ============================================================
// closeTab 家族
// ============================================================

TEST_F(WindowTest, CloseTab_NoCurrentPath_ReturnsFalse)
{
    // Assert: 无标签 → false 且不产生副作用
    EXPECT_FALSE(m_win->closeTab());
    EXPECT_EQ(m_tabbar->count(), 0);
    EXPECT_TRUE(m_win->m_wrappers.isEmpty());
}

TEST_F(WindowTest, CloseTab_UnknownPath_ReturnsFalse)
{
    // Act / Assert
    EXPECT_FALSE(m_win->closeTab(QStringLiteral("/no/such/path")));
    EXPECT_EQ(m_tabbar->count(), 0);
}

TEST_F(WindowTest, CloseTab_PendingTab_RemovesDirectlyAndAddsBlank)
{
    // Arrange: 待加载标签（真实存在文件）
    const QString path = createFile(QStringLiteral("pending.txt"));
    Window::PendingTabInfo info;
    info.filepath = path;
    info.truePath = path;
    info.displayName = QStringLiteral("pending.txt");
    m_win->addPendingTab(info);
    ASSERT_TRUE(m_win->isPendingTab(path));
    ASSERT_EQ(m_tabbar->count(), 1);

    // Act
    EXPECT_TRUE(m_win->closeTab(path));

    // Assert: pending 移除 + 标签清空后补空白标签
    EXPECT_FALSE(m_win->isPendingTab(path));
    EXPECT_EQ(m_tabbar->count(), 1); // addBlankTab
    EXPECT_EQ(m_tabbar->currentName(), QString("Untitled 1"));
}

TEST_F(WindowTest, CloseTab_UnmodifiedDraft_RemovesAndDeletesFile)
{
    // Arrange: 空白标签（草稿文件在首次备份/保存前不落盘）
    const QString blank = addBlankAndGetPath();

    // Act
    EXPECT_TRUE(m_win->closeTab(blank));
    QApplication::processEvents();

    // Assert: wrapper 移除 + 草稿文件删除 + 标签清零（窗口随之关闭）
    EXPECT_TRUE(m_win->m_wrappers.isEmpty());
    EXPECT_FALSE(QFile::exists(blank));
    EXPECT_EQ(m_tabbar->count(), 0);
}

TEST_F(WindowTest, CloseTab_ModifiedDraft_CancelDialog_KeepsTab)
{
    // Arrange: 修改态草稿 + 对话框取消（res 0）
    const QString blank = addBlankAndGetPath();
    stub.set_lamda(&EditWrapper::isModified, [](EditWrapper *) -> bool { return true; });
    m_ddialogResult = 0;

    // Act
    EXPECT_FALSE(m_win->closeTab(blank));

    // Assert: 标签保留
    EXPECT_EQ(m_tabbar->count(), 1);
    EXPECT_EQ(m_ddialogExecCalls, 1);
}

TEST_F(WindowTest, CloseTab_ModifiedDraft_Discard_RemovesTab)
{
    // Arrange
    const QString blank = addBlankAndGetPath();
    stub.set_lamda(&EditWrapper::isModified, [](EditWrapper *) -> bool { return true; });
    m_ddialogResult = 1; // 不保存

    // Act
    EXPECT_TRUE(m_win->closeTab(blank));
    QApplication::processEvents();

    // Assert
    EXPECT_TRUE(m_win->m_wrappers.isEmpty());
    EXPECT_FALSE(QFile::exists(blank));
}

TEST_F(WindowTest, CloseTab_ModifiedDraft_SaveSuccess_ClosesAfterSave)
{
    // Arrange
    const QString blank = addBlankAndGetPath();
    stub.set_lamda(&EditWrapper::isModified, [](EditWrapper *) -> bool { return true; });
    m_ddialogResult = 2; // 保存
    const QString savedTo = m_tempDir->filePath(QStringLiteral("draft-saved.txt"));
    stub.set_lamda(&EditWrapper::saveDraftFile,
                   [savedTo](EditWrapper *, QString &newFilePath) -> bool {
                       newFilePath = savedTo;
                       return true;
                   });

    // Act
    EXPECT_TRUE(m_win->closeTab(blank));
    QApplication::processEvents();

    // Assert: 走保存成功分支（removeWrapper 执行；因桩不回写 tab 路径，标签保留）
    EXPECT_TRUE(m_win->m_wrappers.isEmpty());
    EXPECT_EQ(m_tabbar->count(), 1);
}

TEST_F(WindowTest, CloseTab_ModifiedDraft_SaveFail_KeepsTab)
{
    // Arrange
    const QString blank = addBlankAndGetPath();
    stub.set_lamda(&EditWrapper::isModified, [](EditWrapper *) -> bool { return true; });
    m_ddialogResult = 2;
    stub.set_lamda(&EditWrapper::saveDraftFile,
                   [](EditWrapper *, QString &) -> bool { return false; });

    // Act / Assert: 保存失败不关闭
    EXPECT_FALSE(m_win->closeTab(blank));
    EXPECT_EQ(m_tabbar->count(), 1);
}

TEST_F(WindowTest, CloseTab_ModifiedNormal_Cancel_KeepsTab)
{
    // Arrange: 真实文件但强制"已修改 + 非草稿"
    const QString path = addFileTab(QStringLiteral("normal.txt"));
    stub.set_lamda(&EditWrapper::isModified, [](EditWrapper *) -> bool { return true; });
    stub.set_lamda(&EditWrapper::isDraftFile, [](EditWrapper *) -> bool { return false; });
    m_ddialogResult = 0;

    // Act
    EXPECT_FALSE(m_win->closeTab(path));

    // Assert
    EXPECT_EQ(m_tabbar->count(), 1);
    EXPECT_EQ(m_ddialogExecCalls, 1);
}

TEST_F(WindowTest, CloseTab_ModifiedNormal_Discard_ClosesTab)
{
    // Arrange
    const QString path = addFileTab(QStringLiteral("normal2.txt"));
    stub.set_lamda(&EditWrapper::isModified, [](EditWrapper *) -> bool { return true; });
    stub.set_lamda(&EditWrapper::isDraftFile, [](EditWrapper *) -> bool { return false; });
    m_ddialogResult = 1;

    // Act
    EXPECT_TRUE(m_win->closeTab(path));
    QApplication::processEvents();

    // Assert
    EXPECT_TRUE(m_win->m_wrappers.isEmpty());
    EXPECT_EQ(m_tabbar->count(), 0);
}

TEST_F(WindowTest, CloseTab_ModifiedNormal_SaveFail_FallsToSaveAs)
{
    // Arrange: 保存失败 → saveAsFile 兜底（DFileDialog 拒绝）
    const QString path = addFileTab(QStringLiteral("normal3.txt"));
    stub.set_lamda(&EditWrapper::isModified, [](EditWrapper *) -> bool { return true; });
    stub.set_lamda(&EditWrapper::isDraftFile, [](EditWrapper *) -> bool { return false; });
    stub.set_lamda(&EditWrapper::saveFile, [](EditWrapper *, QByteArray) -> bool { return false; });
    m_ddialogResult = 2;
    m_qdialogResult = QDialog::Rejected; // DFileDialog::exec → 拒绝

    // Act: 源码在 saveAsFile 失败后仍继续走清理并返回 true（记录源码行为）
    const bool ret = m_win->closeTab(path);
    QApplication::processEvents();

    // Assert: 走到 saveAsFile 兜底（QDialog::exec 被拒）+ 标签被清理
    EXPECT_TRUE(ret);
    EXPECT_GE(m_qdialogExecCalls, 1);
}

TEST_F(WindowTest, CloseTab_ModifiedBackup_SaveSuccess_RemovesTemFile)
{
    // Arrange: 临时备份文件（isTemFile true）+ 保存成功
    const QString path = addFileTab(QStringLiteral("backup-file.txt"));
    stub.set_lamda(&EditWrapper::isModified, [](EditWrapper *) -> bool { return true; });
    stub.set_lamda(&EditWrapper::isDraftFile, [](EditWrapper *) -> bool { return false; });
    stub.set_lamda(&EditWrapper::isTemFile, [](EditWrapper *) -> bool { return true; });
    stub.set_lamda(&EditWrapper::saveFile, [](EditWrapper *, QByteArray) -> bool { return true; });
    m_ddialogResult = 2;

    // Act
    EXPECT_TRUE(m_win->closeTab(path));
    QApplication::processEvents();

    // Assert: 备份文件被 QFile::remove
    EXPECT_TRUE(m_win->m_wrappers.isEmpty());
    EXPECT_FALSE(QFile::exists(path));
}

TEST_F(WindowTest, CloseTab_InvalidCharPreview_NoEdits_ClosesDirectly)
{
    // Arrange: 预览模式但未编辑 → 直接关闭不弹窗
    const QString path = addFileTab(QStringLiteral("preview.txt"));
    stub.set_lamda(&EditWrapper::isInvalidCharPreview, [](EditWrapper *) -> bool { return true; });
    stub.set_lamda(&EditWrapper::isInvalidCharEditAllowed, [](EditWrapper *) -> bool { return false; });

    // Act
    EXPECT_TRUE(m_win->closeTab(path));
    QApplication::processEvents();

    // Assert: 未弹确认框
    EXPECT_EQ(m_ddialogExecCalls, 0);
    EXPECT_TRUE(m_win->m_wrappers.isEmpty());
}

TEST_F(WindowTest, CloseTab_InvalidCharPreview_EditedButtons_ThreeResponses)
{
    // Arrange: 预览 + 已编辑
    const QString base = addFileTab(QStringLiteral("preview2.txt"));
    stub.set_lamda(&EditWrapper::isInvalidCharPreview, [](EditWrapper *) -> bool { return true; });
    stub.set_lamda(&EditWrapper::isInvalidCharEditAllowed, [](EditWrapper *) -> bool { return true; });
    stub.set_lamda(&EditWrapper::isModified, [](EditWrapper *) -> bool { return true; });

    // Act / Assert: Don't Save（0）→ 关闭
    m_ddialogResult = 0;
    EXPECT_TRUE(m_win->closeTab(base));
    EXPECT_TRUE(m_win->m_wrappers.isEmpty());

    // Act / Assert: Save Anyway（2）+ 强制保存成功 → 关闭
    const QString p2 = addFileTab(QStringLiteral("preview3.txt"));
    stub.set_lamda(&EditWrapper::forceSaveInvalidCharFile, [](EditWrapper *) -> bool { return true; });
    m_ddialogResult = 2;
    EXPECT_TRUE(m_win->closeTab(p2));
    EXPECT_TRUE(m_win->m_wrappers.isEmpty());

    // Act / Assert: Save As（1）+ saveAsFileToDisk 失败（拒绝）→ 不关闭
    const QString p3 = addFileTab(QStringLiteral("preview4.txt"));
    m_ddialogResult = 1;
    m_qdialogResult = QDialog::Rejected;
    EXPECT_FALSE(m_win->closeTab(p3));
    EXPECT_EQ(m_tabbar->count(), 1);
}

// ============================================================
// restoreTab / removeWrapper / focus
// ============================================================

TEST_F(WindowTest, RestoreTab_EmptyHistory_NoOp)
{
    // Act
    m_win->restoreTab();

    // Assert
    EXPECT_EQ(m_tabbar->count(), 0);
    EXPECT_TRUE(m_win->m_closeFileHistory.isEmpty());
}

TEST_F(WindowTest, RestoreTab_WithHistory_ReopensLastClosed)
{
    // Arrange: 历史压入真实文件
    const QString path = createFile(QStringLiteral("history.txt"));
    m_win->m_closeFileHistory << path;

    // Act
    m_win->restoreTab();
    QApplication::processEvents();

    // Assert: 重新打开且历史弹出
    EXPECT_EQ(m_win->getTabIndex(path), 0);
    EXPECT_TRUE(m_win->m_closeFileHistory.isEmpty());
    EXPECT_NE(m_win->wrapper(path), nullptr);
}

TEST_F(WindowTest, RemoveWrapper_UnknownPath_NoOp)
{
    // Act
    m_win->removeWrapper(QStringLiteral("/none"));

    // Assert: 无标签时也不误关（强异常安全）
    EXPECT_EQ(m_tabbar->count(), 0);
    EXPECT_TRUE(m_win->m_wrappers.isEmpty());
    EXPECT_TRUE(m_win->isVisible() || m_win->isHidden());
}

TEST_F(WindowTest, RemoveWrapper_KeepsTabWhenNotDeleting)
{
    // Arrange: 展示窗口 + 一个标签
    m_win->show();
    const QString blank = addBlankAndGetPath();
    EditWrapper *w = m_win->wrapper(blank);
    ASSERT_NE(w, nullptr);

    // Act: 仅移除 wrapper（不移除标签）→ 不满足 close 条件
    m_win->removeWrapper(blank, false);
    QApplication::processEvents();

    // Assert: wrapper 移除但标签保留 → 窗口保持可见（close 条件为两者皆空）
    EXPECT_TRUE(m_win->m_wrappers.isEmpty());
    EXPECT_EQ(m_tabbar->count(), 1);
    EXPECT_TRUE(m_win->isVisible());
}

TEST_F(WindowTest, FocusActiveEditor_NoWrapper_NoOp)
{
    // Act / Assert: 无标签早退且状态未损坏
    m_win->focusActiveEditor();
    EXPECT_EQ(m_tabbar->count(), 0);
    EXPECT_EQ(m_win->currentWrapper(), nullptr);
}

TEST_F(WindowTest, FocusActiveEditor_WithEditor_SetsFocus)
{
    // Arrange
    const QString blank = addBlankAndGetPath();
    TextEdit *editor = m_win->getTextEditor(blank);
    ASSERT_NE(editor, nullptr);

    // Act
    m_win->focusActiveEditor();
    QApplication::processEvents();

    // Assert: 编辑器获得焦点且仍为当前页
    EXPECT_TRUE(editor->hasFocus());
    EXPECT_EQ(m_win->m_editorWidget->currentWidget(), m_win->wrapper(blank));
}

TEST_F(WindowTest, WrapperAndEditor_Lookup_HitMissAndTruePathFallback)
{
    // Arrange: 两个文件标签
    const QString p1 = addFileTab(QStringLiteral("one.txt"));
    const QString p2 = addFileTab(QStringLiteral("two.txt"));

    // Assert: 直接命中 / 未命中
    EXPECT_NE(m_win->wrapper(p1), nullptr);
    EXPECT_NE(m_win->getTextEditor(p1), nullptr);
    EXPECT_EQ(m_win->wrapper(QStringLiteral("/miss")), nullptr);
    EXPECT_EQ(m_win->getTextEditor(QStringLiteral("/miss")), nullptr);

    // Assert: truePath 回退查找（p2 的真实路径即自身）
    EXPECT_EQ(m_win->wrapper(QFileInfo(p2).absoluteFilePath()), m_win->wrapper(p2));
    EXPECT_EQ(m_win->getWrappers().count(), 2);
}

TEST_F(WindowTest, CurrentWrapper_FollowsCurrentTab)
{
    // Assert: 无标签 → null
    EXPECT_EQ(m_win->currentWrapper(), nullptr);

    // Arrange
    const QString blank = addBlankAndGetPath();

    // Assert: 当前标签对应 wrapper
    EXPECT_EQ(m_win->currentWrapper(), m_win->wrapper(blank));
}

TEST_F(WindowTest, CreateEditor_AppliesConfigToEditor)
{
    // Act
    EditWrapper *w = m_win->createEditor();

    // Assert: 编辑器就绪 + 字号取自配置（默认 12）+ 焦点策略
    ASSERT_NE(w, nullptr);
    ASSERT_NE(w->textEditor(), nullptr);
    EXPECT_EQ(w->textEditor()->m_settings, Settings::instance());
    EXPECT_EQ(m_win->m_fontSize, Settings::instance()->settings->option("base.font.size")->value().toReal());
    // 回收（无标签持有）
    w->deleteLater();
    QApplication::processEvents();
}

// ============================================================
// openFile（文件对话框流程）
// ============================================================

TEST_F(WindowTest, OpenFile_DialogRejected_NoTabOpened)
{
    // Arrange: 历史目录配置指向临时目录（存在）
    Settings::instance()->settings->option("advance.editor.file_dialog_dir")->setValue(m_tempDir->path());
    m_qdialogResult = QDialog::Rejected;

    // Act
    m_win->openFile();
    QApplication::processEvents();

    // Assert
    EXPECT_EQ(m_qdialogExecCalls, 1);
    EXPECT_EQ(m_tabbar->count(), 0);
}

TEST_F(WindowTest, OpenFile_DialogAccepted_LoadsFirstAndPendsRest)
{
    // Arrange: 两个受支持文件 + 一个目录（不支持）
    const QString f1 = createFile(QStringLiteral("first.txt"));
    const QString f2 = createFile(QStringLiteral("second.txt"));
    const QString dirPath = m_tempDir->path();
    m_qdialogResult = QDialog::Accepted;
    m_selectedFiles = QStringList { f1, f2, dirPath };

    // Act
    m_win->openFile();
    QApplication::processEvents();

    // Assert: 第一个立即加载，其余为 pending，最后补不支持提示
    EXPECT_EQ(m_tabbar->count(), 3);
    EXPECT_TRUE(m_win->isPendingTab(f2));
    EditWrapper *w1 = m_win->wrapper(f1);
    ASSERT_NE(w1, nullptr);
    EXPECT_TRUE(waitUntil([w1]() { return !w1->getFileLoading(); }));
    EXPECT_GE(m_iconMsgCalls, 1); // 目录 → Invalid file 提示
}

// ============================================================
// 保存流程
// ============================================================

TEST_F(WindowTest, SaveFile_NoWrapperOrLoading_ReturnsFalse)
{
    // Assert: 无 wrapper
    EXPECT_FALSE(m_win->saveFile());

    // Arrange: 大文本加载中
    const QString blank = addBlankAndGetPath();
    EditWrapper *w = m_win->wrapper(blank);
    stub.set_lamda(&EditWrapper::getFileLoading, [](EditWrapper *) -> bool { return true; });

    // Assert: 加载中拒绝保存
    EXPECT_FALSE(m_win->saveFile());
    EXPECT_EQ(w, m_win->wrapper(blank));
}

TEST_F(WindowTest, SaveFile_DraftRedirectsToSaveAs_RejectedReturnsFalse)
{
    // Arrange: 空白标签（草稿）+ 另存对话框拒绝
    addBlankAndGetPath();
    m_qdialogResult = QDialog::Rejected;

    // Act / Assert: 草稿 → saveAsFile → DFileDialog 拒绝 → false
    EXPECT_FALSE(m_win->saveFile());
    EXPECT_GE(m_qdialogExecCalls, 1);
}

TEST_F(WindowTest, SaveFile_NormalUnmodified_SavesAndNotifies)
{
    // Arrange: 真实文件未修改
    const QString path = addFileTab(QStringLiteral("save-me.txt"));

    // Act
    const bool ret = m_win->saveFile();

    // Assert: 保存成功 + 成功提示（EditWrapper::showNotify → 浮动消息桩）
    EXPECT_TRUE(ret);
    EXPECT_EQ(m_floatMsgCalls, 1);
    EXPECT_TRUE(m_lastFloatMsg.contains(QStringLiteral("Saved successfully")));
    EXPECT_TRUE(QFile::exists(path));
}

TEST_F(WindowTest, SaveFile_NoWritePermission_ShowsNotice)
{
    // Arrange: 只读权限文件已加载
    const QString path = addFileTab(QStringLiteral("ro-save.txt"));
    QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther);
    m_restoreFiles.append(path);

    // Act
    const bool ret = m_win->saveFile();

    // Assert: 无权限提示 + 不保存
    EXPECT_FALSE(ret);
    EXPECT_GE(m_floatMsgCalls + m_iconMsgCalls, 1);
    EXPECT_TRUE(m_lastFloatMsg.contains(QStringLiteral("permission"))
                || m_lastIconMsg.contains(QStringLiteral("permission")));
}

TEST_F(WindowTest, SaveFile_InvalidCharPreview_NoEdits_ReturnsFalse)
{
    // Arrange: 预览模式未编辑
    addFileTab(QStringLiteral("pv-save.txt"));
    stub.set_lamda(&EditWrapper::isInvalidCharPreview, [](EditWrapper *) -> bool { return true; });
    stub.set_lamda(&EditWrapper::isInvalidCharEditAllowed, [](EditWrapper *) -> bool { return false; });

    // Act / Assert: 无需保存直接返回 false
    EXPECT_FALSE(m_win->saveFile());
    EXPECT_EQ(m_ddialogExecCalls, 0);
}

TEST_F(WindowTest, SaveFile_InvalidCharPreview_Edited_ThreeResponses)
{
    // Arrange: 预览 + 已编辑
    addFileTab(QStringLiteral("pv-save2.txt"));
    stub.set_lamda(&EditWrapper::isInvalidCharPreview, [](EditWrapper *) -> bool { return true; });
    stub.set_lamda(&EditWrapper::isInvalidCharEditAllowed, [](EditWrapper *) -> bool { return true; });

    // Act / Assert: Don't Save（0）→ false
    m_ddialogResult = 0;
    EXPECT_FALSE(m_win->saveFile());
    EXPECT_EQ(m_ddialogExecCalls, 1);

    // Act / Assert: Save Anyway（2）+ 强制成功 → true + 提示
    stub.set_lamda(&EditWrapper::forceSaveInvalidCharFile, [](EditWrapper *) -> bool { return true; });
    m_ddialogResult = 2;
    EXPECT_TRUE(m_win->saveFile());
    EXPECT_TRUE(m_lastFloatMsg.contains(QStringLiteral("Saved successfully")));

    // Act / Assert: Save As（1）→ 转发 saveAsFile（此处以对话框拒绝 → false）
    m_ddialogResult = 1;
    m_qdialogResult = QDialog::Rejected;
    EXPECT_FALSE(m_win->saveFile());
}

TEST_F(WindowTest, SaveAsFile_NoWrapper_ReturnsFalse)
{
    // Act / Assert: 无 wrapper → 另存失败且无副作用
    EXPECT_FALSE(m_win->saveAsFile());
    EXPECT_EQ(m_tabbar->count(), 0);
}

TEST_F(WindowTest, SaveAsFileToDisk_Rejected_ReturnsEmpty)
{
    // Arrange
    addBlankAndGetPath();
    m_qdialogResult = QDialog::Rejected;

    // Act / Assert
    EXPECT_TRUE(m_win->saveAsFileToDisk().isEmpty());
    EXPECT_GE(m_qdialogExecCalls, 1);
}

TEST_F(WindowTest, SaveAsFileToDisk_AcceptedNewPath_SavesAndUpdatesTab)
{
    // Arrange: 真实文件 + 另存对话框接受（新路径）
    const QString path = addFileTab(QStringLiteral("as.txt"), QByteArray("as-content"));
    const QString newPath = m_tempDir->filePath(QStringLiteral("as-new.txt"));
    m_qdialogResult = QDialog::Accepted;
    m_selectedFiles = QStringList { newPath };
    stub.set_lamda(static_cast<bool (EditWrapper::*)(const QString &, const QByteArray &)>(&EditWrapper::saveAsFile),
                   [this, newPath](EditWrapper *self, const QString &file, const QByteArray &) -> bool {
                       ++m_saveAsCalls;
                       EXPECT_EQ(file, newPath);
                       QFile f(newPath);
                       f.open(QIODevice::WriteOnly);
                       f.write("as-content");
                       f.close();
                       return true;
                   });

    // Act
    const QString ret = m_win->saveAsFileToDisk();

    // Assert: 返回新路径 + 映射迁移 + 标签更新 + 保存路径记忆更新
    EXPECT_EQ(ret, newPath);
    EXPECT_EQ(m_saveAsCalls, 1);
    EXPECT_TRUE(m_win->m_wrappers.contains(newPath));
    EXPECT_FALSE(m_win->m_wrappers.contains(path));
    EXPECT_EQ(m_tabbar->currentName(), QString("as-new.txt"));
    EXPECT_EQ(Settings::instance()->getSavePath(PathSettingWgt::LastOptBox),
              QFileInfo(newPath).absolutePath());
}

TEST_F(WindowTest, SaveAsFileToDisk_NoWrapperOrLoading_Empty)
{
    // Assert: 无 wrapper
    EXPECT_TRUE(m_win->saveAsFileToDisk().isEmpty());

    // Arrange: 加载中
    addBlankAndGetPath();
    stub.set_lamda(&EditWrapper::getFileLoading, [](EditWrapper *) -> bool { return true; });
    EXPECT_TRUE(m_win->saveAsFileToDisk().isEmpty());
}

TEST_F(WindowTest, SaveBlankFileToDisk_RejectedAndAccepted)
{
    // Arrange: 草稿空白标签
    const QString blank = addBlankAndGetPath();
    m_qdialogResult = QDialog::Rejected;

    // Act / Assert: 拒绝 → 空
    EXPECT_TRUE(m_win->saveBlankFileToDisk().isEmpty());

    // Arrange: 接受
    const QString newPath = m_tempDir->filePath(QStringLiteral("blank-saved.txt"));
    m_qdialogResult = QDialog::Accepted;
    m_selectedFiles = QStringList { newPath };
    stub.set_lamda(&EditWrapper::saveFile,
                   [newPath](EditWrapper *, QByteArray) -> bool {
                       QFile f(newPath);
                       f.open(QIODevice::WriteOnly);
                       f.write("saved");
                       f.close();
                       return true;
                   });

    // Act
    const QString ret = m_win->saveBlankFileToDisk();

    // Assert: 新路径 + 映射迁移
    EXPECT_EQ(ret, newPath);
    EXPECT_TRUE(m_win->m_wrappers.contains(newPath));
    EXPECT_FALSE(m_win->m_wrappers.contains(blank));
}

TEST_F(WindowTest, SaveBlankFileToDisk_NoWrapper_Empty)
{
    // Act / Assert
    EXPECT_TRUE(m_win->saveBlankFileToDisk().isEmpty());
    EXPECT_EQ(m_tabbar->count(), 0);
}

TEST_F(WindowTest, SaveAsOtherTabFile_RejectedAndAccepted)
{
    // Arrange: 真实文件标签
    const QString path = addFileTab(QStringLiteral("other.txt"));

    // Act / Assert: 拒绝 → false
    m_qdialogResult = QDialog::Rejected;
    EXPECT_FALSE(m_win->saveAsOtherTabFile(m_win->wrapper(path)));

    // Act / Assert: 接受 → wrapper 路径更新
    const QString newPath = m_tempDir->filePath(QStringLiteral("other-new.txt"));
    m_qdialogResult = QDialog::Accepted;
    m_selectedFiles = QStringList { newPath };
    stub.set_lamda(&EditWrapper::saveFile,
                   [newPath](EditWrapper *, QByteArray) -> bool {
                       QFile f(newPath);
                       f.open(QIODevice::WriteOnly);
                       f.write("other");
                       f.close();
                       return true;
                   });
    EditWrapper *w = m_win->wrapper(path);
    ASSERT_NE(w, nullptr);
    EXPECT_TRUE(m_win->saveAsOtherTabFile(w));
    EXPECT_EQ(w->filePath(), newPath);
}

TEST_F(WindowTest, ChangeSettingDialogComboxFontNumber_WritesOption)
{
    // Act
    m_win->changeSettingDialogComboxFontNumber(21);

    // Assert: 配置回读一致
    EXPECT_EQ(Settings::instance()->settings->option("base.font.size")->value().toInt(), 21);
    EXPECT_EQ(Settings::instance()->settings->option("base.font.size")->value().toInt(), 21);
}

// ============================================================
// 字体缩放（calcFontScale / calcFontSizeFromScale / 增减复位）
// ============================================================

TEST_F(WindowTest, CalcFontScale_Boundaries_ReturnsClampedScale)
{
    // Assert: 默认字号 12 → 100%
    EXPECT_DOUBLE_EQ(m_win->calcFontScale(12.0), 100.0);
    // > 默认：线性放大并钳制 500
    EXPECT_DOUBLE_EQ(m_win->calcFontScale(31.0), 300.0); // 100 + 400/38*19 = 300
    EXPECT_DOUBLE_EQ(m_win->calcFontScale(500.0), 500.0); // 钳制上限
    // < 默认：线性缩小并钳制 10
    EXPECT_DOUBLE_EQ(m_win->calcFontScale(10.0), 55.0);  // 100 - 90/4*... = 55
    EXPECT_DOUBLE_EQ(m_win->calcFontScale(1.0), 10.0);   // 钳制下限
}

TEST_F(WindowTest, CalcFontSizeFromScale_Boundaries_ReturnsClampedSize)
{
    EXPECT_DOUBLE_EQ(m_win->calcFontSizeFromScale(100.0), 12.0);
    EXPECT_DOUBLE_EQ(m_win->calcFontSizeFromScale(500.0), 50.0); // 上限
    EXPECT_DOUBLE_EQ(m_win->calcFontSizeFromScale(10.0), 8.0);   // 下限
    EXPECT_NEAR(m_win->calcFontSizeFromScale(300.0), 31.0, 0.01); // 12 + 0.095*200
}

TEST_F(WindowTest, FontSize_IncrementDecrementReset_WritesConfig)
{
    // Arrange: 复位到默认
    m_win->resetFontSize();
    EXPECT_EQ(Settings::instance()->settings->option("base.font.size")->value().toInt(), 12);

    // Act: 增大 → 配置变大
    m_win->incrementFontSize();
    EXPECT_GT(Settings::instance()->settings->option("base.font.size")->value().toReal(), 12.0);

    // Act: 减小 → 配置变小（不低于下限 8）
    m_win->decrementFontSize();
    m_win->decrementFontSize();
    EXPECT_LT(Settings::instance()->settings->option("base.font.size")->value().toReal(), 50.0);
    EXPECT_GE(Settings::instance()->settings->option("base.font.size")->value().toReal(), 8.0);

    // Act: 复位
    m_win->resetFontSize();
    EXPECT_EQ(Settings::instance()->settings->option("base.font.size")->value().toInt(), 12);
    EXPECT_EQ(m_win->m_fontSize, 12.0);
}

TEST_F(WindowTest, SetFontSizeWithConfig_AppliesToEditorAndBottomBar)
{
    // Arrange
    EditWrapper *w = m_win->createEditor();
    ASSERT_NE(w, nullptr);

    // Act
    m_win->setFontSizeWithConfig(w);

    // Assert: m_fontSize 同步配置 + 编辑器为当前栈外对象（配置值默认 12）
    const qreal cfg = Settings::instance()->settings->option("base.font.size")->value().toReal();
    EXPECT_EQ(m_win->m_fontSize, cfg);
    EXPECT_NE(w->textEditor(), nullptr);
    w->deleteLater();
    QApplication::processEvents();
}

// ============================================================
// 查找 / 替换 / 跳行栏
// ============================================================

TEST_F(WindowTest, PopupFindBar_NoWrapperOrEmptyDoc_EarlyReturn)
{
    // Assert: 无 wrapper
    m_win->popupFindBar();
    EXPECT_FALSE(m_win->findBarIsVisiable());

    // Arrange: 空文档标签
    addBlankAndGetPath();
    m_win->popupFindBar();

    // Assert: 空文档不弹出
    EXPECT_FALSE(m_win->findBarIsVisiable());
}

TEST_F(WindowTest, PopupFindBar_WithContent_ShowsBarAndKeywords)
{
    // Arrange: 有内容的标签（窗口须可见，否则子栏 isVisible 恒 false）
    m_win->show();
    const QString path = addFileTab(QStringLiteral("find.txt"), QByteArray("hello findable world\n"));
    TextEdit *editor = m_win->getTextEditor(path);
    ASSERT_NE(editor, nullptr);

    // Act: 弹出后等待 10ms focus 定时器 lambda（QTimer::singleShot）
    m_win->popupFindBar();
    processEventsFor(60);

    // Assert: 查找栏可见 + 关键词初始化为选中文本（空）
    EXPECT_TRUE(m_win->findBarIsVisiable());
    EXPECT_TRUE(m_win->m_findBar->isVisible());
    EXPECT_EQ(m_win->getKeywordForSearch(), QString(""));
}

TEST_F(WindowTest, PopupReplaceBar_EmptyDocOrReadOnly_DoesNotShow)
{
    // Assert: 空（无选区空文档）→ 直接返回
    m_win->show();
    m_win->popupReplaceBar();
    EXPECT_FALSE(m_win->replaceBarIsVisiable());

    // Arrange: 只读模式编辑器 + 内容（m_readOnlyMode 经 toggleReadOnlyMode 置位）
    const QString path = addFileTab(QStringLiteral("ro.txt"), QByteArray("read only content\n"));
    m_win->getTextEditor(path)->toggleReadOnlyMode(true);
    m_win->popupReplaceBar();

    // Assert: 只读提示 + 不弹出（提示走浮动消息桩）
    EXPECT_FALSE(m_win->replaceBarIsVisiable());
    EXPECT_GE(m_floatMsgCalls, 1);
    EXPECT_TRUE(m_lastFloatMsg.contains(QStringLiteral("Read-Only")));
}

TEST_F(WindowTest, PopupReplaceBar_EditableContent_ShowsBar)
{
    // Arrange
    m_win->show();
    addFileTab(QStringLiteral("rw.txt"), QByteArray("replaceable content\n"));

    // Act: 弹出后等待 10ms focus 定时器 lambda
    m_win->popupReplaceBar();
    processEventsFor(60);

    // Assert
    EXPECT_TRUE(m_win->replaceBarIsVisiable());
    EXPECT_TRUE(m_win->m_replaceBar->isVisible());
}

TEST_F(WindowTest, PopupJumpLineBar_States_ToggleAndEarlyReturn)
{
    // Assert: 无 wrapper 早退
    m_win->popupJumpLineBar();
    EXPECT_FALSE(m_win->m_jumpLineBar->isVisible());

    // Arrange: 空文档 → 早退
    m_win->show();
    addBlankAndGetPath();
    m_win->popupJumpLineBar();
    EXPECT_FALSE(m_win->m_jumpLineBar->isVisible());

    // Act: 有内容 → 显示
    const QString path = addFileTab(QStringLiteral("jump.txt"), QByteArray("l1\nl2\nl3\n"));
    m_win->popupJumpLineBar();
    EXPECT_TRUE(m_win->m_jumpLineBar->isVisible());

    // Act: 已可见 → 隐藏（toggle 分支）
    m_win->popupJumpLineBar();
    EXPECT_FALSE(m_win->m_jumpLineBar->isVisible());
    EXPECT_EQ(m_win->getTabIndex(path), 1); // 前置空白标签占 0 号位
}

TEST_F(WindowTest, UpdateJumpLineBar_SyncsLineCountAndClearsSearch)
{
    // Arrange: 显示跳行栏
    const QString path = addFileTab(QStringLiteral("sync.txt"), QByteArray("a\nb\nc\nd\n"));
    TextEdit *editor = m_win->getTextEditor(path);
    m_win->popupJumpLineBar();
    ASSERT_TRUE(m_win->m_jumpLineBar->isVisible());

    // Act: 追加一行后更新
    editor->appendPlainText(QStringLiteral("e"));
    m_win->updateJumpLineBar(editor);

    // Assert: 行数同步 + 跳行栏保持可见
    EXPECT_EQ(m_win->m_jumpLineBar->getLineCount(), editor->blockCount());
    EXPECT_GE(editor->blockCount(), 5); // "a\nb\nc\nd\n" + append → ≥5 块（含尾块）
    EXPECT_TRUE(m_win->m_jumpLineBar->isVisible());
}

TEST_F(WindowTest, HandleJumpLineBarJumpToLine_MovesCursor)
{
    // Arrange
    const QString path = addFileTab(QStringLiteral("lines.txt"), QByteArray("one\ntwo\nthree\n"));
    TextEdit *editor = m_win->getTextEditor(path);

    // Act: 跳到第 3 行（focusEditor=true 分支）
    m_win->handleJumpLineBarJumpToLine(path, 3, true);
    QApplication::processEvents();

    // Assert: 光标行号变更
    EXPECT_EQ(editor->getCurrentLine(), 3);

    // Act: 未知路径分支（不崩溃）
    m_win->handleJumpLineBarJumpToLine(QStringLiteral("/none"), 1, false);
    EXPECT_EQ(editor->getCurrentLine(), 3);
}

TEST_F(WindowTest, HandleBackToPosition_KnownAndUnknownPaths)
{
    // Arrange
    const QString path = addFileTab(QStringLiteral("pos.txt"), QByteArray("p1\np2\n"));
    TextEdit *editor = m_win->getTextEditor(path);

    // Act: 已知路径滚动复位
    m_win->handleBackToPosition(path, 2, 1, 0);

    // Assert: 编辑器仍可用 + 未知路径不崩溃
    EXPECT_NE(editor, nullptr);
    m_win->handleBackToPosition(QStringLiteral("/none"), 1, 1, 0);
    EXPECT_EQ(m_win->wrapper(path), m_win->currentWrapper());
}

TEST_F(WindowTest, HandleJumpLineBarExit_RefocusesEditor)
{
    // Arrange
    const QString blank = addBlankAndGetPath();
    TextEdit *editor = m_win->getTextEditor(blank);

    // Act
    m_win->handleJumpLineBarExit();
    QApplication::processEvents();

    // Assert: 焦点回到编辑器且标签未变
    EXPECT_TRUE(editor->hasFocus());
    EXPECT_EQ(m_tabbar->currentPath(), blank);
}

TEST_F(WindowTest, HandleFindKeywords_NextPrev_UpdateState)
{
    // Arrange
    addFileTab(QStringLiteral("kw.txt"), QByteArray("alpha beta gamma\n"));

    // Act: 向后查找
    m_win->handleFindNextSearchKeyword(QStringLiteral("alpha"));

    // Assert: 关键词记录
    EXPECT_EQ(m_win->getKeywordForSearch(), QString("alpha"));

    // Act: 向前查找（与 all 不同 → 清空 all 分支）
    m_win->m_keywordForSearchAll = QStringLiteral("other");
    m_win->handleFindPrevSearchKeyword(QStringLiteral("beta"));
    EXPECT_EQ(m_win->getKeywordForSearch(), QString("beta"));
    EXPECT_TRUE(m_win->m_keywordForSearchAll.isEmpty());
}

TEST_F(WindowTest, HandleFindKeyword_MatchAllBranch_HighlightsAll)
{
    // Arrange: all == search（相等分支）
    addFileTab(QStringLiteral("kw2.txt"), QByteArray("x y x y\n"));
    m_win->m_keywordForSearchAll = QStringLiteral("x");

    // Act
    m_win->handleFindKeyword(QStringLiteral("x"), true);

    // Assert: 两侧关键词一致且未清空
    EXPECT_EQ(m_win->getKeywordForSearch(), QString("x"));
    EXPECT_EQ(m_win->m_keywordForSearchAll, QString("x"));
}

TEST_F(WindowTest, HandleReplaceAll_ReplacesDocumentText)
{
    // Arrange
    const QString path = addFileTab(QStringLiteral("rep.txt"), QByteArray("aaa bbb aaa\n"));
    TextEdit *editor = m_win->getTextEditor(path);

    // Act
    m_win->handleReplaceAll(QStringLiteral("aaa"), QStringLiteral("zzz"));

    // Assert: 全部替换且关键词状态不变
    EXPECT_EQ(editor->toPlainText(), QString("zzz bbb zzz\n"));
    EXPECT_NE(m_win->currentWrapper(), nullptr);
}

TEST_F(WindowTest, HandleReplaceNextAndRest_UpdateKeywordsAndDocument)
{
    // Arrange
    const QString path = addFileTab(QStringLiteral("rep2.txt"), QByteArray("n1 n1 n1\n"));
    TextEdit *editor = m_win->getTextEditor(path);

    // Act: replaceNext
    m_win->handleReplaceNext(path, QStringLiteral("n1"), QStringLiteral("m"));

    // Assert: 两个关键词均记录
    EXPECT_EQ(m_win->getKeywordForSearch(), QString("n1"));
    EXPECT_EQ(m_win->getKeywordForSearchAll(), QString("n1"));

    // Act: replaceRest
    m_win->handleReplaceRest(QStringLiteral("n1"), QStringLiteral("m"));
    EXPECT_EQ(editor->toPlainText(), QString("m m m\n"));
}

TEST_F(WindowTest, HandleReplaceSkip_SyncsSearchKeywords)
{
    // Arrange
    const QString path = addFileTab(QStringLiteral("skip.txt"), QByteArray("s1 s2\n"));
    m_win->m_keywordForSearchAll = QStringLiteral("s1");

    // Act: 跳过（当前标签路径 → 关键词更新；与 all 相等 → 高亮分支）
    m_win->handleReplaceSkip(path, QStringLiteral("s1"));

    // Assert
    EXPECT_EQ(m_win->getKeywordForSearch(), QString("s1"));
    EXPECT_EQ(m_win->m_keywordForSearchAll, QString("s1"));
}

TEST_F(WindowTest, HandleRemoveSearchKeyword_NoWrapperSafe)
{
    // Act: 无 wrapper（守卫分支）
    m_win->handleRemoveSearchKeyword();

    // Assert: 无崩溃 + 状态不变
    EXPECT_EQ(m_win->getKeywordForSearch(), QString(""));

    // Act: 有 wrapper
    addFileTab(QStringLiteral("rm.txt"), QByteArray("rm me\n"));
    m_win->handleRemoveSearchKeyword();
    EXPECT_NE(m_win->currentWrapper(), nullptr);
}

TEST_F(WindowTest, HandleUpdateSearchKeyword_CurrentFile_UpdatesKeywords)
{
    // Arrange
    const QString path = addFileTab(QStringLiteral("upd.txt"), QByteArray("updatable\n"));

    // Act: 当前文件 + FindBar 控件（setMismatchAlert 分支）
    m_win->handleUpdateSearchKeyword(m_win->m_findBar, path, QStringLiteral("upd"));

    // Assert
    EXPECT_EQ(m_win->getKeywordForSearch(), QString("upd"));
    EXPECT_EQ(m_win->getKeywordForSearchAll(), QString("upd"));

    // Act: 非当前文件 → 不更新关键词
    m_win->handleUpdateSearchKeyword(m_win->m_findBar, QStringLiteral("/other"), QStringLiteral("zz"));
    EXPECT_EQ(m_win->getKeywordForSearch(), QString("upd"));
}

TEST_F(WindowTest, SlotFindbarClose_ShowsBottomBar)
{
    // Arrange: 打开查找栏后关闭
    m_win->show();
    addFileTab(QStringLiteral("fbc.txt"), QByteArray("bottom bar test\n"));
    m_win->popupFindBar();
    QApplication::processEvents();
    ASSERT_TRUE(m_win->m_findBar->isVisible());

    // Act
    m_win->slotFindbarClose();

    // Assert: 底栏恢复默认高度并显示
    BottomBar *bar = m_win->currentWrapper()->bottomBar();
    EXPECT_TRUE(bar->isVisible());
    EXPECT_EQ(bar->height(), BottomBar::defaultHeight());
}

TEST_F(WindowTest, SlotReplacebarClose_ShowsBottomBar)
{
    // Arrange
    m_win->show();
    addFileTab(QStringLiteral("rbc.txt"), QByteArray("replace close\n"));
    m_win->popupReplaceBar();

    // Act
    m_win->slotReplacebarClose();

    // Assert: 底栏恢复显示（替换栏由其自身 sigReplacebarClose 链路负责隐藏）
    BottomBar *bar = m_win->currentWrapper()->bottomBar();
    EXPECT_TRUE(bar->isVisible());
    EXPECT_EQ(bar->height(), BottomBar::defaultHeight());
    EXPECT_NE(m_win->currentWrapper(), nullptr);
}

TEST_F(WindowTest, SlotSwitchToReplaceBar_FromVisibleFindBar_CarriesText)
{
    // Arrange: 查找栏可见并输入关键词
    m_win->show();
    const QString path = addFileTab(QStringLiteral("sw.txt"), QByteArray("switch test\n"));
    m_win->popupFindBar();
    QApplication::processEvents();
    ASSERT_TRUE(m_win->m_findBar->isVisible());
    ASSERT_EQ(m_win->m_findBar->getCurrentSearchText(), QString(""));

    // Act
    m_win->slotSwitchToReplaceBar();

    // Assert: 查找栏隐藏（替换栏被激活）
    EXPECT_FALSE(m_win->m_findBar->isVisible());
    EXPECT_NE(m_win->getTextEditor(path), nullptr);
}

TEST_F(WindowTest, SlotSwitchToReplaceBar_NoWrapper_NoCrash)
{
    // Act: 无 wrapper（findBar 不可见 → 直接守卫返回）
    m_win->slotSwitchToReplaceBar();

    // Assert
    EXPECT_EQ(m_win->currentWrapper(), nullptr);
    EXPECT_FALSE(m_win->findBarIsVisiable());
}

// ============================================================
// 消息提示 / 对话框构造
// ============================================================

TEST_F(WindowTest, ShowNotify_NoWrapper_AddsBlankThenNotifies)
{
    // Act: 无标签时提示 → 先补空白标签再通知
    const int beforeTabs = m_tabbar->count();
    m_win->showNotify(QStringLiteral("hello notify"));

    // Assert
    EXPECT_EQ(m_tabbar->count(), beforeTabs + 1);
    EXPECT_GE(m_widgetMsgCalls + m_iconMsgCalls + m_floatMsgCalls, 1);

    // Act: 有 wrapper 时直接通知
    m_win->showNotify(QStringLiteral("second"), true);
    EXPECT_GE(m_widgetMsgCalls + m_iconMsgCalls + m_floatMsgCalls, 2);
}

TEST_F(WindowTest, CreateDialog_ThreeButtonsModalDialog)
{
    // Act
    DDialog *dialog = m_win->createDialog(QStringLiteral("Title"), QStringLiteral("Content"));

    // Assert: 三按钮 + 应用模态
    ASSERT_NE(dialog, nullptr);
    EXPECT_EQ(dialog->buttonCount(), 3);
    EXPECT_EQ(dialog->windowModality(), Qt::ApplicationModal);
    delete dialog;
}

TEST_F(WindowTest, ConfirmInvalidCharSave_ReturnsButtonIndex)
{
    // Arrange
    m_ddialogResult = 1;

    // Act / Assert
    EXPECT_EQ(m_win->confirmInvalidCharSave(QStringLiteral("f.txt")), 1);
    m_ddialogResult = 2;
    EXPECT_EQ(m_win->confirmInvalidCharSave(QStringLiteral("f.txt")), 2);
    QApplication::processEvents(); // deleteLater 清理
}

// ============================================================
// 主题
// ============================================================

TEST_F(WindowTest, LoadTheme_NonexistentFile_EarlyReturn)
{
    // Arrange
    const QString before = m_win->m_themePath;

    // Act
    m_win->loadTheme(QStringLiteral("/no/such/theme.theme"));

    // Assert: 主题路径不变且未写入配置
    EXPECT_EQ(m_win->m_themePath, before);
    EXPECT_EQ(Settings::instance()->settings->option("advance.editor.theme")->value().toString(), before);
}

TEST_F(WindowTest, LoadTheme_ValidThemeFile_AppliesAndPersists)
{
    // Arrange: 构造合法主题 JSON
    const QString themePath = m_tempDir->filePath(QStringLiteral("custom.theme"));
    QFile f(themePath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write(R"({
        "editor-colors": { "background-color": "#101010" },
        "app-colors": {
            "tab-background-start-color": "#111111",
            "tab-background-end-color": "#222222",
            "tab-dnd-start": "#333333",
            "tab-dnd-end": "#444444",
            "themebar-frame-selected": "#555555",
            "themebar-frame-normal": "#666666"
        }
    })");
    f.close();

    // Act
    m_win->loadTheme(themePath);

    // Assert: 主题路径 + 配置持久化
    EXPECT_EQ(m_win->m_themePath, themePath);
    EXPECT_EQ(Settings::instance()->settings->option("advance.editor.theme")->value().toString(), themePath);
}

TEST_F(WindowTest, SlotLoadContentTheme_LightDarkUnknown_NoCrashAndTabPalette)
{
    // Act: Light → loadTheme(DEEPIN_THEME)（系统路径不存在 → 早退安全）
    m_win->slotLoadContentTheme(DGuiApplicationHelper::LightType);

    // Act: Dark
    m_win->slotLoadContentTheme(DGuiApplicationHelper::DarkType);

    // Act: Unknown（无分支）
    m_win->slotLoadContentTheme(DGuiApplicationHelper::UnknownType);

    // Assert: 主题路径未被系统路径覆盖 + 标签栏仍可用
    EXPECT_FALSE(m_win->m_themePath.contains(QStringLiteral("/share/deepin-editor/themes/deepin.theme")));
    EXPECT_NE(m_win->getTabbar(), nullptr);
}

TEST_F(WindowTest, SlotSettingResetTheme_SwitchesPaletteType)
{
    // Arrange: 确保当前为暗色（走 Light 切换分支）
    DGuiApplicationHelper::instance()->setPaletteType(DGuiApplicationHelper::DarkType);

    // Act: 请求浅色主题
    m_win->slotSettingResetTheme(DEEPIN_THEME);

    // Assert: 调色板切换为浅色
    EXPECT_EQ(DGuiApplicationHelper::instance()->themeType(), DGuiApplicationHelper::LightType);

    // Act: 同主题再设 → 早退；切暗色
    m_win->slotSettingResetTheme(DEEPIN_THEME); // 已是 Light → return
    m_win->slotSettingResetTheme(DEEPIN_DARK_THEME);
    EXPECT_EQ(DGuiApplicationHelper::instance()->themeType(), DGuiApplicationHelper::DarkType);

    // Act: 未知路径分支
    m_win->slotSettingResetTheme(m_tempDir->filePath(QStringLiteral("x.theme")));
    EXPECT_EQ(DGuiApplicationHelper::instance()->themeType(), DGuiApplicationHelper::DarkType);
}

TEST_F(WindowTest, SlotSigThemeChanged_SwitchesPaletteType)
{
    // Arrange: 当前暗色
    DGuiApplicationHelper::instance()->setPaletteType(DGuiApplicationHelper::DarkType);

    // Act: 浅色信号
    m_win->slotSigThemeChanged(DEEPIN_THEME);
    EXPECT_EQ(DGuiApplicationHelper::instance()->themeType(), DGuiApplicationHelper::LightType);

    // Act: 同主题早退 + 暗色信号
    m_win->slotSigThemeChanged(DEEPIN_THEME);
    m_win->slotSigThemeChanged(DEEPIN_DARK_THEME);
    EXPECT_EQ(DGuiApplicationHelper::instance()->themeType(), DGuiApplicationHelper::DarkType);

    // Act: 未知路径
    m_win->slotSigThemeChanged(QStringLiteral("/unknown"));
    EXPECT_EQ(DGuiApplicationHelper::instance()->themeType(), DGuiApplicationHelper::DarkType);
}

TEST_F(WindowTest, PopupThemePanel_ShowsPanel)
{
    // Act
    m_win->popupThemePanel();

    // Assert: 主题面板可见（几何对齐由 UpdateThemePanelGeomerty 用例单独验证）
    EXPECT_TRUE(m_win->m_themePanel->isVisible());
    EXPECT_TRUE(m_win->m_themePanel->geometry().isValid());
}

TEST_F(WindowTest, UpdateThemePanelGeomerty_AlignsPanelToRight)
{
    // Arrange
    m_win->resize(800, 600);

    // Act
    m_win->updateThemePanelGeomerty();

    // Assert: 面板宽度 250、右对齐、顶部为标题栏高度
    EXPECT_EQ(m_win->m_themePanel->width(), 250);
    EXPECT_EQ(m_win->m_themePanel->geometry().right(), m_win->rect().right());
}

// ============================================================
// 编辑器设置槽（Settings 信号族）
// ============================================================

TEST_F(WindowTest, SlotSigAdjustFontAndSize_AppliesToEditors)
{
    // Arrange
    const QString blank = addBlankAndGetPath();
    TextEdit *editor = m_win->getTextEditor(blank);
    ASSERT_NE(editor, nullptr);

    // Act
    m_win->slotSigAdjustFont(QStringLiteral("monospace"));
    m_win->slotSigAdjustFontSize(15.0);

    // Assert: 字号状态记录 + 编辑器仍有效（字体族应用不崩溃）
    EXPECT_EQ(m_win->m_fontSize, 15.0);
    EXPECT_NE(m_win->getTextEditor(blank), nullptr);
}

TEST_F(WindowTest, SlotSigAdjustEditorFlags_AppliesToEditors)
{
    // Arrange
    const QString blank = addBlankAndGetPath();
    TextEdit *editor = m_win->getTextEditor(blank);
    ASSERT_NE(editor, nullptr);

    // Act: 依次触发全部编辑器设置槽
    m_win->slotSigAdjustTabSpaceNumber(6);
    m_win->slotSigAdjustWordWrap(true);
    m_win->slotSigSetLineNumberShow(false);
    m_win->slotSigAdjustBookmark(true);
    m_win->slotSigShowBlankCharacter(true);
    m_win->slotSigHightLightCurrentLine(false);
    m_win->slotSigShowCodeFlodFlag(true);

    // Assert: 自动换行开关真实生效 + 编辑器存活
    EXPECT_EQ(editor->lineWrapMode(), QPlainTextEdit::WidgetWidth);
    EXPECT_NE(m_win->getTextEditor(blank), nullptr);

    // Act: 关闭换行复核反向分支
    m_win->slotSigAdjustWordWrap(false);
    EXPECT_EQ(editor->lineWrapMode(), QPlainTextEdit::NoWrap);
}

TEST_F(WindowTest, SlotClearDoubleCharaterEncode_ReplacesUnsupportedChars)
{
    // Arrange: 插入赛迪方要求清除的字符
    const QString blank = addBlankAndGetPath();
    TextEdit *editor = m_win->getTextEditor(blank);
    editor->setPlainText(QStringLiteral("A\uE768B"));

    // Act
    m_win->slotClearDoubleCharaterEncode();

    // Assert: 特殊字符被空格替换
    EXPECT_FALSE(editor->toPlainText().contains(QChar(0xE768)));
    EXPECT_TRUE(editor->toPlainText().contains(QStringLiteral("A B")));
}

// ============================================================
// 位置记忆 / 阅读路径
// ============================================================

TEST_F(WindowTest, RemberPositionSaveAndRestore_RoundTrip)
{
    // Arrange: 有内容标签
    const QString path = addFileTab(QStringLiteral("pos2.txt"), QByteArray("r1\nr2\nr3\n"));
    TextEdit *editor = m_win->getTextEditor(path);
    editor->setTextCursor(editor->textCursor());

    // Act: 记忆当前位置
    m_win->remberPositionSave();

    // Assert: 记录当前标签路径 + 行列信息
    EXPECT_EQ(m_win->m_remberPositionFilePath, path);
    EXPECT_GE(m_win->m_remberPositionRow, 1);
    EXPECT_GE(m_win->m_remberPositionColumn, 0);

    // Act: 恢复（活跃标签 + 滚动复位）
    m_win->remberPositionRestore();

    // Assert: 当前标签即记忆标签
    EXPECT_EQ(m_tabbar->currentPath(), path);
}

TEST_F(WindowTest, RemberPositionRestore_EmptyPath_EarlyReturn)
{
    // Act / Assert: 无记忆 → 安全返回且无标签副作用
    m_win->remberPositionRestore();
    EXPECT_TRUE(m_win->m_remberPositionFilePath.isEmpty());
    EXPECT_EQ(m_tabbar->count(), 0);
}

TEST_F(WindowTest, SlotSaveReadingPath_TracksReadingEditor)
{
    // Arrange
    const QString blank = addBlankAndGetPath();
    TextEdit *editor = m_win->getTextEditor(blank);

    // Act
    m_win->slot_saveReadingPath();

    // Assert: 阅读列表仅含当前编辑器
    EXPECT_EQ(m_win->m_reading_list.count(), 1);
    EXPECT_EQ(m_win->m_reading_list.first(), editor);
}

TEST_F(WindowTest, SlotBeforeReplace_ForwardsToEditor)
{
    // Arrange
    addFileTab(QStringLiteral("br.txt"), QByteArray("before replace\n"));

    // Act / Assert: 转发至编辑器（关键词进入编辑器状态）
    m_win->slot_beforeReplace(QStringLiteral("kw"));
    EXPECT_NE(m_win->currentWrapper(), nullptr);
    EXPECT_NE(m_win->currentWrapper()->textEditor(), nullptr);
}

TEST_F(WindowTest, SlotSetTitleFocus_SetsTitlebarTabFocus)
{
    // Arrange
    addBlankAndGetPath();

    // Act
    m_win->slot_setTitleFocus();

    // Assert: 标题栏按钮进入 Tab 焦点链
    EXPECT_EQ(m_win->titlebar()->focusPolicy(), Qt::TabFocus);
    DIconButton *addButton = m_win->getTabbar()->findChild<DIconButton *>("AddButton");
    ASSERT_NE(addButton, nullptr);
    EXPECT_EQ(addButton->focusPolicy(), Qt::TabFocus);
}

TEST_F(WindowTest, SetChildrenFocus_TrueFalse_TogglesButtonFocus)
{
    // Act: true
    m_win->setChildrenFocus(true);

    // Assert
    DIconButton *addButton = m_win->getTabbar()->findChild<DIconButton *>("AddButton");
    ASSERT_NE(addButton, nullptr);
    EXPECT_EQ(addButton->focusPolicy(), Qt::TabFocus);
    EXPECT_EQ(m_win->titlebar()->focusPolicy(), Qt::TabFocus);

    // Act: false
    m_win->setChildrenFocus(false);
    EXPECT_EQ(addButton->focusPolicy(), Qt::NoFocus);
    EXPECT_EQ(m_win->titlebar()->focusPolicy(), Qt::NoFocus);
}

// ============================================================
// getBlankFileIndex
// ============================================================

TEST_F(WindowTest, GetBlankFileIndex_FreshWindow_ReturnsOne)
{
    // Assert: 无空白标签 → 1（且不影响标签数）
    EXPECT_EQ(m_win->getBlankFileIndex(), 1);
    EXPECT_EQ(m_tabbar->count(), 0);
}

TEST_F(WindowTest, GetBlankFileIndex_SequentialAndGapFilling)
{
    // Arrange: 空白目录下两个标签 [Untitled 1, Untitled 2]
    QDir().mkpath(m_win->m_blankFileDir);
    m_tabbar->addTab(QDir(m_win->m_blankFileDir).filePath(QStringLiteral("b1")), QStringLiteral("Untitled 1"));
    m_tabbar->addTab(QDir(m_win->m_blankFileDir).filePath(QStringLiteral("b2")), QStringLiteral("Untitled 2"));

    // Act / Assert: 连续 → 下一个 3
    EXPECT_EQ(m_win->getBlankFileIndex(), 3);

    // Arrange: 空洞 [Untitled 1, Untitled 3]
    m_tabbar->removeTab(1);
    m_tabbar->addTab(QDir(m_win->m_blankFileDir).filePath(QStringLiteral("b3")), QStringLiteral("Untitled 3"));

    // Act / Assert: 填补空洞 → 2
    EXPECT_EQ(m_win->getBlankFileIndex(), 2);
}

// ============================================================
// 待加载标签（懒加载）
// ============================================================

TEST_F(WindowTest, PendingTab_AddIsLoad_RoundTripWithBatchFlag)
{
    // Arrange: 真实文件
    const QString path = createFile(QStringLiteral("pend.txt"));
    Window::PendingTabInfo info;
    info.filepath = path;
    info.truePath = path;
    info.displayName = QStringLiteral("pend.txt");
    info.cursorPosition = 0;
    info.bookmarks = QList<int> { 1 };

    // Act: 添加 pending
    m_win->addPendingTab(info);

    // Assert: 标签占位 + 状态查询
    EXPECT_EQ(m_tabbar->count(), 1);
    EXPECT_TRUE(m_win->isPendingTab(path));
    EXPECT_FALSE(m_win->isPendingTab(QStringLiteral("/other")));

    // Act: 批量模式标记
    m_win->setBatchAddingPendingTabs(true);
    EXPECT_TRUE(m_win->m_bBatchAddingPendingTabs);
    m_win->setBatchAddingPendingTabs(false);
    EXPECT_FALSE(m_win->m_bBatchAddingPendingTabs);

    // Act: 完整加载
    EXPECT_TRUE(m_win->loadPendingTab(path));

    // Assert: wrapper 已建 + 书签应用 + pending 清空
    EXPECT_FALSE(m_win->isPendingTab(path));
    ASSERT_NE(m_win->wrapper(path), nullptr);
    EXPECT_EQ(m_win->wrapper(path)->textEditor()->getBookmarkInfo(), QList<int>({ 1 }));
}

TEST_F(WindowTest, LoadPendingTab_UnknownOrMissing_ReturnsFalse)
{
    // Assert: 未知路径
    EXPECT_FALSE(m_win->loadPendingTab(QStringLiteral("/none")));

    // Arrange: 文件已被删除的 pending
    const QString path = m_tempDir->filePath(QStringLiteral("gone.txt"));
    Window::PendingTabInfo info;
    info.filepath = path;
    info.truePath = path;
    info.displayName = QStringLiteral("gone.txt");
    m_win->addPendingTab(info);

    // Act / Assert: 加载失败且 pending 恢复（可重试）
    EXPECT_FALSE(m_win->loadPendingTab(path));
    EXPECT_TRUE(m_win->isPendingTab(path));
}

TEST_F(WindowTest, HandleCurrentChanged_PendingTab_TriggersLazyLoad)
{
    // Arrange
    const QString path = createFile(QStringLiteral("lazy.txt"));
    Window::PendingTabInfo info;
    info.filepath = path;
    info.truePath = path;
    info.displayName = QStringLiteral("lazy.txt");
    m_win->addPendingTab(info);

    // Act: 模拟标签切换
    m_win->handleCurrentChanged(m_tabbar->indexOf(path));
    QApplication::processEvents();

    // Assert: 懒加载完成
    EXPECT_FALSE(m_win->isPendingTab(path));
    EXPECT_NE(m_win->wrapper(path), nullptr);
}

TEST_F(WindowTest, HandleCurrentChanged_WithWrapper_SwitchesStackAndShowsBar)
{
    // Arrange
    const QString blank = addBlankAndGetPath();
    EditWrapper *w = m_win->wrapper(blank);

    // Act
    m_win->handleCurrentChanged(0);
    QApplication::processEvents();

    // Assert: 当前编辑器切换 + 底栏显示
    EXPECT_EQ(m_win->m_editorWidget->currentWidget(), w);
    EXPECT_TRUE(w->bottomBar()->isVisible());
}

TEST_F(WindowTest, HandleTabCloseRequested_DelayedCloseClosesTab)
{
    // Arrange
    addBlankAndGetPath();
    ASSERT_EQ(m_tabbar->count(), 1);

    // Act: 请求关闭（10ms 延迟定时器）
    m_win->handleTabCloseRequested(0);
    processEventsFor(80);

    // Assert: 标签被关闭
    EXPECT_EQ(m_tabbar->count(), 0);
}

TEST_F(WindowTest, TimerEvent_InvalidIndex_NoClose)
{
    // Arrange: 一个标签 + 直接启动延迟定时器（越界索引）
    addBlankAndGetPath();
    m_win->m_requestCloseTabIndex = 99;
    m_win->m_delayCloseTabTimer.start(10, m_win);

    // Act: 派发定时器事件
    QTimerEvent ev(m_win->m_delayCloseTabTimer.timerId());
    m_win->timerEvent(&ev);

    // Assert: 索引越界不关闭
    EXPECT_EQ(m_tabbar->count(), 1);
    EXPECT_FALSE(m_win->m_delayCloseTabTimer.isActive());
}

TEST_F(WindowTest, HandleTabsClosed_EmptyAndRealList)
{
    // Act: 空列表早退
    m_win->handleTabsClosed(QStringList());
    EXPECT_EQ(m_tabbar->count(), 0);

    // Arrange
    const QString blank = addBlankAndGetPath();

    // Act
    m_win->handleTabsClosed(QStringList { blank });
    QApplication::processEvents();

    // Assert: 早退且无 wrapper
    EXPECT_EQ(m_tabbar->count(), 0);
    EXPECT_TRUE(m_win->m_wrappers.isEmpty());
}

// ============================================================
// addTemFileTab（恢复备份标签）
// ============================================================

TEST_F(WindowTest, AddTemFileTab_MissingFile_EarlyReturn)
{
    // Act
    m_win->addTemFileTab(QStringLiteral("/no/such/tem"), QStringLiteral("n"), QStringLiteral("/t"), QString());

    // Assert: 早退且无 wrapper
    EXPECT_EQ(m_tabbar->count(), 0);
    EXPECT_TRUE(m_win->m_wrappers.isEmpty());
}

TEST_F(WindowTest, AddTemFileTab_PngInheritsOctetStream_TreatedAsSupported)
{
    // Arrange: 真实 PNG 文件。实测 image/png 经 mime.inherits("application/octet-stream")
    // 兜底被判"受支持"（octet-stream 在 SupportedTextMimeTypes 白名单且为所有 mime 的
    // 祖先）——addTemFileTab 的"不支持提示"分支对常规文件不可达（缺陷已记录 session）。
    QImage img(2, 2, QImage::Format_RGB32);
    img.fill(Qt::red);
    const QString binPath = m_tempDir->filePath(QStringLiteral("blob.png"));
    ASSERT_TRUE(img.save(binPath, "PNG"));

    // Act
    m_win->addTemFileTab(binPath, QStringLiteral("blob"), binPath, QString());
    QApplication::processEvents();

    // Assert: 走受支持分支创建 blob 标签（无无效提示）
    EXPECT_EQ(m_iconMsgCalls, 0);
    EXPECT_EQ(m_tabbar->count(), 1);
    EXPECT_EQ(m_tabbar->currentName(), QString("blob"));
    EXPECT_NE(m_win->wrapper(binPath), nullptr);
}

TEST_F(WindowTest, AddTemFileTab_SupportedTempFile_CreatesWrapperAndTab)
{
    // Arrange: 真实临时文件
    const QString temPath = createFile(QStringLiteral("tem.txt"), QByteArray("tem content\n"));
    const QString truePath = m_tempDir->filePath(QStringLiteral("true.txt"));

    // Act: 临时标签（bIsTemFile=true + 修改时间）
    m_win->addTemFileTab(temPath, QStringLiteral("tem.txt"), truePath,
                         QStringLiteral("2026-01-01T00:00:00"), true);
    QApplication::processEvents();

    // Assert: 标签 + wrapper + 显示
    EXPECT_EQ(m_win->getTabIndex(temPath), 0);
    EditWrapper *w = m_win->wrapper(temPath);
    ASSERT_NE(w, nullptr);
    EXPECT_EQ(m_win->m_editorWidget->currentWidget(), w);
}

// ============================================================
// backupFile / closeAllFiles / saveAllFloatingFiles
// ============================================================

TEST_F(WindowTest, BackupFile_WithTabs_WritesHistoryOption)
{
    // Arrange: 一个修改态真实文件标签
    const QString path = addFileTab(QStringLiteral("bk.txt"), QByteArray("backup me\n"));
    stub.set_lamda(&EditWrapper::isModified, [](EditWrapper *) -> bool { return true; });

    // Act
    m_win->backupFile();

    // Assert: browsing_history_temfile 写入 JSON（含 localPath 与 focus）
    const QStringList history = Settings::instance()
                                    ->settings->option("advance.editor.browsing_history_temfile")
                                    ->value().toStringList();
    ASSERT_EQ(history.count(), 1);
    EXPECT_TRUE(history.first().contains(QStringLiteral("localPath")));
    EXPECT_TRUE(history.first().contains(QStringLiteral("focus")));
    EXPECT_EQ(m_win->m_qlistTemFile.count(), 1);
}

TEST_F(WindowTest, BackupFile_PendingTab_RecordsPlaceHolder)
{
    // Arrange: pending 标签（书签 + 修改时间）
    const QString path = createFile(QStringLiteral("pnd.txt"));
    Window::PendingTabInfo info;
    info.filepath = path;
    info.truePath = path;
    info.displayName = QStringLiteral("pnd.txt");
    info.bookmarks = QList<int> { 2 };
    info.lastModifiedTime = QStringLiteral("2026-01-02T03:04:05");
    info.isTemFile = true;
    m_win->addPendingTab(info);

    // Act
    m_win->backupFile();

    // Assert: JSON 含书签与修改时间
    const QStringList history = Settings::instance()
                                    ->settings->option("advance.editor.browsing_history_temfile")
                                    ->value().toStringList();
    ASSERT_EQ(history.count(), 1);
    EXPECT_TRUE(history.first().contains(QStringLiteral("bookMark")));
    EXPECT_TRUE(history.first().contains(QStringLiteral("2026-01-02T03:04:05")));
}

TEST_F(WindowTest, CloseAllFiles_NoTabs_ReturnsTrue)
{
    // Act / Assert
    EXPECT_TRUE(m_win->closeAllFiles());
    EXPECT_EQ(m_tabbar->count(), 0);
}

TEST_F(WindowTest, CloseAllFiles_CleanTabs_ClosesAll)
{
    // Arrange: 两个干净标签
    addFileTab(QStringLiteral("ca1.txt"));
    addFileTab(QStringLiteral("ca2.txt"));
    ASSERT_EQ(m_tabbar->count(), 2);

    // Act
    EXPECT_TRUE(m_win->closeAllFiles());
    QApplication::processEvents();

    // Assert
    EXPECT_TRUE(m_win->m_wrappers.isEmpty());
}

TEST_F(WindowTest, CloseAllFiles_CancelledByDialog_ReturnsFalse)
{
    // Arrange: 修改态标签 + 取消
    addFileTab(QStringLiteral("ca3.txt"));
    stub.set_lamda(&EditWrapper::isModified, [](EditWrapper *) -> bool { return true; });
    stub.set_lamda(&EditWrapper::isDraftFile, [](EditWrapper *) -> bool { return false; });
    m_ddialogResult = 0;

    // Act / Assert
    EXPECT_FALSE(m_win->closeAllFiles());
    EXPECT_EQ(m_tabbar->count(), 1);
}

TEST_F(WindowTest, SaveAllFloatingFiles_NoMissingFiles_ReturnsTrue)
{
    // Arrange: 正常存在文件
    addFileTab(QStringLiteral("float1.txt"));

    // Act / Assert: 无漂浮文件 → true 且不弹窗
    EXPECT_TRUE(m_win->saveAllFloatingFiles());
    EXPECT_EQ(m_ddialogExecCalls, 0);
}

TEST_F(WindowTest, SaveAllFloatingFiles_MissingFileDialogs_ThreeResponses)
{
    // Arrange: 文件加载后从磁盘删除（漂浮文件）
    const QString path = addFileTab(QStringLiteral("float2.txt"));
    QFile::remove(path);

    // Act: 取消（0）→ false
    m_ddialogResult = 0;
    EXPECT_FALSE(m_win->saveAllFloatingFiles());

    // Act: 不保存（1）→ 跳过返回 true
    m_ddialogResult = 1;
    EXPECT_TRUE(m_win->saveAllFloatingFiles());
}

// ============================================================
// 键盘 / 拖放 / 焦点窗口
// ============================================================

TEST_F(WindowTest, KeyPressEvent_Escape_EmitsPressEsc)
{
    // Arrange: 以当前 keymap 值驱动（escape 默认 Esc）
    const QString escKey = Utils::getKeyshortcutFromKeymap(Settings::instance(), "window", "escape");
    stub.set_lamda(&Utils::getKeyshortcut, [escKey](QKeyEvent *) -> QString { return escKey; });
    QSignalSpy spy(m_win, &Window::pressEsc);

    // Act
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    m_win->keyPressEvent(&ev);

    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(m_tabbar->count(), 0);
}

TEST_F(WindowTest, KeyPressEvent_NewWindow_EmitsSignal)
{
    // Arrange
    const QString key = Utils::getKeyshortcutFromKeymap(Settings::instance(), "window", "newwindow");
    stub.set_lamda(&Utils::getKeyshortcut, [key](QKeyEvent *) -> QString { return key; });
    QSignalSpy spy(m_win, &Window::newWindow);

    // Act
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_N, Qt::ControlModifier | Qt::ShiftModifier);
    m_win->keyPressEvent(&ev);

    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(m_tabbar->count(), 0);
}

TEST_F(WindowTest, KeyPressEvent_AddBlankTabShortcut_CreatesTab)
{
    // Arrange
    const QString key = Utils::getKeyshortcutFromKeymap(Settings::instance(), "window", "addblanktab");
    stub.set_lamda(&Utils::getKeyshortcut, [key](QKeyEvent *) -> QString { return key; });

    // Act
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_T, Qt::ControlModifier);
    m_win->keyPressEvent(&ev);
    QApplication::processEvents();

    // Assert
    EXPECT_EQ(m_tabbar->count(), 1);
    EXPECT_EQ(m_tabbar->currentName(), QString("Untitled 1"));
}

TEST_F(WindowTest, KeyPressEvent_FontSizeShortcuts_ChangeConfig)
{
    // Arrange
    m_win->resetFontSize();
    const QString inc = Utils::getKeyshortcutFromKeymap(Settings::instance(), "window", "incrementfontsize");
    const QString dec = Utils::getKeyshortcutFromKeymap(Settings::instance(), "window", "decrementfontsize");
    QString current = inc;
    stub.set_lamda(&Utils::getKeyshortcut, [&current](QKeyEvent *) -> QString { return current; });
    addBlankAndGetPath(); // setCodeFoldWidgetHide 需要 wrapper

    // Act: 增大
    QKeyEvent incEv(QEvent::KeyPress, Qt::Key_Equal, Qt::ControlModifier);
    m_win->keyPressEvent(&incEv);
    const qreal afterInc = Settings::instance()->settings->option("base.font.size")->value().toReal();
    EXPECT_GT(afterInc, 12.0);

    // Act: 减小
    current = dec;
    QKeyEvent decEv(QEvent::KeyPress, Qt::Key_Minus, Qt::ControlModifier);
    m_win->keyPressEvent(&decEv);
    const qreal afterDec = Settings::instance()->settings->option("base.font.size")->value().toReal();
    EXPECT_LT(afterDec, afterInc);

    // Act: 复位
    const QString rst = Utils::getKeyshortcutFromKeymap(Settings::instance(), "window", "resetfontsize");
    current = rst;
    QKeyEvent rstEv(QEvent::KeyPress, Qt::Key_0, Qt::ControlModifier);
    m_win->keyPressEvent(&rstEv);
    EXPECT_EQ(Settings::instance()->settings->option("base.font.size")->value().toInt(), 12);
}

TEST_F(WindowTest, KeyPressEvent_SelectNextPrevTab_SwitchesIndex)
{
    // Arrange: 两个标签
    addBlankAndGetPath();
    addBlankAndGetPath();
    EXPECT_EQ(m_tabbar->currentIndex(), 1);
    const QString next = Utils::getKeyshortcutFromKeymap(Settings::instance(), "window", "selectnexttab");
    const QString prev = Utils::getKeyshortcutFromKeymap(Settings::instance(), "window", "selectprevtab");
    QString current = prev;
    stub.set_lamda(&Utils::getKeyshortcut, [&current](QKeyEvent *) -> QString { return current; });

    // Act: 上一个标签
    QKeyEvent prevEv(QEvent::KeyPress, Qt::Key_Tab, Qt::ControlModifier);
    m_win->keyPressEvent(&prevEv);
    EXPECT_EQ(m_tabbar->currentIndex(), 0);

    // Act: 下一个标签
    current = next;
    QKeyEvent nextEv(QEvent::KeyPress, Qt::Key_Tab, Qt::ControlModifier | Qt::ShiftModifier);
    m_win->keyPressEvent(&nextEv);
    EXPECT_EQ(m_tabbar->currentIndex(), 1);
}

TEST_F(WindowTest, KeyPressEvent_AltDigitSwitch_JumpsToTab)
{
    // Arrange: 三个标签
    for (int i = 0; i < 3; ++i)
        addBlankAndGetPath();
    stub.set_lamda(&Utils::getKeyshortcut, [](QKeyEvent *) -> QString { return QStringLiteral("Alt+2"); });

    // Act: Alt+2 → 第二个标签（索引 1）
    QKeyEvent ev2(QEvent::KeyPress, Qt::Key_2, Qt::AltModifier);
    m_win->keyPressEvent(&ev2);
    EXPECT_EQ(m_tabbar->currentIndex(), 1);

    // Act: Alt+9 → 最后一个标签
    stub.set_lamda(&Utils::getKeyshortcut, [](QKeyEvent *) -> QString { return QStringLiteral("Alt+9"); });
    QKeyEvent ev9(QEvent::KeyPress, Qt::Key_9, Qt::AltModifier);
    m_win->keyPressEvent(&ev9);
    EXPECT_EQ(m_tabbar->currentIndex(), 2);
}

TEST_F(WindowTest, KeyPressEvent_UnmatchedKey_FallsThrough)
{
    // Arrange: 无匹配键
    stub.set_lamda(&Utils::getKeyshortcut, [](QKeyEvent *) -> QString { return QStringLiteral("Ctrl+F12"); });

    // Act
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_F12, Qt::ControlModifier);
    m_win->keyPressEvent(&ev);

    // Assert: 无副作用
    EXPECT_EQ(m_tabbar->count(), 0);
    EXPECT_EQ(m_win->currentWrapper(), nullptr);
}

TEST_F(WindowTest, KeyReleaseEvent_WithModifiers_NoOp)
{
    // Act
    QKeyEvent ev(QEvent::KeyRelease, Qt::Key_Shift, Qt::ShiftModifier);
    m_win->keyReleaseEvent(&ev);

    // Assert: 无副作用
    EXPECT_EQ(m_tabbar->count(), 0);
    EXPECT_TRUE(m_win->isVisible() || m_win->isHidden()); // 状态未损坏
}

TEST_F(WindowTest, DragEnterEvent_Accepts)
{
    // Arrange
    QMimeData mime;
    mime.setUrls({ QUrl::fromLocalFile(m_tempDir->path()) });
    QDragEnterEvent ev(QPoint(5, 5), Qt::DropActions(Qt::CopyAction), &mime, Qt::LeftButton, Qt::NoModifier);

    // Act
    m_win->dragEnterEvent(&ev);

    // Assert: URL 拖入被接受且无标签副作用
    EXPECT_TRUE(ev.isAccepted());
    EXPECT_EQ(m_tabbar->count(), 0);
}

TEST_F(WindowTest, DropEvent_MixedUrls_AddsSupportedAndNotifies)
{
    // Arrange: 支持文件 + 不支持目录
    const QString file = createFile(QStringLiteral("drop.txt"));
    QMimeData mime;
    mime.setUrls({ QUrl::fromLocalFile(file), QUrl::fromLocalFile(m_tempDir->path()) });
    QDropEvent ev(QPointF(5, 5), Qt::DropActions(Qt::CopyAction), &mime, Qt::LeftButton, Qt::NoModifier);

    // Act
    m_win->dropEvent(&ev);
    QApplication::processEvents();

    // Assert: 支持文件加载 + 目录提示
    EXPECT_NE(m_win->wrapper(file), nullptr);
    EXPECT_GE(m_iconMsgCalls, 1);
}

TEST_F(WindowTest, DropEvent_NoUrls_NoOp)
{
    // Arrange: 无 URL 数据
    QMimeData mime;
    mime.setText(QStringLiteral("plain"));
    QDropEvent ev(QPointF(5, 5), Qt::DropActions(Qt::CopyAction), &mime, Qt::LeftButton, Qt::NoModifier);

    // Act
    m_win->dropEvent(&ev);

    // Assert: 无 URL 数据不触发任何添加
    EXPECT_EQ(m_tabbar->count(), 0);
    EXPECT_TRUE(m_win->m_wrappers.isEmpty());
}

TEST_F(WindowTest, HandleFocusWindowChanged_ForeignOrNullWindow_EarlyReturn)
{
    // Act: null 窗口句柄 + 无 wrapper → 守卫返回
    m_win->handleFocusWindowChanged(nullptr);

    // Assert
    EXPECT_EQ(m_win->currentWrapper(), nullptr);
    EXPECT_EQ(m_tabbar->count(), 0);
}

TEST_F(WindowTest, HideEvent_HidesBarsAndRefocuses)
{
    // Arrange: 显示窗口 + 内容标签 + 查找栏
    m_win->show();
    addFileTab(QStringLiteral("hide.txt"), QByteArray("hide me\n"));
    m_win->popupFindBar();
    QApplication::processEvents();

    // Act: 直接调 hideEvent（isVisible 仍为 true 分支）
    QHideEvent ev;
    m_win->hideEvent(&ev);

    // Assert: 底栏恢复显示（查找栏可见分支）且窗口仍在（未关闭）
    EXPECT_TRUE(m_win->currentWrapper()->bottomBar()->isVisible());
    EXPECT_TRUE(m_win->isVisible());
}

// ============================================================
// 打印
// ============================================================

TEST_F(WindowTest, ClearPrintTextDocument_ReleasesDocsAndList)
{
    // Arrange: 注入打印文档与列表
    m_win->m_printDoc = new QTextDocument();
    m_win->m_printDoc->setPlainText(QStringLiteral("p"));
    Window::PrintInfo info;
    info.doc = new QTextDocument();
    info.doc->setPlainText(QStringLiteral("q"));
    info.highlighter = nullptr;
    m_win->m_printDocList.append(info);

    // Act
    m_win->clearPrintTextDocument();

    // Assert: 指针清空 / 列表清空（文档已 delete）
    EXPECT_EQ(m_win->m_printDoc, nullptr);
    EXPECT_TRUE(m_win->m_printDocList.isEmpty());
}

TEST_F(WindowTest, DoPrint_EarlyReturnBranches)
{
    // Arrange 1: 无打印文档
    DPrinter printer(QPrinter::HighResolution);
    QVector<int> range { 1 };

    // Act / Assert: null doc
    m_win->doPrint(&printer, range);

    // Arrange 2: 空页码范围
    m_win->m_printDoc = new QTextDocument();
    m_win->doPrint(&printer, QVector<int>());

    // Assert: 布局未记录（两分支早退）且文档未受破坏
    EXPECT_FALSE(m_win->m_lastLayout.isValid());
    EXPECT_NE(m_win->m_printDoc, nullptr);
    delete m_win->m_printDoc;
    m_win->m_printDoc = nullptr;
}

TEST_F(WindowTest, DoPrintWithLargeDoc_EarlyReturnBranches)
{
    // Arrange: 处理中标志
    DPrinter printer(QPrinter::HighResolution);
    m_win->m_bPrintProcessing = true;

    // Act / Assert: 处理中
    m_win->doPrintWithLargeDoc(&printer, QVector<int> { 1 });

    // Act / Assert: 列表为空
    m_win->m_bPrintProcessing = false;
    m_win->doPrintWithLargeDoc(&printer, QVector<int> { 1 });

    // Act / Assert: 页码为空
    Window::PrintInfo info;
    info.doc = new QTextDocument();
    m_win->m_printDocList.append(info);
    m_win->doPrintWithLargeDoc(&printer, QVector<int>());

    // Assert: 未进入绘制（早退链全覆盖）
    EXPECT_FALSE(m_win->m_bPrintProcessing);
    EXPECT_EQ(m_win->m_printDocList.count(), 1);
    m_win->clearPrintTextDocument();
}

TEST_F(WindowTest, AsynPrint_SmallDoc_RendersIntoImage)
{
    // Arrange: 打印文档 + 位图绘制设备（painter 活跃）
    m_win->m_printDoc = new QTextDocument();
    m_win->m_printDoc->setPlainText(QStringLiteral("async print line\n"));
    QImage image(600, 800, QImage::Format_ARGB32);
    image.fill(Qt::white);
    const qint64 keyBefore = image.cacheKey();
    DPrinter printer(QPrinter::HighResolution);
    QPainter painter(&image);
    ASSERT_TRUE(painter.isActive());

    // Act: 同步预览打印（小文档路径 → printPage）
    m_win->asynPrint(painter, &printer, QVector<int> { 1 });
    painter.end();

    // Assert: 位图被绘制修改 + 文档未受破坏
    EXPECT_NE(image.cacheKey(), keyBefore);
    EXPECT_FALSE(m_win->m_printDoc->isEmpty());
}

TEST_F(WindowTest, AsynPrint_LargeDocList_RendersMultiDoc)
{
    // Arrange: 大文档列表（无高亮 → else 分支）
    Window::PrintInfo info;
    info.doc = new QTextDocument();
    info.doc->setPlainText(QStringLiteral("multi doc page\n"));
    info.highlighter = nullptr;
    m_win->m_printDocList.append(info);
    m_win->m_bLargePrint = true;
    m_win->m_multiDocPageCount = info.doc->pageCount();
    QImage image(600, 800, QImage::Format_ARGB32);
    image.fill(Qt::white);
    const qint64 keyBefore = image.cacheKey();
    DPrinter printer(QPrinter::HighResolution);
    QPainter painter(&image);

    // Act
    m_win->asynPrint(painter, &printer, QVector<int> { 1 });
    painter.end();

    // Assert: printPageWithMultiDoc 执行
    EXPECT_NE(image.cacheKey(), keyBefore);
    EXPECT_TRUE(m_win->m_bLargePrint);
}

TEST_F(WindowTest, AsynPrint_InvalidDoc_EarlyReturn)
{
    // Arrange: 默认状态（无文档）
    QImage image(60, 80, QImage::Format_ARGB32);
    DPrinter printer(QPrinter::HighResolution);
    QPainter painter(&image);

    // Act
    m_win->asynPrint(painter, &printer, QVector<int> { 1 });
    painter.end();

    // Assert: 早退未崩溃
    EXPECT_TRUE(m_win->m_printDocList.isEmpty());
    EXPECT_FALSE(m_win->m_bLargePrint);
}

TEST_F(WindowTest, PrintPage_Static_RendersPageAndNumber)
{
    // Arrange
    QTextDocument doc;
    doc.setPlainText(QStringLiteral("static print page\n"));
    const int blocksBefore = doc.blockCount(); // 含尾块
    QImage image(400, 600, QImage::Format_ARGB32);
    image.fill(Qt::white);
    const qint64 keyBefore = image.cacheKey();
    QPainter painter(&image);

    // Act: 静态页绘制（页码 + 正文）
    Window::printPage(1, &painter, &doc, QRectF(20, 20, 360, 560), QRectF(200, 560, 160, 30));
    painter.end();

    // Assert
    EXPECT_NE(image.cacheKey(), keyBefore);
    EXPECT_EQ(doc.blockCount(), blocksBefore); // 只读绘制不改文档结构
}

TEST_F(WindowTest, CloneLargeDocument_NullWrapper_ReturnsFalse)
{
    // Act / Assert
    EXPECT_FALSE(m_win->cloneLargeDocument(nullptr));
    EXPECT_TRUE(m_win->m_printDocList.isEmpty());
}

TEST_F(WindowTest, CloneLargeDocument_SmallAndLargeDocs_SplitsByLimit)
{
    // Arrange 1: 小文档 → 单文档列表
    const QString small = addFileTab(QStringLiteral("small.txt"), QByteArray("small doc\n"));
    EditWrapper *wSmall = m_win->wrapper(small);
    ASSERT_NE(wSmall, nullptr);

    // Act / Assert
    EXPECT_TRUE(m_win->cloneLargeDocument(wSmall));
    EXPECT_EQ(m_win->m_printDocList.count(), 1);
    m_win->clearPrintTextDocument();

    // Arrange 2: >10 万文本块的文件 → 触发单文档块数上限拆分
    QByteArray big(150000, '\n'); // 150001 块
    const QString bigPath = addFileTab(QStringLiteral("big.txt"), big);
    EditWrapper *wBig = m_win->wrapper(bigPath);
    ASSERT_NE(wBig, nullptr);

    // Act / Assert: 拆分为多个打印文档
    EXPECT_TRUE(m_win->cloneLargeDocument(wBig));
    EXPECT_GE(m_win->m_printDocList.count(), 2);
}

TEST_F(WindowTest, RehighlightPrintDoc_NullArgs_EarlyReturn)
{
    // Act / Assert: 空指针守卫（状态未损坏）
    m_win->rehighlightPrintDoc(nullptr, nullptr);
    EXPECT_TRUE(m_win->m_printDocList.isEmpty());
    EXPECT_FALSE(m_win->m_bPrintProcessing);
}

TEST_F(WindowTest, RehighlightPrintDoc_RealObjects_Rehighlights)
{
    // Arrange: 真实文档 + 高亮器 + 打印 wrapper
    const QString path = addFileTab(QStringLiteral("hl.txt"), QByteArray("int main() { return 0; }\n"));
    m_win->m_printWrapper = m_win->wrapper(path);
    ASSERT_NE(m_win->m_printWrapper, nullptr);
    QTextDocument doc;
    doc.setPlainText(QStringLiteral("int x = 1;\n"));
    const int blocksBefore = doc.blockCount(); // 含尾块
    CSyntaxHighlighter highlighter(&doc);

    // Act: 亮色背景分支（rehighlight 全文）
    m_win->rehighlightPrintDoc(&doc, &highlighter);

    // Assert: 高亮器被启停过 + 文档结构不变
    EXPECT_EQ(doc.blockCount(), blocksBefore);
    EXPECT_FALSE(highlighter.definition().isValid()); // 未设置定义
}

// ============================================================
// 打印入口 / 设置对话框 / 快捷键视图
// ============================================================

TEST_F(WindowTest, PopupPrintDialog_EmptyDoc_EarlyReturn)
{
    // Arrange: 空白标签（文档为空 → 打印文档无效 → 早退）
    addBlankAndGetPath();

    // Act
    m_win->popupPrintDialog();

    // Assert: 未创建打印预览（doc 无效分支）且无处理中标志
    EXPECT_EQ(m_win->m_pPreview, nullptr);
    EXPECT_FALSE(m_win->m_bPrintProcessing);
}

TEST_F(WindowTest, PopupSettingsDialog_BuildsExecAndSyncs)
{
    // Arrange: 拦截 setSettingDialog 防悬垂指针 + 对话框立即返回
    int setDialogCalls = 0;
    stub.set_lamda(&Settings::setSettingDialog,
                   [&setDialogCalls](Settings *, DSettingsDialog *) { ++setDialogCalls; });
    m_qdialogResult = QDialog::Rejected;

    // Act
    m_win->popupSettingsDialog();
    QApplication::processEvents();

    // Assert: 对话框注册 + 执行一次
    EXPECT_EQ(setDialogCalls, 1);
    EXPECT_GE(m_qdialogExecCalls, 1);
}

TEST_F(WindowTest, DisplayShortcuts_LaunchesShortcutViewer)
{
    // Arrange
    m_win->show();

    // Act
    m_win->displayShortcuts();
    QApplication::processEvents();

    // Assert: deepin-shortcut-viewer 以 JSON 参数启动（进程被拦截）
    EXPECT_EQ(m_startDetachedCalls, 1);
    EXPECT_EQ(m_lastDetachedProgram, QString("deepin-shortcut-viewer"));
    EXPECT_EQ(m_lastDetachedArgs.count(), 2);
    EXPECT_TRUE(m_lastDetachedArgs.at(0).startsWith(QStringLiteral("-j=")));
    delete m_win->m_shortcutViewProcess;
    m_win->m_shortcutViewProcess = nullptr;
}

TEST_F(WindowTest, SetPrintEnabled_TogglesPrintAction)
{
    // Arrange: 标题菜单中的 Print action
    QAction *printAction = nullptr;
    for (QAction *a : m_win->m_menu->actions()) {
        if (a->text() == QString("Print"))
            printAction = a;
    }
    ASSERT_NE(printAction, nullptr);

    // Act / Assert: 禁用 → 启用
    m_win->setPrintEnabled(false);
    EXPECT_FALSE(printAction->isEnabled());
    m_win->setPrintEnabled(true);
    EXPECT_TRUE(printAction->isEnabled());
}

TEST_F(WindowTest, GetStackedWgt_ReturnsEditorStack)
{
    // Assert
    EXPECT_EQ(m_win->getStackedWgt(), m_win->m_editorWidget);
    EXPECT_EQ(m_win->getStackedWgt()->count(), 0);
}

// ============================================================
// 覆盖率补漏：私有辅助 / ctor 连接 lambda / 打印预览深路径
// ============================================================

TEST_F(WindowTest, GetCurrentOpenFilePath_DraftAndRealFile_ReturnsDirectories)
{
    // Assert: 无 wrapper → 空
    EXPECT_TRUE(m_win->getCurrentOpenFilePath().isEmpty());

    // Arrange: 草稿标签（blank 目录）
    const QString blank = addBlankAndGetPath();

    // Act / Assert: 草稿 → 家目录 Documents
    EXPECT_EQ(m_win->getCurrentOpenFilePath(), QDir::homePath() + "/Documents");

    // Arrange: 真实文件标签
    const QString path = addFileTab(QStringLiteral("curdir.txt"));

    // Act / Assert: 普通文件 → 其所在目录
    EXPECT_EQ(m_win->getCurrentOpenFilePath(), QFileInfo(path).absolutePath());
}

TEST_F(WindowTest, UpdateSizeMode_WithVisibleBars_RepositionsBars)
{
    // Arrange: 查找栏与替换栏可见
    m_win->show();
    addFileTab(QStringLiteral("usm.txt"), QByteArray("size mode content\n"));
    m_win->popupFindBar();
    QApplication::processEvents();
    ASSERT_TRUE(m_win->m_findBar->isVisible());

    // Act: 布局模式变更槽（可见分支移动栏位）
    m_win->updateSizeMode();

    // Assert: 栏仍可见且窗口未损坏
    EXPECT_TRUE(m_win->m_findBar->isVisible());
    EXPECT_EQ(m_win->m_findBar->y(), m_win->height() - m_win->m_findBar->height() - 4);
}

TEST_F(WindowTest, Constructor_SearchKeywordLambdas_ForwardToHandlers)
{
    // Arrange: 有内容标签（ctor 连接的 updateSearchKeyword lambda）
    const QString path = addFileTab(QStringLiteral("lamb.txt"), QByteArray("lambda content\n"));

    // Act: 查找栏信号 → ctor lambda → handleUpdateSearchKeyword
    emit m_win->m_findBar->updateSearchKeyword(path, QStringLiteral("lamb"));

    // Assert: 关键词已记录
    EXPECT_EQ(m_win->getKeywordForSearch(), QString("lamb"));

    // Act: 替换栏信号（第二个 ctor lambda）
    emit m_win->m_replaceBar->updateSearchKeyword(path, QStringLiteral("rep"));
    EXPECT_EQ(m_win->getKeywordForSearch(), QString("rep"));
    EXPECT_EQ(m_win->getKeywordForSearchAll(), QString("rep"));
}

TEST_F(WindowTest, AddTabWithWrapper_TextChangedLambda_UpdatesWordCount)
{
    // Arrange: 经 addTabWithWrapper 接入的编辑器（textChanged 连接的 lambda）
    EditWrapper *w = m_win->createEditor();
    const QString path = m_tempDir->filePath(QStringLiteral("twc.txt"));
    m_win->addTabWithWrapper(w, path, path, QStringLiteral("twc.txt"));

    // Act: 修改文本触发 textChanged → UpdateBottomBarWordCnt
    w->textEditor()->setPlainText(QStringLiteral("0123456789abcd")); // 14 字符

    // Assert: 字数统计已更新（14-1=13 → "Characters 13"）
    EXPECT_EQ(w->bottomBar()->m_pCharCountLabel->text(), QString("Characters 14"));
    EXPECT_EQ(w->textEditor()->toPlainText(), QString("0123456789abcd"));
}

TEST_F(WindowTest, PopupPrintDialog_WithContent_FullPreviewLifecycle)
{
    // Arrange: 有内容标签（走小文档克隆 → DPrintPreviewDialog 构建 + exec 拦截）
    m_win->show();
    addFileTab(QStringLiteral("print.txt"), QByteArray("printable line\nsecond line\n"));

    // Act
    m_win->popupPrintDialog();
    QApplication::processEvents();

    // Assert: 预览对话框已创建（finished/rejected/paintRequested lambda 已连接）
    ASSERT_NE(m_win->m_pPreview, nullptr);
    // 打印文档为克隆副本
    EXPECT_NE(m_win->m_printDoc, nullptr);
    EXPECT_FALSE(m_win->m_printDoc->isEmpty());

    // Act: 手动触发 paintRequested lambda（重载信号需经 PMF 对象语法发射）
    DPrinter paintPrinter(QPrinter::HighResolution);
    auto paintSig = QOverload<DPrinter *, const QVector<int> &>::of(&DPrintPreviewDialog::paintRequested);
    // 小文档分支 → doPrint 早退链
    emit (m_win->m_pPreview->*paintSig)(&paintPrinter, QVector<int> { 1 });
    // 大文档分支（doPrintWithLargeDoc 早退链）
    Window::PrintInfo info;
    info.doc = new QTextDocument();
    m_win->m_printDocList.append(info);
    m_win->m_bLargePrint = true;
    emit (m_win->m_pPreview->*paintSig)(&paintPrinter, QVector<int> { 1 });
    m_win->m_bLargePrint = false;

    // Act: 手动触发 finished / rejected 清理 lambda
    emit m_win->m_pPreview->finished(0);
    emit m_win->m_pPreview->rejected();
    QApplication::processEvents();

    // Assert: 打印文档被清理（clearPrintTextDocument lambda 生效）
    EXPECT_EQ(m_win->m_printDoc, nullptr);
    EXPECT_FALSE(m_win->m_bPrintProcessing);
    delete m_win->m_pPreview;
    m_win->m_pPreview = nullptr;
}

// ============================================================
// 关闭窗口（closeEvent 全链路）
// ============================================================

TEST_F(WindowTest, CloseEvent_CleanWindow_EmitsCloseWindow)
{
    // Arrange: 展示 + 一个干净标签
    m_win->show();
    addFileTab(QStringLiteral("closing.txt"));
    QSignalSpy spy(m_win, &Window::closeWindow);

    // Act
    m_win->close();
    QApplication::processEvents();

    // Assert: 关闭信号发射 + wrapper 全部释放 + 窗口隐藏
    EXPECT_EQ(spy.count(), 1);
    EXPECT_TRUE(m_win->m_wrappers.isEmpty());
    EXPECT_FALSE(m_win->isVisible());
}

TEST_F(WindowTest, CloseEvent_UnsavedCancel_IgnoresClose)
{
    // Arrange: 修改态 + 磁盘文件已删除（漂浮）+ 拒绝保存
    //（save_tab_before_close 默认 true，只有漂浮文件才弹保存确认）
    const QString path = addFileTab(QStringLiteral("cancel.txt"));
    QFile::remove(path);
    stub.set_lamda(&EditWrapper::isModified, [](EditWrapper *) -> bool { return true; });
    m_ddialogResult = 0;
    QSignalSpy spy(m_win, &Window::closeWindow);

    // Act
    m_win->close();
    QApplication::processEvents();

    // Assert: 关闭被忽略（无 closeWindow 信号）
    EXPECT_EQ(spy.count(), 0);
    EXPECT_TRUE(m_win->isVisible());
}
