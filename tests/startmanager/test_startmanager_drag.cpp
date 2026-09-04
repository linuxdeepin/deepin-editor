// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// ============================================================================
// StartManager::createWindow 拖拽 lambda 单元测试（startmanager/test_startmanager_drag.cpp，
// startmanager.cpp:773/777 两个 FNDA:0 lambda）
//
// 根因：createWindow 内 connect(window->getTabbar(), &DTabBar::dragStarted/dragEnd,
// this, [this]{...}) 为信号-私接 lambda，需真实 DTabBar（Tabbar : public DTabBar）
// 实例连接后方可触发；test_startmanager 为 QCoreApplication 语境（禁真实 Window），
// 故本目标独立可执行文件，改用 QApplication + 真实 Window（widgets_ut 已验证矩阵）。
//
// 触发方式：QMetaObject::invokeMethod(tabbar, "dragStarted"/"dragEnd") —— 信号经
// moc qt_metacall 发射（绕过 C++ protected 限制），lambda 副作用经 -fno-access-control
// 白盒断言。
//
// 分支清单 → 用例映射：
//   lambda#1 dragStarted(773)：m_bIsTagDragging = true
//     → DragStarted_EmittedViaDTabBarMetaCall_PausesBackup
//   lambda#2 dragEnd(777)：m_bIsTagDragging = false + slotDelayBackupFile()
//     （20ms QBasicTimer 启动 = 备份已排程）
//     → DragEnd_EmittedViaDTabBarMetaCall_ResumesBackupAndSchedules
//   createWindow 集成前提：窗口入列 m_windows / tabbar 真为 DTabBar
//     → CreateWindow_RealWindow_AppendsToListWithRealDTabBar
//
// teardown 防护：StartManager::slotCloseWindow 桩为空操作（其内部按进程 CWD 删文件，
// 真实执行会误删测试运行目录）；m_DelayTimer.stop() 防 20ms 后台备份竞态。
// ============================================================================

#include <gtest/gtest.h>
#include "stubext.h"

#include <QApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QTimer>

#include <DDialog>
#include <DMessageManager>
#include <DFileDialog>

#include <QDBusConnection>
#include <QDBusAbstractInterface>
#include <QDBusMessage>
#include <QProcess>

#include "startmanager.h"
#include "widgets/window.h"
#include "controls/tabbar.h"
#include "common/settings.h"
#include "common/utils.h"
#include "common/iflytek_ai_assistant.h"

class StartManagerDragTest : public ::testing::Test {
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
        char *argv[] = { s_argv, nullptr };
        s_app = new QApplication(argc, argv);
        QApplication::setOrganizationName(QStringLiteral("deepin"));
        QApplication::setApplicationName(QStringLiteral("deepin-editor"));

        Settings::instance();
        qRegisterMetaType<ViewMode>("ViewMode");

        QDir().mkpath(xdgData + "/deepin/deepin-editor/blank-files");
        QDir().mkpath(xdgData + "/deepin/deepin-editor/backup-files");
        QDir().mkpath(xdgData + "/deepin/deepin-editor/autoBackup-files");

        // StartManager 构造链 iflytek 查询隔离（零化伪对象，永不析构）
        s_fakeIflytek = static_cast<IflytekAiAssistant *>(calloc(sizeof(IflytekAiAssistant) + 256, 1));
    }

    static void TearDownTestSuite()
    {
        qunsetenv("XDG_CONFIG_HOME");
        qunsetenv("XDG_DATA_HOME");
        free(s_fakeIflytek);
        s_fakeIflytek = nullptr;
    }

    void SetUp() override
    {
        m_tempDir = new QTemporaryDir();
        installCommonStubs();

        // 全桩环境下构造被测对象并真实建窗（含 createWindow 的 5 组 connect）
        obj = new StartManager();
        m_win = obj->createWindow(false);
        m_tabbar = m_win->getTabbar();
    }

    void TearDown() override
    {
        obj->m_DelayTimer.stop();   // 防 20ms 延迟备份在断言后竞态触发
        delete obj->m_pTimer;       // 构造中 new 的周期定时器（无 parent），手动回收
        delete obj;
        obj = nullptr;
        StartManager::m_instance = nullptr;   // 兜底清理单例静态指针

        // stub 保持激活状态下析构（析构链中的 DBus/消息调用仍被拦截）
        delete m_win;
        m_win = nullptr;
        m_tabbar = nullptr;
        QApplication::processEvents();
        stub.clear();
        delete m_tempDir;
    }

    // ==================== 运行期 stub 矩阵（widgets_ut 同源）====================

    void installCommonStubs()
    {
        stub.set_lamda(&QDBusConnection::systemBus,
                       []() -> QDBusConnection { return QDBusConnection(QStringLiteral("ut_no_bus")); });
        stub.set_lamda(&QDBusConnection::sessionBus,
                       []() -> QDBusConnection { return QDBusConnection(QStringLiteral("ut_no_bus")); });
        stub.set_lamda(
            static_cast<QDBusMessage (QDBusAbstractInterface::*)(QDBus::CallMode, const QString &, const QList<QVariant> &)>(
                &QDBusAbstractInterface::callWithArgumentList),
            [](QDBusAbstractInterface *, QDBus::CallMode, const QString &,
               const QList<QVariant> &) -> QDBusMessage { return QDBusMessage(); });
        stub.set_lamda(
            static_cast<bool (QDBusConnection::*)(const QString &)>(&QDBusConnection::unregisterService),
            [](QDBusConnection *, const QString &) -> bool { return true; });

        stub.set_lamda(
            static_cast<void (DMessageManager::*)(QWidget *, const QIcon &, const QString &)>(
                &DMessageManager::sendMessage),
            [](DMessageManager *, QWidget *, const QIcon &, const QString &) -> void {});
        stub.set_lamda(
            static_cast<void (DMessageManager::*)(QWidget *, DFloatingMessage *)>(
                &DMessageManager::sendMessage),
            [](DMessageManager *, QWidget *, DFloatingMessage *) -> void {});
        stub.set_lamda(&Utils::sendFloatMessageFixedFont,
                       [](QWidget *, const QIcon &, const QString &) -> void {});

        stub.set_lamda(&StartManager::closeAboutForWindow,
                       [](StartManager *, Window *) -> void {});
        // 防护：slotCloseWindow 按进程 CWD 删文件，真实执行会误删测试运行目录
        stub.set_lamda(&StartManager::slotCloseWindow, [](StartManager *) -> void {});

        stub.set_lamda(static_cast<bool (*)(const QString &, const QStringList &, const QString &, qint64 *)>(&QProcess::startDetached),
                       [](const QString &, const QStringList &, const QString &, qint64 *) -> bool { return true; });

        stub.set_lamda(VADDR(DDialog, exec), []() -> int { return 0; });
        stub.set_lamda(VADDR(QDialog, exec), []() -> int { return 0; });
        stub.set_lamda(VADDR(QFileDialog, selectedFiles),
                       [](QFileDialog *) -> QStringList { return QStringList(); });
        stub.set_lamda(&DFileDialog::getComboBoxValue,
                       [](DFileDialog *, const QString &) -> QString { return QString(); });

        // StartManager 构造链 iflytek 查询隔离
        stub.set_lamda(&IflytekAiAssistant::instance,
                       []() -> IflytekAiAssistant * { return s_fakeIflytek; });
        stub.set_lamda(&IflytekAiAssistant::checkAiExists,
                       [](IflytekAiAssistant *) -> void {});
    }

    // ==================== 状态 ====================
    static char s_argv[];
    static QApplication *s_app;
    static QTemporaryDir *s_configHome;
    static QTemporaryDir *s_dataHome;
    static IflytekAiAssistant *s_fakeIflytek;

    stub_ext::StubExt stub;
    StartManager *obj = nullptr;
    Window *m_win = nullptr;
    Tabbar *m_tabbar = nullptr;
    QTemporaryDir *m_tempDir = nullptr;
};

char StartManagerDragTest::s_argv[] = "test_startmanager_drag";
QApplication *StartManagerDragTest::s_app = nullptr;
QTemporaryDir *StartManagerDragTest::s_configHome = nullptr;
QTemporaryDir *StartManagerDragTest::s_dataHome = nullptr;
IflytekAiAssistant *StartManagerDragTest::s_fakeIflytek = nullptr;

// 集成前提：真实窗口入列、tabbar 真为 DTabBar（lambda 连接对象）
TEST_F(StartManagerDragTest, CreateWindow_RealWindow_AppendsToListWithRealDTabBar)
{
    // Arrange / Act：SetUp 已 createWindow

    // Assert
    ASSERT_NE(m_win, nullptr);
    EXPECT_EQ(obj->m_windows.size(), 1);
    EXPECT_EQ(obj->m_windows.first(), m_win);
    ASSERT_NE(m_tabbar, nullptr);
    EXPECT_NE(dynamic_cast<DTabBar *>(m_tabbar), nullptr) << "Tabbar must derive DTabBar";
    EXPECT_FALSE(obj->m_bIsTagDragging) << "dragging flag starts cleared";
}

// lambda#1(773)：dragStarted → m_bIsTagDragging=true（暂停备份，不排程）
TEST_F(StartManagerDragTest, DragStarted_EmittedViaDTabBarMetaCall_PausesBackup)
{
    // Arrange
    ASSERT_FALSE(obj->m_bIsTagDragging);

    // Act：经 moc 发射 DTabBar::dragStarted（私接 lambda 直收）
    const bool emitted = QMetaObject::invokeMethod(m_tabbar, "dragStarted");

    // Assert
    EXPECT_TRUE(emitted) << "dragStarted must be invokable on real DTabBar metaobject";
    EXPECT_TRUE(obj->m_bIsTagDragging) << "drag lambda must set dragging flag";
    EXPECT_FALSE(obj->m_DelayTimer.isActive()) << "drag start must not schedule backup";
}

// lambda#2(777)：dragEnd → 复位标志并 slotDelayBackupFile()（20ms 定时器启动）
TEST_F(StartManagerDragTest, DragEnd_EmittedViaDTabBarMetaCall_ResumesBackupAndSchedules)
{
    // Arrange：先进入拖拽态
    ASSERT_TRUE(QMetaObject::invokeMethod(m_tabbar, "dragStarted"));
    ASSERT_TRUE(obj->m_bIsTagDragging);

    // Act：经 moc 发射 dragEnd(Qt::DropAction)
    const bool emitted = QMetaObject::invokeMethod(m_tabbar, "dragEnd",
                                                   Q_ARG(Qt::DropAction, Qt::CopyAction));

    // Assert
    EXPECT_TRUE(emitted) << "dragEnd must be invokable on real DTabBar metaobject";
    EXPECT_FALSE(obj->m_bIsTagDragging) << "drag end lambda must clear dragging flag";
    EXPECT_TRUE(obj->m_DelayTimer.isActive()) << "drag end must schedule delay backup (20ms QBasicTimer)";
}
