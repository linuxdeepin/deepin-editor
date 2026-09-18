// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// editor_core（B7：TextEdit）共享测试夹具。
//
// 隔离策略（全部在 SetUp 安装、TearDown stub.clear() 还原）：
// - QDBusConnection::sessionBus/systemBus → 未连接伪连接，绝不触碰真实总线
//   （TextEdit 构造/析构会 connect/disconnect Gesture/Audio DBus 信号）。
// - EditWrapper/Window/BottomBar → fake 指针（指向真实 QWidget 载体对象，
//   dynamic_cast/qobject_cast 安全返回 null）+ 桩函数喂返回值；绝不真实构造 Window。
// - QMenu::exec（虚函数，VADDR 取真实地址）→ 返回 nullptr，防止右键菜单模态阻塞。
// - XDG_CONFIG_HOME 重定向到 QTemporaryDir，Settings 单例读写临时配置，不碰 ~/.config。
// - QApplication + QT_QPA_PLATFORM=offscreen（Wave1 已验证无头构造可行）。
// - 语法高亮定义：QTemporaryDir 写入自定义 XML + m_repository.addCustomSearchPath，
//   toggleComment/setSyntaxDefinition 全链路封闭（不依赖系统 syntax 目录）。
// - 剪贴板：offscreen 平台内置内存剪贴板（进程内隔离）。

#ifndef EDITOR_CORE_FIXTURE_H
#define EDITOR_CORE_FIXTURE_H

#include <gtest/gtest.h>
#include "stubext.h"

#include "dtextedit.h"
#include "editwrapper.h"
#include "../widgets/window.h"
#include "../widgets/bottombar.h"
#include "settings.h"

#include <QApplication>
#include <QClipboard>
#include <DSettings>
#include <DSettingsOption>
#include <DSettingsGroup>
#include <QTemporaryDir>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QSignalSpy>
#include <QWheelEvent>
#include <QInputMethodEvent>
#include <QGestureEvent>
#include <QTapGesture>
#include <QTapAndHoldGesture>
#include <QPanGesture>
#include <QPinchGesture>
#include <QSwipeGesture>
#include <QDBusConnection>
#include <QDBusAbstractInterface>
#include <QDBusPendingCall>
#include <QMenu>
#include <QDir>
#include <QFile>
#include <QTextCursor>

namespace {

const char *kEcOrgName = "deepin";
const char *kEcAppName = "deepin-editor";

// 临时语法高亮定义：单行 // 与多行 /* */ 注释，扩展名 *.utlang
// （<general><comments> 为注释标记解析位置；kateversion>=5.62 保证新式解析）
const char *kEcSyntaxXml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!DOCTYPE language>\n"
        "<language name=\"UTLang\" section=\"Sources\" extensions=\"*.utlang\""
        " kateversion=\"5.62\" version=\"2\" priority=\"100\">\n"
        "  <highlighting>"
        "<contexts><context attribute=\"Normal Text\" lineEndContext=\"#stay\" name=\"Normal Text\"/></contexts>"
        "<itemDatas><itemData name=\"Normal Text\" defStyleNum=\"dsNormal\"/></itemDatas>"
        "</highlighting>\n"
        "  <general>"
        "<comments><comment name=\"singleLine\" start=\"//\"/>"
        "<comment name=\"multiLine\" start=\"/*\" end=\"*/\"/></comments>"
        "</general>\n"
        "</language>\n";

} // namespace

// deepin-editor.qrc 编入静态库时其静态初始化器不会被链接器自动拉入，
// 显式声明 rcc 生成的 init 函数强制引用（qrc 文件名 '-' → '_'）
int qInitResources_deepin_editor();

class TextEditTestBase : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        s_configHome = new QTemporaryDir();
        const QString xdgConfig = s_configHome->filePath("xdg-config");
        QDir().mkpath(xdgConfig);
        qputenv("XDG_CONFIG_HOME", xdgConfig.toUtf8());
        qputenv("QT_QPA_PLATFORM", "offscreen");
        if (QApplication::instance() == nullptr) {
            s_app = new QApplication(s_ecArgc, s_ecArgv);
        }
        QApplication::setOrganizationName(QString::fromLatin1(kEcOrgName));
        QApplication::setApplicationName(QString::fromLatin1(kEcAppName));
        // 注册 qrc 资源（:/resources/settings.json 等），随后构造共享单例
        qInitResources_deepin_editor();
        Settings::instance();
        // 临时语法高亮定义目录（toggleComment 等使用；
        // addCustomSearchPath 要求定义位于 <path>/syntax/*.xml）
        s_syntaxDir = new QTemporaryDir();
        QDir().mkpath(s_syntaxDir->filePath(QString("syntax")));
        QFile syntaxFile(s_syntaxDir->filePath(QString("syntax/utlang.xml")));
        if (syntaxFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            syntaxFile.write(kEcSyntaxXml);
            syntaxFile.close();
        }
    }

    static void TearDownTestSuite()
    {
        // QApplication 故意不销毁：套件顺序与进程退出安全优先
        // 还原环境变量（与 qputenv 数量配平，避免环境泄漏）
        qunsetenv("XDG_CONFIG_HOME");
        qunsetenv("QT_QPA_PLATFORM");
        delete s_syntaxDir;
        s_syntaxDir = nullptr;
        delete s_configHome;
        s_configHome = nullptr;
    }

    void SetUp() override
    {
        stub.clear();
        resetSeamState();
        installDbusIsolation();
        installWrapperSeams();
        installMenuExecStub();

        edit = new TextEdit();
        ASSERT_NE(edit, nullptr);
        edit->setSettings(Settings::instance());
        edit->setWrapper(fakeWrapper());
        // 封闭语法高亮检索路径：自定义定义目录 + reload 立即生效（不依赖系统 syntax 目录）
        edit->m_repository.addCustomSearchPath(s_syntaxDir->path());
        edit->m_repository.reload();
        edit->resize(480, 360);
    }

    void TearDown() override
    {
        // 析构期间 DBus/版本号桩必须仍然生效（dtor 会 disconnect DBus 信号）
        if (edit) {
            delete edit;
            edit = nullptr;
        }
        stub.clear();
    }

    // ============ 桩状态（每用例 SetUp 复位，lambda 以指针捕获读写） ============

    void resetSeamState()
    {
        seamFileLoading = false;
        seamFindBarVisible = false;
        seamReplaceBarVisible = false;
        seamEndlineFormat = BottomBar::Unix;
        seamKeywordSearchAll = QString("ut_sync"); // 与 keywordSearch 保持一致以进入高亮分支
        seamKeywordSearch = QString("ut_sync");
        seamFakeEndlineCalls = 0;
        highlighterCalls = 0;
        wordCntCalls = 0;
        updatePosCalls = 0;
        updateModifyCalls = 0;
        setTemFileCalls = 0;
    }

    // ============ DBus 隔离 ============

    void installDbusIsolation()
    {
        stub.set_lamda(&QDBusConnection::sessionBus,
                       []() -> QDBusConnection {
                           return QDBusConnection(QStringLiteral("ut-editor-core-bus"));
                       });
        stub.set_lamda(&QDBusConnection::systemBus,
                       []() -> QDBusConnection {
                           return QDBusConnection(QStringLiteral("ut-editor-core-sys"));
                       });
        // Qt6.8：所有 call() 重载经 callWithArgumentList 汇聚（含 DTK DConfig 后台线程的
        // isServiceRegistered 探测），统一拦截为"无回复"，伪连接上的真实调用即安全失败
        stub.set_lamda(&QDBusAbstractInterface::callWithArgumentList,
                       [](QDBusAbstractInterface *, QDBus::CallMode,
                          const QString &, const QList<QVariant> &) -> QDBusMessage {
                           return QDBusMessage();
                       });
        stub.set_lamda(&QDBusAbstractInterface::asyncCallWithArgumentList,
                       [](QDBusAbstractInterface *, const QString &,
                          const QList<QVariant> &) -> QDBusPendingCall {
                           return QDBusPendingCall::fromCompletedCall(QDBusMessage());
                       });
    }

    // ============ EditWrapper / Window / BottomBar 接缝 ============

    EditWrapper *fakeWrapper() { return reinterpret_cast<EditWrapper *>(&m_wrapCarrier); }
    Window *fakeWindow() { return reinterpret_cast<Window *>(&m_wndCarrier); }
    BottomBar *fakeBottomBar() { return reinterpret_cast<BottomBar *>(&m_barCarrier); }

    void installWrapperSeams()
    {
        stub.set_lamda(&EditWrapper::window,
                       [this](EditWrapper *) -> Window * { return fakeWindow(); });
        stub.set_lamda(&EditWrapper::bottomBar,
                       [this](EditWrapper *) -> BottomBar * { return fakeBottomBar(); });
        stub.set_lamda(&EditWrapper::OnUpdateHighlighter,
                       [this](EditWrapper *) { ++highlighterCalls; });
        stub.set_lamda(&EditWrapper::getFileLoading,
                       [this](EditWrapper *) -> bool { return seamFileLoading; });
        stub.set_lamda(&EditWrapper::setTemFile,
                       [this](EditWrapper *, bool) { ++setTemFileCalls; });
        stub.set_lamda(&EditWrapper::isTemFile,
                       [](EditWrapper *) -> bool { return false; });
        stub.set_lamda(&EditWrapper::isBackupFile,
                       [](EditWrapper *) -> bool { return false; });
        stub.set_lamda(&EditWrapper::isDraftFile,
                       [](EditWrapper *) -> bool { return false; });
        stub.set_lamda(&EditWrapper::isPlainTextEmpty,
                       [](EditWrapper *) -> bool { return true; });
        stub.set_lamda(&EditWrapper::UpdateBottomBarWordCnt,
                       [this](EditWrapper *, int) { ++wordCntCalls; });

        stub.set_lamda(&Window::findBarIsVisiable,
                       [this](Window *) -> bool { return seamFindBarVisible; });
        stub.set_lamda(&Window::replaceBarIsVisiable,
                       [this](Window *) -> bool { return seamReplaceBarVisible; });
        stub.set_lamda(&Window::getKeywordForSearchAll,
                       [this](Window *) -> QString { return seamKeywordSearchAll; });
        stub.set_lamda(&Window::getKeywordForSearch,
                       [this](Window *) -> QString { return seamKeywordSearch; });
        stub.set_lamda(&Window::updateModifyStatus,
                       [this](Window *, const QString &, bool) { ++updateModifyCalls; });

        // BottomBar::getEndlineFormat 有 static QByteArray 重载，static_cast 选成员版本
        stub.set_lamda(static_cast<BottomBar::EndlineFormat (BottomBar::*)()>(&BottomBar::getEndlineFormat),
                       [this](BottomBar *) -> BottomBar::EndlineFormat { return seamEndlineFormat; });
        stub.set_lamda(&BottomBar::setEndlineMenuText,
                       [this](BottomBar *, BottomBar::EndlineFormat) { ++seamFakeEndlineCalls; });
        stub.set_lamda(&BottomBar::updatePosition,
                       [this](BottomBar *, int, int) { ++updatePosCalls; });
    }

    // ============ 菜单模态阻塞隔离（QMenu::exec 虚函数 → VADDR 取真实地址） ============

    void installMenuExecStub()
    {
        // QMenu::exec 三个重载均非虚（Qt 6.8 qmenu.h），static_cast 选定定点版本
        using QMenuExecSig = QAction *(QMenu::*)(const QPoint &, QAction *);
        QMenuExecSig execPmf = static_cast<QMenuExecSig>(&QMenu::exec);
        stub.set_lamda(execPmf,
                       [](QMenu *, const QPoint &, QAction *) -> QAction * { return nullptr; });
    }

    // ============ iflytek AI 语音桩（slotVoiceReading/slotdictation/slot_translate 等） ============

    void installIflytekStubs(IflytekAiAssistant::CallStatus speechStatus = IflytekAiAssistant::Success,
                             bool hasOutput = true, bool hasInput = true)
    {
        stub.set_lamda(&IflytekAiAssistant::textToSpeech,
                       [speechStatus](IflytekAiAssistant *) -> IflytekAiAssistant::CallStatus {
                           return speechStatus;
                       });
        stub.set_lamda(&IflytekAiAssistant::stopTtsDirectly,
                       [](IflytekAiAssistant *) -> IflytekAiAssistant::CallStatus {
                           return IflytekAiAssistant::Success;
                       });
        stub.set_lamda(&IflytekAiAssistant::speechToText,
                       [speechStatus](IflytekAiAssistant *) -> IflytekAiAssistant::CallStatus {
                           return speechStatus;
                       });
        stub.set_lamda(&IflytekAiAssistant::textToTranslate,
                       [speechStatus](IflytekAiAssistant *) -> IflytekAiAssistant::CallStatus {
                           return speechStatus;
                       });
        stub.set_lamda(&IflytekAiAssistant::errorString,
                       [](IflytekAiAssistant *, IflytekAiAssistant::CallStatus) -> QString {
                           return QString("ut-ai-error");
                       });
        stub.set_lamda(&IflytekAiAssistant::hasAudioOutputDevice,
                       [hasOutput](const IflytekAiAssistant *) -> bool { return hasOutput; });
        stub.set_lamda(&IflytekAiAssistant::hasAudioInputDevice,
                       [hasInput](const IflytekAiAssistant *) -> bool { return hasInput; });
    }

    // ============ 通用驱动辅助 ============

    // 设置文档内容并回到起始位置（绕过撤销栈，纯数据准备）
    void setDocText(const QString &text)
    {
        QTextCursor cur(edit->document());
        cur.select(QTextCursor::Document);
        cur.insertText(text);
        cur.movePosition(QTextCursor::Start);
        edit->setTextCursor(cur);
    }

    QTextCursor makeCursor(int pos)
    {
        QTextCursor cur(edit->document());
        cur.setPosition(pos);
        return cur;
    }

    void moveCursorTo(int pos)
    {
        QTextCursor cur = edit->textCursor();
        cur.setPosition(pos);
        edit->setTextCursor(cur);
    }

    // 按 settings 中配置的 editor 快捷键名发送按键（默认 keymap 下与 JSON default 一致）
    void sendShortcut(const QString &actionName)
    {
        const QString keyStr = Settings::instance()->settings
                ->option(QString("shortcuts.editor.%1").arg(actionName))->value().toString();
        ASSERT_FALSE(keyStr.isEmpty()) << "shortcut option empty: " << actionName.toStdString();
        QKeySequence seq(keyStr);
        ASSERT_FALSE(seq.isEmpty());
        const int combined = seq[0].toCombined();
        const Qt::KeyboardModifiers mods =
                Qt::KeyboardModifiers(combined & static_cast<int>(Qt::KeyboardModifierMask));
        const int key = combined & ~static_cast<int>(Qt::KeyboardModifierMask);
        QKeyEvent press(QEvent::KeyPress, key, mods, QString());
        QApplication::sendEvent(edit, &press);
    }

    void sendKey(int key, Qt::KeyboardModifiers mods = Qt::NoModifier, const QString &text = QString())
    {
        QKeyEvent press(QEvent::KeyPress, key, mods, text);
        QApplication::sendEvent(edit, &press);
    }

    void sendMouse(QEvent::Type type, const QPointF &localPos, Qt::MouseButton button,
                   Qt::KeyboardModifiers mods = Qt::NoModifier)
    {
        // 直接驱动受保护事件处理器：offscreen 下未 show 控件的 QApplication::sendEvent
        // 鼠标派发受窗口状态影响不稳定，白盒直调确定性更高（-fno-access-control）
        QMouseEvent ev(type, localPos, localPos, button,
                       (type == QEvent::MouseButtonRelease) ? Qt::NoButton : button, mods);
        switch (type) {
        case QEvent::MouseButtonPress:
            edit->mousePressEvent(&ev);
            break;
        case QEvent::MouseMove:
            edit->mouseMoveEvent(&ev);
            break;
        case QEvent::MouseButtonRelease:
            edit->mouseReleaseEvent(&ev);
            break;
        default:
            break;
        }
    }

    // 触发一次真实 paintEvent（offscreen grab 渲染）
    void triggerPaint() { edit->grab(); }

    // ============ 成员 ============

    stub_ext::StubExt stub;
    TextEdit *edit = nullptr;

    // fake 指针载体：真实 QWidget 对象，保证误触发 vcalls/dynamic_cast 安全
    QWidget m_wrapCarrier;
    QWidget m_wndCarrier;
    QWidget m_barCarrier;

    // 接缝可变状态
    bool seamFileLoading = false;
    bool seamFindBarVisible = false;
    bool seamReplaceBarVisible = false;
    BottomBar::EndlineFormat seamEndlineFormat = BottomBar::Unix;
    QString seamKeywordSearchAll;
    QString seamKeywordSearch;

    // 调用计数
    int seamFakeEndlineCalls = 0;
    int highlighterCalls = 0;
    int wordCntCalls = 0;
    int updatePosCalls = 0;
    int updateModifyCalls = 0;
    int setTemFileCalls = 0;

    static QTemporaryDir *s_configHome;
    static QTemporaryDir *s_syntaxDir;
    static QApplication *s_app;

private:
    static int s_ecArgc;
    static char s_ecArgv0[];
    static char *s_ecArgv[2];
};

QTemporaryDir *TextEditTestBase::s_configHome = nullptr;
QTemporaryDir *TextEditTestBase::s_syntaxDir = nullptr;
QApplication *TextEditTestBase::s_app = nullptr;
int TextEditTestBase::s_ecArgc = 1;
char TextEditTestBase::s_ecArgv0[] = "test_editor_core";
char *TextEditTestBase::s_ecArgv[2] = { TextEditTestBase::s_ecArgv0, nullptr };

// 每个功能组一个派生 Fixture（命名规范 {ClassName}Test 派生）
class TextEditTest : public TextEditTestBase
{
};

#endif // EDITOR_CORE_FIXTURE_H
