// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QKeyEvent>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDBusInterface>
#include <QSharedPointer>
#include <QTextDocument>
#include <QTemporaryFile>
#include <QPropertyAnimation>

#include "../../src/editor/markdown/markdownbridge.h"
#include "../../src/editor/markdown/themeserializer.h"
#include "../../src/widgets/bottombar.h"
#include "../../src/editor/editwrapper.h"
#include "../../src/widgets/window.h"
#include "../../src/editor/markdown/imarkdownrenderer.h"
#include "../../src/editor/dtextedit.h"
#include "../../src/common/settings.h"
#include "../../src/common/config.h"
#include "../../src/common/iflytek_ai_assistant.h"
#include "../../src/common/text_file_saver.h"
#include "../../src/editorapplication.h"
#include "../../src/startmanager.h"
#include "src/stub.h"

// ============================================================
// MarkdownBridge: tooltip getters + retranslate + tr
// ============================================================
TEST(UT_CoverageGap_MarkdownBridge, TooltipGetters)
{
    MarkdownBridge bridge;
    EXPECT_FALSE(bridge.collapseTooltip().isEmpty());
    EXPECT_FALSE(bridge.expandTooltip().isEmpty());
    EXPECT_FALSE(bridge.copyTooltip().isEmpty());
    EXPECT_FALSE(bridge.expandText().isEmpty());
    EXPECT_FALSE(bridge.collapsedLinesText().isEmpty());
}

TEST(UT_CoverageGap_MarkdownBridge, Retranslate_EmitsSignal)
{
    MarkdownBridge bridge;
    QSignalSpy spy(&bridge, &MarkdownBridge::retranslated);
    bridge.retranslate();
    EXPECT_EQ(spy.count(), 1);
}

TEST(UT_CoverageGap_MarkdownBridge, Tr_Static)
{
    QString result = MarkdownBridge::tr("test");
    EXPECT_EQ(result, QStringLiteral("test"));
}

// ============================================================
// ThemeSerializer: all bg/fg valid/invalid x dark/light combos
// ============================================================
TEST(UT_CoverageGap_ThemeSerializer, DarkValidBgValidFg)
{
    QVariantMap themeMap;
    QVariantMap ec;
    ec["background-color"] = "#1a1a1a";  // dark (lightness < 128)
    ec["text-color"] = "#ffffff";
    themeMap["editor-colors"] = ec;
    QString json = ThemeSerializer::serialize(themeMap);
    EXPECT_FALSE(json.isEmpty());
    EXPECT_TRUE(ThemeSerializer::isDark(themeMap));
}

TEST(UT_CoverageGap_ThemeSerializer, DarkInvalidBg)
{
    QVariantMap themeMap;
    QVariantMap ec;
    ec["background-color"] = "invalid";
    ec["text-color"] = "#ffffff";
    themeMap["editor-colors"] = ec;
    QString json = ThemeSerializer::serialize(themeMap);
    EXPECT_FALSE(json.isEmpty());
}

TEST(UT_CoverageGap_ThemeSerializer, LightValidBgValidFg)
{
    QVariantMap themeMap;
    QVariantMap ec;
    ec["background-color"] = "#ffffff";  // light
    ec["text-color"] = "#1f1f1f";
    themeMap["editor-colors"] = ec;
    QString json = ThemeSerializer::serialize(themeMap);
    EXPECT_FALSE(json.isEmpty());
    EXPECT_FALSE(ThemeSerializer::isDark(themeMap));
}

TEST(UT_CoverageGap_ThemeSerializer, LightInvalidBgInvalidFg)
{
    QVariantMap themeMap;
    QVariantMap ec;
    ec["background-color"] = "bad";
    themeMap["editor-colors"] = ec;
    // No text-color, no text-styles fallback
    QString json = ThemeSerializer::serialize(themeMap);
    EXPECT_FALSE(json.isEmpty());
}

TEST(UT_CoverageGap_ThemeSerializer, DarkInvalidFgWithTextStylesFallback)
{
    QVariantMap themeMap;
    QVariantMap ec;
    ec["background-color"] = "#1a1a1a";  // dark
    // no text-color in editor-colors
    themeMap["editor-colors"] = ec;
    QVariantMap ts;
    QVariantMap normal;
    normal["text-color"] = "#e0e0e0";
    ts["Normal"] = normal;
    themeMap["text-styles"] = ts;
    QString json = ThemeSerializer::serialize(themeMap);
    EXPECT_FALSE(json.isEmpty());
}

// ============================================================
// BottomBar: eventFilter
// ============================================================
TEST(UT_CoverageGap_BottomBar, EventFilter)
{
    BottomBar bar;
    QEvent appFontEvent(QEvent::ApplicationFontChange);
    bar.eventFilter(&bar, &appFontEvent);
    // Also test a different event type (fallthrough path)
    QEvent noneEvent(QEvent::None);
    bar.eventFilter(&bar, &noneEvent);
    bar.deleteLater();
}

// ============================================================
// IflytekAiAssistant: early-return paths + isCopilotEnabled + countEnabledPorts
// ============================================================
TEST(UT_CoverageGap_Iflytek, GetIatEnable_NotEnabled)
{
    IflytekAiAssistant *ins = IflytekAiAssistant::instance();
    auto oldStatus = ins->m_status;
    ins->m_status = IflytekAiAssistant::Invalid;
    auto ret = ins->getIatEnable();
    EXPECT_EQ(ret, IflytekAiAssistant::Invalid);
    ins->m_status = oldStatus;
}

TEST(UT_CoverageGap_Iflytek, GetTransEnable_NotEnabled)
{
    IflytekAiAssistant *ins = IflytekAiAssistant::instance();
    auto oldStatus = ins->m_status;
    ins->m_status = IflytekAiAssistant::Invalid;
    auto ret = ins->getTransEnable();
    EXPECT_EQ(ret, IflytekAiAssistant::Invalid);
    ins->m_status = oldStatus;
}

TEST(UT_CoverageGap_Iflytek, IsTtsEnable_NotEnabled)
{
    IflytekAiAssistant *ins = IflytekAiAssistant::instance();
    auto oldStatus = ins->m_status;
    ins->m_status = IflytekAiAssistant::Invalid;
    auto ret = ins->isTtsEnable();
    EXPECT_EQ(ret, IflytekAiAssistant::Invalid);
    ins->m_status = oldStatus;
}

TEST(UT_CoverageGap_Iflytek, IsTtsInWorking_NotEnabled)
{
    IflytekAiAssistant *ins = IflytekAiAssistant::instance();
    auto oldStatus = ins->m_status;
    ins->m_status = IflytekAiAssistant::Invalid;
    auto ret = ins->isTtsInWorking();
    EXPECT_EQ(ret, IflytekAiAssistant::Invalid);
    ins->m_status = oldStatus;
}

TEST(UT_CoverageGap_Iflytek, IsCopilotEnabled_InvalidInterface)
{
    QSharedPointer<QDBusInterface> copilot =
        QSharedPointer<QDBusInterface>::create("com.invalid.service",
                                               "/com/invalid/path",
                                               "com.invalid.iface");
    auto ret = IflytekAiAssistant::isCopilotEnabled(copilot);
    // With invalid interface, state.isValid() is false → returns Enable (adapt old version)
    EXPECT_EQ(ret, IflytekAiAssistant::Enable);
}

class UT_DeferredDeleteBlocker : public QObject
{
public:
    bool eventFilter(QObject *, QEvent *e) override
    {
        return e->type() == QEvent::DeferredDelete;
    }
};

// ============================================================
// EditWrapper: viewMode, isMarkdownFile, setMarkdownRendererForTest,
//              ensureLiveSplitterCreated, constructor lambdas
// ============================================================
TEST(UT_CoverageGap_EditWrapper, InlineGettersAndSetters)
{
    Window *window = new Window();
    EditWrapper *wrapper = new EditWrapper(window);

    // viewMode() and isMarkdownFile() getters
    (void)wrapper->viewMode();
    (void)wrapper->isMarkdownFile();

    // setMarkdownRendererForTest with nullptr
    wrapper->setMarkdownRendererForTest(nullptr);

    // ensureLiveSplitterCreated
    wrapper->ensureLiveSplitterCreated();
    // Call again to hit early-return
    wrapper->ensureLiveSplitterCreated();

    wrapper->deleteLater();
    window->deleteLater();
}

TEST(UT_CoverageGap_EditWrapper, ConstructorLambdas_ViewModeRequested)
{
    Window *window = new Window();
    EditWrapper *wrapper = new EditWrapper(window);
    UT_DeferredDeleteBlocker blocker;
    qApp->installEventFilter(&blocker);

    // Trigger the viewModeRequested lambda from BottomBar
    emit wrapper->bottomBar()->viewModeRequested(ViewMode::Edit);
    QTest::qWait(10);

    qApp->removeEventFilter(&blocker);
    wrapper->deleteLater();
    window->deleteLater();
}

TEST(UT_CoverageGap_EditWrapper, ConstructorLambdas_ViewModeChanged)
{
    Window *window = new Window();
    EditWrapper *wrapper = new EditWrapper(window);
    UT_DeferredDeleteBlocker blocker;
    qApp->installEventFilter(&blocker);

    // Trigger the viewModeChanged lambda via TextEdit
    emit wrapper->textEditor()->viewModeRequested(ViewMode::Edit);
    QTest::qWait(10);

    qApp->removeEventFilter(&blocker);
    wrapper->deleteLater();
    window->deleteLater();
}

// ============================================================
// Window: popupFindBar/popupReplaceBar timer lambdas
// ============================================================
TEST(UT_CoverageGap_Window, popupFindBar_TimerLambda)
{
    Window *w = new Window();
    UT_DeferredDeleteBlocker blocker;
    qApp->installEventFilter(&blocker);
    w->show();
    w->addBlankTab();
    w->currentWrapper()->textEditor()->setPlainText("12345 content");
    w->popupFindBar();
    QTest::qWait(50);  // let singleShot(10) fire
    qApp->removeEventFilter(&blocker);
    w->deleteLater();
}

TEST(UT_CoverageGap_Window, popupReplaceBar_TimerLambda)
{
    Window *w = new Window();
    UT_DeferredDeleteBlocker blocker;
    qApp->installEventFilter(&blocker);
    w->show();
    w->addBlankTab();
    w->currentWrapper()->textEditor()->setPlainText("12345 content");
    w->popupReplaceBar();
    QTest::qWait(50);  // let singleShot(10) fire
    qApp->removeEventFilter(&blocker);
    w->deleteLater();
}

// ============================================================
// TextEdit: highlight timer lambda + initRightClickedMenu action triggers
// ============================================================
TEST(UT_CoverageGap_TextEdit, Highlight_TimerLambda)
{
    Window *window = new Window();
    EditWrapper *wrapper = new EditWrapper(window);
    TextEdit *te = wrapper->textEditor();
    UT_DeferredDeleteBlocker blocker;
    qApp->installEventFilter(&blocker);
    te->highlight();
    QTest::qWait(10);  // let singleShot(0) fire
    qApp->removeEventFilter(&blocker);
    wrapper->deleteLater();
    window->deleteLater();
}

TEST(UT_CoverageGap_TextEdit, InitRightClickedMenu_MarkCurrentAct)
{
    Window *window = new Window();
    EditWrapper *wrapper = new EditWrapper(window);
    TextEdit *te = wrapper->textEditor();
    UT_DeferredDeleteBlocker blocker;
    qApp->installEventFilter(&blocker);
    te->initRightClickedMenu();
    if (te->m_markCurrentAct) {
        te->m_markCurrentAct->trigger();
    }
    QTest::qWait(10);
    qApp->removeEventFilter(&blocker);
    wrapper->deleteLater();
    window->deleteLater();
}

TEST(UT_CoverageGap_TextEdit, InitRightClickedMenu_MarkAllAct)
{
    Window *window = new Window();
    EditWrapper *wrapper = new EditWrapper(window);
    TextEdit *te = wrapper->textEditor();
    UT_DeferredDeleteBlocker blocker;
    qApp->installEventFilter(&blocker);
    te->initRightClickedMenu();
    if (te->m_markAllAct) {
        te->m_markAllAct->trigger();
    }
    QTest::qWait(10);
    qApp->removeEventFilter(&blocker);
    wrapper->deleteLater();
    window->deleteLater();
}

// ============================================================
// EditorApplication: notify with QCheckBox/QComboBox + pressSpace lambda
// ============================================================
TEST(UT_CoverageGap_EditorApp, Notify_QCheckBox)
{
    int argc = 1;
    char *argv[] = {"test"};
    EditorApplication *app = new EditorApplication(argc, argv);

    QCheckBox *checkbox = new QCheckBox;
    QKeyEvent *e = new QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, "\r");
    bool ret = app->notify(checkbox, e);
    EXPECT_TRUE(ret);
    QTest::qWait(100);  // let pressSpace timer fire

    delete e;
    // Intentionally leak checkbox AND app: pressSpace() 的 80ms 定时器裸捕获该控件，
    // 负载高时定时器可能在 qWait 结束、控件销毁之后才触发（悬空指针）；
    // 销毁第二个 QApplication 会置空 qApp，破坏后续所有测试。
}

TEST(UT_CoverageGap_EditorApp, Notify_QComboBox)
{
    int argc = 1;
    char *argv[] = {"test"};
    EditorApplication *app = new EditorApplication(argc, argv);

    QComboBox *combo = new QComboBox;
    QKeyEvent *e = new QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, "\r");
    bool ret = app->notify(combo, e);
    EXPECT_TRUE(ret);
    QTest::qWait(100);

    delete e;
    // Intentionally leak combo AND app (see Notify_QCheckBox).
}

// Stub functions for triggering catch blocks in saveToFile
static int stub_characterCount_badalloc(QTextDocument *)
{
    throw std::bad_alloc();
    return 0;
}

static QString stub_toPlainText_exception(QTextDocument *)
{
    throw std::runtime_error("test exception");
    return {};
}

TEST(UT_CoverageGap_TextFileSaver, SaveToFile_BadAlloc)
{
    QTextDocument *doc = new QTextDocument();
    doc->setPlainText("test");
    TextFileSaver saver(doc);

    QTemporaryFile tmpFile;
    ASSERT_TRUE(tmpFile.open());
    tmpFile.close();

    Stub s;
    s.set(ADDR(QTextDocument, characterCount), stub_characterCount_badalloc);

    bool ret = saver.saveToFile(tmpFile);
    EXPECT_FALSE(ret);
    delete doc;
}

TEST(UT_CoverageGap_TextFileSaver, SaveToFile_Exception)
{
    QTextDocument *doc = new QTextDocument();
    doc->setPlainText("test");
    TextFileSaver saver(doc);

    QTemporaryFile tmpFile;
    ASSERT_TRUE(tmpFile.open());
    tmpFile.close();

    Stub s;
    s.set(ADDR(QTextDocument, toPlainText), stub_toPlainText_exception);

    bool ret = saver.saveToFile(tmpFile);
    EXPECT_FALSE(ret);
    delete doc;
}

// ============================================================
// Config: constructor valueChanged lambda
// (only runs if DConfig is valid in test env)
// ============================================================
#ifdef DTKCORE_CLASS_DConfigFile
TEST(UT_CoverageGap_Config, ValueChangedLambda_AllKeys)
{
    Config *cfg = Config::instance();
    if (cfg->dconfig && cfg->dconfig->isValid()) {
        QMetaObject::invokeMethod(cfg->dconfig, "valueChanged",
                                  Qt::DirectConnection,
                                  Q_ARG(QString, "disableImproveGB18030"));
        QMetaObject::invokeMethod(cfg->dconfig, "valueChanged",
                                  Qt::DirectConnection,
                                  Q_ARG(QString, "enablePatchedIconv"));
        QMetaObject::invokeMethod(cfg->dconfig, "valueChanged",
                                  Qt::DirectConnection,
                                  Q_ARG(QString, "defaultEncoding"));
    }
    SUCCEED();
}
#endif

// ============================================================
// Additional gap tests (Continuation 10)
// ============================================================

// --- Mock IMarkdownRenderer for EditWrapper tests ---
class GapMockRenderer : public IMarkdownRenderer
{
public:
    bool m_ready = true;
    QString m_lastMd;
    void setMarkdown(const QString &md) override { m_lastMd = md; }
    bool isReady() const override { return m_ready; }
    void setMode(int) override {}
    void applyTheme(const QVariantMap &) override {}
    void setLayout(int, bool) override {}
    void scrollToRatio(double) override {}
};

// ThemeSerializer: dark (valid bg) + invalid fg (no text-color, no text-styles)
// Covers the dark fallback lambdas for --fg, --fg-secondary, --code-fg
TEST(UT_CoverageGap_ThemeSerializer, DarkValidBgInvalidFg_NoTextStyles)
{
    QVariantMap themeMap;
    QVariantMap ec;
    ec["background-color"] = "#1a1a1a";  // dark (lightness < 128)
    // No text-color — fg will be invalid
    themeMap["editor-colors"] = ec;
    // No text-styles fallback either
    QString json = ThemeSerializer::serialize(themeMap);
    EXPECT_FALSE(json.isEmpty());
    EXPECT_TRUE(ThemeSerializer::isDark(themeMap));

    // Verify the fallback values for dark mode
    auto doc = QJsonDocument::fromJson(json.toUtf8());
    auto obj = doc.object();
    EXPECT_EQ(obj.value("--fg").toString(), QStringLiteral("#ffffff"));
    EXPECT_EQ(obj.value("--fg-secondary").toString(), QStringLiteral("#a6a6a6"));
    EXPECT_EQ(obj.value("--code-fg").toString(), QStringLiteral("#ffffff"));
}

// EditWrapper: renderRequested lambda (const QString&)
// The constructor connects RenderThrottle::renderRequested to a lambda that
// calls m_pRenderer->setMarkdown when m_pRenderer is set.
TEST(UT_CoverageGap_EditWrapper, RenderRequestedLambda)
{
    Window *window = new Window();
    EditWrapper *wrapper = new EditWrapper(window);

    GapMockRenderer *mock = new GapMockRenderer;
    wrapper->setMarkdownRendererForTest(mock);

    // Make the throttle ready and trigger content
    wrapper->m_renderThrottle.setReady(true);
    wrapper->m_renderThrottle.setInterval(0);  // immediate flush
    wrapper->m_renderThrottle.noteContent(QStringLiteral("# test markdown"));

    // The lambda should have called setMarkdown on the mock
    EXPECT_EQ(mock->m_lastMd, QStringLiteral("# test markdown"));

    wrapper->setMarkdownRendererForTest(nullptr);
    delete mock;
    wrapper->deleteLater();
    window->deleteLater();
}

// EditWrapper: markdownAvailabilityChanged lambda (bool)
// The constructor connects this signal to a lambda that calls
// m_pTextEdit->updateViewModeActions(m_viewMode, m_isMarkdown)
TEST(UT_CoverageGap_EditWrapper, MarkdownAvailabilityChangedLambda)
{
    Window *window = new Window();
    EditWrapper *wrapper = new EditWrapper(window);

    UT_DeferredDeleteBlocker blocker;
    qApp->installEventFilter(&blocker);

    // Emit the signal to trigger the lambda
    emit wrapper->markdownAvailabilityChanged(true);
    QTest::qWait(10);

    qApp->removeEventFilter(&blocker);
    wrapper->deleteLater();
    window->deleteLater();
}

// TextEdit: trigger redo and readView actions in initRightClickedMenu
// These cover the remaining initRightClickedMenu lambdas
TEST(UT_CoverageGap_TextEdit, InitRightClickedMenu_RedoAndReadView)
{
    Window *window = new Window();
    EditWrapper *wrapper = new EditWrapper(window);
    TextEdit *te = wrapper->textEditor();

    UT_DeferredDeleteBlocker blocker;
    qApp->installEventFilter(&blocker);

    te->initRightClickedMenu();
    // Trigger redo action lambda
    if (te->m_redoAction) {
        te->m_redoAction->trigger();
    }
    QTest::qWait(10);
    // Trigger read view action lambda (emits viewModeRequested)
    if (te->m_actReadView) {
        te->m_actReadView->trigger();
    }
    QTest::qWait(10);

    qApp->removeEventFilter(&blocker);
    wrapper->deleteLater();
    window->deleteLater();
}

// TextEdit: trigger undo, edit view, and live preview actions
TEST(UT_CoverageGap_TextEdit, InitRightClickedMenu_UndoEditViewLivePreview)
{
    Window *window = new Window();
    EditWrapper *wrapper = new EditWrapper(window);
    TextEdit *te = wrapper->textEditor();

    UT_DeferredDeleteBlocker blocker;
    qApp->installEventFilter(&blocker);

    te->initRightClickedMenu();
    if (te->m_undoAction) {
        te->m_undoAction->trigger();
    }
    QTest::qWait(10);
    if (te->m_actEditView) {
        te->m_actEditView->trigger();
    }
    QTest::qWait(10);
    if (te->m_actLivePreview) {
        te->m_actLivePreview->trigger();
    }
    QTest::qWait(10);

    qApp->removeEventFilter(&blocker);
    wrapper->deleteLater();
    window->deleteLater();
}

// EditorApplication: pressSpace timer lambda is NOT testable safely.
// pressSpace() creates QTimer::singleShot(80, this, [btn]{...}) which captures
// the button by raw pointer. Previous EditorApplication tests (notify_001 etc.)
// leave pending singleShot timers; any QTest::qWait() in this test causes those
// timers to fire on deleteLater'd buttons → use-after-free. The pressSpace()
// function itself is already covered by UT_EditorApplication_pressSpace.pressSpace.
