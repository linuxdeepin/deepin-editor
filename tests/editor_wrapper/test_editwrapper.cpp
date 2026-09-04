// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// ============================================================================
// EditWrapper 单元测试（批次 B8，单类：src/editor/editwrapper.h/.cpp）
//
// 测试方法论清单（test-types.md §8，完成情况）：
// 1. 公开方法已列出（见下方方法映射），每个已定义方法 ≥ 1 用例
//    [注] clearAllFocus / getTextChangeFlag / setTextChangeFlag / initToastPosition
//    在 editwrapper.h 声明但全 src/ 无定义（链接无符号），无法调用，记 uncovered。
// 2. 输入维度等价类：编码（同/异/非法）、文件（存在/缺失/只读/草稿/备份）、
//    文件名（含 double/four/user/普通）、视图模式（Edit/ReadView/LivePreview/Wysiwyg）
// 3. 边界值：空消息/空内容/空路径、300MB 分段边界不构造（以 endline 边界代替）、
//    500KB 备份阈值上下界（clearDoubleCharaterEncode TEST_P）
// 4. 同质多组输入用 TEST_P：ClearDouble / Endline / Recognition / Theme
// 5. 分支清单已列出并映射到用例名（见下）
// 6. 每条 if/switch/early-return 分支有触发用例（dialog 取消/保存/丢弃三分支等）
// 7. 无异常抛出路径（Qt 代码用 bool 返回），错误路径按 §5.3 断言返回值+状态
// 8. 负面场景：缺失文件/非法编码/不可写目录/空消息/重复编码
// 9. 负面用例验证强异常安全（编码回滚、内容未损坏）
// 10. 依赖均为 Qt/DTK 类与项目内非注入类 → 统一 stub_ext（无 gMock 混用目标）
//
// 分支清单（来源：editwrapper.cpp，B 编号 → 用例名映射）：
// - ctor                        → Constructor_WithParent_CreatesCoreChildren
// - ~EditWrapper                → Destructor_OwnedWidgetsDeleted_QPointersNull
// - setQuitFlag/isQuit/getFileLoading → QuitFlag_AfterSet_BothFlagsTrue
// - openFile                    → OpenFile_RealTempFile_LoadsContentEndToEnd /
//                                OpenFile_TemFileFlag_KeepsTemState
// - readFile B1:open失败        → ReadFile_MissingFile_ReturnsFalse
//          B2:编码转换失败      → ReadFile_InvalidEncode_ReturnsFalse
//          B3:自动探测+成功     → ReadFile_AutoDetect_LoadsContentAndEncode
//          B4:指定编码成功      → ReadFile_SpecifiedEncode_UpdatesCurEncode
// - saveFile B5:预览模式拒绝    → SaveFile_InPreviewMode_ReturnsFalse
//           B6:encode非空更新   → SaveFile_WithEncode_UpdatesCurEncodeState
//           B7:保存成功         → SaveFile_ValidFile_PersistsContentAndStatus
//           B8:保存失败         → SaveFile_UnwritableTarget_ReturnsFalse
// - getPlainTextContent B9:Win/Unix → GetPlainTextContent_EndlineVariants_ReturnsExpectedBytes (TEST_P)
// - saveAsFile(path,enc) B10/11  → SaveAsFile_ValidPath_WritesFileAndMtime /
//                                SaveAsFile_UnwritablePath_ReturnsFalse
// - saveAsFile() B12:dialog拒绝  → SaveAsFileNoArg_DialogRejected_ReturnsFalse
//                                 SaveAsFileNoArg_AcceptedEmptySelection_ReturnsFalse
// - reloadFileEncode B13:相同编码 → ReloadEncode_SameEncode_ReturnsFalse
//                 B14:草稿+空    → ReloadEncode_DraftEmpty_ReturnsTrue
//                 B15:修改+取消  → ReloadEncode_ModifiedDialogCancel_ReturnsFalse
//                 B16:修改+保存草稿失败 → ReloadEncode_ModifiedDraftSaveFail_RollsBackEncode
//                 B17:修改+另存成功     → ReloadEncode_ModifiedSaveAsThenRead_ReturnsTrue
//                 B18:未修改直接读      → ReloadEncode_NotModified_ReadsFile
// - reloadFileHighlight B19/20   → ReloadHighlight_ValidDef_CreatesHighlighter /
//                                ReloadHighlight_InvalidDef_RemovesHighlighter
// - reloadModifyFile B21:未修改  → ReloadModify_NotModified_ReloadsDiskContent
//                  B22:修改+关闭 → ReloadModify_ModifiedCancel_KeepsBuffer
//                  B23:修改+丢弃 → ReloadModify_ModifiedDiscard_ReloadsAndClearsTem
//                  B24:修改+另存草稿失败 → ReloadModify_ModifiedSaveDraftFail_KeepsBuffer
// - getTextEncode               → ReadFile_SpecifiedEncode_UpdatesCurEncode（等）
// - saveTemFile B25/26          → SaveTemFile_ValidDir_WritesBackupAndEncode /
//                                SaveTemFile_UnwritableDir_ReturnsFalse
// - updatePath B27:空truePath   → UpdatePath_EmptyTruePath_FallsBackToFile
//                            → UpdatePath_ValidPaths_SetsPathsAndMtime
// - isModified B28              → IsModified_ParamVariants_ReturnsExpected (TEST_P)
// - isDraftFile/isBackupFile    → DraftAndBackupFile_VariousPaths_DetectCorrectly
// - isPlainTextEmpty            → PlainTextEmpty_DefaultTrue_AfterInsertFalse
// - isTemFile/setTemFile        → SetTemFile_TrueAndFalse_RoundTrips
// - hideWarningNotices          → HideWarningNotices_WhenVisible_HidesNotices
// - checkForReload B29:草稿早退 → CheckForReload_DraftFile_SkipsCheck
//                B30:未变更     → CheckForReload_UnchangedFile_NoNotice
//                B31:文件被删   → CheckForReload_FileRemoved_ShowsSaveAsNotice
//                B32:文件变更   → CheckForReload_FileChanged_ShowsReloadNotice
// - showNotify B33:空消息       → ShowNotify_EmptyMessage_SendsNothing
//            B34:告警/只读分支  → ShowNotify_WarningFlag_SendsWarningFloat
//            B35:正常分支       → ShowNotify_NormalMessage_SendsOkFloat
// - setLineNumberShow B36/37    → SetLineNumberShow_ShowAndHide_TogglesAreaAndFlag
// - setShowBlankCharacter B38/39→ SetShowBlankCharacter_TrueAndFalse_TogglesTextOptionFlags
// - clearDoubleCharaterEncode B40:关键字+小文件 B41:大文件 B42:无关键字
//                              → ClearDoubleCharaterEncode_ParamVariants_EmitsAsExpected (TEST_P)
// - bottomBar/filePath/textEditor/window → Accessors_AfterConstruction_ReturnCoreChildren
// - updateHighlighterAll B43/44 → UpdateHighlighterAll_WithHighlighter_MarksAllDone /
//                                UpdateHighlighterAll_WhenQuit_Skips
// - get/setLastModifiedTime    → LastModifiedTime_TextDateRoundTrip_PreservesTime / UpdatePath_ValidPaths...
// - updateModifyStatus B45:加载中跳过 → UpdateModifyStatus_DuringLoading_SkipsWindow
//                     B46:正常      → UpdateModifyStatus_TrueAndFalse_RecordsInWindow
// - updateSaveAsFileName       → UpdateSaveAsFileName_OldAndNewPaths_DelegatesToWindow
// - exitInvalidCharPreview     → ExitInvalidCharPreview_AfterPreview_ResetsAllState
// - forceSaveInvalidCharFile B47/48 → ForceSave_WritableOriginal_WritesAndExitsPreview /
//                                    ForceSave_UnwritableTarget_KeepsPreview
// - setViewMode B49:FSM拒绝     → SetViewMode_WysiwygOrNonMdLivePreview_Rejected
//           B50:非md只读视图    → SetViewMode_ReadViewOnNonMd_TogglesReadOnlyText
//           B51:md渲染路径      → SetViewMode_MarkdownLivePreview_RendersViaRenderer
//           B52:ReadView布局    → SetViewMode_MarkdownReadView_Layout800Centered
// - updateMarkdownRecognition B53:识别/信号 (TEST_P)
//                        B54:丢失回退 → UpdateMarkdownRecognition_MdLost_FallsBackToEdit
// - setMarkdownRendererForTest → SetMarkdownRenderer_Null_RevertsToNoRenderer
// - customEvent B55:大文件分步  → CustomEvent_LargeContent_ParsesInSteps（经
//                   handleFileLoadFinished→loadContent>40MB 间接驱动；
//                   bad_alloc/exception 分支无法确定性触发，不覆盖）
// - handleFilePreProcess B56    → FilePreProcess_GivenEncodeAndContent_InsertsAndFlags
// - handleFileLoadFinished B57:无预处理 B58:error+文件在 B59:error+文件缺
//                            B60:hasNul预览 B61:预设光标 B62:历史光标
//                            → HandleLoadFinished_* 系列
// - OnThemeChangeSlot B63/64    → ThemeChange_LightAndDark_AppliesPaletteAndTheme (TEST_P) /
//                                ThemeChange_WithoutHighlighter_StillAppliesPalette
// - UpdateBottomBarWordCnt      → UpdateBottomBarWordCnt_GivenCount_RefreshesLabel
// - OnUpdateHighlighter         → OnUpdateHighlighter_VariousStates_NoUndueStateChange（含 quit 分支）
// - setTemFile/setRestoreCursorPosition/onEditAnyway → SetTemFile_TrueAndFalse_RoundTrips /
//                                HandleLoadFinished_CursorPreset_RestoresOnce /
//                                OnEditAnyway_InPreviewMode_EnablesEditing
//
// 环境隔离：
// - XDG_CONFIG_HOME / XDG_DATA_HOME → QTemporaryDir（SetUpTestSuite 一次，
//   TearDownTestSuite qunsetenv 配平），Settings/草稿目录/备份目录全部落在临时区
// - DRecentManager::addItem / DMessageManager::sendMessage /
//   Utils::sendFloatMessageFixedFont / DDialog::exec / QDialog::exec /
//   QFileDialog::selectedFiles / DFileDialog::getComboBoxValue 全 stub
// - Window/Tabbar 伪指针（reinterpret_cast 自真实 QWidget/QObject 内存），
//   被调非虚成员全部 stub_ext 拦截，绝不构造真实 Window
// - 文件用例全部基于 QTemporaryDir 真实临时文件，无硬编码绝对路径
// ============================================================================

#include <gtest/gtest.h>
#include "stubext.h"

#include "editwrapper.h"
#include "bottombar.h"
#include "warningnotices.h"
#include "window.h"
#include "tabbar.h"
#include "leftareaoftextedit.h"
#include "../common/utils.h"
#include "../common/settings.h"
#include "markdown/imarkdownrenderer.h"
#include "markdown/markdownlogic.h"
#include "markdown/markdownview.h"
#include "markdown/viewmodefsm.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QPointer>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QTabBar>
#include <DTabBar>
#include <QTemporaryDir>
#include <QTextCursor>
#include <QThread>

#include <DDialog>
#include <DDockWidget>
#include <DFileDialog>
#include <DMessageManager>
#include <DSettingsOption>
#include <drecentmanager.h>

#include <functional>

DCORE_USE_NAMESPACE

namespace {

// ==================== 测试替身：渲染器记录桩 ====================
// IMarkdownRenderer 为项目内纯虚接口且可经 setMarkdownRendererForTest 注入，
// 按技能约定本可用 gMock；为与全文件 stub_ext 记录桩风格统一并避免
// A9（同一目标混用 stub_ext 与 gMock），此处用手写记录桩实现。
class RecordingRenderer : public IMarkdownRenderer {
public:
    bool isReady() const override { return ready; }

    void setMarkdown(const QString &md) override
    {
        ++setMarkdownCalls;
        lastMarkdown = md;
    }
    void setMode(int mode) override
    {
        ++setModeCalls;
        lastMode = mode;
    }
    void applyTheme(const QVariantMap &themeMap) override
    {
        ++applyThemeCalls;
        lastTheme = themeMap;
    }
    void setLayout(int maxContentWidth, bool center) override
    {
        ++setLayoutCalls;
        lastMaxWidth = maxContentWidth;
        lastCenter = center;
    }
    void scrollToRatio(double ratio) override
    {
        ++scrollCalls;
        lastRatio = ratio;
    }

    bool ready = false;
    int setMarkdownCalls = 0;
    int setModeCalls = 0;
    int applyThemeCalls = 0;
    int setLayoutCalls = 0;
    int scrollCalls = 0;
    QString lastMarkdown;
    int lastMode = -1;
    int lastMaxWidth = -1;
    bool lastCenter = false;
    double lastRatio = -1.0;
    QVariantMap lastTheme;
};

// ==================== TEST_P 参数结构 ====================

// clearDoubleCharaterEncode：文件名关键字 × 文件大小（B40/B41/B42）
struct ClearDoubleCase {
    QString fileName;
    int fileSize;
    bool expectEmit;
};

// getPlainTextContent：换行格式（B9）
struct EndlineCase {
    BottomBar::EndlineFormat format;
    const char *expected;
};

// updateMarkdownRecognition：文件名 × definition 名（B53）
struct RecognitionCase {
    QString fileName;
    QString definitionName;
    bool expectMarkdown;
};

// OnThemeChangeSlot：亮暗主题（B63/B64）
struct ThemeCase {
    QString backgroundColor;
    QString textColor;
};

} // namespace

class EditWrapperTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        s_configHome = new QTemporaryDir();
        s_dataHome = new QTemporaryDir();
        const QString xdgConfig = s_configHome->filePath("xdg-config");
        const QString xdgData = s_dataHome->filePath("xdg-data");
        QDir().mkpath(xdgConfig);
        QDir().mkpath(xdgData);
        // 环境隔离：XDG 重定向到临时目录（TearDownTestSuite qunsetenv 配平）
        qputenv("XDG_CONFIG_HOME", xdgConfig.toUtf8());
        qputenv("XDG_DATA_HOME", xdgData.toUtf8());

        int argc = 1;
        char *argv[] = {s_argv, nullptr};
        s_app = new QApplication(argc, argv);
        QApplication::setOrganizationName(QStringLiteral("deepin"));
        QApplication::setApplicationName(QStringLiteral("deepin-editor"));

        // 真实 Settings 单例：读取 :/resources/settings.json，写入临时 config.conf
        Settings::instance();

        // QMetaObject::invokeMethod 按名发射 ViewMode 信号参数所需
        qRegisterMetaType<ViewMode>("ViewMode");

        // 草稿/备份目录（AppDataLocation = $XDG_DATA_HOME/deepin/deepin-editor）
        QDir().mkpath(xdgData + "/deepin/deepin-editor/blank-files");
        QDir().mkpath(xdgData + "/deepin/deepin-editor/backup-files");
    }

    static void TearDownTestSuite()
    {
        // QApplication 故意不销毁（套件顺序与进程退出安全优先）
        qunsetenv("XDG_CONFIG_HOME");
        qunsetenv("XDG_DATA_HOME");
    }

    void SetUp() override
    {
        m_tempDir = new QTemporaryDir();

        m_container = new QWidget;
        m_stackedWgt = new QStackedWidget(m_container);
        m_stackedWgt->addWidget(new QWidget(m_stackedWgt)); // currentWidget() 非空
        // 伪 Tabbar：用真实 QTabBar 内存承载（单继承链偏移 0），
        // Tabbar 自有非虚成员全部被 stub 拦截，基类原生调用安全
        m_fakeTabbar = new QTabBar(m_container);

        installCommonStubs();

        // EditWrapper 真实构造：Window* 为伪指针（真实 QWidget 内存），
        // 所有 Window/Tabbar 非虚成员已被 stub 拦截
        m_wrapper = new EditWrapper(reinterpret_cast<Window *>(m_container), m_container);

        // 真实 app 中由 Window 注入；单测无 Window，此处补注（否则
        // writeEncodeHistoryRecord 等路径空指针——源码侧缺陷另行记录）
        m_wrapper->textEditor()->setSettings(Settings::instance());
    }

    void TearDown() override
    {
        // 恢复测试内降权的文件（QTemporaryDir 析构需要可写权限）
        for (const QString &p : m_filesToRestore) {
            QFile f(p);
            if (f.exists())
                f.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        }
        // stub 保持激活状态下析构控件树（析构链中的 Window 调用仍被拦截）
        delete m_container;
        m_container = nullptr;
        m_wrapper = nullptr;
        m_fakeTabbar = nullptr; // QTabBar 父链已随 container 析构
        stub.clear();
        delete m_tempDir;
    }

    // ==================== 运行期 stub 矩阵（SetUp 每用例重装，TearDown clear） ====================
    void installCommonStubs()
    {
        // Window 查找/替换栏状态（ctor 的滚动条 lambda 与高亮链路会读取）
        stub.set_lamda(&Window::findBarIsVisiable, [](Window *) -> bool { return false; });
        stub.set_lamda(&Window::replaceBarIsVisiable, [](Window *) -> bool { return false; });
        stub.set_lamda(&Window::getKeywordForSearchAll, [](Window *) -> QString { return QString(); });
        stub.set_lamda(&Window::getKeywordForSearch, [](Window *) -> QString { return QString(); });

        // Window 状态回写（记录桩）
        stub.set_lamda(&Window::updateModifyStatus,
                       [this](Window *, const QString &path, bool modified) {
                           ++m_windowModifyCalls;
                           m_lastWindowModifyPath = path;
                           m_lastWindowModifyFlag = modified;
                       });
        stub.set_lamda(&Window::updateSaveAsFileName,
                       [this](Window *, QString oldPath, QString newPath) {
                           ++m_updateSaveAsCalls;
                           m_lastSaveAsOld = oldPath;
                           m_lastSaveAsNew = newPath;
                       });
        stub.set_lamda(&Window::saveAsFile, [this](Window *) -> bool {
            ++m_windowSaveAsCalls;
            return m_windowSaveAsResult;
        });
        stub.set_lamda(&Window::setPrintEnabled, [this](Window *, bool enabled) {
            ++m_setPrintEnabledCalls;
            m_lastPrintEnabled = enabled;
        });
        stub.set_lamda(&Window::getStackedWgt,
                       [this](Window *) -> QStackedWidget * { return m_stackedWgt; });
        stub.set_lamda(&Window::getTabbar,
                       [this](Window *) -> Tabbar * {
                           return reinterpret_cast<Tabbar *>(m_fakeTabbar);
                       });

        // Tabbar（伪指针上的非虚成员；DTabBar 为 QWidget+DObject 多继承，
        // currentIndex 若不拦截会在伪对象上解引用 DObject 私有指针 → 崩溃）
        stub.set_lamda(&Tabbar::currentName,
                       [this](Tabbar *) -> QString { return m_tabCurrentName; });
        stub.set_lamda(&Tabbar::textAt,
                       [this](Tabbar *, int) -> QString { return m_tabTextAt; });
        stub.set_lamda(&DTabBar::currentIndex, [](DTabBar *) -> int { return 0; });

        // 模态对话框：offscreen 下阻塞等待会挂起，全部拦截并返回受控结果
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
                       [this](DFileDialog *, const QString &) -> QString {
                           return m_comboValue;
                       });

        // 最近使用记录：拦截，避免写真实 recently-used
        stub.set_lamda(&DRecentManager::addItem,
                       [this](const QString &uri, DRecentData &) -> bool {
                           ++m_recentAddCalls;
                           m_lastRecentUri = uri;
                           return true;
                       });

        // 浮动消息（showNotify 两条路径 + 只读模式切换提示）
        stub.set_lamda(&Utils::sendFloatMessageFixedFont,
                       [this](QWidget *, const QIcon &, const QString &message) {
                           ++m_floatMsgCalls;
                           m_lastFloatMsg = message;
                       });
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
            [this](DMessageManager *, QWidget *, DFloatingMessage *) {
                ++m_widgetMsgCalls;
            });
    }

    // ==================== 辅助 ====================

    QString createFile(const QString &name, const QByteArray &content)
    {
        const QString path = m_tempDir->filePath(name);
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly))
            return QString();
        f.write(content);
        f.close();
        return path;
    }

    QString draftFilePath(const QString &name, const QByteArray &content = "draft")
    {
        const QString dir = s_dataHome->filePath("xdg-data/deepin/deepin-editor/blank-files");
        QDir().mkpath(dir);
        const QString path = QDir(dir).filePath(name);
        QFile f(path);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(content);
            f.close();
        }
        return path;
    }

    void makeUnwritable(const QString &path)
    {
        QFile::setPermissions(path, QFileDevice::ReadOwner);
        m_filesToRestore.append(path);
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

    // ==================== 夹具状态（每用例重建，计数器自动归零） ====================
    stub_ext::StubExt stub;
    QWidget *m_container = nullptr;
    QStackedWidget *m_stackedWgt = nullptr;
    QTabBar *m_fakeTabbar = nullptr;
    EditWrapper *m_wrapper = nullptr;
    QTemporaryDir *m_tempDir = nullptr;
    RecordingRenderer m_renderer;

    // Window 记录
    int m_windowModifyCalls = 0;
    QString m_lastWindowModifyPath;
    bool m_lastWindowModifyFlag = false;
    int m_updateSaveAsCalls = 0;
    QString m_lastSaveAsOld;
    QString m_lastSaveAsNew;
    int m_windowSaveAsCalls = 0;
    bool m_windowSaveAsResult = false;
    int m_setPrintEnabledCalls = 0;
    bool m_lastPrintEnabled = false;

    // Tabbar 受控值（reloadFileEncode/reloadModifyFile 读取标签名）
    QString m_tabCurrentName = QStringLiteral("f.txt");
    QString m_tabTextAt = QStringLiteral("f.txt");

    // 对话框受控结果（0=取消/拒绝，1/2=按钮索引）
    int m_ddialogResult = 0;
    int m_qdialogResult = 0;
    int m_ddialogExecCalls = 0;
    int m_qdialogExecCalls = 0;
    QStringList m_selectedFiles;
    QString m_comboValue = QStringLiteral("UTF-8");

    // 外部副作用记录
    int m_recentAddCalls = 0;
    QString m_lastRecentUri;
    int m_floatMsgCalls = 0;
    QString m_lastFloatMsg;
    int m_iconMsgCalls = 0;
    QString m_lastIconMsg;
    int m_widgetMsgCalls = 0;

    QStringList m_filesToRestore;

    static QApplication *s_app;
    static QTemporaryDir *s_configHome;
    static QTemporaryDir *s_dataHome;
    static char s_argv[];
};

QApplication *EditWrapperTest::s_app = nullptr;
QTemporaryDir *EditWrapperTest::s_configHome = nullptr;
QTemporaryDir *EditWrapperTest::s_dataHome = nullptr;
char EditWrapperTest::s_argv[] = "test_editwrapper";

// ============================================================================
// 构造 / 析构 / 基础状态
// ============================================================================

TEST_F(EditWrapperTest, Constructor_WithParent_CreatesCoreChildren)
{
    // Arrange: SetUp 已真实构造 EditWrapper（含真实 TextEdit/BottomBar）

    // Assert: 核心子控件就绪 + 初始状态
    ASSERT_NE(m_wrapper->textEditor(), nullptr);
    ASSERT_NE(m_wrapper->bottomBar(), nullptr);
    EXPECT_EQ(m_wrapper->bottomBar()->parentWidget(), m_wrapper); // mainLayout 挂在 wrapper 上
    EXPECT_NE(m_wrapper->textEditor()->getLeftAreaWidget(), nullptr);
    EXPECT_EQ(m_wrapper->viewMode(), ViewMode::Edit);               // 默认编辑视图
    EXPECT_FALSE(m_wrapper->isMarkdownFile());                      // 默认非 markdown
    EXPECT_TRUE(m_wrapper->filePath().isEmpty());                   // 未加载文件
    EXPECT_FALSE(m_wrapper->isQuit());                              // m_bQuit=false
    EXPECT_EQ(m_wrapper->getTextEncode(), QStringLiteral("UTF-8")); // 默认编码
}

TEST_F(EditWrapperTest, Destructor_OwnedWidgetsDeleted_QPointersNull)
{
    // Arrange: 记录子控件指针
    QPointer<EditWrapper> wrapperGuard(m_wrapper);
    QPointer<TextEdit> textGuard(m_wrapper->textEditor());
    QPointer<BottomBar> barGuard(m_wrapper->bottomBar());

    // Act: 析构整个容器树（触发 ~EditWrapper 的显式 delete 分支）
    delete m_container;
    m_container = nullptr;
    m_wrapper = nullptr;

    // Assert: 显式 delete 的两个子控件均被释放
    EXPECT_TRUE(wrapperGuard.isNull());
    EXPECT_TRUE(textGuard.isNull());
    EXPECT_TRUE(barGuard.isNull());
}

TEST_F(EditWrapperTest, QuitFlag_AfterSet_BothFlagsTrue)
{
    // Assert: 初始态（B: m_bQuit=false / m_bFileLoading=false）
    EXPECT_FALSE(m_wrapper->isQuit());
    EXPECT_FALSE(m_wrapper->getFileLoading());

    // Act
    m_wrapper->setQuitFlag();

    // Assert: 置位后 getFileLoading 按 OR 语义变真
    EXPECT_TRUE(m_wrapper->isQuit());
    EXPECT_TRUE(m_wrapper->getFileLoading());
}

TEST_F(EditWrapperTest, Accessors_AfterConstruction_ReturnCoreChildren)
{
    // Assert: window() 解析为容器顶层（QWidget::window 回溯）
    EXPECT_EQ(m_wrapper->window(), reinterpret_cast<Window *>(m_container));
    // textEditor()/bottomBar() 与构造持有的实例一致
    EXPECT_EQ(m_wrapper->textEditor()->objectName(), QStringLiteral("PTextEdit"));
    EXPECT_EQ(m_wrapper->bottomBar()->accessibleName(), QStringLiteral("EditorBottomBar"));
}

TEST_F(EditWrapperTest, SetTemFile_TrueAndFalse_RoundTrips)
{
    // Arrange: 默认 false
    EXPECT_FALSE(m_wrapper->isTemFile());

    // Act
    m_wrapper->setTemFile(true);

    // Assert
    EXPECT_TRUE(m_wrapper->isTemFile());
    m_wrapper->setTemFile(false);
    EXPECT_FALSE(m_wrapper->isTemFile());
}

TEST_F(EditWrapperTest, PlainTextEmpty_DefaultTrue_AfterInsertFalse)
{
    // Assert: 空文档
    EXPECT_TRUE(m_wrapper->isPlainTextEmpty());

    // Act
    m_wrapper->textEditor()->setPlainText(QStringLiteral("x"));

    // Assert
    EXPECT_FALSE(m_wrapper->isPlainTextEmpty());
}

// ============================================================================
// isModified（TEST_P：m_bIsTemFile × 撤销栈状态，B28）
// ============================================================================

struct ModifiedCase {
    bool temFile;
    bool injectUndoDivergence;
    bool expected;
};

class EditWrapperModifiedTest : public EditWrapperTest,
                                public ::testing::WithParamInterface<ModifiedCase> {};

TEST_P(EditWrapperModifiedTest, IsModified_ParamVariants_ReturnsExpected)
{
    const ModifiedCase c = GetParam();

    // Arrange
    if (c.temFile)
        m_wrapper->setTemFile(true);
    if (c.injectUndoDivergence) {
        // getModified = document()->isModified() && (canUndo || index != lastSaveIndex)
        // 注入 undo 栈分叉使文档修改态可观测
        m_wrapper->textEditor()->m_lastSaveIndex = 7;
        m_wrapper->textEditor()->document()->setModified(true);
    }

    // Act & Assert
    EXPECT_EQ(m_wrapper->isModified(), c.expected);
    EXPECT_EQ(m_wrapper->textEditor()->document()->isModified(), c.injectUndoDivergence);
}

INSTANTIATE_TEST_SUITE_P(
    ModifiedMatrix, EditWrapperModifiedTest,
    ::testing::Values(
        ModifiedCase{false, false, false}, // 普通文件未修改
        ModifiedCase{true, false, true},   // 备份文件恒视为已修改
        ModifiedCase{false, true, true})); // 文档撤销栈分叉视为已修改

// ============================================================================
// updatePath / 时间戳 / 草稿与备份判定
// ============================================================================

TEST_F(EditWrapperTest, UpdatePath_ValidPaths_SetsPathsAndMtime)
{
    // Arrange
    const QString path = createFile("u.txt", QByteArray("content"));
    ASSERT_FALSE(path.isEmpty());

    // Act
    m_wrapper->updatePath(path, path);

    // Assert: 路径写入 TextEdit，mtime 记录为磁盘时间
    EXPECT_EQ(m_wrapper->filePath(), path);
    EXPECT_EQ(m_wrapper->textEditor()->getTruePath(), path);
    EXPECT_EQ(m_wrapper->getLastModifiedTime(), QFileInfo(path).lastModified());
}

TEST_F(EditWrapperTest, UpdatePath_EmptyTruePath_FallsBackToFile)
{
    // Arrange
    const QString path = createFile("u2.txt", QByteArray("c"));

    // Act: 空 truePath → 回退为 file（B27）
    m_wrapper->updatePath(path, QString());

    // Assert
    EXPECT_EQ(m_wrapper->filePath(), path);
    EXPECT_EQ(m_wrapper->textEditor()->getTruePath(), path);
}

TEST_F(EditWrapperTest, LastModifiedTime_TextDateRoundTrip_PreservesTime)
{
    // Arrange: 生产契约（window.cpp）——传入 QDateTime::toString() 的 TextDate 文本。
    // 注：源码 setLastModifiedTime 使用 fromString 默认格式重载（locale 相关），
    // ISO 字符串（含 'T'）无法解析，疑似源码健壮性缺陷，已记 defects 不改源码。
    const QDateTime stamp(QDate(2020, 5, 5), QTime(12, 30, 0));

    // Act
    m_wrapper->setLastModifiedTime(stamp.toString());

    // Assert: 按 toString() 往返契约恢复同一时刻
    EXPECT_EQ(m_wrapper->getLastModifiedTime(), stamp);
    EXPECT_FALSE(m_wrapper->getLastModifiedTime().isNull());
}

TEST_F(EditWrapperTest, DraftAndBackupFile_VariousPaths_DetectCorrectly)
{
    // Arrange: 临时 XDG 数据目录下的草稿/备份文件（真实目录判定）
    const QString draft = draftFilePath("d1.txt");
    const QString backupDir = s_dataHome->filePath("xdg-data/deepin/deepin-editor/backup-files");
    QDir().mkpath(backupDir);
    const QString backup = QDir(backupDir).filePath("b1.txt");

    const QString normal = createFile("n.txt", "n");

    // Act & Assert
    m_wrapper->updatePath(draft, draft);
    EXPECT_TRUE(m_wrapper->isDraftFile());
    EXPECT_FALSE(m_wrapper->isBackupFile());

    m_wrapper->updatePath(backup, backup);
    EXPECT_TRUE(m_wrapper->isBackupFile());
    EXPECT_FALSE(m_wrapper->isDraftFile());

    m_wrapper->updatePath(normal, normal);
    EXPECT_FALSE(m_wrapper->isDraftFile());
    EXPECT_FALSE(m_wrapper->isBackupFile());
}

// ============================================================================
// updateModifyStatus / updateSaveAsFileName
// ============================================================================

TEST_F(EditWrapperTest, UpdateModifyStatus_TrueAndFalse_RecordsInWindow)
{
    // Arrange
    const QString path = createFile("m.txt", "m");
    m_wrapper->updatePath(path, path);

    // Act: 置修改（B46 true 分支）
    m_wrapper->updateModifyStatus(true);

    // Assert: 文档修改态 + Window 回写参数透传
    EXPECT_TRUE(m_wrapper->textEditor()->document()->isModified());
    EXPECT_EQ(m_windowModifyCalls, 1);
    EXPECT_EQ(m_lastWindowModifyPath, path);
    EXPECT_TRUE(m_lastWindowModifyFlag);

    // Act: 复位（false 分支，额外调用 updateSaveIndex）
    m_wrapper->updateModifyStatus(false);
    EXPECT_EQ(m_windowModifyCalls, 2);
    EXPECT_FALSE(m_lastWindowModifyFlag);
    EXPECT_FALSE(m_wrapper->textEditor()->document()->isModified());
}

TEST_F(EditWrapperTest, UpdateModifyStatus_DuringLoading_SkipsWindow)
{
    // Arrange: getFileLoading()==true（quit 置位触发早退分支 B45）
    m_wrapper->setQuitFlag();
    EXPECT_EQ(m_windowModifyCalls, 0);

    // Act
    m_wrapper->updateModifyStatus(true);

    // Assert: 早退——未回写 Window，文档修改态未被触碰
    EXPECT_EQ(m_windowModifyCalls, 0);
    EXPECT_FALSE(m_wrapper->textEditor()->document()->isModified());
}

TEST_F(EditWrapperTest, UpdateSaveAsFileName_OldAndNewPaths_DelegatesToWindow)
{
    // Act
    m_wrapper->updateSaveAsFileName(QStringLiteral("/old/a.txt"), QStringLiteral("/new/b.txt"));

    // Assert
    EXPECT_EQ(m_updateSaveAsCalls, 1);
    EXPECT_EQ(m_lastSaveAsOld, QStringLiteral("/old/a.txt"));
    EXPECT_EQ(m_lastSaveAsNew, QStringLiteral("/new/b.txt"));
}

// ============================================================================
// readFile（B1-B4）
// ============================================================================

TEST_F(EditWrapperTest, ReadFile_MissingFile_ReturnsFalse)
{
    // Arrange: 指向不存在的临时路径
    const QString missing = m_tempDir->filePath("no-such-file.txt");
    m_wrapper->updatePath(missing, missing);

    // Act
    const bool ok = m_wrapper->readFile();

    // Assert: open 失败分支；状态未损坏
    EXPECT_FALSE(ok);
    EXPECT_TRUE(m_wrapper->isPlainTextEmpty());
    EXPECT_EQ(m_wrapper->getTextEncode(), QStringLiteral("UTF-8"));
}

TEST_F(EditWrapperTest, ReadFile_AutoDetect_LoadsContentAndEncode)
{
    // Arrange
    const QString path = createFile("auto.txt", QByteArray("hello autodetect"));
    m_wrapper->updatePath(path, path);

    // Act: 缺省编码 → chardet 自动探测（B3）
    const bool ok = m_wrapper->readFile();

    // Assert: 内容入文档、编码落位、修改态复位
    EXPECT_TRUE(ok);
    EXPECT_EQ(m_wrapper->textEditor()->toPlainText(), QStringLiteral("hello autodetect"));
    EXPECT_FALSE(m_wrapper->textEditor()->document()->isModified());
    EXPECT_EQ(m_lastWindowModifyFlag, false); // updateModifyStatus(false) 已回写
    EXPECT_GT(m_windowModifyCalls, 0);
}

TEST_F(EditWrapperTest, ReadFile_SpecifiedEncode_UpdatesCurEncode)
{
    // Arrange: 纯 ASCII 内容按 GBK 读回不受损
    const QString path = createFile("gbk.txt", QByteArray("ascii only"));
    m_wrapper->updatePath(path, path);

    // Act: 指定编码（B4）
    const bool ok = m_wrapper->readFile(QByteArray("GBK"));

    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(m_wrapper->textEditor()->toPlainText(), QStringLiteral("ascii only"));
    EXPECT_EQ(m_wrapper->getTextEncode(), QStringLiteral("GBK"));
}

TEST_F(EditWrapperTest, ReadFile_InvalidEncode_ReturnsFalse)
{
    // Arrange
    const QString path = createFile("bad.txt", QByteArray("some content"));
    m_wrapper->updatePath(path, path);

    // Act: 非法编码名 → 编码转换失败（B2）
    const bool ok = m_wrapper->readFile(QByteArray("__NO_SUCH_CODEC__"));

    // Assert: 返回 false 且文档未被污染（强异常安全）
    EXPECT_FALSE(ok);
    EXPECT_TRUE(m_wrapper->isPlainTextEmpty());
}

// ============================================================================
// saveFile（B5-B8）
// ============================================================================

TEST_F(EditWrapperTest, SaveFile_ValidFile_PersistsContentAndStatus)
{
    // Arrange
    const QString path = createFile("save.txt", QByteArray(""));
    m_wrapper->updatePath(path, path);
    m_wrapper->textEditor()->setPlainText(QStringLiteral("persist me"));
    m_wrapper->setTemFile(true); // 保存成功后应复位

    // Act（B7 成功分支）
    const bool ok = m_wrapper->saveFile();

    // Assert: 落盘内容 + 状态复位 + Window 回写
    EXPECT_TRUE(ok);
    QFile f(path);
    ASSERT_TRUE(f.open(QIODevice::ReadOnly));
    EXPECT_EQ(f.readAll(), QByteArray("persist me"));
    f.close();
    EXPECT_FALSE(m_wrapper->isTemFile());
    EXPECT_FALSE(m_wrapper->isModified());
    EXPECT_GT(m_windowModifyCalls, 0);
    EXPECT_EQ(m_lastWindowModifyPath, path);
}

TEST_F(EditWrapperTest, SaveFile_WithEncode_UpdatesCurEncodeState)
{
    // Arrange
    const QString path = createFile("save2.txt", QByteArray(""));
    m_wrapper->updatePath(path, path);
    m_wrapper->textEditor()->setPlainText(QStringLiteral("abc"));

    // Act（B6: encode 非空分支）
    const bool ok = m_wrapper->saveFile(QByteArray("GBK"));

    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(m_wrapper->getTextEncode(), QStringLiteral("GBK"));
}

TEST_F(EditWrapperTest, SaveFile_UnwritableTarget_ReturnsFalse)
{
    // Arrange: 不存在的父目录
    const QString bad = m_tempDir->filePath("no-dir/x.txt");
    m_wrapper->updatePath(bad, bad);
    m_wrapper->textEditor()->setPlainText(QStringLiteral("will fail"));
    m_iconMsgCalls = 0;

    // Act（B8 失败分支：TextFileSaver 失败 + 浮动告警）
    const bool ok = m_wrapper->saveFile();

    // Assert
    EXPECT_FALSE(ok);
    EXPECT_EQ(m_iconMsgCalls, 1); // 无权限保存提示经 DMessageManager 发出
    EXPECT_TRUE(m_lastIconMsg.contains("permission"));
}

TEST_F(EditWrapperTest, SaveFile_InPreviewMode_ReturnsFalse)
{
    // Arrange: 进入 NUL 预览模式（B5 早退分支）
    const QString path = createFile("nul.txt", QByteArray("a\0b", 3));
    m_wrapper->updatePath(path, path);
    m_wrapper->handleFileLoadFinished("UTF-8", QByteArray("a\0b", 3), false, true);
    ASSERT_TRUE(m_wrapper->isInvalidCharPreview());

    // Act
    const bool ok = m_wrapper->saveFile();

    // Assert: 预览模式拒绝静默保存，状态未变
    EXPECT_FALSE(ok);
    EXPECT_TRUE(m_wrapper->isInvalidCharPreview());
}

// ============================================================================
// getPlainTextContent（TEST_P：换行格式，B9）
// ============================================================================

class EditWrapperEndlineTest : public EditWrapperTest,
                               public ::testing::WithParamInterface<EndlineCase> {};

TEST_P(EditWrapperEndlineTest, GetPlainTextContent_EndlineVariants_ReturnsExpectedBytes)
{
    const EndlineCase c = GetParam();

    // Arrange
    m_wrapper->bottomBar()->setEndlineMenuText(c.format);
    m_wrapper->textEditor()->setPlainText(QStringLiteral("a\nb"));

    // Act
    QByteArray out;
    m_wrapper->getPlainTextContent(out);

    // Assert: Unix 保持 \n；Windows 替换为 \r\n
    EXPECT_EQ(out, QByteArray(c.expected));
    EXPECT_EQ(m_wrapper->bottomBar()->getEndlineFormat(), c.format);
}

INSTANTIATE_TEST_SUITE_P(
    EndlineMatrix, EditWrapperEndlineTest,
    ::testing::Values(
        EndlineCase{BottomBar::EndlineFormat::Unix, "a\nb"},
        EndlineCase{BottomBar::EndlineFormat::Windows, "a\r\nb"}));

// ============================================================================
// saveAsFile（B10-B12）
// ============================================================================

TEST_F(EditWrapperTest, SaveAsFile_ValidPath_WritesFileAndMtime)
{
    // Arrange: 先挂旧路径（成功分支以 filePath() 记录 mtime）
    const QString oldPath = createFile("as-old.txt", QByteArray("old"));
    m_wrapper->updatePath(oldPath, oldPath);
    m_wrapper->textEditor()->setPlainText(QStringLiteral("save-as-body"));
    const QString target = m_tempDir->filePath("as-new.txt");

    // Act（B10 成功）
    const bool ok = m_wrapper->saveAsFile(target, QByteArray("UTF-8"));

    // Assert: 新文件落盘 + mtime 记录为原文件时间
    EXPECT_TRUE(ok);
    QFile f(target);
    ASSERT_TRUE(f.exists());
    ASSERT_TRUE(f.open(QIODevice::ReadOnly));
    EXPECT_EQ(f.readAll(), QByteArray("save-as-body"));
    f.close();
    EXPECT_EQ(m_wrapper->getLastModifiedTime(), QFileInfo(oldPath).lastModified());
}

TEST_F(EditWrapperTest, SaveAsFile_UnwritablePath_ReturnsFalse)
{
    // Arrange: 不存在的父目录（B11 失败 + 告警）
    m_wrapper->textEditor()->setPlainText(QStringLiteral("x"));
    m_iconMsgCalls = 0;

    // Act
    const bool ok = m_wrapper->saveAsFile(m_tempDir->filePath("no-dir/y.txt"), QByteArray("UTF-8"));

    // Assert
    EXPECT_FALSE(ok);
    EXPECT_EQ(m_iconMsgCalls, 1);
}

TEST_F(EditWrapperTest, SaveAsFileNoArg_DialogRejected_ReturnsFalse)
{
    // Arrange: DFileDialog（QDialog::exec 虚派发）返回拒绝
    m_qdialogResult = 0;

    // Act（B12 拒绝分支）
    const bool ok = m_wrapper->saveAsFile();

    // Assert
    EXPECT_FALSE(ok);
    EXPECT_EQ(m_qdialogExecCalls, 1);
}

TEST_F(EditWrapperTest, SaveAsFileNoArg_AcceptedEmptySelection_ReturnsFalse)
{
    // Arrange: 接受但未选择文件（selectedFiles 为空 → 早退）
    m_qdialogResult = 1; // QDialog::Accepted
    m_selectedFiles.clear();

    // Act
    const bool ok = m_wrapper->saveAsFile();

    // Assert
    EXPECT_FALSE(ok);
    EXPECT_EQ(m_qdialogExecCalls, 1);
}

TEST_F(EditWrapperTest, SaveAsFileNoArg_AcceptedWithSelection_SavesFile)
{
    // Arrange: 接受且选中临时路径
    m_qdialogResult = 1;
    m_selectedFiles = QStringList{m_tempDir->filePath("dlg-save.txt")};
    m_wrapper->textEditor()->setPlainText(QStringLiteral("dialog body"));

    // Act
    const bool ok = m_wrapper->saveAsFile();

    // Assert
    EXPECT_TRUE(ok);
    QFile f(m_selectedFiles.first());
    ASSERT_TRUE(f.exists());
    ASSERT_TRUE(f.open(QIODevice::ReadOnly));
    EXPECT_EQ(f.readAll(), QByteArray("dialog body"));
}

// ============================================================================
// saveTemFile（B25/B26）
// ============================================================================

TEST_F(EditWrapperTest, SaveTemFile_ValidDir_WritesBackupAndEncode)
{
    // Arrange
    const QString target = m_tempDir->filePath("backup.txt");
    m_wrapper->textEditor()->setPlainText(QStringLiteral("tem-body"));
    const QString encBefore = m_wrapper->getTextEncode();

    // Act（B25）
    const bool ok = m_wrapper->saveTemFile(target);

    // Assert
    EXPECT_TRUE(ok);
    QFile f(target);
    ASSERT_TRUE(f.exists());
    ASSERT_TRUE(f.open(QIODevice::ReadOnly));
    EXPECT_EQ(f.readAll(), QByteArray("tem-body"));
    f.close();
    EXPECT_EQ(m_wrapper->m_sFirstEncode, encBefore); // m_sFirstEncode 同步为 m_sCurEncode
}

TEST_F(EditWrapperTest, SaveTemFile_UnwritableDir_ReturnsFalse)
{
    // Arrange（B26）
    m_wrapper->textEditor()->setPlainText(QStringLiteral("x"));

    // Act
    const bool ok = m_wrapper->saveTemFile(m_tempDir->filePath("no-dir/t.txt"));

    // Assert: 失败且目标文件未产生（强异常安全）
    EXPECT_FALSE(ok);
    EXPECT_FALSE(QFile::exists(m_tempDir->filePath("no-dir/t.txt")));
}

// ============================================================================
// saveDraftFile（dialog 拒绝分支；成功路径在 reloadModify B24 一并覆盖）
// ============================================================================

TEST_F(EditWrapperTest, SaveDraftFile_DialogRejected_ReturnsFalse)
{
    // Arrange
    m_qdialogResult = 0;

    // Act
    QString newFilePath;
    const bool ok = m_wrapper->saveDraftFile(newFilePath);

    // Assert: 拒绝即失败，出参未被污染
    EXPECT_FALSE(ok);
    EXPECT_TRUE(newFilePath.isEmpty());
}

// ============================================================================
// reloadFileEncode（B13-B18）
// ============================================================================

TEST_F(EditWrapperTest, ReloadEncode_SameEncode_ReturnsFalse)
{
    // Arrange: 当前即 UTF-8
    ASSERT_EQ(m_wrapper->getTextEncode(), QStringLiteral("UTF-8"));

    // Act（B13）
    const bool ok = m_wrapper->reloadFileEncode(QByteArray("UTF-8"));

    // Assert: 编码未变、不重载
    EXPECT_FALSE(ok);
    EXPECT_EQ(m_wrapper->getTextEncode(), QStringLiteral("UTF-8"));
}

TEST_F(EditWrapperTest, ReloadEncode_DraftEmpty_ReturnsTrue)
{
    // Arrange: 草稿文件 + 空内容（B14）
    const QString draft = draftFilePath("d2.txt", "");
    m_wrapper->updatePath(draft, draft);
    ASSERT_TRUE(m_wrapper->textEditor()->toPlainText().isEmpty());

    // Act
    const bool ok = m_wrapper->reloadFileEncode(QByteArray("GBK"));

    // Assert: 直接切换编码并同步首编码
    EXPECT_TRUE(ok);
    EXPECT_EQ(m_wrapper->getTextEncode(), QStringLiteral("GBK"));
    EXPECT_EQ(m_wrapper->m_sFirstEncode, QStringLiteral("GBK"));
}

TEST_F(EditWrapperTest, ReloadEncode_NotModified_ReadsFile)
{
    // Arrange: 未修改的普通文件（B18 else 分支 → readFile）
    const QString path = createFile("plain.txt", QByteArray("reload-body"));
    m_wrapper->updatePath(path, path);
    m_tabCurrentName = QStringLiteral("plain.txt"); // 无 * 前缀

    // Act
    const bool ok = m_wrapper->reloadFileEncode(QByteArray("GBK"));

    // Assert: 文件内容重新读入
    EXPECT_TRUE(ok);
    EXPECT_EQ(m_wrapper->textEditor()->toPlainText(), QStringLiteral("reload-body"));
}

TEST_F(EditWrapperTest, ReloadEncode_ModifiedDialogCancel_ReturnsFalse)
{
    // Arrange: 修改态（标签 * 前缀）+ 对话框取消（B15 res==0）
    const QString path = createFile("mod.txt", QByteArray("mod-body"));
    m_wrapper->updatePath(path, path);
    m_tabCurrentName = QStringLiteral("*mod.txt"); // hasFlag=true
    m_ddialogResult = 0;
    const int dialogsBefore = m_ddialogExecCalls;

    // Act
    const bool ok = m_wrapper->reloadFileEncode(QByteArray("GBK"));

    // Assert: 取消 → 不重载、编码不变（强异常安全）
    EXPECT_FALSE(ok);
    EXPECT_EQ(m_ddialogExecCalls, dialogsBefore + 1);
    EXPECT_EQ(m_wrapper->getTextEncode(), QStringLiteral("UTF-8"));
    EXPECT_TRUE(m_wrapper->isPlainTextEmpty()); // 未触发 readFile
}

TEST_F(EditWrapperTest, ReloadEncode_ModifiedDraftSaveFail_RollsBackEncode)
{
    // Arrange: 草稿 + 有内容（避开 B14 空/草稿早退）+ 标签 * 前缀（hasFlag）+
    // 对话框选保存 + 草稿另存对话框被拒绝（B16 失败回滚）
    const QString draft = draftFilePath("d3.txt", "draft-body");
    m_wrapper->updatePath(draft, draft);
    m_wrapper->textEditor()->setPlainText(QStringLiteral("draft-body"));
    m_tabCurrentName = QStringLiteral("*d3.txt");
    m_ddialogResult = 1; // 保存
    m_qdialogResult = 0; // 草稿另存对话框拒绝 → saveDraftFile 失败

    // Act
    const bool ok = m_wrapper->reloadFileEncode(QByteArray("GBK"));

    // Assert: 保存失败 → reloadSucc=false → 编码回滚为 tempEncode
    EXPECT_FALSE(ok);
    EXPECT_EQ(m_wrapper->getTextEncode(), QStringLiteral("UTF-8"));
}

TEST_F(EditWrapperTest, ReloadEncode_ModifiedSaveAsThenRead_ReturnsTrue)
{
    // Arrange: 普通文件 + 修改态 + 窗口另存成功（B17）
    const QString path = createFile("sv.txt", QByteArray("sv-body"));
    m_wrapper->updatePath(path, path);
    m_tabCurrentName = QStringLiteral("*sv.txt"); // hasFlag → 对话框分支
    m_ddialogResult = 1;                          // 保存
    m_windowSaveAsResult = true;                  // Window::saveAsFile 成功
    m_windowSaveAsCalls = 0;

    // Act
    const bool ok = m_wrapper->reloadFileEncode(QByteArray("GBK"));

    // Assert: 另存 + 重读成功
    EXPECT_TRUE(ok);
    EXPECT_EQ(m_windowSaveAsCalls, 1);
    EXPECT_EQ(m_wrapper->textEditor()->toPlainText(), QStringLiteral("sv-body"));
    EXPECT_EQ(m_wrapper->getTextEncode(), QStringLiteral("GBK"));
}

// ============================================================================
// reloadFileHighlight（B19/B20）
// ============================================================================

TEST_F(EditWrapperTest, ReloadHighlight_ValidDef_CreatesHighlighter)
{
    // Arrange: 注入渲染器（"Markdown" 识别成功后会触发视图切换链路）
    m_renderer.ready = true;
    m_wrapper->setMarkdownRendererForTest(&m_renderer);

    // Act（B19: 有效 definition → 创建/配置高亮器）
    m_wrapper->reloadFileHighlight(QStringLiteral("Markdown"));

    // Assert: 高亮器创建 + markdown 识别 + 视图跃迁
    EXPECT_NE(m_wrapper->getSyntaxHighlighter(), nullptr);
    EXPECT_TRUE(m_wrapper->isMarkdownFile());
    EXPECT_EQ(m_wrapper->viewMode(), ViewMode::LivePreview); // md 默认实时预览
}

TEST_F(EditWrapperTest, ReloadHighlight_InvalidDef_RemovesHighlighter)
{
    // Arrange: 先建高亮器再移除
    m_renderer.ready = false;
    m_wrapper->setMarkdownRendererForTest(&m_renderer);
    m_wrapper->reloadFileHighlight(QStringLiteral("Markdown"));
    ASSERT_NE(m_wrapper->getSyntaxHighlighter(), nullptr);

    // Act（B20: 无效 definition → 移除高亮）
    m_wrapper->reloadFileHighlight(QStringLiteral("NoSuchLang"));

    // Assert
    EXPECT_EQ(m_wrapper->getSyntaxHighlighter(), nullptr);
    EXPECT_FALSE(m_wrapper->isMarkdownFile());
}

// ============================================================================
// reloadModifyFile（B21-B24）
// ============================================================================

TEST_F(EditWrapperTest, ReloadModify_NotModified_ReloadsDiskContent)
{
    // Arrange: 缓冲与磁盘分叉（未修改分支 B21）
    const QString path = createFile("rm1.txt", QByteArray("v1"));
    m_wrapper->updatePath(path, path);
    m_tabCurrentName = QStringLiteral("rm1.txt"); // 无 * 前缀
    m_tabTextAt = QStringLiteral("rm1.txt");

    // 磁盘更新为 v2（mtime 变化不影响 readFile）
    QFile f(path);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.write("v2");
    f.close();

    // Act
    m_wrapper->reloadModifyFile();

    // Assert: 未修改 → 直接重读磁盘；读入后修改态复位并回写 Window
    EXPECT_EQ(m_wrapper->textEditor()->toPlainText(), QStringLiteral("v2"));
    EXPECT_GT(m_windowModifyCalls, 0);
    EXPECT_FALSE(m_lastWindowModifyFlag);
}

TEST_F(EditWrapperTest, ReloadModify_ModifiedCancel_KeepsBuffer)
{
    // Arrange: 修改态 + 对话框关闭（B22 res==0）
    const QString path = createFile("rm2.txt", QByteArray("disk"));
    m_wrapper->updatePath(path, path);
    m_wrapper->textEditor()->setPlainText(QStringLiteral("buffer"));
    m_tabCurrentName = QStringLiteral("*rm2.txt");
    m_tabTextAt = QStringLiteral("*rm2.txt");
    m_ddialogResult = 0;
    const int dialogsBefore = m_ddialogExecCalls;

    // Act
    m_wrapper->reloadModifyFile();

    // Assert: 关闭 → 缓冲保留，且确认弹窗只咨询了一次
    EXPECT_EQ(m_wrapper->textEditor()->toPlainText(), QStringLiteral("buffer"));
    EXPECT_EQ(m_ddialogExecCalls, dialogsBefore + 1);
}

TEST_F(EditWrapperTest, ReloadModify_ModifiedDiscard_ReloadsAndClearsTem)
{
    // Arrange: 修改态 + 丢弃（B23 res==1 → readFile + 复位 m_bIsTemFile）
    const QString path = createFile("rm3.txt", QByteArray("fresh"));
    m_wrapper->updatePath(path, path);
    m_wrapper->textEditor()->setPlainText(QStringLiteral("stale"));
    m_tabCurrentName = QStringLiteral("*rm3.txt");
    m_tabTextAt = QStringLiteral("*rm3.txt");
    m_ddialogResult = 1;
    m_wrapper->setTemFile(true);
    ASSERT_TRUE(m_wrapper->isTemFile());

    // Act
    m_wrapper->reloadModifyFile();

    // Assert
    EXPECT_EQ(m_wrapper->textEditor()->toPlainText(), QStringLiteral("fresh"));
    EXPECT_FALSE(m_wrapper->isTemFile());
}

TEST_F(EditWrapperTest, ReloadModify_ModifiedSaveDraftFail_KeepsBuffer)
{
    // Arrange: 草稿 + 修改态 + 另存（B24 res==2）+ 草稿保存对话框拒绝
    const QString draft = draftFilePath("d4.txt", "draft-disk");
    m_wrapper->updatePath(draft, draft);
    m_wrapper->textEditor()->setPlainText(QStringLiteral("draft-buffer"));
    m_tabCurrentName = QStringLiteral("*d4.txt");
    m_tabTextAt = QStringLiteral("*d4.txt");
    m_ddialogResult = 2;
    m_qdialogResult = 0; // saveDraftFile 拒绝 → 早退
    const int fileDialogsBefore = m_qdialogExecCalls;

    // Act
    m_wrapper->reloadModifyFile();

    // Assert: 保存失败早退，缓冲未被磁盘内容覆盖，草稿另存对话框被咨询
    EXPECT_EQ(m_wrapper->textEditor()->toPlainText(), QStringLiteral("draft-buffer"));
    EXPECT_EQ(m_qdialogExecCalls, fileDialogsBefore + 1);
}

// ============================================================================
// hideWarningNotices / showNotify / checkForReload
// ============================================================================

TEST_F(EditWrapperTest, HideWarningNotices_WhenVisible_HidesNotices)
{
    // Arrange: 显示容器使 isVisible 语义生效
    m_container->show();
    QApplication::processEvents();
    m_wrapper->m_pWaringNotices->show();
    QApplication::processEvents();
    EXPECT_TRUE(m_wrapper->m_pWaringNotices->isVisible());

    // Act
    m_wrapper->hideWarningNotices();

    // Assert
    EXPECT_FALSE(m_wrapper->m_pWaringNotices->isVisible());
}

TEST_F(EditWrapperTest, ShowNotify_EmptyMessage_SendsNothing)
{
    // Act（B33 早退）
    m_wrapper->showNotify(QString());

    // Assert: 不发浮动消息
    EXPECT_EQ(m_floatMsgCalls, 0);
    EXPECT_EQ(m_iconMsgCalls, 0);
}

TEST_F(EditWrapperTest, ShowNotify_WarningFlag_SendsWarningFloat)
{
    // Act（B34: warning=true 走告警路径）
    m_wrapper->showNotify(QStringLiteral("something went wrong"), true);

    // Assert
    EXPECT_EQ(m_floatMsgCalls, 1);
    EXPECT_EQ(m_lastFloatMsg, QStringLiteral("something went wrong"));
}

TEST_F(EditWrapperTest, ShowNotify_NormalMessage_SendsOkFloat)
{
    // Act（B35: 非告警且无只读限制 → 正常路径）
    m_wrapper->showNotify(QStringLiteral("all good"), false);

    // Assert
    EXPECT_EQ(m_floatMsgCalls, 1);
    EXPECT_EQ(m_lastFloatMsg, QStringLiteral("all good"));
}

TEST_F(EditWrapperTest, ShowNotify_ReadOnlyPermission_SendsWarningFloat)
{
    // Arrange: 文件只读权限 → 即使非告警也走告警分支（B34 的 || 右支）
    m_wrapper->textEditor()->setReadOnlyPermission(true);

    // Act
    m_wrapper->showNotify(QStringLiteral("readonly file"), false);

    // Assert
    EXPECT_EQ(m_floatMsgCalls, 1);
    EXPECT_EQ(m_lastFloatMsg, QStringLiteral("readonly file"));
}

TEST_F(EditWrapperTest, CheckForReload_DraftFile_SkipsCheck)
{
    // Arrange: 草稿文件早退（B29），不会进入 50ms 定时检查
    const QString draft = draftFilePath("d5.txt", "d");
    m_wrapper->updatePath(draft, draft);
    m_widgetMsgCalls = 0;

    // Act
    m_wrapper->checkForReload();
    EXPECT_TRUE(waitUntil([] { return true; }, 300)); // 越过 singleShot(50ms)

    // Assert: 无任何通知
    EXPECT_EQ(m_widgetMsgCalls, 0);
    EXPECT_TRUE(m_wrapper->m_pWaringNotices->isHidden());
}

TEST_F(EditWrapperTest, CheckForReload_UnchangedFile_NoNotice)
{
    // Arrange: mtime 与磁盘一致（B30）
    const QString path = createFile("ck1.txt", "c");
    m_wrapper->updatePath(path, path); // m_tModifiedDateTime = 磁盘 mtime
    m_widgetMsgCalls = 0;

    // Act: 越过 singleShot(50ms) 定时检查
    m_wrapper->checkForReload();
    QThread::msleep(200);
    QApplication::processEvents();

    // Assert: 无任何通知
    EXPECT_EQ(m_widgetMsgCalls, 0);
    EXPECT_TRUE(m_wrapper->m_pWaringNotices->isHidden());
}

TEST_F(EditWrapperTest, CheckForReload_FileRemoved_ShowsSaveAsNotice)
{
    // Arrange: 文件被删除（B31）
    const QString path = createFile("ck2.txt", "c");
    m_wrapper->updatePath(path, path);
    QFile::remove(path);
    m_widgetMsgCalls = 0;

    // Act
    m_wrapper->checkForReload();

    // Assert: 提示"文件被删除，立即保存？"
    EXPECT_TRUE(waitUntil([this] { return m_widgetMsgCalls > 0; }, 2000));
    EXPECT_EQ(m_widgetMsgCalls, 1);
    EXPECT_FALSE(m_wrapper->m_pWaringNotices->isHidden());
}

TEST_F(EditWrapperTest, CheckForReload_FileChanged_ShowsReloadNotice)
{
    // Arrange: mtime 分叉（B32）——手工把记录时间拨到过去（TextDate 契约，见 LastModifiedTime）
    const QString path = createFile("ck3.txt", "c");
    m_wrapper->updatePath(path, path);
    m_wrapper->setLastModifiedTime(
        QDateTime(QDate(2001, 1, 1), QTime(0, 0, 0)).toString());
    m_widgetMsgCalls = 0;

    // Act
    m_wrapper->checkForReload();

    // Assert: 提示"文件已在磁盘上变更，重新加载？"
    EXPECT_TRUE(waitUntil([this] { return m_widgetMsgCalls > 0; }, 2000));
    EXPECT_EQ(m_widgetMsgCalls, 1);
    EXPECT_FALSE(m_wrapper->m_pWaringNotices->isHidden());
}

// ============================================================================
// 行号 / 空白符 / 编码清理信号
// ============================================================================

TEST_F(EditWrapperTest, SetLineNumberShow_ShowAndHide_TogglesAreaAndFlag)
{
    auto *area = m_wrapper->textEditor()->getLeftAreaWidget()->m_pLineNumberArea;
    ASSERT_NE(area, nullptr);

    // Act 1: 隐藏（B37 !bIsShow 分支）
    m_wrapper->setLineNumberShow(false, false);
    // Assert 1
    EXPECT_TRUE(area->isHidden());
    EXPECT_FALSE(m_wrapper->textEditor()->bIsSetLineNumberWidth);

    // Act 2: 显示（B36 bIsShow && !bIsFirstShow 分支）
    m_wrapper->setLineNumberShow(true, false);
    // Assert 2
    EXPECT_FALSE(area->isHidden());
    EXPECT_TRUE(m_wrapper->textEditor()->bIsSetLineNumberWidth);

    // Act 3: 首次显示（bIsFirstShow=true → 不触发 show/hide，仅置位）
    m_wrapper->setLineNumberShow(true, true);
    EXPECT_TRUE(m_wrapper->textEditor()->bIsSetLineNumberWidth);
}

TEST_F(EditWrapperTest, SetShowBlankCharacter_TrueAndFalse_TogglesTextOptionFlags)
{
    // Act 1（B38 置位分支）
    m_wrapper->setShowBlankCharacter(true);
    // Assert 1
    EXPECT_TRUE(m_wrapper->textEditor()->document()->defaultTextOption().flags()
                & QTextOption::ShowTabsAndSpaces);

    // Act 2（B39 清除分支）
    m_wrapper->setShowBlankCharacter(false);
    // Assert 2
    EXPECT_FALSE(m_wrapper->textEditor()->document()->defaultTextOption().flags()
                 & QTextOption::ShowTabsAndSpaces);
}

class EditWrapperClearDoubleTest : public EditWrapperTest,
                                   public ::testing::WithParamInterface<ClearDoubleCase> {};

TEST_P(EditWrapperClearDoubleTest, ClearDoubleCharaterEncode_ParamVariants_EmitsAsExpected)
{
    const ClearDoubleCase c = GetParam();

    // Arrange: 真实临时文件（baseName + 大小决定分支）
    const QString path = createFile(c.fileName, QByteArray(c.fileSize, 'a'));
    ASSERT_FALSE(path.isEmpty());
    m_wrapper->updatePath(path, path);

    QSignalSpy spy(m_wrapper, &EditWrapper::sigClearDoubleCharaterEncode);

    // Act
    m_wrapper->clearDoubleCharaterEncode();

    // Assert（B40: 关键字+≤500KB 发信号；B41: >500KB 不发；B42: 无关键字不发）
    EXPECT_EQ(spy.count(), c.expectEmit ? 1 : 0);
    if (c.expectEmit)
        EXPECT_FALSE(m_wrapper->filePath().isEmpty());
}

INSTANTIATE_TEST_SUITE_P(
    ClearDoubleMatrix, EditWrapperClearDoubleTest,
    ::testing::Values(
        ClearDoubleCase{QStringLiteral("double_a.txt"), 100, true},
        ClearDoubleCase{QStringLiteral("four.txt"), 100, true},
        ClearDoubleCase{QStringLiteral("user.txt"), 100, true},
        ClearDoubleCase{QStringLiteral("normal.txt"), 100, false},
        ClearDoubleCase{QStringLiteral("double_big.txt"), 500 * 1024 + 1, false}));

// ============================================================================
// 底部栏 / 高亮刷新
// ============================================================================

TEST_F(EditWrapperTest, UpdateBottomBarWordCnt_GivenCount_RefreshesLabel)
{
    // Act
    m_wrapper->UpdateBottomBarWordCnt(42);

    // Assert: 标签更新为 count-1（源码语义）
    const QString label = m_wrapper->bottomBar()->m_pCharCountLabel->text();
    EXPECT_TRUE(label.contains(QStringLiteral("41")));
    EXPECT_FALSE(label.contains(QStringLiteral("42")));
}

TEST_F(EditWrapperTest, OnUpdateHighlighter_VariousStates_NoUndueStateChange)
{
    // Arrange 1: 无高亮器 → 空跑（B: m_pSyntaxHighlighter null）
    m_wrapper->OnUpdateHighlighter();
    EXPECT_EQ(m_wrapper->getSyntaxHighlighter(), nullptr);
    EXPECT_FALSE(m_wrapper->m_bHighlighterAll);

    // Arrange 2: 有高亮器 + quit → 跳过（B: !m_bQuit 短路）
    m_renderer.ready = false;
    m_wrapper->setMarkdownRendererForTest(&m_renderer);
    m_wrapper->reloadFileHighlight(QStringLiteral("Markdown"));
    m_wrapper->setQuitFlag();
    m_wrapper->OnUpdateHighlighter();
    EXPECT_FALSE(m_wrapper->m_bHighlighterAll); // quit 分支未置位
}

TEST_F(EditWrapperTest, UpdateHighlighterAll_WithHighlighter_MarksAllDone)
{
    // Arrange
    m_renderer.ready = false;
    m_wrapper->setMarkdownRendererForTest(&m_renderer);
    m_wrapper->reloadFileHighlight(QStringLiteral("Markdown"));
    m_wrapper->textEditor()->setPlainText(QStringLiteral("line1\nline2\nline3"));
    ASSERT_FALSE(m_wrapper->m_bHighlighterAll);

    // Act（B43: 全量重刷）
    m_wrapper->updateHighlighterAll();

    // Assert: 全量标志置位且块结构未被破坏（3 行文本 = 3 块）
    EXPECT_TRUE(m_wrapper->m_bHighlighterAll);
    EXPECT_EQ(m_wrapper->textEditor()->document()->blockCount(), 3);
}

TEST_F(EditWrapperTest, UpdateHighlighterAll_WhenQuit_Skips)
{
    // Arrange（B44: m_bQuit → 跳过）
    m_renderer.ready = false;
    m_wrapper->setMarkdownRendererForTest(&m_renderer);
    m_wrapper->reloadFileHighlight(QStringLiteral("Markdown"));
    m_wrapper->setQuitFlag();

    // Act
    m_wrapper->updateHighlighterAll();

    // Assert: quit 分支跳过重刷，空文档块结构保持初始 1 块
    EXPECT_FALSE(m_wrapper->m_bHighlighterAll);
    EXPECT_EQ(m_wrapper->textEditor()->document()->blockCount(), 1);
}

// ============================================================================
// 主题切换（TEST_P：亮/暗，B63/B64）
// ============================================================================

QString writeThemeFile(const QString &backgroundColor, const QString &textColor)
{
    static QTemporaryDir themeDir;
    const QString path = themeDir.filePath("t.theme");
    QFile f(path);
    f.open(QIODevice::WriteOnly | QIODevice::Truncate);
    f.write(QString("{\"editor-colors\": {\"background-color\": \"%1\"},"
                    " \"Normal\": {\"text-color\": \"%2\"}}")
                .arg(backgroundColor, textColor)
                .toUtf8());
    f.close();
    return path;
}

class EditWrapperThemeTest : public EditWrapperTest,
                             public ::testing::WithParamInterface<ThemeCase> {};

TEST_P(EditWrapperThemeTest, ThemeChange_LightAndDark_AppliesPaletteAndTheme)
{
    const ThemeCase c = GetParam();

    // Arrange: 渲染器注入（applyTheme 可观测）+ 主题文件
    m_renderer.ready = false;
    m_wrapper->setMarkdownRendererForTest(&m_renderer);
    const int applyBefore = m_renderer.applyThemeCalls;
    const QString themePath = writeThemeFile(c.backgroundColor, c.textColor);

    // Act
    m_wrapper->OnThemeChangeSlot(themePath);

    // Assert: 底栏调色板 + 渲染器主题注入（B63/B64 两分支按背景亮度选择深浅主题）
    EXPECT_EQ(m_wrapper->bottomBar()->palette().color(QPalette::Window).name(),
              QColor(c.backgroundColor).name());
    EXPECT_EQ(m_wrapper->bottomBar()->palette().color(QPalette::Text).name(),
              QColor(c.textColor).name());
    EXPECT_EQ(m_renderer.applyThemeCalls, applyBefore + 1);
}

INSTANTIATE_TEST_SUITE_P(
    ThemeMatrix, EditWrapperThemeTest,
    ::testing::Values(
        ThemeCase{QStringLiteral("#2b2b2b"), QStringLiteral("#ffffff")}, // 暗色 → DarkTheme
        ThemeCase{QStringLiteral("#ffffff"), QStringLiteral("#000000")})); // 亮色 → LightTheme

TEST_F(EditWrapperTest, ThemeChange_WithoutHighlighter_StillAppliesPalette)
{
    // Arrange: 无高亮器 + 无渲染器（B: m_pSyntaxHighlighter null 分支）
    ASSERT_EQ(m_wrapper->getSyntaxHighlighter(), nullptr);
    const QString themePath = writeThemeFile(QStringLiteral("#101010"), QStringLiteral("#eeeeee"));

    // Act
    m_wrapper->OnThemeChangeSlot(themePath);

    // Assert: 调色板仍生效（背景 + 前景），无崩溃
    EXPECT_EQ(m_wrapper->bottomBar()->palette().color(QPalette::Window).name(),
              QStringLiteral("#101010"));
    EXPECT_EQ(m_wrapper->bottomBar()->palette().color(QPalette::Text).name(),
              QStringLiteral("#eeeeee"));
}

// ============================================================================
// 文件加载管线：handleFilePreProcess / handleFileLoadFinished（B56-B62）
// ============================================================================

TEST_F(EditWrapperTest, HandleLoadFinished_HappyPath_LoadsContentAndEncode)
{
    // Arrange（B57: 无预处理 → reinitOnFileLoad）
    const QString path = createFile("happy.txt", QByteArray("ignored-disk"));
    m_wrapper->updatePath(path, path);
    m_recentAddCalls = 0;

    // Act
    m_wrapper->handleFileLoadFinished("UTF-8", QByteArray("happy-body"), false, false);

    // Assert: 内容 + 编码 + 最近使用记录（非草稿）
    EXPECT_EQ(m_wrapper->textEditor()->toPlainText(), QStringLiteral("happy-body"));
    EXPECT_EQ(m_wrapper->getTextEncode(), QStringLiteral("UTF-8"));
    EXPECT_EQ(m_wrapper->m_sFirstEncode, QStringLiteral("UTF-8"));
    EXPECT_EQ(m_recentAddCalls, 1);
    EXPECT_FALSE(m_wrapper->isInvalidCharPreview());
}

TEST_F(EditWrapperTest, HandleLoadFinished_ErrorExistingFile_ShowsNotices)
{
    // Arrange（B58: error=true 且文件存在 → onReadAllocError）
    const QString path = createFile("err.txt", QByteArray("x"));
    m_wrapper->updatePath(path, path);
    m_widgetMsgCalls = 0;

    // Act
    m_wrapper->handleFileLoadFinished("UTF-8", QByteArray(), true, false);

    // Assert: 只读模式 + 告警通知 + 文档清空
    EXPECT_TRUE(m_wrapper->textEditor()->getReadOnlyMode());
    EXPECT_EQ(m_widgetMsgCalls, 1);
    EXPECT_FALSE(m_wrapper->m_pWaringNotices->isHidden());
    EXPECT_TRUE(m_wrapper->isPlainTextEmpty());
}

TEST_F(EditWrapperTest, HandleLoadFinished_ErrorMissingFile_Silent)
{
    // Arrange（B59: error=true 且文件不存在 → 静默处理（新建文件场景））
    const QString path = m_tempDir->filePath("brand-new.txt");
    m_wrapper->updatePath(path, path);
    m_widgetMsgCalls = 0;

    // Act
    m_wrapper->handleFileLoadFinished("UTF-8", QByteArray(), true, false);

    // Assert: 不弹通知、不进只读
    EXPECT_EQ(m_widgetMsgCalls, 0);
    EXPECT_TRUE(m_wrapper->m_pWaringNotices->isHidden());
    EXPECT_FALSE(m_wrapper->textEditor()->getReadOnlyMode());
}

TEST_F(EditWrapperTest, HandleLoadFinished_HasNul_EntersPreview)
{
    // Arrange（B60: hasNul=true → 无效字符预览模式）
    const QString path = createFile("nul2.txt", QByteArray("a\0b", 3));
    m_wrapper->updatePath(path, path);

    // Act
    m_wrapper->handleFileLoadFinished("UTF-8", QByteArray("a\0b", 3), false, true);

    // Assert: 预览态 + 只读 + 原路径记录 + 通知展示
    EXPECT_TRUE(m_wrapper->isInvalidCharPreview());
    EXPECT_FALSE(m_wrapper->isInvalidCharEditAllowed());
    EXPECT_EQ(m_wrapper->invalidCharOriginalPath(), path);
    EXPECT_TRUE(m_wrapper->textEditor()->isReadOnly());
    EXPECT_FALSE(m_wrapper->m_pWaringNotices->isHidden());
    EXPECT_EQ(m_widgetMsgCalls, 1);
    EXPECT_NE(m_wrapper->getSyntaxHighlighter(), nullptr); // 预览高亮器已创建
}

TEST_F(EditWrapperTest, HandleLoadFinished_CursorPreset_RestoresOnce)
{
    // Arrange（B61: 预设恢复光标）
    const QString path = createFile("cur.txt", QByteArray("x"));
    m_wrapper->updatePath(path, path);
    m_wrapper->setRestoreCursorPosition(5);

    // Act
    m_wrapper->handleFileLoadFinished("UTF-8", QByteArray("0123456789"), false, false);

    // Assert: 光标恢复且预设值一次性消费
    EXPECT_EQ(m_wrapper->textEditor()->textCursor().position(), 5);
    EXPECT_EQ(m_wrapper->m_nRestoreCursorPosition, -1);
}

TEST_F(EditWrapperTest, HandleLoadFinished_HistoryJsonMatch_RestoresCursor)
{
    // Arrange（B62: 浏览历史 JSON 恢复光标）
    const QString path = createFile("hist.txt", QByteArray("x"));
    m_wrapper->updatePath(path, path);

    QJsonObject obj;
    obj.insert("localPath", path);
    obj.insert("cursorPosition", QStringLiteral("7"));
    const QStringList history{QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact))};
    Settings::instance()->settings->option("advance.editor.browsing_history_temfile")
        ->setValue(history);

    // Act
    m_wrapper->handleFileLoadFinished("UTF-8", QByteArray("0123456789"), false, false);

    // Assert: 历史命中 → 光标置 7，且预设恢复值未被占用（保持 -1）
    EXPECT_EQ(m_wrapper->textEditor()->textCursor().position(), 7);
    EXPECT_EQ(m_wrapper->m_nRestoreCursorPosition, -1);
    EXPECT_EQ(m_wrapper->textEditor()->toPlainText(), QStringLiteral("0123456789"));

    // Cleanup: 复位共享单例设置，避免用例间污染
    Settings::instance()->settings->option("advance.editor.browsing_history_temfile")
        ->setValue(QStringList());
}

TEST_F(EditWrapperTest, FilePreProcess_GivenEncodeAndContent_InsertsAndFlags)
{
    // Arrange（B56）
    const QString path = createFile("pre.txt", QByteArray("x"));
    m_wrapper->updatePath(path, path);
    EXPECT_FALSE(m_wrapper->m_bHasPreProcess);

    // Act
    m_wrapper->handleFilePreProcess("GBK", QByteArray("pre-view"));

    // Assert: 内容先插入 + 预处理标识置位
    EXPECT_EQ(m_wrapper->textEditor()->toPlainText(), QStringLiteral("pre-view"));
    EXPECT_TRUE(m_wrapper->m_bHasPreProcess);

    // Act 2: 预处理后再 finish → 跳过 reinitOnFileLoad（编码保持预处理值）
    m_wrapper->handleFileLoadFinished("UTF-8", QByteArray("pre-view"), false, false);
    EXPECT_EQ(m_wrapper->getTextEncode(), QStringLiteral("GBK"));
}

TEST_F(EditWrapperTest, HandleLoadFinished_TemFileFlag_MarksModified)
{
    // Arrange
    const QString path = createFile("tem-load.txt", QByteArray("x"));
    m_wrapper->updatePath(path, path);
    m_wrapper->setTemFile(true);
    m_windowModifyCalls = 0;

    // Act
    m_wrapper->handleFileLoadFinished("UTF-8", QByteArray("body"), false, false);

    // Assert: 备份文件加载完成后回写修改态（m_bFileLoading 复位后的那次生效）
    EXPECT_TRUE(m_lastWindowModifyFlag);
    EXPECT_GT(m_windowModifyCalls, 0);
}

// ============================================================================
// openFile（端到端：真实 FileLoadThread 异步读取临时文件）
// ============================================================================

TEST_F(EditWrapperTest, OpenFile_RealTempFile_LoadsContentEndToEnd)
{
    // Arrange
    const QString path = createFile("open.txt", QByteArray("open-content"));
    ASSERT_FALSE(path.isEmpty());

    // Act: 异步线程加载，轮询事件循环直至内容就位
    m_wrapper->openFile(path, path, false);
    ASSERT_TRUE(waitUntil([this] { return !m_wrapper->isPlainTextEmpty(); }));

    // Assert: 路径 + 内容 + 非备份态
    EXPECT_EQ(m_wrapper->filePath(), path);
    EXPECT_EQ(m_wrapper->textEditor()->toPlainText(), QStringLiteral("open-content"));
    EXPECT_FALSE(m_wrapper->isTemFile());
    EXPECT_FALSE(m_wrapper->getFileLoading());
}

TEST_F(EditWrapperTest, OpenFile_TemFileFlag_KeepsTemState)
{
    // Arrange
    const QString path = createFile("open-tem.txt", QByteArray("tem-content"));

    // Act
    m_wrapper->openFile(path, path, true);
    ASSERT_TRUE(waitUntil([this] { return !m_wrapper->isPlainTextEmpty(); }));

    // Assert: 备份标志透传，加载完成后回写修改态 true
    EXPECT_TRUE(m_wrapper->isTemFile());
    EXPECT_TRUE(waitUntil([this] { return m_windowModifyCalls > 0; }));
    EXPECT_TRUE(m_lastWindowModifyFlag);
}

// ============================================================================
// customEvent（B55: >40MB 内容走 ParseFileEvent 事件分片队列）
// ============================================================================

TEST_F(EditWrapperTest, CustomEvent_LargeContent_ParsesInSteps)
{
    // Arrange: 40MB+1（> 40MB 阈值）→ loadContent 投递 ParseFileEvent，
    // customEvent 在事件循环中分片插入（每片 1MB）。
    // 内容按行组织（避免单巨型文本块的二次级布局开销）
    const QString path = createFile("big.txt", QByteArray("x"));
    m_wrapper->updatePath(path, path);
    const int size = 40 * 1024 * 1024 + 1;
    QByteArray line = QByteArray(63, 'a') + '\n';
    QByteArray big;
    big.reserve(size);
    while (big.size() + line.size() <= size)
        big.append(line);
    big.append(QByteArray(size - big.size(), 'a')); // 补足至精确大小

    // Act
    m_wrapper->handleFileLoadFinished("UTF-8", big, false, false);

    // Assert: 全量内容经事件队列分片进入文档
    EXPECT_EQ(m_wrapper->textEditor()->toPlainText().size(), size);
    EXPECT_FALSE(m_wrapper->getFileLoading());
}

// ============================================================================
// 无效字符预览模式：onEditAnyway / exit / forceSave（B47/B48）
// ============================================================================

static void enterPreview(EditWrapper *wrapper, QTemporaryDir *dir, const QByteArray &content)
{
    const QString path = QDir(dir->path()).filePath("pv.txt");
    QFile f(path);
    f.open(QIODevice::WriteOnly);
    f.write(content);
    f.close();
    wrapper->updatePath(path, path);
    wrapper->handleFileLoadFinished("UTF-8", content, false, true);
}

TEST_F(EditWrapperTest, OnEditAnyway_InPreviewMode_EnablesEditing)
{
    // Arrange: 进入预览
    enterPreview(m_wrapper, m_tempDir, QByteArray("a\0b", 3));
    ASSERT_TRUE(m_wrapper->isInvalidCharPreview());
    ASSERT_TRUE(m_wrapper->textEditor()->isReadOnly());

    // Act
    m_wrapper->onEditAnyway();

    // Assert: 允许编辑但预览标记保留（直到另存成功）
    EXPECT_TRUE(m_wrapper->isInvalidCharEditAllowed());
    EXPECT_FALSE(m_wrapper->textEditor()->isReadOnly());
    EXPECT_TRUE(m_wrapper->isInvalidCharPreview()); // 语义：预览态保持
}

TEST_F(EditWrapperTest, ExitInvalidCharPreview_AfterPreview_ResetsAllState)
{
    // Arrange
    enterPreview(m_wrapper, m_tempDir, QByteArray("a\0b", 3));
    ASSERT_TRUE(m_wrapper->isInvalidCharPreview());

    // Act
    m_wrapper->exitInvalidCharPreview();

    // Assert: 全量复位
    EXPECT_FALSE(m_wrapper->isInvalidCharPreview());
    EXPECT_FALSE(m_wrapper->isInvalidCharEditAllowed());
    EXPECT_TRUE(m_wrapper->invalidCharOriginalPath().isEmpty());
    EXPECT_FALSE(m_wrapper->textEditor()->isReadOnly());
    EXPECT_TRUE(m_wrapper->m_pWaringNotices->isHidden());
}

TEST_F(EditWrapperTest, ForceSave_WritableOriginal_WritesAndExitsPreview)
{
    // Arrange: 编辑后强制保存回原路径
    enterPreview(m_wrapper, m_tempDir, QByteArray("a\0b", 3));
    m_wrapper->onEditAnyway();
    m_wrapper->textEditor()->setPlainText(QStringLiteral("forced-clean"));
    m_wrapper->setTemFile(true);

    // Act（B47 成功）
    const bool ok = m_wrapper->forceSaveInvalidCharFile();

    // Assert: 原路径内容更新 + 预览退出 + 备份态复位
    EXPECT_TRUE(ok);
    QFile f(m_tempDir->filePath("pv.txt"));
    ASSERT_TRUE(f.open(QIODevice::ReadOnly));
    EXPECT_EQ(f.readAll(), QByteArray("forced-clean"));
    f.close();
    EXPECT_FALSE(m_wrapper->isInvalidCharPreview());
    EXPECT_FALSE(m_wrapper->isTemFile());
}

TEST_F(EditWrapperTest, ForceSave_UnwritableTarget_KeepsPreview)
{
    // Arrange: 原路径只读 → 保存失败（B48）
    enterPreview(m_wrapper, m_tempDir, QByteArray("a\0b", 3));
    const QString original = m_wrapper->invalidCharOriginalPath();
    makeUnwritable(original);

    // Act
    const bool ok = m_wrapper->forceSaveInvalidCharFile();

    // Assert: 失败且预览态保持（强异常安全）
    EXPECT_FALSE(ok);
    EXPECT_TRUE(m_wrapper->isInvalidCharPreview());
    EXPECT_EQ(m_wrapper->invalidCharOriginalPath(), original);
}

// ============================================================================
// 视图模式（B49-B52）+ Markdown 识别（B53/B54）
// ============================================================================

TEST_F(EditWrapperTest, SetViewMode_WysiwygOrNonMdLivePreview_Rejected)
{
    // Arrange
    QSignalSpy spy(m_wrapper, &EditWrapper::viewModeChanged);

    // Act（B49: FSM 拒绝——Wysiwyg 阶段二不可达；非 md 不可 LivePreview）
    const bool wysiwyg = m_wrapper->setViewMode(ViewMode::Wysiwyg);
    const bool live = m_wrapper->setViewMode(ViewMode::LivePreview); // 当前非 md

    // Assert: 拒绝且模式不变、无信号
    EXPECT_FALSE(wysiwyg);
    EXPECT_FALSE(live);
    EXPECT_EQ(m_wrapper->viewMode(), ViewMode::Edit);
    EXPECT_EQ(spy.count(), 0);
}

TEST_F(EditWrapperTest, SetViewMode_ReadViewOnNonMd_TogglesReadOnlyText)
{
    // Arrange
    QSignalSpy spy(m_wrapper, &EditWrapper::viewModeChanged);
    ASSERT_FALSE(m_wrapper->textEditor()->getReadOnlyMode());

    // Act（B50: ReadView + 非 md → 纯文本只读）
    const bool ok = m_wrapper->setViewMode(ViewMode::ReadView);

    // Assert: 只读 + 信号
    EXPECT_TRUE(ok);
    EXPECT_TRUE(m_wrapper->textEditor()->getReadOnlyMode());
    EXPECT_EQ(m_wrapper->viewMode(), ViewMode::ReadView);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).value<ViewMode>(), ViewMode::ReadView);

    // Act 2: 切回 Edit → 恢复可编辑（B: m_bReadOnlyByViewMode 复位分支）
    EXPECT_TRUE(m_wrapper->setViewMode(ViewMode::Edit));
    EXPECT_FALSE(m_wrapper->textEditor()->getReadOnlyMode());
    EXPECT_EQ(m_wrapper->viewMode(), ViewMode::Edit);
    EXPECT_EQ(spy.count(), 2);
}

TEST_F(EditWrapperTest, SetViewMode_MarkdownLivePreview_RendersViaRenderer)
{
    // Arrange: 注入就绪的渲染器（懒创建被跳过，不触碰 WebEngine）
    m_renderer.ready = true;
    m_wrapper->setMarkdownRendererForTest(&m_renderer);
    m_wrapper->textEditor()->setPlainText(QStringLiteral("# title"));
    const QString mdPath = createFile("doc.md", QByteArray("# title"));
    m_wrapper->updatePath(mdPath, mdPath);

    // Act: 识别为 md → Edit 自动跃迁 LivePreview（B51）
    m_wrapper->updateMarkdownRecognition(QStringLiteral("doc.md"), QString());

    // Assert: 模式跃迁 + 渲染器编排（setMode/setLayout/setMarkdown 均被驱动）
    EXPECT_TRUE(m_wrapper->isMarkdownFile());
    EXPECT_EQ(m_wrapper->viewMode(), ViewMode::LivePreview);
    EXPECT_GE(m_renderer.setModeCalls, 1);
    EXPECT_EQ(m_renderer.lastMaxWidth, 0); // LivePreview: 不限宽不居中
    EXPECT_FALSE(m_renderer.lastCenter);
    EXPECT_GE(m_renderer.setMarkdownCalls, 1);
    EXPECT_TRUE(m_renderer.lastMarkdown.contains(QStringLiteral("# title")));
}

TEST_F(EditWrapperTest, SetViewMode_MarkdownReadView_Layout800Centered)
{
    // Arrange: 先进入 md LivePreview
    m_renderer.ready = true;
    m_wrapper->setMarkdownRendererForTest(&m_renderer);
    const QString mdPath = createFile("doc2.md", QByteArray("body"));
    m_wrapper->updatePath(mdPath, mdPath);
    m_wrapper->updateMarkdownRecognition(QStringLiteral("doc2.md"), QString());
    ASSERT_EQ(m_wrapper->viewMode(), ViewMode::LivePreview);

    // Act: 切到查看视图（B52）
    const bool ok = m_wrapper->setViewMode(ViewMode::ReadView);

    // Assert: 渲染页独占 + 800px 居中布局
    EXPECT_TRUE(ok);
    EXPECT_EQ(m_wrapper->viewMode(), ViewMode::ReadView);
    EXPECT_EQ(m_renderer.lastMaxWidth, 800);
    EXPECT_TRUE(m_renderer.lastCenter);
}

TEST_F(EditWrapperTest, UpdateMarkdownRecognition_MdLost_FallsBackToEdit)
{
    // Arrange: md + LivePreview
    m_renderer.ready = true;
    m_wrapper->setMarkdownRendererForTest(&m_renderer);
    const QString mdPath = createFile("doc3.md", QByteArray("x"));
    m_wrapper->updatePath(mdPath, mdPath);
    m_wrapper->updateMarkdownRecognition(QStringLiteral("doc3.md"), QString());
    ASSERT_EQ(m_wrapper->viewMode(), ViewMode::LivePreview);
    QSignalSpy spy(m_wrapper, &EditWrapper::markdownAvailabilityChanged);

    // Act（B54: 语言切走 → LivePreview 回退 Edit）
    m_wrapper->updateMarkdownRecognition(QStringLiteral("doc3.txt"), QStringLiteral("C++"));

    // Assert
    EXPECT_FALSE(m_wrapper->isMarkdownFile());
    EXPECT_EQ(m_wrapper->viewMode(), ViewMode::Edit);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_FALSE(spy.at(0).at(0).toBool());
}

class EditWrapperRecognitionTest : public EditWrapperTest,
                                   public ::testing::WithParamInterface<RecognitionCase> {};

TEST_P(EditWrapperRecognitionTest, UpdateMarkdownRecognition_ParamVariants_RecognizesAsExpected)
{
    const RecognitionCase c = GetParam();

    // Arrange: 注入渲染器（识别成功会触发 LivePreview 跃迁链路）
    m_renderer.ready = false;
    m_wrapper->setMarkdownRendererForTest(&m_renderer);
    QSignalSpy spy(m_wrapper, &EditWrapper::markdownAvailabilityChanged);

    // Act（B53: definition 名优先，其次扩展名）
    m_wrapper->updateMarkdownRecognition(c.fileName, c.definitionName);

    // Assert
    EXPECT_EQ(m_wrapper->isMarkdownFile(), c.expectMarkdown);
    EXPECT_EQ(spy.count(), c.expectMarkdown ? 1 : 0);
    if (c.expectMarkdown)
        EXPECT_EQ(m_wrapper->viewMode(), ViewMode::LivePreview); // Edit → 自动跃迁
}

INSTANTIATE_TEST_SUITE_P(
    RecognitionMatrix, EditWrapperRecognitionTest,
    ::testing::Values(
        RecognitionCase{QStringLiteral("note.md"), QString(), true},       // 扩展名
        RecognitionCase{QStringLiteral("b.MARKDOWN"), QString(), true},    // 扩展名大小写不敏感
        RecognitionCase{QStringLiteral("a.txt"), QStringLiteral("Markdown"), true}, // definition 名
        RecognitionCase{QStringLiteral("a.txt"), QStringLiteral("C++"), false}));

TEST_F(EditWrapperTest, SetMarkdownRenderer_Null_RevertsToNoRenderer)
{
    // Arrange: 先注入再清空
    m_wrapper->setMarkdownRendererForTest(&m_renderer);
    EXPECT_TRUE(m_wrapper->m_pRendererInjected);

    // Act
    m_wrapper->setMarkdownRendererForTest(nullptr);

    // Assert: 注入标记复位；此后 Edit 模式切换不触碰任何渲染器
    EXPECT_FALSE(m_wrapper->m_pRendererInjected);
    const bool ok = m_wrapper->setViewMode(ViewMode::Edit);
    EXPECT_TRUE(ok);
    EXPECT_EQ(m_renderer.setMarkdownCalls, 0);
}

// ============================================================================
// 信号槽联动（B8 增量）：ctor 连接的 lambda 槽与懒创建视图的事件处理
// ============================================================================

TEST_F(EditWrapperTest, ViewModeRequestedSignals_FromBarAndEditor_SwitchMode)
{
    // Arrange: 底栏 combobox / 编辑器右键菜单两条 viewModeRequested 入口（§8.1/§8.2）
    QSignalSpy spy(m_wrapper, &EditWrapper::viewModeChanged);

    // Act 1: BottomBar 发起切换请求（ctor lambda: viewModeRequested → setViewMode）
    QMetaObject::invokeMethod(m_wrapper->bottomBar(), "viewModeRequested",
                              Q_ARG(ViewMode, ViewMode::ReadView));

    // Assert 1: 非 md 查看视图 = 纯文本只读
    EXPECT_EQ(m_wrapper->viewMode(), ViewMode::ReadView);
    EXPECT_TRUE(m_wrapper->textEditor()->getReadOnlyMode());
    ASSERT_GE(spy.count(), 1);

    // Act 2: TextEdit 右键菜单发起回切请求（另一条 ctor lambda）
    QMetaObject::invokeMethod(m_wrapper->textEditor(), "viewModeRequested",
                              Q_ARG(ViewMode, ViewMode::Edit));

    // Assert 2: 回到编辑视图并恢复可编辑
    EXPECT_EQ(m_wrapper->viewMode(), ViewMode::Edit);
    EXPECT_FALSE(m_wrapper->textEditor()->getReadOnlyMode());
    EXPECT_GE(spy.count(), 2);
}

TEST_F(EditWrapperTest, ScrollBarLambdas_OnScrollInLivePreview_SyncRenderer)
{
    // Arrange: md + LivePreview + 就绪渲染器（滚动同步 lambda 激活条件齐备）
    const QString mdPath = createFile("scroll.md", QByteArray("# t"));
    m_wrapper->updatePath(mdPath, mdPath);
    m_renderer.ready = true;
    m_wrapper->setMarkdownRendererForTest(&m_renderer);
    m_wrapper->updateMarkdownRecognition(QStringLiteral("scroll.md"), QString());
    ASSERT_EQ(m_wrapper->viewMode(), ViewMode::LivePreview);

    QString text;
    for (int i = 0; i < 300; ++i)
        text += QStringLiteral("line %1\n").arg(i);
    m_wrapper->textEditor()->setPlainText(text);

    auto *sb = m_wrapper->textEditor()->verticalScrollBar();
    ASSERT_GT(sb->maximum(), sb->minimum());

    // Act: 滚动 → 直连滚动同步 lambda + 排队高亮 lambda（两路 ctor connect）
    sb->setValue(sb->maximum() / 2);
    QApplication::processEvents();

    // Assert: 右栏按比例收到滚动同步；排队高亮链路已消化
    EXPECT_GE(m_renderer.scrollCalls, 1);
    EXPECT_GT(m_renderer.lastRatio, 0.0);
    EXPECT_LT(m_renderer.lastRatio, 1.0);
    EXPECT_EQ(sb->value(), sb->maximum() / 2);
}

TEST_F(EditWrapperTest, OnUpdateHighlighter_FoldRegion_RehighlightsUnmarkedBlocks)
{
    // Arrange: 有效 definition + 关闭高亮插入括号文本（块无 userData）
    m_renderer.ready = false;
    m_wrapper->setMarkdownRendererForTest(&m_renderer);
    m_wrapper->reloadFileHighlight(QStringLiteral("C++"));
    auto *hl = m_wrapper->getSyntaxHighlighter();
    ASSERT_NE(hl, nullptr);
    hl->setEnableHighlight(false);

    QString text;
    for (int i = 0; i < 200; ++i)
        text += QStringLiteral("aaaa\n");
    text += QStringLiteral("{\n");
    for (int i = 0; i < 200; ++i)
        text += QStringLiteral("bbbb\n");
    text += QStringLiteral("}\n");
    for (int i = 0; i < 120; ++i)
        text += QStringLiteral("cccc\n");
    m_wrapper->textEditor()->setPlainText(text);

    auto *sb = m_wrapper->textEditor()->verticalScrollBar();
    ASSERT_GT(sb->maximum(), sb->minimum());
    const QTextBlock midBlock = m_wrapper->textEditor()->document()->findBlockByNumber(210);
    ASSERT_EQ(midBlock.userData(), nullptr); // 高亮关闭 → 块未标记

    // Act 1: 滚到底部 → 排队 OnUpdateHighlighter（折叠区循环对未标记块调用重高亮 lambda）
    sb->setValue(sb->maximum());
    QApplication::processEvents();

    // Act 2: 打开高亮后显式触发 → lambda 对未标记块真正执行 kate 高亮（注入 userData）
    hl->setEnableHighlight(true);
    m_wrapper->OnUpdateHighlighter();

    // Assert: '{' 区间内的块已被重高亮标记；全文高亮标志未被置位（局部路径）
    EXPECT_NE(m_wrapper->textEditor()->document()->findBlockByNumber(210).userData(), nullptr);
    EXPECT_FALSE(m_wrapper->m_bHighlighterAll);
    EXPECT_EQ(m_wrapper->textEditor()->toPlainText().count(QLatin1Char('\n')), 522);
}

TEST_F(EditWrapperTest, MarkdownViewSignalHandlers_ViewEvents_ReactCorrectly)
{
    // Arrange: 不注入渲染器 → 真实 MarkdownView 懒创建；init 置空桩避免
    // WebEngine 页面加载（构造函数只建桥接与连接，offscreen 安全）
    stub.set_lamda(&MarkdownView::init, [](MarkdownView *) {});

    const QString mdPath = createFile("mv.md", QByteArray("# t"));
    m_wrapper->updatePath(mdPath, mdPath);
    m_wrapper->updateMarkdownRecognition(QStringLiteral("mv.md"), QString());
    ASSERT_EQ(m_wrapper->viewMode(), ViewMode::LivePreview);
    MarkdownView *view = m_wrapper->m_pMarkdownView;
    ASSERT_NE(view, nullptr);

    // Act 1: 渲染内核就绪信号 → RenderThrottle 解除缓存
    QMetaObject::invokeMethod(view, "ready");

    // Assert 1: throttle 进入 ready 态
    EXPECT_TRUE(m_wrapper->m_renderThrottle.isReady());

    // Act 2: 右栏滚动比例 → 反向同步驱动左栏（§4.6 双向）
    QString text;
    for (int i = 0; i < 300; ++i)
        text += QStringLiteral("l%1\n").arg(i);
    m_wrapper->textEditor()->setPlainText(text);
    auto *sb = m_wrapper->textEditor()->verticalScrollBar();
    ASSERT_GT(sb->maximum(), sb->minimum());
    QMetaObject::invokeMethod(view, "scrollRatioChanged", Q_ARG(double, 0.5));

    // Assert 2: 左栏光标条按比例定位（min + round(0.5*(max-min))）
    const int expected = sb->minimum() + qRound(0.5 * (sb->maximum() - sb->minimum()));
    EXPECT_EQ(sb->value(), expected);

    // Act 3: 渲染页右键 → 视图模式菜单（QMenu::exec 置空桩防模态阻塞）
    stub.set_lamda(static_cast<QAction *(QMenu::*)(const QPoint &, QAction *)>(&QMenu::exec),
                   [](QMenu *, const QPoint &, QAction *) -> QAction * { return nullptr; });
    QMetaObject::invokeMethod(view, "customContextMenuRequested",
                              Q_ARG(QPoint, QPoint(2, 2)));

    // Assert 3: 未崩溃且模式保持 LivePreview（菜单事件未改状态）
    EXPECT_EQ(m_wrapper->viewMode(), ViewMode::LivePreview);
    EXPECT_EQ(m_wrapper->textEditor()->toPlainText().size(), text.size());
}
