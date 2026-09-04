// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// ============================================================================
// TextEdit×Window 集成单元测试（widgets_ut / src/editor/dtextedit.cpp 最后 2 个
// FNDA:0：dragEnterEvent 与 pinchTriggered）
//
// 根因：两函数都无条件 qobject_cast<Window*>(window())->... 解引用，需要
// TextEdit 挂在真实 Window 父链下（window() 返回真实 Window）。editor_core
// 模块设计上禁止真实构造 Window（接缝伪指针注入），故按任务论证放入 widgets_ut
// ——本模块已具备 B8/B11 验证过的"真实 Window + 真实 EditWrapper/TextEdit"
// 组合与 DBus/模态/消息桩矩阵，新增独立 target 复用该环境，零侵入既有用例。
//
// 分支清单 → 用例映射：
//   dragEnterEvent(dtextedit.cpp:7768)
//     唯一路径：QPlainTextEdit::dragEnterEvent + Window::requestDragEnterEvent 转发
//     → DragEnterEvent_TextEditUnderRealWindow_ForwardsToWindowAndAccepts
//       （信号转发计数 + 事件指针同一性 + Window 槽同步 accept）
//   pinchTriggered(dtextedit.cpp:4434)
//     B1 GestureStarted  → m_gestureAction=GA_pinch；m_scaleFactor 同步 m_fontSize
//     B2 GestureUpdated+ScaleFactorChanged → m_currentStepScaleFactor=totalScaleFactor
//     B3 GestureFinished → m_scaleFactor 累乘并复位步进；clamp[8,50] 后 setFontSize
//        + Window::changeSettingDialogComboxFontNumber 写 base.font.size
//     B4 GestureCanceled → 无副作用
//     → PinchTriggered_NormalScale / LargeScale_ClampsTo50 / TinyScale_ClampsTo8 /
//       CanceledGesture_KeepsState
//
// QGesture::state 只读无私有头依赖：VADDR 桩 QGesture::state() 控制
// 手势状态序列（QPinchGesture 的 setChangeFlags/setTotalScaleFactor 为公有 API）。
// pinchTriggered 为私有槽，经 -fno-access-control 白盒直调（B6 先例）。
// ============================================================================

#include <gtest/gtest.h>
#include "stubext.h"

#include <QApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QElapsedTimer>
#include <QThread>
#include <QUrl>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QGesture>
#include <QPinchGesture>

#include <DDialog>
#include <DMessageManager>
#include <DFileDialog>
#include <DSettingsOption>

#include <QDBusConnection>
#include <QDBusAbstractInterface>
#include <QDBusMessage>
#include <QProcess>

#include "widgets/window.h"
#include "controls/tabbar.h"
#include "editor/editwrapper.h"
#include "editor/dtextedit.h"
#include "common/settings.h"
#include "common/utils.h"
#include "startmanager.h"

class TextEditWindowIntegrationTest : public ::testing::Test {
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

        m_win = new Window();
        m_tabbar = m_win->getTabbar();

        // QGesture::state 只读（无私有头）：VADDR 桩控制捏合状态序列
        m_gestureState = Qt::GestureStarted;
        stub.set_lamda(VADDR(QGesture, state),
                       [this](QGesture *) -> Qt::GestureState { return m_gestureState; });
    }

    void TearDown() override
    {
        // stub 保持激活状态下析构（析构链中的 DBus/消息调用仍被拦截）
        delete m_win;
        m_win = nullptr;
        m_tabbar = nullptr;
        QApplication::processEvents();
        stub.clear();
        delete m_tempDir;
    }

    // ==================== 运行期 stub 矩阵（test_window.cpp 同源）====================

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

        stub.set_lamda(static_cast<bool (*)(const QString &, const QStringList &, const QString &, qint64 *)>(&QProcess::startDetached),
                       [](const QString &, const QStringList &, const QString &, qint64 *) -> bool { return true; });

        stub.set_lamda(VADDR(DDialog, exec), []() -> int { return 0; });
        stub.set_lamda(VADDR(QDialog, exec), []() -> int { return 0; });
        stub.set_lamda(VADDR(QFileDialog, selectedFiles),
                       [](QFileDialog *) -> QStringList { return QStringList(); });
        stub.set_lamda(&DFileDialog::getComboBoxValue,
                       [](DFileDialog *, const QString &) -> QString { return QString(); });
    }

    // ==================== 辅助 ====================

    QString createFile(const QString &name, const QByteArray &content = "hello world\nsecond line\n")
    {
        const QString path = m_tempDir->filePath(name);
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly))
            return QString();
        f.write(content);
        f.close();
        return path;
    }

    // 添加真实文件标签页并等待内容载入（"文档非空 + 非加载中"为完成判据）
    QString addFileTab(const QString &name)
    {
        const QString path = createFile(name);
        m_win->addTab(path, true);
        EditWrapper *wrapper = m_win->wrapper(path);
        if (wrapper != nullptr) {
            waitUntil([wrapper]() {
                return !wrapper->getFileLoading() && !wrapper->textEditor()->document()->isEmpty();
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

    // 捏合手势白盒驱动：Started → Updated(ScaleFactorChanged, factor) → Finished
    void runPinchSequence(TextEdit *textEdit, QPinchGesture *gesture, qreal totalFactor)
    {
        m_gestureState = Qt::GestureStarted;
        textEdit->pinchTriggered(gesture);

        gesture->setChangeFlags(QPinchGesture::ScaleFactorChanged);
        gesture->setTotalScaleFactor(totalFactor);
        m_gestureState = Qt::GestureUpdated;
        textEdit->pinchTriggered(gesture);

        m_gestureState = Qt::GestureFinished;
        textEdit->pinchTriggered(gesture);
    }

    int fontSizeSettingValue() const
    {
        return m_win->m_settings->settings->option(QStringLiteral("base.font.size"))->value().toInt();
    }

    // ==================== 状态 ====================
    static char s_argv[];
    static QApplication *s_app;
    static QTemporaryDir *s_configHome;
    static QTemporaryDir *s_dataHome;

    stub_ext::StubExt stub;
    Window *m_win = nullptr;
    Tabbar *m_tabbar = nullptr;
    QTemporaryDir *m_tempDir = nullptr;
    Qt::GestureState m_gestureState = Qt::GestureStarted;
};

char TextEditWindowIntegrationTest::s_argv[] = "test_dtextedit_window_integration";
QApplication *TextEditWindowIntegrationTest::s_app = nullptr;
QTemporaryDir *TextEditWindowIntegrationTest::s_configHome = nullptr;
QTemporaryDir *TextEditWindowIntegrationTest::s_dataHome = nullptr;

// ---------------- dragEnterEvent ----------------

// 真实 Window 父链下直发 QDragEnterEvent：转发信号携带同一事件，Window 槽同步 accept
TEST_F(TextEditWindowIntegrationTest, DragEnterEvent_TextEditUnderRealWindow_ForwardsToWindowAndAccepts)
{
    // Arrange
    const QString path = addFileTab(QStringLiteral("drag.txt"));
    EditWrapper *wrapper = m_win->wrapper(path);
    ASSERT_NE(wrapper, nullptr);
    TextEdit *textEdit = wrapper->textEditor();
    ASSERT_NE(textEdit, nullptr);
    // 前置：真实父链成立（qobject_cast<Window*> 可命中的根因条件）
    ASSERT_EQ(textEdit->window(), m_win);

    QMimeData mimeData;
    QList<QUrl> urls;
    urls << QUrl::fromLocalFile(path);
    mimeData.setUrls(urls);
    QDragEnterEvent event(QPoint(5, 5), Qt::CopyAction, &mimeData,
                          Qt::LeftButton, Qt::NoModifier);

    int forwardedCount = 0;
    QDragEnterEvent *forwardedEvent = nullptr;
    QMetaObject::Connection connection = QObject::connect(
        m_win, &Window::requestDragEnterEvent,
        [&forwardedCount, &forwardedEvent](QDragEnterEvent *forwarded) {
            ++forwardedCount;
            forwardedEvent = forwarded;
        });

    // Act：直调 protected 事件处理器（-fno-access-control，editor_core 既定模式）。
    // 注：QApplication::notify 对非 spontaneous 的 DragEnter 会把 QPlainTextEdit 系
    // 控件重定向到祖先 drop site（gdb 断点实证 sendEvent 不可达本处理器），
    // 生产路径由真实 QDragManager 投递，与 sendEvent 语义不同。
    textEdit->dragEnterEvent(&event);
    QObject::disconnect(connection);

    // Assert：转发发生、携带同一事件、Window::dragEnterEvent（直连）同步 accept
    EXPECT_EQ(forwardedCount, 1);
    EXPECT_EQ(forwardedEvent, &event);
    EXPECT_TRUE(event.isAccepted());
}

// ---------------- pinchTriggered ----------------

// 正常缩放：16 →×2→ 32；同步字体栏数值（base.font.size）与文档字体
TEST_F(TextEditWindowIntegrationTest, PinchTriggered_NormalScale_UpdatesFontSizeAndSettingsOption)
{
    // Arrange
    const QString path = addFileTab(QStringLiteral("pinch_normal.txt"));
    TextEdit *textEdit = m_win->wrapper(path)->textEditor();
    ASSERT_NE(textEdit, nullptr);
    textEdit->m_fontSize = 16;   // 白盒钉死初值，保证不落入 clamp 分支
    QPinchGesture gesture;

    // Act
    runPinchSequence(textEdit, &gesture, 2.0);

    // Assert
    EXPECT_EQ(textEdit->m_gestureAction, TextEdit::GA_null) << "finished must reset gesture action";
    EXPECT_DOUBLE_EQ(textEdit->m_scaleFactor, 32.0);
    EXPECT_DOUBLE_EQ(textEdit->m_currentStepScaleFactor, 1.0);
    EXPECT_DOUBLE_EQ(textEdit->m_fontSize, 32.0);
    EXPECT_NEAR(textEdit->font().pointSizeF(), 32.0, 0.6) << "updateFont must apply new size";
    EXPECT_EQ(fontSizeSettingValue(), 32) << "Window::changeSettingDialogComboxFontNumber must sync settings";
}

// 上界：40 ×5 → clamp 50
TEST_F(TextEditWindowIntegrationTest, PinchTriggered_LargeScale_ClampsTo50)
{
    // Arrange
    const QString path = addFileTab(QStringLiteral("pinch_large.txt"));
    TextEdit *textEdit = m_win->wrapper(path)->textEditor();
    ASSERT_NE(textEdit, nullptr);
    textEdit->m_fontSize = 40;
    QPinchGesture gesture;

    // Act
    runPinchSequence(textEdit, &gesture, 5.0);

    // Assert
    EXPECT_DOUBLE_EQ(textEdit->m_fontSize, 50.0);
    EXPECT_EQ(fontSizeSettingValue(), 50);
    EXPECT_EQ(textEdit->m_gestureAction, TextEdit::GA_null);
}

// 下界：5 ×0.5 → 2.5 → clamp 8
TEST_F(TextEditWindowIntegrationTest, PinchTriggered_TinyScale_ClampsTo8)
{
    // Arrange
    const QString path = addFileTab(QStringLiteral("pinch_tiny.txt"));
    TextEdit *textEdit = m_win->wrapper(path)->textEditor();
    ASSERT_NE(textEdit, nullptr);
    textEdit->m_fontSize = 5;
    QPinchGesture gesture;

    // Act
    runPinchSequence(textEdit, &gesture, 0.5);

    // Assert
    EXPECT_DOUBLE_EQ(textEdit->m_fontSize, 8.0);
    EXPECT_EQ(fontSizeSettingValue(), 8);
}

// GestureCanceled：switch 分支无状态迁移（保持 GA_pinch），但 switch 之后的
// 字号应用无条件执行（源码语义：任何状态都以 m_scaleFactor×步进 设定字号）
TEST_F(TextEditWindowIntegrationTest, PinchTriggered_CanceledGesture_KeepsActionButAppliesFontSize)
{
    // Arrange
    const QString path = addFileTab(QStringLiteral("pinch_cancel.txt"));
    TextEdit *textEdit = m_win->wrapper(path)->textEditor();
    ASSERT_NE(textEdit, nullptr);
    textEdit->m_fontSize = 16;
    QPinchGesture gesture;

    // Act：Started 置位后直接 Canceled
    m_gestureState = Qt::GestureStarted;
    textEdit->pinchTriggered(&gesture);
    ASSERT_EQ(textEdit->m_gestureAction, TextEdit::GA_pinch);
    ASSERT_DOUBLE_EQ(textEdit->m_fontSize, 16.0);   // Started 同步 16×1=16

    m_gestureState = Qt::GestureCanceled;
    textEdit->pinchTriggered(&gesture);

    // Assert：手势状态保持 GA_pinch；字号仍按 16×1 应用（switch 后无条件段）
    EXPECT_EQ(textEdit->m_gestureAction, TextEdit::GA_pinch) << "canceled must not reset action";
    EXPECT_DOUBLE_EQ(textEdit->m_fontSize, 16.0);
    EXPECT_EQ(fontSizeSettingValue(), 16) << "font size application runs for every state";
}
