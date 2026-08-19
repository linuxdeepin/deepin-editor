// SPDX-FileCopyrightText: 2017-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef EDITORBUFFER_H
#define EDITORBUFFER_H

#include "../editor/dtextedit.h"
#include "../widgets/bottombar.h"
#include "../controls/warningnotices.h"
#include "../editor/leftareaoftextedit.h"
#include "../common/CSyntaxHighlighter.h"
#include "../common/utils.h"
#include <QVBoxLayout>
#include <QWidget>
#include <DMessageManager>
#include <DFloatingMessage>
#include <QByteArray>
#include <QTextCodec>
#include <DDialog>
#include <DMessageBox>
#include <DFileDialog>
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/SyntaxHighlighter>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/Theme>
#include <QStackedWidget>
#include <QSplitter>
#include "markdown/viewmode.h"
#include "markdown/renderthrottle.h"

class Window;
class MarkdownView;
class IMarkdownRenderer;
class EditWrapper : public QWidget
{
    Q_OBJECT

public:
    struct FileStateItem {
        QDateTime modified;
        QFile::Permissions permissions;
    };

    EditWrapper(Window *window = nullptr, QWidget *parent = nullptr);
    ~EditWrapper();

    //清除焦点　梁卫东　２０２０－０９－１４　１１：００：５０
    void clearAllFocus();
    void setQuitFlag();
    bool isQuit();
    bool getFileLoading();

    /**
     * @brief openFile 打开文件
     * @param filepath　打开文件路径
     * @param qstrTruePath　真实文件路径
     * @param bIsTemFile　修改状态
     */
    void openFile(const QString &filepath, QString qstrTruePath, bool bIsTemFile = false);
    // 以编码 encode 重新读取文件
    bool readFile(QByteArray encode = "");
    // 按编码 encode 保存文件
    bool saveFile(QByteArray encode = "");
    /**
     * @brief getPlainTextContent 获取文本框里的文本内容
     * @param plainTextConteng 存放获取到的内容
     */
    void getPlainTextContent(QByteArray &plainTextContent);
    //重新加载文件编码
    bool saveAsFile(const QString &newFilePath, const QByteArray &encodeName);
    //保存草稿文件
    bool saveDraftFile(QString &newFilePath);
    //另存为第一次打开文件编码文件
    bool saveAsFile();
    //重新加载文件编码 1.文件修改 2.文件未修改处理逻辑一样 切换编码重新加载和另存为 梁卫东
    bool reloadFileEncode(QByteArray encode);
    // 重新加载文件高亮类型
    void reloadFileHighlight(QString definitionName);
    //重写加载修改文件
    void reloadModifyFile();
    //获取文件编码
    QString getTextEncode();

    // 保存备份文件
    bool saveTemFile(QString qstrDir);
    //更新路径
    void updatePath(const QString &file, QString qstrTruePath = QString());
    //判断是否修改
    bool isModified();
    //判断是否草稿文件
    bool isDraftFile();
    //判断是否为备份文件
    bool isBackupFile();
    //判断内容是否为空
    bool isPlainTextEmpty();

    bool isTemFile();

    void hideWarningNotices();
    void checkForReload();
    void initToastPosition();
    void showNotify(const QString &message, bool warning = false);
    bool getTextChangeFlag();
    void setTextChangeFlag(bool bFlag);
    void setLineNumberShow(bool bIsShow, bool bIsFirstShow = false);
    void setShowBlankCharacter(bool ok);
    void clearDoubleCharaterEncode();
    //
    BottomBar *bottomBar();
    QString filePath();
    TextEdit *textEditor();
    Window *window();
    void updateHighlighterAll();

    //get and set m_tModifiedDateTime
    QDateTime getLastModifiedTime() const;
    void setLastModifiedTime(const QString &time);

    void updateModifyStatus(bool isModified);
    void updateSaveAsFileName(QString strOldFilePath, QString strNewFilePath);

    // 取得当前编辑器使用的高亮处理(用于打印高亮)
    inline CSyntaxHighlighter *getSyntaxHighlighter() const
    { return m_pSyntaxHighlighter; }

    // 无效字符预览模式访问器
    inline bool isInvalidCharPreview() const
    { return m_bInvalidCharPreview; }
    inline bool isInvalidCharEditAllowed() const
    { return m_bInvalidCharEditAllowed; }
    inline QString invalidCharOriginalPath() const
    { return m_sInvalidCharOriginalPath; }
    void exitInvalidCharPreview();
    bool forceSaveInvalidCharFile();

    // —— Markdown 视图模式（阶段三）：判定/切换委托 ViewModeFsm / MarkdownLogic ——
    // 切换视图模式：FSM 校验通过返回 true 并发 viewModeChanged；被拒返回 false 且模式不变
    bool setViewMode(ViewMode mode);
    // 当前视图模式（默认 Edit）
    inline ViewMode viewMode() const
    { return m_viewMode; }
    // 当前文件是否为 Markdown（识别结果见 MarkdownLogic）
    inline bool isMarkdownFile() const
    { return m_isMarkdown; }
    // 文件加载/高亮语言变更时刷新 Markdown 识别；识别结果变化时发 markdownAvailabilityChanged，
    // 丢失 md 且当前处于 LivePreview 时按 FSM 回退到 Edit
    void updateMarkdownRecognition(const QString &fileName, const QString &definitionName);
    // 测试注入 Mock 渲染器：注入后 ensureMarkdownViewCreated 不再创建真实 MarkdownView
    void setMarkdownRendererForTest(IMarkdownRenderer *renderer);

signals:
    void sigClearDoubleCharaterEncode();
    // 视图模式切换成功后发出
    void viewModeChanged(ViewMode mode);
    // Markdown 识别结果变化（true 可用 / false 不可用）
    void markdownAvailabilityChanged(bool available);

protected:
    // 处理文件加载事件
    virtual void customEvent(QEvent *e) override;

private:
    // 类似setPlainText(QString) 接口支持大文本加载 不卡顿 秒退出 梁卫东 2020年11月11日16:56:27
    void loadContent(const QByteArray &);
    void handleHightlightChanged(const QString &name);
    int GetCorrectUnicode1(const QByteArray &ba);
    // 文件加载时重新初始化部分设置
    void reinitOnFileLoad(const QByteArray &encode);
    // 懒创建 MarkdownView 渲染页（ReadView/LivePreview 且非纯文本只读模式时），已注入测试渲染器则跳过
    void ensureMarkdownViewCreated();
    // 懒创建实时阅览分栏（Page2：左编辑页 + 右渲染视图）
    void ensureLiveSplitterCreated();

    Q_SLOT void onReadAllocError();

public slots:
    // 处理文档预加载数据
    void handleFilePreProcess(const QByteArray &encode, const QByteArray &content);
    void handleFileLoadFinished(const QByteArray &encode, const QByteArray &content, bool error, bool hasNul = false);
    void OnThemeChangeSlot(QString theme);
    void UpdateBottomBarWordCnt(int cnt);
    void OnUpdateHighlighter();
    //set the value of m_bIsTemFile
    void setTemFile(bool value);
    // 设置恢复光标位置（用于懒加载恢复，避免 O(N²) 扫描）
    void setRestoreCursorPosition(int position);
    void onEditAnyway();

private:
    //第一次打开文件编码
    QString m_sFirstEncode = QString("UTF-8");
    //当前切换文件编码
    QString  m_sCurEncode = QString("UTF-8");

    //左边栏　标记　行号　折叠三合一控件
    LeftAreaTextEdit *m_pLeftAreaTextEdit = nullptr;
    //
    Window *m_pWindow = nullptr;
    //
    TextEdit *m_pTextEdit = nullptr;
    //
    BottomBar *m_pBottomBar = nullptr;
    //
    WarningNotices *m_pWaringNotices = nullptr;

    QDateTime m_tModifiedDateTime;
    //退出
    bool m_bQuit = false;
    //文件是否加载
    bool m_bFileLoading = false;
    bool m_bIsTemFile = false;
    //撤销重做栈操作任务文件修改
    bool m_bUndoRedoOption = false;
    //语法高亮
    KSyntaxHighlighting::Repository m_Repository;
    KSyntaxHighlighting::Definition m_Definition;
    //KSyntaxHighlighting::SyntaxHighlighter *m_pSyntaxHighlighter = nullptr;
    CSyntaxHighlighter *m_pSyntaxHighlighter = nullptr;
    bool m_bHighlighterAll = false;

    bool m_bAsyncReadFileFinished = false;
    bool m_bHasPreProcess = false;               // 预处理标识
    int m_nRestoreCursorPosition = -1;           // 恢复光标位置提示（-1 表示不指定）

    // 无效字符（NUL）预览模式状态
    bool m_bInvalidCharPreview = false;
    bool m_bInvalidCharEditAllowed = false;
    QString m_sInvalidCharOriginalPath;

    // —— Markdown 视图模式状态（阶段三）——
    // 当前文件是否为 md（决定 ReadView/LivePreview 是否可用）
    bool m_isMarkdown = false;
    // 当前视图模式，默认编辑视图
    ViewMode m_viewMode = ViewMode::Edit;
    // 懒创建的渲染页（首次进入 ReadView/LivePreview 时 new；测试注入后保持 nullptr）
    MarkdownView *m_pMarkdownView = nullptr;
    // 上层统一依赖的渲染接口（真实实现为 m_pMarkdownView，或测试注入的 Mock）
    IMarkdownRenderer *m_pRenderer = nullptr;
    // 是否已注入测试渲染器
    bool m_pRendererInjected = false;
    // 三页视图栈（§4.4）：Page0 编辑页 / Page1 查看视图渲染页 / Page2 实时阅览分栏
    QStackedWidget *m_viewStack = nullptr;
    // Page0：[LeftArea|TextEdit] 原有编辑布局容器（LivePreview 时整体挪入分栏左栏，实例不变）
    QWidget *m_pEditPage = nullptr;
    // Page1：查看视图渲染页（m_pMarkdownView 独占）
    QWidget *m_pReadPage = nullptr;
    // Page2：实时阅览分栏（懒创建）
    QSplitter *m_pLiveSplitter = nullptr;
    // 内容渲染去抖（§4.5：300ms 去抖/首切立即/相同跳过/ready 前缓存）
    RenderThrottle m_renderThrottle;
    // 只读是「查看视图(非 md)」设置的，切回 Edit 时需恢复
    bool m_bReadOnlyByViewMode = false;
    // 滚动同步防回环护栏（§4.6）：应用一侧同步时置位，另一侧的 valueChanged 跳过
    bool m_bScrollSyncing = false;
};

#endif
