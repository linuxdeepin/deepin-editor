// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// ============================================================================
// EditorApplication 单元测试（app_ut / src/editorapplication.cpp，7 FN 全覆盖）
//
// 策略：EditorApplication 本身即进程级 QApplication —— 本目标独立可执行文件内
// 以它作为唯一 QApplication 实例真实构造（QT_QPA_PLATFORM=offscreen，B8/B11 已
// 验证 DApplication 系 offscreen 可构造）。DBus/Iflytek 重依赖经 suite 级 stub
// 矩阵拦截；XDG_CONFIG_HOME/XDG_DATA_HOME 重定向 QTemporaryDir。
//
// 分支清单 → 用例映射（editorapplication.cpp）：
//   ctor(9)    → SetUpTestSuite 真实构造 + Ctor_应用属性就位（Qt>=6 分支）
//   dtor(45)   → TearDownTestSuite delete s_app：
//                D2/D0 双记录 + "StartManager::instance() 非空 → delete"分支
//                （else 空分支不可二次构造 QApplication，函数覆盖已达成）
//   handleQuitAction(59)
//     B1 activeWindow 非空 → close
//        → HandleQuitAction_ActiveWindow_ClosesIt（closeEvent 计数 + 隐藏断言）
//     B2 activeWindow 空 → qWarning
//        → HandleQuitAction_NoActiveWindow_KeepsNull（消息捕获断言）
//   notify(73)
//     B3 KeyPress + className∈{QPushButton,QCheckBox,QComboBox,Dtk::Widget::DIconButton}
//        + Key_Return/Enter → pressSpace + return true（TEST_P 4 类）
//     B4 KeyPress + objectName∈{CustomRebackButton,RemoteSearchRebackButton,
//        RemoteGroupRebackButton} + Key_Left → pressSpace + return true（TEST_P 3 组）
//     B5 其余 KeyPress / 非 KeyPress → 委托 QApplication::notify
//        → Notify_UnmatchedEvent_DelegatesToBase（返回 false + 无 space 注入）
//   pressSpace(118) + 80ms lambda(126)
//     → PressSpace_DirectCall（同步 press + 80ms 后 release + clicked）
//     → notify 用例间接覆盖（全部先 qWait 交付 80ms lambda 再删按钮，
//       e07d697d 教训：防删除被 singleShot 捕获的对象）
//
// 不可达说明：className "IconButton"/"ComboBox" 为外部工程遗留类名，本仓库无
// 定义无法构造；两者与已覆盖类名同处一个 OR 条件基本块，不影响函数/行覆盖。
// ============================================================================

#include <gtest/gtest.h>
#include "stubext.h"

#include <QKeyEvent>
#include <QWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QTemporaryDir>
#include <QDir>
#include <QtTest>

#include <DPushButton>
#include <DIconButton>
#include <QDBusConnection>
#include <QDBusAbstractInterface>
#include <QDBusMessage>

#include "editorapplication.h"
#include "startmanager.h"
#include "common/settings.h"
#include "common/iflytek_ai_assistant.h"

// ---------------- 测试辅助 ----------------

// 统计目标控件收到的事件：空格 KeyPress/KeyRelease（notify 拦截回车后注入的
// 模拟事件）与回车 KeyPress/鼠标按下（未被拦截、委托基类送达的原始事件）
class SpaceEventCounter : public QObject {
public:
    using QObject::QObject;
    int spacePress = 0;
    int spaceRelease = 0;
    int returnPress = 0;
    int mousePress = 0;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event->type() == QEvent::KeyPress
            || event->type() == QEvent::KeyRelease) {
            auto *key = static_cast<QKeyEvent *>(event);
            if (key->key() == Qt::Key_Space) {
                if (event->type() == QEvent::KeyPress)
                    ++spacePress;
                else
                    ++spaceRelease;
            } else if (key->key() == Qt::Key_Return
                       && event->type() == QEvent::KeyPress) {
                ++returnPress;
            }
        } else if (event->type() == QEvent::MouseButtonPress) {
            ++mousePress;
        }
        return QObject::eventFilter(watched, event);
    }
};

// close 事件计数控件（handleQuitAction 副作用断言）
class CloseCountWidget : public QWidget {
public:
    using QWidget::QWidget;
    int closeCount = 0;

protected:
    void closeEvent(QCloseEvent *event) override
    {
        ++closeCount;
        QWidget::closeEvent(event);
    }
};

// e07d697d 教训固化：80ms singleShot lambda 捕获按钮裸指针——析构前先 qWait
// 交付挂起定时器，再删除控件，杜绝 UAF
struct LateDeleteWidget {
    QWidget *w = nullptr;
    ~LateDeleteWidget()
    {
        if (w != nullptr) {
            QTest::qWait(200);
            delete w;
        }
    }
};

// TEST_P 参数：目标控件种类 + 触发键
struct NotifyCase {
    const char *name;
    int widgetKind;   // 0=QPushButton 1=QCheckBox 2=QComboBox 3=DIconButton 4~6=DPushButton+objectName
    int key;          // Qt::Key_Return / Qt::Key_Enter / Qt::Key_Left
};

static std::ostream &operator<<(std::ostream &os, const NotifyCase &c)
{
    return os << c.name;
}

// 消息捕获（handleQuitAction 空分支的 qWarning 断言）
static QStringList *g_capturedMessages = nullptr;
static QtMessageHandler g_oldMessageHandler = nullptr;
static void captureMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(type);
    Q_UNUSED(context);
    if (g_capturedMessages != nullptr)
        g_capturedMessages->append(msg);
}

class EditorApplicationTest : public ::testing::Test {
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

        // ---- suite 级 stub：DBus / Iflytek（构造前安装，贯穿全 suite）----
        s_stub.set_lamda(&QDBusConnection::systemBus,
                         []() -> QDBusConnection { return QDBusConnection(QStringLiteral("ut_no_bus")); });
        s_stub.set_lamda(&QDBusConnection::sessionBus,
                         []() -> QDBusConnection { return QDBusConnection(QStringLiteral("ut_no_bus")); });
        s_stub.set_lamda(
            static_cast<QDBusMessage (QDBusAbstractInterface::*)(QDBus::CallMode, const QString &, const QList<QVariant> &)>(
                &QDBusAbstractInterface::callWithArgumentList),
            [](QDBusAbstractInterface *, QDBus::CallMode, const QString &,
               const QList<QVariant> &) -> QDBusMessage { return QDBusMessage(); });
        s_stub.set_lamda(
            static_cast<bool (QDBusConnection::*)(const QString &)>(&QDBusConnection::unregisterService),
            [](QDBusConnection *, const QString &) -> bool { return true; });
        // StartManager 构造链的 iflytek 查询隔离（零化伪对象，永不释放/析构）
        s_fakeIflytek = static_cast<IflytekAiAssistant *>(calloc(sizeof(IflytekAiAssistant) + 256, 1));
        s_stub.set_lamda(&IflytekAiAssistant::instance,
                         []() -> IflytekAiAssistant * { return s_fakeIflytek; });
        s_stub.set_lamda(&IflytekAiAssistant::checkAiExists,
                         [](IflytekAiAssistant *) -> void {});

        // ---- 被测对象：进程唯一 QApplication 实例（真实构造）----
        static int argc = 1;
        static char appName[] = "test_editorapplication";
        static char *argv[] = { appName, nullptr };
        s_app = new EditorApplication(argc, argv);
        ASSERT_NE(s_app, nullptr);

        // 真实 Settings 单例（qrc settings.json + 临时 XDG）
        Settings::instance();
        // StartManager 构造路径的 AppData 子目录
        QDir().mkpath(xdgData + "/deepin/deepin-editor/blank-files");
        QDir().mkpath(xdgData + "/deepin/deepin-editor/backup-files");
        QDir().mkpath(xdgData + "/deepin/deepin-editor/autoBackup-files");
    }

    static void TearDownTestSuite()
    {
        // dtor 覆盖（D2/D0 双记录 + StartManager 非空 delete 分支；
        // Dtor_Precondition 已保证 instance() 存活）
        delete s_app;
        s_app = nullptr;

        // ~StartManager 将 m_instance 置空（private static，-fno-access-control 直读）
        EXPECT_EQ(StartManager::m_instance, nullptr);

        s_stub.clear();
        free(s_fakeIflytek);
        s_fakeIflytek = nullptr;
        qunsetenv("XDG_CONFIG_HOME");
        qunsetenv("XDG_DATA_HOME");
        delete s_configHome;
        delete s_dataHome;
    }

    void SetUp() override {}
    void TearDown() override
    {
        // 每用例兜底清空事件队列（80ms lambda 已在 LateDeleteWidget 内交付）
        QTest::qWait(20);
    }

    // TEST_P 工厂：按 kind 构造目标控件
    static QWidget *makeTarget(int kind)
    {
        switch (kind) {
        case 0:
            return new QPushButton(QStringLiteral("ok"));
        case 1:
            return new QCheckBox(QStringLiteral("check"));
        case 2:
            return new QComboBox();
        case 3:
            return new DIconButton();
        case 4:
        case 5:
        case 6: {
            auto *btn = new DPushButton(QStringLiteral("back"));
            if (kind == 4)
                btn->setObjectName(QStringLiteral("CustomRebackButton"));
            else if (kind == 5)
                btn->setObjectName(QStringLiteral("RemoteSearchRebackButton"));
            else
                btn->setObjectName(QStringLiteral("RemoteGroupRebackButton"));
            return btn;
        }
        }
        return new QPushButton();
    }

    static QAbstractButton *asButton(QWidget *w) { return qobject_cast<QAbstractButton *>(w); }

    // ---------------- 状态 ----------------
    static EditorApplication *s_app;
    static stub_ext::StubExt s_stub;
    static IflytekAiAssistant *s_fakeIflytek;
    static QTemporaryDir *s_configHome;
    static QTemporaryDir *s_dataHome;
};

EditorApplication *EditorApplicationTest::s_app = nullptr;
stub_ext::StubExt EditorApplicationTest::s_stub;
IflytekAiAssistant *EditorApplicationTest::s_fakeIflytek = nullptr;
QTemporaryDir *EditorApplicationTest::s_configHome = nullptr;
QTemporaryDir *EditorApplicationTest::s_dataHome = nullptr;

// ---------------- ctor ----------------

// ctor：组织名/应用名/版本/关闭策略等应用属性就位（非翻译维度，避免 locale 耦合）
TEST_F(EditorApplicationTest, Ctor_Constructed_ApplicationPropertiesInPlace)
{
    // Arrange / Act：构造已在 SetUpTestSuite 完成

    // Assert
    EXPECT_EQ(s_app->organizationName().toStdString(), "deepin");
    EXPECT_EQ(s_app->applicationName().toStdString(), "deepin-editor");
    EXPECT_EQ(s_app->applicationVersion().toStdString(), VERSION);
    EXPECT_FALSE(s_app->quitOnLastWindowClosed());
}

// ---------------- handleQuitAction ----------------

// B2：无活动窗口 → qWarning 提示且不崩溃，activeWindow 保持空
TEST_F(EditorApplicationTest, HandleQuitAction_NoActiveWindow_WarnsAndKeepsNull)
{
    // Arrange
    QApplication::setActiveWindow(nullptr);
    ASSERT_EQ(s_app->activeWindow(), nullptr);
    QStringList captured;
    g_capturedMessages = &captured;
    g_oldMessageHandler = qInstallMessageHandler(captureMessageHandler);

    // Act
    s_app->handleQuitAction();

    // Assert
    qInstallMessageHandler(g_oldMessageHandler);
    g_capturedMessages = nullptr;
    EXPECT_EQ(s_app->activeWindow(), nullptr);
    bool warned = false;
    for (const QString &msg : captured) {
        if (msg.contains(QStringLiteral("No active window found to close")))
            warned = true;
    }
    EXPECT_TRUE(warned) << "expected qWarning for missing active window, got: "
                        << captured.join(';').toStdString();
}

// B1：有活动窗口 → close 送达（closeEvent 计数）并隐藏
TEST_F(EditorApplicationTest, HandleQuitAction_ActiveWindow_ClosesIt)
{
    // Arrange
    CloseCountWidget widget;
    widget.setMinimumSize(100, 80);
    widget.show();
    QApplication::setActiveWindow(&widget);
    widget.setFocus();
    ASSERT_EQ(s_app->activeWindow(), &widget);
    ASSERT_EQ(widget.closeCount, 0);

    // Act
    s_app->handleQuitAction();

    // Assert
    EXPECT_EQ(widget.closeCount, 1) << "active window must receive exactly one close";
    EXPECT_FALSE(widget.isVisible()) << "closed widget must be hidden";
}

// ---------------- notify ----------------

class EditorApplicationNotifyTest : public EditorApplicationTest,
                                    public ::testing::WithParamInterface<NotifyCase> {};

// B3/B4：回车/左键命中拦截矩阵 → 注入空格按下（同步）+ 80ms 后松开 + 消费原事件
TEST_P(EditorApplicationNotifyTest, KeyPress_OnInterceptedTarget_SimulatesSpaceClick)
{
    const NotifyCase param = GetParam();

    // Arrange
    LateDeleteWidget guard;
    guard.w = makeTarget(param.widgetKind);
    QWidget *target = guard.w;
    SpaceEventCounter counter;
    target->installEventFilter(&counter);

    int clickedCount = 0;
    QAbstractButton *button = asButton(target);
    if (button != nullptr)
        QObject::connect(button, &QAbstractButton::clicked, [&clickedCount]() { ++clickedCount; });
    if (param.widgetKind == 2)
        static_cast<QComboBox *>(target)->addItem(QStringLiteral("item"));

    QKeyEvent keyEvent(QEvent::KeyPress, param.key, Qt::NoModifier);

    // Act
    const bool consumed = s_app->notify(target, &keyEvent);

    // Assert（同步段：空格按下已送达、原事件被消费）
    EXPECT_TRUE(consumed) << param.name << ": intercepted event must be consumed";
    EXPECT_EQ(counter.spacePress, 1) << param.name << ": space press must be injected synchronously";

    // Assert（异步段：80ms singleShot 松开 → 按钮产生 clicked / 组合框弹开）
    QTest::qWait(150);
    EXPECT_EQ(counter.spaceRelease, 1) << param.name << ": space release must arrive after 80ms";
    if (button != nullptr)
        EXPECT_EQ(clickedCount, 1) << param.name << ": simulated space must click the button";
    if (param.widgetKind == 1)
        EXPECT_TRUE(static_cast<QCheckBox *>(target)->isChecked()) << "space must toggle the checkbox";
    if (param.widgetKind == 2)
        EXPECT_TRUE(static_cast<QComboBox *>(target)->view()->isVisible()) << "space must open combo popup";
}

INSTANTIATE_TEST_SUITE_P(InterceptTargets, EditorApplicationNotifyTest,
                         ::testing::Values(
                             NotifyCase{ "QPushButtonReturn", 0, Qt::Key_Return },
                             NotifyCase{ "QPushButtonEnter", 0, Qt::Key_Enter },
                             NotifyCase{ "QCheckBoxReturn", 1, Qt::Key_Return },
                             NotifyCase{ "QComboBoxReturn", 2, Qt::Key_Return },
                             NotifyCase{ "DIconButtonReturn", 3, Qt::Key_Return },
                             NotifyCase{ "CustomRebackLeft", 4, Qt::Key_Left },
                             NotifyCase{ "RemoteSearchRebackLeft", 5, Qt::Key_Left },
                             NotifyCase{ "RemoteGroupRebackLeft", 6, Qt::Key_Left }));

// B5：未命中矩阵的 KeyPress / 非 KeyPress → 委托 QApplication::notify，无空格注入
TEST_F(EditorApplicationTest, Notify_UnmatchedEvent_DelegatesToBaseWithoutSpaceInjection)
{
    // Arrange 1：普通 QWidget 的回车（className/objectName 均不命中）
    LateDeleteWidget guard;
    guard.w = new QWidget();
    SpaceEventCounter counter;
    guard.w->installEventFilter(&counter);
    QKeyEvent returnEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);

    // Act 1
    s_app->notify(guard.w, &returnEvent);

    // Assert 1：原始回车送达目标控件（委托基类），且无空格注入
    EXPECT_EQ(counter.returnPress, 1) << "unmatched key must reach the widget itself";
    EXPECT_EQ(counter.spacePress, 0);
    EXPECT_EQ(counter.spaceRelease, 0);

    // Arrange 2：非 KeyPress（MouseButtonPress → QLabel）
    QLabel label(QStringLiteral("lbl"));
    label.installEventFilter(&counter);
    QMouseEvent mousePress(QEvent::MouseButtonPress, QPointF(3, 3), QPointF(3, 3),
                           Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    // Act 2
    s_app->notify(&label, &mousePress);

    // Assert 2：鼠标事件直达，同样无空格注入
    EXPECT_EQ(counter.mousePress, 1);
    EXPECT_EQ(counter.spacePress, 0);
    EXPECT_EQ(counter.spaceRelease, 0);
}

// ---------------- pressSpace（含 80ms lambda）----------------

// 直调私有 pressSpace（-fno-access-control）：同步注入按下、80ms 后注入松开并触发点击
TEST_F(EditorApplicationTest, PressSpace_Button_ReleasesAfter80msAndClicks)
{
    // Arrange
    LateDeleteWidget guard;
    guard.w = new QPushButton(QStringLiteral("space"));
    auto *button = static_cast<QPushButton *>(guard.w);
    SpaceEventCounter counter;
    button->installEventFilter(&counter);
    int clickedCount = 0;
    QObject::connect(button, &QPushButton::clicked, [&clickedCount]() { ++clickedCount; });
    ASSERT_EQ(counter.spacePress, 0);

    // Act：同步按下
    s_app->pressSpace(button);

    // Assert：按下立即送达，松开未到
    EXPECT_EQ(counter.spacePress, 1);
    EXPECT_EQ(counter.spaceRelease, 0);

    // Act：等待 80ms singleShot 交付
    QTest::qWait(150);

    // Assert：松开送达且按钮被点击
    EXPECT_EQ(counter.spaceRelease, 1);
    EXPECT_EQ(clickedCount, 1);
}

// ---------------- dtor 前置 ----------------

// dtor(45) 前置：保证 TearDownTestSuite delete s_app 时 StartManager::instance()
// 非空，命中“Deleting StartManager instance”分支（D2/D0 双记录在 delete 时落点）
TEST_F(EditorApplicationTest, Dtor_Precondition_StartManagerInstanceAlive)
{
    // Arrange / Act：惰性单例真实构造（DBus/Iflytek 已 stub，XDG 已重定向）
    StartManager *instance = StartManager::instance();

    // Assert
    EXPECT_NE(instance, nullptr);
    EXPECT_EQ(StartManager::m_instance, instance);
}
