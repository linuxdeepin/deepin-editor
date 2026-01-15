// SPDX-FileCopyrightText: 2011-2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later


#include "../common/utils.h"
#include "../widgets/window.h"
#include "../widgets/bottombar.h"
#include "dtextedit.h"
#include "leftareaoftextedit.h"
#include "editwrapper.h"
#include "showflodcodewidget.h"
#include "deletebackcommond.h"
#include "replaceallcommond.h"
#include "insertblockbytextcommond.h"
#include "indenttextcommond.h"
#include "undolist.h"
#include "changemarkcommand.h"
#include "endlineformatcommond.h"
#include <QSet>

#include <KSyntaxHighlighting/definition.h>
#include <KSyntaxHighlighting/syntaxhighlighter.h>
#include <KSyntaxHighlighting/theme.h>

#include <QAbstractTextDocumentLayout>
#include <QTextDocumentFragment>
#include <QInputMethodEvent>
#include <DDesktopServices>
#include <QApplication>
#include <DSettingsGroup>
#include <DSettingsOption>
#include <DSettings>
#include <QClipboard>
#include <QFileInfo>
#include <QDebug>
#include <QPainter>
#include <QScroller>
#include <QScrollBar>
#include <QStyleFactory>
#include <QTextBlock>
#include <QMimeData>
#include <QTimer>
#include <QGesture>
#include <QStyleHints>
#include <DSysInfo>

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#include <private/qguiapplication_p.h>
#include <qpa/qplatformtheme.h>
#endif
#include <QtSvg/qsvgrenderer.h>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

TextEdit::TextEdit(QWidget *parent)
    : DPlainTextEdit(parent),
      m_wrapper(nullptr)
{
    qDebug() << "Initializing TextEdit component";
    // 更新单独添加的高亮格式文件
    m_repository.addCustomSearchPath(KF5_HIGHLIGHT_PATH);

    setUndoRedoEnabled(false);
    //撤销重做栈
    m_pUndoStack = new QUndoStack();

    m_nLines = 0;
    m_nBookMarkHoverLine = -1;
    m_bIsFileOpen = false;
    m_qstrCommitString = "";
    m_bIsShortCut = false;
    m_bIsInputMethod = false;
    m_isSelectAll = false;
    m_fontLineNumberArea.setFamily("SourceHanSansSC-Normal");
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    m_touchTapDistance = QGuiApplicationPrivate::platformTheme()->themeHint(QPlatformTheme::TouchDoubleTapDistance).toInt();
#endif

    m_pLeftAreaWidget = new LeftAreaTextEdit(this);
    m_pLeftAreaWidget->m_pFlodArea->setAttribute(Qt::WA_Hover, true); //开启悬停事件
    m_pLeftAreaWidget->m_pBookMarkArea->setAttribute(Qt::WA_Hover, true);
    m_pLeftAreaWidget->m_pFlodArea->installEventFilter(this);
    m_pLeftAreaWidget->m_pBookMarkArea->installEventFilter(this);
    m_foldCodeShow = new ShowFlodCodeWidget(this);
    m_foldCodeShow->setVisible(false);

    viewport()->installEventFilter(this);
    viewport()->setCursor(Qt::IBeamCursor);

    // Don't draw frame around editor widget.
    setFrameShape(QFrame::NoFrame);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setContentsMargins(0, 0, 0, 0);
    setEditPalette(palette().windowText().color().name(), palette().windowText().color().name());

    grabGesture(Qt::TapGesture);
    grabGesture(Qt::TapAndHoldGesture);
    grabGesture(Qt::SwipeGesture);
    grabGesture(Qt::PanGesture);
    grabGesture(Qt::PinchGesture);

    // Init widgets.
    //左边栏控件　滑动条滚动跟新行号 折叠标记
    connect(this->verticalScrollBar(), &QScrollBar::valueChanged, this, &TextEdit::slotValueChanged);
    connect(this, &QPlainTextEdit::textChanged, this, &TextEdit::updateLeftAreaWidget);
    connect(this, &QPlainTextEdit::textChanged, this, [this]() {
        this->m_wrapper->UpdateBottomBarWordCnt(this->characterCount());
    });

    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &TextEdit::cursorPositionChanged);
    connect(this, &QPlainTextEdit::selectionChanged, this, &TextEdit::onSelectionArea);

    connect(document(), &QTextDocument::contentsChange, this, &TextEdit::updateMark);
    connect(document(), &QTextDocument::contentsChange, this, &TextEdit::checkBookmarkLineMove);
    connect(document(), &QTextDocument::contentsChange, this, &TextEdit::onTextContentChanged);

    connect(m_pUndoStack, &QUndoStack::canRedoChanged, this, &TextEdit::slotCanRedoChanged);
    connect(m_pUndoStack, &QUndoStack::canUndoChanged, this, &TextEdit::slotCanUndoChanged);

    QDBusConnection dbus = QDBusConnection::sessionBus();
    switch (Utils::getSystemVersion()) {
        case Utils::V23:
            qDebug() << "Utils::V23";
            dbus.systemBus().connect("org.deepin.dde.Gesture1",
                                    "/org/deepin/dde/Gesture1", "org.deepin.dde.Gesture1",
                                    "Event",
                                    this, SLOT(fingerZoom(QString, QString, int)));
            break;
        default:
            qDebug() << "default";
            dbus.systemBus().connect("com.deepin.daemon.Gesture",
                                    "/com/deepin/daemon/Gesture", "com.deepin.daemon.Gesture",
                                    "Event",
                                    this, SLOT(fingerZoom(QString, QString, int)));
            break;
    }
    
    // 连接音频设备状态变化信号 (根据Qt版本选择不同的DBus服务)
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    dbus.sessionBus().connect("org.deepin.dde.Audio1",
                            "/org/deepin/dde/Audio1", "org.deepin.dde.Audio1",
                            "PortEnabledChanged",
                            this, SLOT(onAudioPortEnabledChanged(quint32, QString, bool)));
#else
    dbus.sessionBus().connect("com.deepin.daemon.Audio",
                            "/com/deepin/daemon/Audio", "com.deepin.daemon.Audio",
                            "PortEnabledChanged",
                            this, SLOT(onAudioPortEnabledChanged(quint32, QString, bool)));
#endif

    //初始化右键菜单
    initRightClickedMenu();

    // Init scroll animation.
    m_scrollAnimation = new QPropertyAnimation(verticalScrollBar(), "value");
    m_scrollAnimation->setEasingCurve(QEasingCurve::InOutExpo);
    m_scrollAnimation->setDuration(300);

    m_cursorMode = Insert;

    connect(m_scrollAnimation, &QPropertyAnimation::finished, this, &TextEdit::handleScrollFinish, Qt::QueuedConnection);

    // Monitor cursor mark status to update in line number area.
    connect(this, &TextEdit::cursorMarkChanged, this, &TextEdit::handleCursorMarkChanged);

    // configure content area
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, &TextEdit::adjustScrollbarMargins, Qt::QueuedConnection);
    // Don't blink the cursor when selecting text
    // Recover blink when not selecting text.
    connect(this, &TextEdit::selectionChanged, this, &TextEdit::slotSelectionChanged, Qt::QueuedConnection);
    qDebug() << "TextEdit initialized";
}

TextEdit::~TextEdit()
{
    qDebug() << "Destroying TextEdit component";
    if (m_scrollAnimation != nullptr) {
        qDebug() << "m_scrollAnimation is not null";
        if (m_scrollAnimation->state() != QAbstractAnimation::Stopped) {
            m_scrollAnimation->stop();
        }
        delete m_scrollAnimation;
        m_scrollAnimation = nullptr;
    }
    if (m_colorMarkMenu != nullptr) {
        qDebug() << "m_colorMarkMenu is not null";
        delete m_colorMarkMenu;
        m_colorMarkMenu = nullptr;
    }
    if (m_convertCaseMenu != nullptr) {
        qDebug() << "m_convertCaseMenu is not null";
        delete m_convertCaseMenu;
        m_convertCaseMenu = nullptr;
    }
    if (m_rightMenu != nullptr) {
        qDebug() << "m_rightMenu is not null";
        delete m_rightMenu;
        m_rightMenu = nullptr;
    }

    if (m_pUndoStack != nullptr) {
        qDebug() << "m_pUndoStack is not null";
        m_pUndoStack->deleteLater();
    }
}

void TextEdit::insertTextEx(QTextCursor cursor, QString text)
{
    qDebug() << "Inserting text at position:" << cursor.position() << "Length:" << text.length();
    QUndoCommand *pInsertStack = new InsertTextUndoCommand(cursor, text, this);
    m_pUndoStack->push(pInsertStack);
    ensureCursorVisible();
    qDebug() << "Text inserted successfully";
}

/**
 * @brief 将多组插入信息 \a multiText 压入单个撤销栈，便于撤销栈管理。
 * @param multiText 多组插入信息，每组含插入光标位置和插入文本。
 */
void TextEdit::insertMultiTextEx(const QList<QPair<QTextCursor, QString> > &multiText)
{
    qDebug() << "Inserting multiple texts, count:" << multiText.size();
    if (multiText.isEmpty()) {
        qDebug() << "Multi text insert skipped - empty input";
        return;
    }

    QUndoCommand *pMultiCommand = new QUndoCommand;
    // 将所有的插入信息添加到单个撤销项中，便于单次处理
    for (auto pairText : multiText) {
        // pMultiCommand 析构时会自动析构子撤销项
        (void)new InsertTextUndoCommand(pairText.first, pairText.second, this, pMultiCommand);
    }
    m_pUndoStack->push(pMultiCommand);
    ensureCursorVisible();
    qDebug() << "Multi text inserted successfully";
}

void TextEdit::deleteSelectTextEx(QTextCursor cursor)
{
    qDebug() << "Deleting selected text at position:" << cursor.position()
               << "Selection length:" << cursor.selectedText().length();
    if (cursor.hasSelection()) {
        qDebug() << "cursor.hasSelection() is true";
        QUndoCommand *pDeleteStack = new DeleteTextUndoCommand(cursor, this);
        m_pUndoStack->push(pDeleteStack);
    }
    qDebug() << "Selected text deleted successfully";
}

void TextEdit::deleteSelectTextEx(QTextCursor cursor, QString text, bool currLine)
{
    qDebug() << "Deleting selected text at position:" << cursor.position();
    QUndoCommand *pDeleteStack = new DeleteTextUndoCommand2(cursor, text, this, currLine);
    m_pUndoStack->push(pDeleteStack);
    qDebug() << "Selected text deleted successfully";
}

void TextEdit::deleteTextEx(QTextCursor cursor)
{
    qDebug() << "Deleting text at position:" << cursor.position();
    QUndoCommand *pDeleteStack = new DeleteTextUndoCommand(cursor, this);
    m_pUndoStack->push(pDeleteStack);
    qDebug() << "Text deleted successfully";
}

/**
 * @brief 将多组删除信息 \a multiText 压入单个撤销栈，便于撤销栈管理。
 * @param multiText 多组删除信息，每组含删除光标信息(删除位置和选取区域)。
 */
void TextEdit::deleteMultiTextEx(const QList<QTextCursor> &multiText)
{
    qDebug() << "Deleting multiple texts, count:" << multiText.size();
    if (multiText.isEmpty()) {
        qDebug() << "Multi text delete skipped - empty input";
        return;
    }

    QUndoCommand *pMultiCommand = new QUndoCommand;
    // 将所有的插入信息添加到单个撤销项中，便于单次处理
    for (auto textCursor : multiText) {
        // pMultiCommand 析构时会自动析构子撤销项
        (void)new DeleteTextUndoCommand(textCursor, this, pMultiCommand);
    }
    m_pUndoStack->push(pMultiCommand);
    qDebug() << "Multi text deleted successfully";
}

void TextEdit::insertSelectTextEx(QTextCursor cursor, QString text)
{
    qDebug() << "Inserting selected text at position:" << cursor.position() << "Length:" << text.length();
    QUndoCommand *pInsertStack = new InsertTextUndoCommand(cursor, text, this);
    m_pUndoStack->push(pInsertStack);
    ensureCursorVisible();
    qDebug() << "Selected text inserted successfully";
}

void TextEdit::insertColumnEditTextEx(QString text)
{
    qDebug() << "Inserting column text";
    if (m_isSelectAll) {
        QPlainTextEdit::selectAll();
    }

    QUndoCommand *pInsertStack = new InsertTextUndoCommand(m_altModSelections, text, this);
    m_pUndoStack->push(pInsertStack);
    ensureCursorVisible();
    qDebug() << "Column text inserted successfully";
}

void TextEdit::initRightClickedMenu()
{
    qDebug() << "Initializing right clicked menu";
    // Init menu.
    m_rightMenu = new DMenu();
    m_undoAction = new QAction(tr("Undo"), this);
    m_redoAction = new QAction(tr("Redo"), this);
    m_cutAction = new QAction(tr("Cut"), this);
    m_copyAction = new QAction(tr("Copy"), this);
    m_pasteAction = new QAction(tr("Paste"), this);
    m_deleteAction = new QAction(tr("Delete"), this);
    m_selectAllAction = new QAction(tr("Select All"), this);
    m_findAction = new QAction(tr("Find"), this);
    m_replaceAction = new QAction(tr("Replace"), this);
    m_jumpLineAction = new QAction(tr("Go to Line"), this);
    m_enableReadOnlyModeAction = new QAction(tr("Turn on Read-Only mode"), this);
    m_disableReadOnlyModeAction = new QAction(tr("Turn off Read-Only mode"), this);
    m_fullscreenAction = new QAction(tr("Fullscreen"), this);
    m_exitFullscreenAction = new QAction(tr("Exit fullscreen"), this);
    m_openInFileManagerAction = new QAction(tr("Display in file manager"), this);
    m_toggleCommentAction = new QAction(tr("Add Comment"), this);
    m_voiceReadingAction = new QAction(tr("Text to Speech"), this);
    m_dictationAction = new QAction(tr("Speech to Text"), this);
    m_translateAction = new QAction(tr("Translate"), this);
    m_columnEditAction = new QAction(tr("Column Mode"), this);
    m_addBookMarkAction = new QAction(tr("Add bookmark"), this);
    m_cancelBookMarkAction = new QAction(tr("Remove Bookmark"), this);
    m_preBookMarkAction = new QAction(tr("Previous bookmark"), this);
    m_nextBookMarkAction = new QAction(tr("Next bookmark"), this);
    m_clearBookMarkAction = new QAction(tr("Remove All Bookmarks"), this);
    m_flodAllLevel = new QAction(tr("Fold All"), this);
    m_flodCurrentLevel = new QAction(tr("Fold Current Level"), this);
    m_unflodAllLevel = new QAction(tr("Unfold All"), this);
    m_unflodCurrentLevel = new QAction(tr("Unfold Current Level"), this);

    //yanyuhan
    //颜色标记、折叠/展开、书签、列编辑、设置注释、取消注释;
    //点击颜色标记菜单，显示二级菜单，包括：标记、清除上次标记、清除标记、标记所有;
    m_colorMarkMenu = new DMenu(tr("Color Mark"));

    // 为颜色标记Menu，增加事件过滤
    m_colorMarkMenu->installEventFilter(this);
    m_cancleMarkAllLine = new QAction(tr("Clear All Marks"), this);
    m_cancleLastMark = new QAction(tr("Clear Last Mark"), this);

    //添加当前颜色选择控件　梁卫东
    ColorSelectWdg *pColorsSelectWdg = new ColorSelectWdg(QString(), this);
    connect(pColorsSelectWdg, &ColorSelectWdg::sigColorSelected, this, &TextEdit::slotSigColorSelected);

    m_actionColorStyles = new QWidgetAction(this);
    m_actionColorStyles->setDefaultWidget(pColorsSelectWdg);

    m_markCurrentAct = new QAction(tr("Mark"), this);
    connect(m_markCurrentAct, &QAction::triggered, this, [this, pColorsSelectWdg]() {
        qDebug() << "Marking current line";
        isMarkCurrentLine(true, pColorsSelectWdg->getDefaultColor().name());
        renderAllSelections();
    });

    //添加全部颜色选择控件　梁卫东
    ColorSelectWdg *pColorsAllSelectWdg = new ColorSelectWdg(QString(), this);
    connect(pColorsAllSelectWdg, &ColorSelectWdg::sigColorSelected, this, &TextEdit::slotSigColorAllSelected);
    m_actionAllColorStyles = new QWidgetAction(this);
    m_actionAllColorStyles->setDefaultWidget(pColorsAllSelectWdg);

    m_markAllAct = new QAction(tr("Mark All"), this);
    connect(m_markAllAct, &QAction::triggered, this, [this, pColorsAllSelectWdg]() {
        qDebug() << "Marking all lines";
        m_strMarkAllLineColorName = pColorsAllSelectWdg->getDefaultColor().name();
        isMarkAllLine(true, pColorsAllSelectWdg->getDefaultColor().name());
        renderAllSelections();
    });

    m_addComment = new QAction(tr("Add Comment"), this);
    m_cancelComment = new QAction(tr("Remove Comment"), this);

    connect(m_rightMenu, &DMenu::aboutToHide, this, &TextEdit::removeHighlightWordUnderCursor);
    connect(m_undoAction, &QAction::triggered, this, [ = ]() {
        this->undo_();
    });
    connect(m_redoAction, &QAction::triggered, this, [ = ]() {
        this->redo_();
    });
    connect(m_cutAction, &QAction::triggered, this, &TextEdit::slotCutAction);
    connect(m_copyAction, &QAction::triggered, this, &TextEdit::slotCopyAction);
    connect(m_pasteAction, &QAction::triggered, this, &TextEdit::slotPasteAction);
    connect(m_deleteAction, &QAction::triggered, this, &TextEdit::slotDeleteAction);
    connect(m_selectAllAction, &QAction::triggered, this, &TextEdit::slotSelectAllAction);
    connect(m_findAction, &QAction::triggered, this, &TextEdit::clickFindAction);
    connect(m_replaceAction, &QAction::triggered, this, &TextEdit::clickReplaceAction);
    connect(m_jumpLineAction, &QAction::triggered, this, &TextEdit::clickJumpLineAction);
    connect(m_fullscreenAction, &QAction::triggered, this, &TextEdit::clickFullscreenAction);
    connect(m_exitFullscreenAction, &QAction::triggered, this, &TextEdit::clickFullscreenAction);
    connect(m_enableReadOnlyModeAction, &QAction::triggered, this, &TextEdit::toggleReadOnlyMode);
    connect(m_disableReadOnlyModeAction, &QAction::triggered, this, &TextEdit::toggleReadOnlyMode);
    connect(m_openInFileManagerAction, &QAction::triggered, this, &TextEdit::slotOpenInFileManagerAction);
    connect(m_addComment, &QAction::triggered, this, &TextEdit::slotAddComment);
    connect(m_cancelComment, &QAction::triggered, this, &TextEdit::slotCancelComment);
    connect(m_voiceReadingAction, &QAction::triggered, this, &TextEdit::slotVoiceReadingAction);
    connect(m_dictationAction, &QAction::triggered, this, &TextEdit::slotdictationAction);
    connect(m_translateAction, &QAction::triggered, this, &TextEdit::slot_translate);
    connect(m_columnEditAction, &QAction::triggered, this, &TextEdit::slotColumnEditAction);
    connect(m_addBookMarkAction, &QAction::triggered, this, &TextEdit::addOrDeleteBookMark);
    connect(m_cancelBookMarkAction, &QAction::triggered, this, &TextEdit::addOrDeleteBookMark);
    connect(m_preBookMarkAction, &QAction::triggered, this, &TextEdit::slotPreBookMarkAction);
    connect(m_nextBookMarkAction, &QAction::triggered, this, &TextEdit::slotNextBookMarkAction);
    connect(m_clearBookMarkAction, &QAction::triggered, this, &TextEdit::slotClearBookMarkAction);
    connect(m_flodAllLevel, &QAction::triggered, this, &TextEdit::slotFlodAllLevel);
    connect(m_unflodAllLevel, &QAction::triggered, this, &TextEdit::slotUnflodAllLevel);
    connect(m_flodCurrentLevel, &QAction::triggered, this, &TextEdit::slotFlodCurrentLevel);
    connect(m_unflodCurrentLevel, &QAction::triggered, this, &TextEdit::slotUnflodCurrentLevel);
    connect(m_cancleMarkAllLine, &QAction::triggered, this, &TextEdit::slotCancleMarkAllLine);
    connect(m_cancleLastMark, &QAction::triggered, this, &TextEdit::slotCancleLastMark);

    // Init convert case sub menu.
    m_haveWordUnderCursor = false;
    m_convertCaseMenu = new DMenu(tr("Change Case"));
    m_upcaseAction = new QAction(tr("Upper Case"), this);
    m_downcaseAction = new QAction(tr("Lower Case"), this);
    m_capitalizeAction = new QAction(tr("Capitalize"), this);
    m_convertCaseMenu->addAction(m_upcaseAction);
    m_convertCaseMenu->addAction(m_downcaseAction);
    m_convertCaseMenu->addAction(m_capitalizeAction);

    connect(m_upcaseAction, &QAction::triggered, this, &TextEdit::upcaseWord);
    connect(m_downcaseAction, &QAction::triggered, this, &TextEdit::downcaseWord);
    connect(m_capitalizeAction, &QAction::triggered, this, &TextEdit::capitalizeWord);

    m_canUndo = false;
    m_canRedo = false;

    connect(this, &TextEdit::undoAvailable, this, &TextEdit::slotUndoAvailable);
    connect(this, &TextEdit::redoAvailable, this, &TextEdit::slotRedoAvailable);
    qDebug() << "Right clicked menu initialized";
}

void TextEdit::popRightMenu(QPoint pos)
{
    qDebug() << "Popping right clicked menu";
    //清除菜单分割线残影
    if (m_rightMenu != nullptr) {
        delete m_rightMenu;
        m_rightMenu = nullptr;
    }
    m_rightMenu = new DMenu;

    m_rightMenu->clear();
    QTextCursor selectionCursor = textCursor();
    selectionCursor.movePosition(QTextCursor::StartOfBlock);
    selectionCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    QString text = selectionCursor.selectedText();

    // init base.
    bool isBlankLine = text.trimmed().isEmpty();

    bool isAddUndoRedo = false;
    if (m_pUndoStack->canUndo() && m_bReadOnlyPermission == false && m_readOnlyMode == false) {
        qDebug() << "Adding undo action";
        m_rightMenu->addAction(m_undoAction);
        isAddUndoRedo = true;
    }

    if (m_pUndoStack->canRedo() && m_bReadOnlyPermission == false && m_readOnlyMode == false) {
        qDebug() << "Adding redo action";
        m_rightMenu->addAction(m_redoAction);
        isAddUndoRedo = true;
    }

    if (isAddUndoRedo) {
        qDebug() << "Adding separator";
        m_rightMenu->addSeparator();
    }

    if (textCursor().hasSelection() || m_hasColumnSelection) {
        qDebug() << "Adding cut action";
        if (m_bReadOnlyPermission == false && m_readOnlyMode == false) {
            qDebug() << "Adding cut action";
            m_rightMenu->addAction(m_cutAction);
        }
        m_rightMenu->addAction(m_copyAction);
    }

    if (canPaste()) {
        qDebug() << "Adding paste action";
        if (m_bReadOnlyPermission == false && m_readOnlyMode == false) {
            qDebug() << "Adding paste action";
            m_rightMenu->addAction(m_pasteAction);
        }
    }

    if (textCursor().hasSelection() || m_hasColumnSelection) {
        qDebug() << "Adding delete action";
        if (m_bReadOnlyPermission == false && m_readOnlyMode == false) {
            qDebug() << "Adding delete action";
            m_rightMenu->addAction(m_deleteAction);
        }

    }

    if (!document()->isEmpty()) {
        qDebug() << "Adding select all action";
        m_rightMenu->addAction(m_selectAllAction);
    }

    m_rightMenu->addSeparator();

    if (!document()->isEmpty()) {
        qDebug() << "Adding find action";
        m_rightMenu->addAction(m_findAction);
        if (m_bReadOnlyPermission == false && m_readOnlyMode == false) {
            qDebug() << "Adding replace action";
            m_rightMenu->addAction(m_replaceAction);
        }
        qDebug() << "Adding jump line action";
        m_rightMenu->addAction(m_jumpLineAction);
        m_rightMenu->addSeparator();
    }

    if (textCursor().hasSelection()) {
        qDebug() << "Adding convert case menu";
        if (m_bReadOnlyPermission == false && m_readOnlyMode == false) {
            qDebug() << "Adding convert case menu";
            m_rightMenu->addMenu(m_convertCaseMenu);
        }
    } else {
        qDebug() << "Hiding convert case menu";
        m_convertCaseMenu->hide();
    }

    // intelligent judge whether to support comments.
    const auto def = m_repository.definitionForFileName(QFileInfo(m_sFilePath).fileName());
    if (characterCount() &&
            (textCursor().hasSelection() || !isBlankLine) &&
            !def.filePath().isEmpty()) {
        qDebug() << "Adding add comment action";
        /*
         * 不支持注释的文件类型，右键菜单不显示“添加注释/取消注释”
         * 不支持注释的文件类型：Markdown(.d)/vCard(.vcf)/JSON(.json)
         */
        if (def.name() != "Markdown"
                && !def.name().contains(QString("vCard"))
                && !def.name().contains(QString("JSON"))) {
            qDebug() << "Adding add comment action";
            m_rightMenu->addAction(m_addComment);
            m_rightMenu->addAction(m_cancelComment);
        }
    }

    if (m_bReadOnlyPermission || m_readOnlyMode) {
        qDebug() << "Adding add comment action";
        m_addComment->setEnabled(false);
        m_cancelComment->setEnabled(false);
    } else {
        qDebug() << "Adding add comment action";
        m_addComment->setEnabled(true);
        m_cancelComment->setEnabled(true);
    }

    m_rightMenu->addSeparator();
    if (m_bReadOnlyPermission == false) {
        qDebug() << "Adding read only mode action";
        if (m_readOnlyMode) {
            qDebug() << "Adding disable read only mode action";
            m_rightMenu->addAction(m_disableReadOnlyModeAction);
        } else {
            qDebug() << "Adding enable read only mode action";
            m_rightMenu->addAction(m_enableReadOnlyModeAction);
        }
    }

    m_rightMenu->addAction(m_openInFileManagerAction);
    m_rightMenu->addSeparator();
    if (static_cast<Window *>(this->window())->isFullScreen()) {
        qDebug() << "Adding exit fullscreen action";
        m_rightMenu->addAction(m_exitFullscreenAction);
    } else {
        qDebug() << "Adding fullscreen action";
        m_rightMenu->addAction(m_fullscreenAction);
    }

    // Block ai actions on mips by default.
#ifdef __mips__
    if (IflytekAiAssistant::instance()->valid()) {
        qDebug() << "Adding ai actions";
#endif // __mips__

    // 'UOS AI' actions
    qDebug() << "Adding voice reading action";
    m_rightMenu->addAction(m_voiceReadingAction);
    m_voiceReadingAction->setEnabled((textCursor().hasSelection() || m_hasColumnSelection));

    m_rightMenu->addAction(m_dictationAction);
    m_dictationAction->setEnabled(!(m_bReadOnlyPermission || m_readOnlyMode));

    // temporarily disable text to translate
#ifdef ENABLE_IFLYTEK_TRANSLATE
    m_rightMenu->addAction(m_translateAction);
    m_translateAction->setEnabled((textCursor().hasSelection() || m_hasColumnSelection));
#endif

#ifdef __mips__
    }
#endif  // __mips__

    if (!this->document()->isEmpty()) {
        qDebug() << "Adding color mark menu";

        m_colorMarkMenu->clear();
        // 清空 tab order list
        m_MarkColorMenuTabOrder.clear();

        // 增加 Mark Color Menu Item
        m_colorMarkMenu->addAction(m_markCurrentAct);
        m_colorMarkMenu->addAction(m_actionColorStyles);
        // 将对应 Mark Color Menu Item 加入 Tab Order
        // QPair<QAction*, bool> , bool决定tab过程中是否可以获取focus
        m_MarkColorMenuTabOrder.append(QPair<QAction *, bool>(m_markCurrentAct, true));
        m_MarkColorMenuTabOrder.append(QPair<QAction *, bool>(m_actionColorStyles, false));

        // 增加 Mark Color Menu Item
        m_colorMarkMenu->addSeparator();
        m_colorMarkMenu->addAction(m_markAllAct);
        m_colorMarkMenu->addAction(m_actionAllColorStyles);
        m_colorMarkMenu->addSeparator();
        // 将对应 Mark Color Menu Item 加入 Tab Order
        m_MarkColorMenuTabOrder.append(QPair<QAction *, bool>(m_markAllAct, true));
        m_MarkColorMenuTabOrder.append(QPair<QAction *, bool>(m_actionAllColorStyles, false));

        if (m_markOperations.size() > 0) {
            qDebug() << "Adding cancel last mark action";
            // 增加 Mark Color Menu Item
            m_colorMarkMenu->addAction(m_cancleLastMark);
            m_colorMarkMenu->addSeparator();
            m_colorMarkMenu->addAction(m_cancleMarkAllLine);
            // 将对应 Mark Color Menu Item 加入 Tab Order
            m_MarkColorMenuTabOrder.append(QPair<QAction *, bool>(m_cancleLastMark, true));
            m_MarkColorMenuTabOrder.append(QPair<QAction *, bool>(m_cancleMarkAllLine, true));
        }

        m_rightMenu->addSeparator();
        if (m_bReadOnlyPermission == false && m_readOnlyMode == false) {
            qDebug() << "Adding column edit action";
            m_rightMenu->addAction(m_columnEditAction);
        }
        m_rightMenu->addMenu(m_colorMarkMenu);
    }

    QPoint pos1 = cursorRect().bottomRight();
    //当全选大文本 坐标超出屏幕外显示不了 梁卫东　２０２０－０８－１９　１０：２３：２９
    if (pos1.y() > this->rect().height()) {
        pos1.setY((this->rect().height()) / 2);
        pos1.setX((this->rect().width()) / 2);
    }
    if (pos.isNull()) {
        qDebug() << "pos is null";
        m_rightMenu->exec(mapToGlobal(pos1));
    } else {
        qDebug() << "Executing right menu at position:" << pos;
        m_rightMenu->exec(pos);
    }
}

void TextEdit::setWrapper(EditWrapper *w)
{
    qDebug() << "Setting wrapper";
    m_wrapper = w;
}

EditWrapper *TextEdit::getWrapper()
{
    qDebug() << "Getting wrapper";
    return m_wrapper;
}

bool TextEdit::isUndoRedoOpt()
{
    qDebug() << "Checking undo/redo availability";
    return (m_pUndoStack->canRedo() || m_pUndoStack->canUndo());
}

bool TextEdit::getModified()
{
    qDebug() << "Checking modified status";
    return (document()->isModified() && (m_pUndoStack->canUndo() || m_pUndoStack->index() != m_lastSaveIndex));
}


int TextEdit::getCurrentLine()
{
    qDebug() << "Getting current line";
    return textCursor().blockNumber() + 1;
}

int TextEdit::getCurrentColumn()
{
    qDebug() << "Getting current column";
    return textCursor().columnNumber();
}

int TextEdit::getPosition()
{
    qDebug() << "Getting position";
    return textCursor().position();
}

int TextEdit::getScrollOffset()
{
    qDebug() << "Getting scroll offset";
    return verticalScrollBar()->value();
}

void TextEdit::forwardChar()
{
    qDebug() << "Moving forward by character";
    if (m_cursorMark) {
        qDebug() << "Moving forward by character with mark";
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        qDebug() << "Moving forward by character without mark";
        moveCursorNoBlink(QTextCursor::NextCharacter);
    }
}

void TextEdit::backwardChar()
{
    qDebug() << "Moving backward by character";
    if (m_cursorMark) {
        qDebug() << "Moving backward by character with mark";
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        qDebug() << "Moving backward by character without mark";
        moveCursorNoBlink(QTextCursor::PreviousCharacter);
    }
    qDebug() << "Moving backward by character successfully";
}

void TextEdit::forwardWord()
{
    qDebug() << "Moving forward by word";
    QTextCursor cursor = textCursor();

    if (m_cursorMark) {
        qDebug() << "Moving forward by word with mark";
        cursor.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor);
    } else {
        qDebug() << "Moving forward by word without mark";
        cursor.movePosition(QTextCursor::NextWord, QTextCursor::MoveAnchor);
    }

    setTextCursor(cursor);
    qDebug() << "Moving forward by word successfully";
}

void TextEdit::backwardWord()
{
    qDebug() << "Moving backward by word";
    QTextCursor cursor = textCursor();

    if (m_cursorMark) {
        qDebug() << "Moving backward by word with mark";
        // cursor.setPosition(getPrevWordPosition(cursor, QTextCursor::KeepAnchor), QTextCursor::KeepAnchor);
        cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
    } else {
        qDebug() << "Moving backward by word without mark";
        // cursor.setPosition(getPrevWordPosition(cursor, QTextCursor::MoveAnchor), QTextCursor::MoveAnchor);
        cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::MoveAnchor);
    }

    setTextCursor(cursor);
    qDebug() << "Moving backward by word successfully";
}

void TextEdit::forwardPair()
{
    qDebug() << "Moving forward by pair";
    // Record cursor and seleciton position before move cursor.
    int actionStartPos = textCursor().position();
    int selectionStartPos = textCursor().selectionStart();
    int selectionEndPos = textCursor().selectionEnd();

    // Because find always search start from selection end position.
    // So we need clear selection to make search start from cursor.
    QTextCursor removeSelectionCursor = textCursor();
    removeSelectionCursor.clearSelection();
    setTextCursor(removeSelectionCursor);

    // Start search.
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    qDebug() << "Moving forward by pair with mark";
    QRegExp regExp("[\\]>)}]");
#else
    QRegularExpression regExp("[\\]>)}]");
#endif
    qDebug() << "Moving forward by pair without mark";
    if (find(regExp)) {
        qDebug() << "Moving forward by pair successfully";
        int findPos = textCursor().position();

        QTextCursor cursor = textCursor();
        auto moveMode = m_cursorMark ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;

        if (actionStartPos == selectionStartPos) {
            qDebug() << "Moving forward by pair with mark";
            cursor.setPosition(selectionEndPos, QTextCursor::MoveAnchor);
            cursor.setPosition(findPos, moveMode);
        } else {
            qDebug() << "Moving forward by pair without mark";
            cursor.setPosition(selectionStartPos, QTextCursor::MoveAnchor);
            cursor.setPosition(findPos, moveMode);
        }

        setTextCursor(cursor);
    }
    qDebug() << "Moving forward by pair successfully";
}

void TextEdit::backwardPair()
{
    qDebug() << "Moving backward by pair";
    // Record cursor and seleciton position before move cursor.
    int actionStartPos = textCursor().position();
    int selectionStartPos = textCursor().selectionStart();
    int selectionEndPos = textCursor().selectionEnd();

    // Because find always search start from selection end position.
    // So we need clear selection to make search start from cursor.
    QTextCursor removeSelectionCursor = textCursor();
    removeSelectionCursor.clearSelection();
    setTextCursor(removeSelectionCursor);

    QTextDocument::FindFlags options;
    options |= QTextDocument::FindBackward;

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QRegExp regExp("[\[<({]");
#else
    QRegularExpression regExp("[\[<({]");
#endif
    if (find(regExp, options)) {
        qDebug() << "Moving backward by pair successfully";
        QTextCursor cursor = textCursor();
        auto moveMode = m_cursorMark ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;

        cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor);

        int findPos = cursor.position();

        if (actionStartPos == selectionStartPos) {
            qDebug() << "Moving backward by pair with mark";
            cursor.setPosition(selectionEndPos, QTextCursor::MoveAnchor);
            cursor.setPosition(findPos, moveMode);
            setTextCursor(cursor);
        } else {
            qDebug() << "Moving backward by pair without mark";
            cursor.setPosition(selectionStartPos, QTextCursor::MoveAnchor);
            cursor.setPosition(findPos, moveMode);
            setTextCursor(cursor);
        }
    }
    qDebug() << "Moving backward by pair successfully";
}

int TextEdit::blockCount() const
{
    qDebug() << "Getting block count";
    return document()->blockCount();
}

int TextEdit::characterCount() const
{
    qDebug() << "Getting character count";
    return document()->characterCount();
}

QTextBlock TextEdit::firstVisibleBlock()
{
    qDebug() << "Getting first visible block";
    return document()->findBlockByLineNumber(getFirstVisibleBlockId());
}

void TextEdit::moveToStart()
{
    qDebug() << "Moving to start";
    verticalScrollBar()->setValue(0);
    if (m_cursorMark) {
        qDebug() << "Moving to start with mark";
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Start, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        qDebug() << "Moving to start without mark";
        moveCursorNoBlink(QTextCursor::Start);
    }

    // 移动展示区域，手动高亮文本
    m_wrapper->OnUpdateHighlighter();
    qDebug() << "Moving to start successfully";
}

void TextEdit::moveToEnd()
{
    qDebug() << "Moving to end";
    if (m_cursorMark) {
        qDebug() << "Moving to end with mark";
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        qDebug() << "Moving to end without mark";
        moveCursorNoBlink(QTextCursor::End);
    }

    // 移动展示区域，手动高亮文本
    m_wrapper->OnUpdateHighlighter();
    qDebug() << "Moving to end successfully";
}

void TextEdit::moveToStartOfLine()
{
    qDebug() << "Moving to start of line";
    if (m_cursorMark) {
        qDebug() << "Moving to start of line with mark";
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        qDebug() << "Moving to start of line without mark";
        moveCursorNoBlink(QTextCursor::StartOfBlock);
    }
    qDebug() << "Moving to start of line successfully";
}

void TextEdit::moveToEndOfLine()
{
    qDebug() << "Moving to end of line";
    if (m_cursorMark) {
        qDebug() << "Moving to end of line with mark";
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        qDebug() << "Moving to end of line without mark";
        moveCursorNoBlink(QTextCursor::EndOfBlock);
    }
    qDebug() << "Moving to end of line successfully";
}

void TextEdit::moveToLineIndentation()
{
    qDebug() << "Moving to line indentation";
    // Init cursor and move type.
    QTextCursor cursor = textCursor();
    // Get line start position.
    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
    int startColumn = cursor.columnNumber();
    // Get line end position.
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::MoveAnchor);
    int endColumn = cursor.columnNumber();
    // Move the cursor to line start first while keep the anchor to end of block.
    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
    int column = startColumn;
    while (column < endColumn)
    {
        QChar currentChar = *cursor.selection().toPlainText().data();
        if (!currentChar.isSpace())
        {
            qDebug() << "Moving to line indentation successfully";
            //stop and reset anchor while be at row indentation.
            cursor.setPosition(cursor.position(), QTextCursor::MoveAnchor);
            break;
        }
        else
        {
            qDebug() << "Moving to line indentation with space";
            //while including 'space',just move ahead.
            cursor.setPosition(cursor.position() + 1, QTextCursor::KeepAnchor);
        }
        column++;
    }
    cursor.clearSelection();
    setTextCursor(cursor);
    qDebug() << "Moving to line indentation successfully";
}

void TextEdit::nextLine()
{
    qDebug() << "Moving to next line";
    m_isSelectAll = false;
    if (!characterCount()) {
        qDebug() << "No character count";
        return;
    }

    if (m_cursorMark) {
        qDebug() << "Moving to next line with mark";
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        qDebug() << "Moving to next line without mark";
        moveCursorNoBlink(QTextCursor::Down);
    }

    if (m_wrapper != nullptr) {
        qDebug() << "Updating highlighter";
        m_wrapper->OnUpdateHighlighter();
        if ((m_wrapper->window()->findBarIsVisiable() || m_wrapper->window()->replaceBarIsVisiable()) &&
                (QString::compare(m_wrapper->window()->getKeywordForSearchAll(), m_wrapper->window()->getKeywordForSearch(), Qt::CaseInsensitive) == 0)) {
            qDebug() << "Highlighting keyword in view";
            highlightKeywordInView(m_wrapper->window()->getKeywordForSearchAll());
        }

        qDebug() << "Marking all keyword in view";
        markAllKeywordInView();
    }

    qDebug() << "Moving to next line successfully";
}

void TextEdit::prevLine()
{
    qDebug() << "Moving to previous line";
    m_isSelectAll = false;
    if (!characterCount()) {
        qDebug() << "No character count";
        return;
    }

    if (m_cursorMark) {
        qDebug() << "Moving to previous line with mark";
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Up, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        qDebug() << "Moving to previous line without mark";
        moveCursorNoBlink(QTextCursor::Up);
    }

    if (m_wrapper != nullptr) {
        qDebug() << "Updating highlighter";
        m_wrapper->OnUpdateHighlighter();
        if ((m_wrapper->window()->findBarIsVisiable() || m_wrapper->window()->replaceBarIsVisiable()) &&
                (QString::compare(m_wrapper->window()->getKeywordForSearchAll(), m_wrapper->window()->getKeywordForSearch(), Qt::CaseInsensitive) == 0)) {
            qDebug() << "Highlighting keyword in view";
            highlightKeywordInView(m_wrapper->window()->getKeywordForSearchAll());
        }

        qDebug() << "Marking all keyword in view";
        markAllKeywordInView();
    }
    qDebug() << "Moving to previous line successfully";
}

void TextEdit::moveCursorNoBlink(QTextCursor::MoveOperation operation, QTextCursor::MoveMode mode)
{
    qDebug() << "Moving cursor without blink";
    // Function moveCursorNoBlink will blink cursor when move cursor.
    // But function movePosition won't, so we use movePosition to avoid that cursor link when moving cursor.
    QTextCursor cursor = textCursor();
    cursor.movePosition(operation, mode);
    setTextCursor(cursor);
}

void TextEdit::jumpToLine(int line, bool keepLineAtCenter)
{
    qDebug() << "Jumping to line";
    QTextCursor cursor(document()->findBlockByNumber(line - 1)); // line - 1 because line number starts from 0
    //verticalScrollBar()->setValue(fontMetrics().height() * line - height());
    // Update cursor.
    setTextCursor(cursor);

    if (keepLineAtCenter) {
        qDebug() << "Keeping current line at center";
        keepCurrentLineAtCenter();
    }
    qDebug() << "Jumping to line successfully";
    m_pLeftAreaWidget->m_pLineNumberArea->update();
}

void TextEdit::newline()
{
    qDebug() << "Opening new line";
    // Stop mark if mark is set.
    tryUnsetMark();

    QTextCursor cursor = textCursor();
    auto com = new InsertTextUndoCommand(cursor, "\n", this);
    m_pUndoStack->push(com);
    setTextCursor(cursor);
    qDebug() << "Opening new line successfully";
}

void TextEdit::openNewlineAbove()
{
    qDebug() << "Opening new line above";
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
    InsertTextUndoCommand *com = new InsertTextUndoCommand(cursor, "\n", this);
    m_pUndoStack->push(com);
    cursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor);
    setTextCursor(cursor);
    qDebug() << "Opening new line above successfully";
}

void TextEdit::openNewlineBelow()
{
    qDebug() << "Opening new line below";
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::MoveAnchor);
    InsertTextUndoCommand *com = new InsertTextUndoCommand(cursor, "\n", this);
    m_pUndoStack->push(com);

    //make the vertical scroll bar change together.
    this->setTextCursor(cursor);
    qDebug() << "Opening new line below successfully";
}

/*
 * swap tow lines.
 * firstly,combine the contents of the current line with the contents of the previous or next line.
 * then,insert the combined content.
 * */
void TextEdit::moveLineDownUp(bool up)
{
    qDebug() << "Moving line down/up";
    if (up) {
        qDebug() << "Moving line up";
        QTextCursor cursor = this->textCursor();
        //current line isn't the first line of this document
        if (0 != cursor.blockNumber()) {
            int startpos = 0;
            int endpos = 0;
            cursor.movePosition(QTextCursor::StartOfLine);
            cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
            endpos = cursor.position();
            QString curtext = cursor.selectedText();

            cursor.movePosition(QTextCursor::PreviousBlock);
            startpos = cursor.position();
            cursor.movePosition(QTextCursor::StartOfLine);
            cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
            QString uptext = cursor.selectedText();

            qDebug() << "Moving line down from:" << cursor.blockNumber() << "to:" << cursor.blockNumber()+1;
            cursor.setPosition(startpos);
            cursor.setPosition(endpos, QTextCursor::KeepAnchor);
            InsertTextUndoCommand *com = new InsertTextUndoCommand(cursor, curtext + "\n" + uptext, this);
            m_pUndoStack->push(com);
            qDebug() << "Line move completed";

            //ensure that this operation can be performed multiple times in succession.
            //and make the vertical scroll bar change together at the same time.
            cursor.setPosition(startpos);
            this->setTextCursor(cursor);
        }
    } else {
        qDebug() << "Moving line down";
        QTextCursor cursor = this->textCursor();
        //current line isn't the last line of this document
        if (cursor.blockNumber() + 1 != this->document()->blockCount()) {
            qDebug() << "Moving line down";
            int startpos = 0;
            int endpos = 0;
            cursor.movePosition(QTextCursor::StartOfLine);
            startpos = cursor.position();
            cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
            QString curtext = cursor.selectedText();

            cursor.movePosition(QTextCursor::NextBlock);
            cursor.movePosition(QTextCursor::StartOfLine);
            cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
            endpos = cursor.position();
            QString downtext = cursor.selectedText();

            qDebug() << "Moving line up from:" << cursor.blockNumber()+1 << "to:" << cursor.blockNumber();
            cursor.setPosition(startpos);
            cursor.setPosition(endpos, QTextCursor::KeepAnchor);
            InsertTextUndoCommand *com = new InsertTextUndoCommand(cursor, downtext + "\n" + curtext, this);
            m_pUndoStack->push(com);
            qDebug() << "Line move completed";

            //make the vertical scroll bar change together.
            cursor.setPosition(endpos);
            this->setTextCursor(cursor);
        }
    }

    qDebug() << "Moving line down/up successfully";
}

void TextEdit::scrollLineUp()
{
    qDebug() << "Scrolling line up";
    QScrollBar *scrollbar = verticalScrollBar();

    scrollbar->setValue(scrollbar->value() - 1);

    if (cursorRect().y() > rect().height() - fontMetrics().height()) {
        qDebug() << "Scrolling line up with mark";
        auto moveMode = m_cursorMark ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;

        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Up, moveMode);
        setTextCursor(cursor);
    } else {
        qDebug() << "Scrolling line up without mark";
    }
    qDebug() << "Scrolling line up successfully";
}

void TextEdit::scrollLineDown()
{
    qDebug() << "Scrolling line down";
    QScrollBar *scrollbar = verticalScrollBar();

    scrollbar->setValue(scrollbar->value() + 1);

    if (cursorRect().y() < 0) {
        qDebug() << "Scrolling line down with mark";
        auto moveMode = m_cursorMark ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;

        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Down, moveMode);
        setTextCursor(cursor);
    } else {
        qDebug() << "Scrolling line down without mark";
    }
    qDebug() << "Scrolling line down successfully";
}

void TextEdit::scrollUp()
{
    qDebug() << "Scrolling up";
    QScrollBar *scrollbar = verticalScrollBar();
    scrollbar->triggerAction(QAbstractSlider::SliderPageStepSub);

    m_pLeftAreaWidget->m_pLineNumberArea->update();
    //m_pLeftAreaWidget->m_pFlodArea->update();
    //m_pLeftAreaWidget->m_pBookMarkArea->update();

    if (verticalScrollBar()->maximum() > 0) {
        qDebug() << "Scrolling up with mark";
        auto moveMode = m_cursorMark ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;
        QPoint startPoint = QPointF(0, fontMetrics().height()).toPoint();
        QTextCursor cur = cursorForPosition(startPoint);
        QTextCursor cursor = textCursor();
        cursor.setPosition(cur.position(), moveMode);
        setTextCursor(cursor);
    }

    if (m_wrapper != nullptr) {
        qDebug() << "Updating highlighter";
        m_wrapper->OnUpdateHighlighter();
        if ((m_wrapper->window()->findBarIsVisiable() || m_wrapper->window()->replaceBarIsVisiable()) &&
                (QString::compare(m_wrapper->window()->getKeywordForSearchAll(), m_wrapper->window()->getKeywordForSearch(), Qt::CaseInsensitive) == 0)) {
            qDebug() << "Highlighting keyword in view";
            highlightKeywordInView(m_wrapper->window()->getKeywordForSearchAll());
        }

        markAllKeywordInView();
    }
    qDebug() << "Scrolling up successfully";
}

void TextEdit::scrollDown()
{
    qDebug() << "Scrolling down";
    QScrollBar *scrollbar = verticalScrollBar();
    scrollbar->triggerAction(QAbstractSlider::SliderPageStepAdd);

    m_pLeftAreaWidget->m_pLineNumberArea->update();
    //m_pLeftAreaWidget->m_pFlodArea->update();
    //m_pLeftAreaWidget->m_pBookMarkArea->update();

    if (verticalScrollBar()->maximum() > 0) {
        qDebug() << "Scrolling down with mark";
        auto moveMode = m_cursorMark ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;
        QPoint endPoint = QPointF(0, height() - fontMetrics().height()).toPoint();
        QTextCursor cur = cursorForPosition(endPoint);
        QTextCursor cursor = textCursor();
        cursor.setPosition(cur.position(), moveMode);
        setTextCursor(cursor);
    }

    if (m_wrapper != nullptr) {
        qDebug() << "Updating highlighter";
        m_wrapper->OnUpdateHighlighter();
        if ((m_wrapper->window()->findBarIsVisiable() || m_wrapper->window()->replaceBarIsVisiable()) &&
                (QString::compare(m_wrapper->window()->getKeywordForSearchAll(), m_wrapper->window()->getKeywordForSearch(), Qt::CaseInsensitive) == 0)) {
            qDebug() << "Highlighting keyword in view";
            highlightKeywordInView(m_wrapper->window()->getKeywordForSearchAll());
        }

        markAllKeywordInView();
    }
    qDebug() << "Scrolling down successfully";
}

/*
 * copy the current line
 * firstly,get the text of current line.
 * then,insert the text with '\n' at the end of current line.
 * */
void TextEdit::duplicateLine()
{
    qDebug() << "Duplicating line";
    auto cursor = textCursor();
    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    auto text = cursor.selectedText();

    text = "\n" + text;
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::MoveAnchor);
    auto com = new InsertTextUndoCommand(cursor, text, this);
    m_pUndoStack->push(com);

    //make the vertical scroll bar change together.
    this->setTextCursor(cursor);
    qDebug() << "Duplicating line successfully";
}

void TextEdit::copyLines()
{
    qDebug() << "Copying lines";
    // 添加权限判断是否允许拷贝，剪切；防止后续可能调用接口，冗余处理
    if (!Utils::enableClipCopy(getFilePath())) {
        qDebug() << "Copying lines failed";
        return;
    }

    // Record current cursor and build copy cursor.
    QTextCursor currentCursor = textCursor();
    QTextCursor copyCursor = textCursor();

    if (textCursor().hasSelection()) {
        qDebug() << "Copying lines with selection";
        // Sort selection bound cursors.
        int startPos = textCursor().anchor();
        int endPos = textCursor().position();

        if (startPos > endPos) {
            qDebug() << "Swapping start and end positions";
            std::swap(startPos, endPos);
        }

        // Selectoin multi-lines.
        QTextCursor startCursor = textCursor();
        startCursor.setPosition(startPos, QTextCursor::MoveAnchor);
        startCursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);

        QTextCursor endCursor = textCursor();
        endCursor.setPosition(endPos, QTextCursor::MoveAnchor);
        endCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::MoveAnchor);

        copyCursor.setPosition(startCursor.position(), QTextCursor::MoveAnchor);
        copyCursor.setPosition(endCursor.position(), QTextCursor::KeepAnchor);

        popupNotify(tr("Selected line(s) copied"));
    } else {
        qDebug() << "Copying lines without selection";
        // Selection current line.
        copyCursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        copyCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

        popupNotify(tr("Current line copied"));
    }

    // Copy lines to system clipboard.
    setTextCursor(copyCursor);
    copySelectedText(true);

    // Reset cursor before copy lines.
    copyCursor.setPosition(currentCursor.position(), QTextCursor::MoveAnchor);
    setTextCursor(copyCursor);
    qDebug() << "Copying lines successfully";
}

void TextEdit::cutlines()
{
    qDebug() << "Cutting lines";
    // 添加权限判断是否允许拷贝，剪切；防止后续可能调用接口，冗余处理
    if (!Utils::enableClipCopy(getFilePath())) {
        qDebug() << "Cutting lines failed";
        return;
    }

    if(m_isSelectAll) {
        qDebug() << "Cutting lines with select all";
        QPlainTextEdit::selectAll();
    }

    if (textCursor().hasSelection() || m_bIsAltMod) {
        qDebug() << "Cutting lines with selection";
        this->cut(true);
        popupNotify(tr("Selected line(s) clipped"));
    } else {
        qDebug() << "Cutting lines without selection";
        auto cursor = textCursor();
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        if (cursor.hasSelection()) {
            QString data = cursor.selectedText();
            QUndoCommand *pDeleteStack = new DeleteTextUndoCommand(cursor, this);
            m_pUndoStack->push(pDeleteStack);
            QClipboard *clipboard = QApplication::clipboard();   //获取系统剪贴板指针
            clipboard->setText(data);
            popupNotify(tr("Current line clipped"));
        }
    }
    qDebug() << "Cutting lines successfully";
}

void TextEdit::joinLines()
{
    qDebug() << "Joining lines";
    QTextCursor cursor = this->textCursor();
    //the current line isn't the last line of text.
    if (cursor.blockNumber() + 1 != this->document()->blockCount()) {
        qDebug() << "Joining lines with next line";
        int startpos = 0;
        int endpos = 0;
        cursor.movePosition(QTextCursor::EndOfBlock);
        startpos = cursor.position();
        cursor.movePosition(QTextCursor::NextBlock);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        endpos = cursor.position();
        QString t = cursor.selectedText();

        cursor.setPosition(startpos);
        cursor.setPosition(endpos, QTextCursor::KeepAnchor);
        auto com = new InsertTextUndoCommand(cursor, t, this);
        m_pUndoStack->push(com);

        cursor.setPosition(startpos);
        this->setTextCursor(cursor);
    }
    qDebug() << "Joining lines successfully";
}

void TextEdit::killLine()
{
    qDebug() << "Killing line";
    if (tryUnsetMark()) {
        qDebug() << "Killing line failed";
        return;
    }

    // Remove selection content if has selection.
    if (m_isSelectAll) {
        qDebug() << "Killing line with select all";
        QPlainTextEdit::selectAll();
    }

    if (textCursor().hasSelection()) {
        qDebug() << "Killing line with selection";
        //textCursor().removeSelectedText();
        //deleteSelectTextEx(textCursor());
        deleteSelectTextEx(textCursor(), textCursor().selectedText(), false);
    } else {
        qDebug() << "Killing line without selection";
        auto cursor = this->textCursor();
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

        //the right of current line has no text but it is not the end line of this document
        if (cursor.selectedText().isEmpty() && cursor.blockNumber() + 1 != this->document()->blockCount()) {
            qDebug() << "Killing line with next block";
            cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
        }

        if (!cursor.selectedText().isEmpty()) {
            qDebug() << "Killing line with selected text";
            DeleteBackCommand *com = new DeleteBackCommand(cursor, this);
            m_pUndoStack->push(com);
        }
    }
    qDebug() << "Killing line successfully";
}

void TextEdit::killCurrentLine()
{
    qDebug() << "Killing current line";
    if (tryUnsetMark()) {
        qDebug() << "Killing current line failed";
        return;
    }

    if (m_isSelectAll) {
        qDebug() << "Killing current line with select all";
        QPlainTextEdit::selectAll();
    }

    auto cursor = this->textCursor();
    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
    if (cursor.blockNumber() + 1 != this->document()->blockCount()) {
        qDebug() << "Killing current line with next block";
        cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
    } else {
        qDebug() << "Killing current line with end block";
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    }
    if (!cursor.selectedText().isEmpty()) {
        qDebug() << "Killing current line with selected text";
        DeleteBackCommand *com = new DeleteBackCommand(cursor, this);
        m_pUndoStack->push(com);
    }
    qDebug() << "Killing current line successfully";
}

void TextEdit::killBackwardWord()
{
    qDebug() << "Killing backward word";
    tryUnsetMark();

    if (m_isSelectAll) {
        qDebug() << "Killing backward word with select all";
        QPlainTextEdit::selectAll();
    }

    if (textCursor().hasSelection()) {
        qDebug() << "Killing backward word with selection";
        //textCursor().removeSelectedText();
    } else {
        qDebug() << "Killing backward word without selection";
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
        deleteSelectTextEx(cursor);
    }
    qDebug() << "Killing backward word successfully";
}

void TextEdit::killForwardWord()
{
    qDebug() << "Killing forward word";
    tryUnsetMark();

    if (m_isSelectAll) {
        qDebug() << "Killing forward word with select all";
        QPlainTextEdit::selectAll();
    }

    if (textCursor().hasSelection()) {
        qDebug() << "Killing forward word with selection";
        //textCursor().removeSelectedText();
    } else {
        qDebug() << "Killing forward word without selection";
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor);
        deleteSelectTextEx(cursor);
    }
    qDebug() << "Killing forward word successfully";
}

void TextEdit::indentText()
{
    qDebug() << "Indenting text";
    auto cursor = this->textCursor();
    if (cursor.hasSelection()) {
        qDebug() << "Indenting text with selection";
        //calculate the start postion and the end postion of current selection.
        int pos1 = cursor.position();
        int pos2 = cursor.anchor();
        if (pos1 > pos2)
            std::swap(pos1, pos2);

        //calculate the start line and end line of current selection.
        cursor.setPosition(pos1);
        int line1 = cursor.blockNumber();
        cursor.setPosition(pos2);
        int line2 = cursor.blockNumber();

        //do the indent operation
        auto com = new IndentTextCommand(this, pos1, pos2, line1, line2);
        m_pUndoStack->push(com);
    }
    qDebug() << "Indenting text successfully";
}

void TextEdit::unindentText()
{
    qDebug() << "Unindenting text";
    QTextCursor cursor = this->textCursor();
    cursor.movePosition(QTextCursor::StartOfBlock);
    int pos = cursor.position();
    cursor.setPosition(cursor.position() + 1, QTextCursor::KeepAnchor);

    //the text in front of current line is '\t'.
    if ("\t" == cursor.selectedText()) {
        qDebug() << "Unindenting text with tab";
        DeleteBackCommand *com = new DeleteBackCommand(cursor, this);
        m_pUndoStack->push(com);
    }
    //the text in front of current line is ' '.
    else if (" " == cursor.selectedText()) {
        qDebug() << "Unindenting text with space";
        int startpos = pos;
        int cnt = 0;
        // calculate the number of ' '.
        while (document()->characterAt(pos) == ' ' && cnt < m_tabSpaceNumber) {
            pos++;
            cnt++;
        }
        cursor.setPosition(startpos);
        cursor.setPosition(pos, QTextCursor::KeepAnchor);
        DeleteBackCommand *com = new DeleteBackCommand(cursor, this);
        m_pUndoStack->push(com);
    }
    qDebug() << "Unindenting text successfully";
}

void TextEdit::setTabSpaceNumber(int number)
{
    qDebug() << "Setting tab space number";
    m_tabSpaceNumber = number;
    updateFont();
    //updateLineNumber();
    updateLeftAreaWidget();
    qDebug() << "Setting tab space number successfully";
}

void TextEdit::upcaseWord()
{
    qDebug() << "Uppercasing word";
    tryUnsetMark();
    convertWordCase(UPPER);
    qDebug() << "Uppercasing word successfully";
}

void TextEdit::downcaseWord()
{
    qDebug() << "Lowercasing word";
    tryUnsetMark();
    convertWordCase(LOWER);
    qDebug() << "Lowercasing word successfully";
}

void TextEdit::capitalizeWord()
{
    qDebug() << "Capitalizing word";
    tryUnsetMark();

    convertWordCase(CAPITALIZE);
    qDebug() << "Capitalizing word successfully";
}

void TextEdit::transposeChar()
{
    qDebug() << "Transposing character";
    QTextCursor cursor = this->textCursor();
    int pos = cursor.position();
    cursor.setPosition(pos + 1, QTextCursor::KeepAnchor);
    QString r = cursor.selectedText();
    cursor.setPosition(pos - 1);
    cursor.setPosition(pos, QTextCursor::KeepAnchor);
    QString l = cursor.selectedText();

    if (!l.isEmpty() && !r.isEmpty()) {
        qDebug() << "Transposing character with selected text";
        cursor.setPosition(pos - 1);
        cursor.setPosition(pos + 1, QTextCursor::KeepAnchor);
        auto com = new InsertTextUndoCommand(cursor, r + l, this);
        m_pUndoStack->push(com);
        ensureCursorVisible();
    }
    qDebug() << "Transposing character successfully";
}

void TextEdit::handleCursorMarkChanged(bool mark, QTextCursor cursor)
{
    qDebug() << "Handling cursor mark changed";
    if (mark) {
        qDebug() << "Handling cursor mark changed with mark";
        m_markStartLine = cursor.blockNumber() + 1;
    } else {
        qDebug() << "Handling cursor mark changed without mark";
        m_markStartLine = -1;
    }

    m_pLeftAreaWidget->m_pLineNumberArea->update();
    m_pLeftAreaWidget->m_pBookMarkArea->update();
    qDebug() << "Handling cursor mark changed successfully";
}

void TextEdit::slotValueChanged(int iValue)
{
    qDebug() << "Value changed";
    Q_UNUSED(iValue);
    if (m_isSelectAll) {
        qDebug() << "Value changed with select all";
        this->selectTextInView();
    }

    this->updateLeftAreaWidget();
    qDebug() << "Value changed successfully";
}

void TextEdit::convertWordCase(ConvertCase convertCase)
{
    qDebug() << "Converting word case";
#if 0
    if (textCursor().hasSelection()) {
        qDebug() << "Converting word case with selection";
        QString text = textCursor().selectedText();

        if (convertCase == UPPER) {
            qDebug() << "Converting word case with upper";
            textCursor().insertText(text.toUpper());
        } else if (convertCase == LOWER) {
            qDebug() << "Converting word case with lower";
            textCursor().insertText(text.toLower());
        } else {
            qDebug() << "Converting word case with capitalize";
            textCursor().insertText(capitalizeText(text));
        }
    } else {
        qDebug() << "Converting word case without selection";
        QTextCursor cursor;

        // Move cursor to mouse position first. if have word under mouse pointer.
        if (m_haveWordUnderCursor) {
            qDebug() << "Converting word case with word under cursor";
            setTextCursor(m_wordUnderPointerCursor);
        }

        cursor = textCursor();
        cursor.movePosition(QTextCursor::NoMove, QTextCursor::MoveAnchor);
        cursor.setPosition(getNextWordPosition(cursor, QTextCursor::KeepAnchor), QTextCursor::KeepAnchor);

        QString text = cursor.selectedText();
        if (convertCase == UPPER) {
            qDebug() << "Converting word case with upper";
            cursor.insertText(text.toUpper());
        } else if (convertCase == LOWER) {
            qDebug() << "Converting word case with lower";
            cursor.insertText(text.toLower());
        } else {
            qDebug() << "Converting word case with capitalize";
            cursor.insertText(capitalizeText(text));
        }

        setTextCursor(cursor);

        m_haveWordUnderCursor = false;
    }
#endif

    if (m_isSelectAll) {
        qDebug() << "Converting word case with select all";
        QPlainTextEdit::selectAll();
    }

    if (textCursor().hasSelection()) {
        qDebug() << "Converting word case with selection";
        QString text = textCursor().selectedText();
        if (convertCase == UPPER) {
            qDebug() << "Converting word case with upper";
            text = text.toUpper();
        } else if (convertCase == LOWER) {
            qDebug() << "Converting word case with lower";
            text = text.toLower();
        } else {
            qDebug() << "Converting word case with capitalize";
            text = capitalizeText(text);
        }

        // 如果没有实际文本更改效果，不进行文本替换操作
        if (text != textCursor().selectedText()) {
            qDebug() << "Converting word case with selected text";
            InsertTextUndoCommand *insertCommand = new InsertTextUndoCommand(textCursor(), text, this);
            m_pUndoStack->push(insertCommand);
        }
    } else {
        qDebug() << "Converting word case without selection";
        QTextCursor cursor;

        // Move cursor to mouse position first. if have word under mouse pointer.
        if (m_haveWordUnderCursor) {
            qDebug() << "Converting word case with word under cursor";
            setTextCursor(m_wordUnderPointerCursor);
        }

        cursor = textCursor();
        cursor.movePosition(QTextCursor::NoMove, QTextCursor::MoveAnchor);
        cursor.setPosition(getNextWordPosition(cursor, QTextCursor::KeepAnchor), QTextCursor::KeepAnchor);

        QString text = cursor.selectedText();
        if (!text.isEmpty()) {
            if (convertCase == UPPER) {
                qDebug() << "Converting word case with upper";
                text = text.toUpper();
            } else if (convertCase == LOWER) {
                qDebug() << "Converting word case with lower";
                text = text.toLower();
            } else {
                qDebug() << "Converting word case with capitalize";
                text = capitalizeText(text);
            }

            qDebug() << "Converting word case with selected text";
            InsertTextUndoCommand *insertCommand = new InsertTextUndoCommand(cursor, text, this);
            m_pUndoStack->push(insertCommand);

            setTextCursor(cursor);

            m_haveWordUnderCursor = false;
        }
    }
    qDebug() << "Converting word case successfully";
}

QString TextEdit::capitalizeText(QString text)
{
    qDebug() << "Capitalizing text";
    QString newText = text.toLower();
    QChar currentChar;
    QChar nextChar;
    if (!newText.at(0).isSpace()) {
        newText.replace(0, 1, newText.at(0).toUpper());
    }

    for (int i = 0; i < newText.size(); i++) {
        currentChar = newText.at(i);
        if (i + 1 < newText.size()) {
            qDebug() << "Capitalizing text with next char";
            nextChar = newText.at(i + 1);
        }
        if (currentChar.isSpace() && !nextChar.isSpace()) {
            qDebug() << "Capitalizing text with space and next char";
            newText.replace(i + 1, 1, nextChar.toUpper());
        }
    }

    qDebug() << "Capitalizing text successfully";
    return newText;
}

void TextEdit::keepCurrentLineAtCenter()
{
    qDebug() << "Keeping current line at center";
    QScrollBar *scrollbar = verticalScrollBar();

    int currentLine = cursorRect().top() / cursorRect().height();
    int halfEditorLines = rect().height() / 2 / cursorRect().height();
    scrollbar->setValue(scrollbar->value() + currentLine - halfEditorLines);
    qDebug() << "Keeping current line at center successfully";
}

void TextEdit::scrollToLine(int scrollOffset, int row, int column)
{
    qDebug() << "Scrolling to line";
    // Save cursor postion.
    m_restoreRow = row;
    m_restoreColumn = column;

    // Start scroll animation.
    m_scrollAnimation->setStartValue(verticalScrollBar()->value());
    m_scrollAnimation->setEndValue(scrollOffset);
    m_scrollAnimation->start();
    qDebug() << "Scrolling to line successfully";
}

void TextEdit::setLineWrapMode(bool enable)
{
    qDebug() << "Setting line wrap mode";
    QTextCursor cursor = textCursor();
    int nJumpLine = textCursor().blockNumber() + 1;
    this->setWordWrapMode(QTextOption::WrapAnywhere);
    QPlainTextEdit::setLineWrapMode(enable ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
    m_pLeftAreaWidget->m_pLineNumberArea->update();
    m_pLeftAreaWidget->m_pFlodArea->update();
    m_pLeftAreaWidget->m_pBookMarkArea->update();

    jumpToLine(nJumpLine, false);
    setTextCursor(cursor);
    qDebug() << "Setting line wrap mode successfully";
}

void TextEdit::setFontFamily(QString name)
{
    qDebug() << "Setting font family";
    // Update font.
    m_fontName = name;
    updateFont();
    updateLeftAreaWidget();
    qDebug() << "Setting font family successfully";
}

void TextEdit::setFontSize(qreal size)
{
    qDebug() << "Setting font size";
    // Update font.
    m_fontSize = size;
    updateFont();

    // Update line number after adjust font size.
    updateLeftAreaWidget();
    qDebug() << "Setting font size successfully";
}

void TextEdit::updateFont()
{
    qDebug() << "Updating font";
    QFont font = document()->defaultFont();
    font.setFixedPitch(true);
    font.setPointSizeF(m_fontSize);
    font.setFamily(m_fontName);
    setFont(font);
    setTabStopDistance(m_tabSpaceNumber * QFontMetrics(font).horizontalAdvance(QChar(0x2192)));

    if (m_isSelectAll) {
        qDebug() << "Updating font with select all";
        selectTextInView();
    }
    qDebug() << "Updating font successfully";
}

void TextEdit::replaceAll(const QString &replaceText, const QString &withText, Qt::CaseSensitivity caseFlag)
{
    qDebug() << "Starting replace all operation";
    
    if (m_readOnlyMode || m_bReadOnlyPermission) {
        qDebug() << "Replace cancelled - read only mode";
        return;
    }

    if (replaceText.isEmpty()) {
        qDebug() << "Replace cancelled - empty search text";
        return;
    }

    // 替换文本相同，返回
    if (replaceText == withText) {
        qDebug() << "Replace cancelled - same replace text";
        return;
    }

    qDebug() << "Replacing all occurrences of:" << replaceText.left(20) << "with:" << withText.left(20);

    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::Start);
    QTextCursor startCursor = textCursor();

    QString oldText = this->toPlainText();

    // 保存旧的标记索引光标记录信息，只需要更新其坐标偏移信息即可
    QList<TextEdit::MarkReplaceInfo> backupMarkList = convertMarkToReplace(m_markOperations);
    auto replaceList = backupMarkList;
    // 计算替换颜色标记信息
    calcMarkReplaceList(replaceList, oldText, replaceText, withText, 0, caseFlag);

    QString newText = oldText;
    newText.replace(replaceText, withText, caseFlag);

    if (oldText != newText) {
        qDebug() << "Replace all operation with old text and new text";
        ChangeMarkCommand *pChangeMark = new ChangeMarkCommand(this, backupMarkList, replaceList);
        // 设置替换撤销项为颜色标记变更撤销项的子项
        new ReplaceAllCommand(oldText, newText, cursor, pChangeMark);
        m_pUndoStack->push(pChangeMark);
    }
    qDebug() << "Replace all operation completed";
}

void TextEdit::replaceNext(const QString &replaceText, const QString &withText, Qt::CaseSensitivity caseFlag)
{
    qDebug() << "Starting replace next operation";
    if (m_readOnlyMode || m_bReadOnlyPermission) {
        qDebug() << "Replace next operation cancelled - read only mode";
        return;
    }

    if (m_isSelectAll) {
        qDebug() << "Replace next operation with select all";
        QPlainTextEdit::selectAll();
    }

    if (replaceText.isEmpty() || !m_findHighlightSelection.cursor.hasSelection()) {
        qDebug() << "Replace next operation cancelled - empty search text";
        //无限替换的根源
        return;
    }

    QTextCursor cursor = textCursor();

    if (m_cursorStart != -1) {
        qDebug() << "Replace next operation with cursor start";
        cursor.setPosition(m_cursorStart);
        m_cursorStart = -1;
    } else {
        qDebug() << "Replace next operation with find highlight selection";
        cursor.setPosition(m_findHighlightSelection.cursor.selectionStart());
    }
    cursor.movePosition(QTextCursor::NoMove, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, replaceText.size());

    // 文本替换长度变更调整量
    int adjustlen = withText.size() - replaceText.size();
    // 保存旧的标记索引光标记录信息，只需要更新其坐标偏移信息即可
    QList<TextEdit::MarkReplaceInfo> backupMarkList = convertMarkToReplace(m_markOperations);
    auto replaceList = backupMarkList;
    for (auto &info : replaceList) {
        qDebug() << "Replace next operation with mark info";
        if (MarkAll == info.opt.type
                || MarkAllMatch == info.opt.type) {
            qDebug() << "Replace next operation with mark info - mark all";
            continue;
        }

        // 获取替换文本区域和颜色标记区域的交叉关系
        Utils::RegionIntersectType type = Utils::checkRegionIntersect(
                                              cursor.selectionStart(), cursor.selectionStart() + replaceText.size(), info.start, info.end);
        // 仅进行单次处理
        switch (type) {
        case Utils::ELeft:
            // 当前无交集，颜色标记在替换文本左侧，表示当前颜色标记已经经过
            qDebug() << "Replace next operation with mark info - left";
            break;
        case Utils::ERight: {
            // 颜色标记位于右侧
            qDebug() << "Replace next operation with mark info - right";
            info.start += adjustlen;
            info.end += adjustlen;
            break;
        }
        case Utils::EIntersectLeft: {
            // 交集在替换文本左侧，拓展颜色标记右侧到替换文本右侧
            qDebug() << "Replace next operation with mark info - intersect left";
            info.end = cursor.selectionStart() + withText.size();
            break;
        }
        case Utils::EIntersectRight: {
            // 交集在替换文本右侧，拓展颜色标记左侧到替换文本左侧
            qDebug() << "Replace next operation with mark info - intersect right";
            info.start = cursor.selectionStart();
            info.end += adjustlen;
            break;
        }
        case Utils::EIntersectOutter: {
            // 标记内容包含替换文本
            qDebug() << "Replace next operation with mark info - intersect outter";
            info.end += adjustlen;
            break;
        }
        case Utils::EIntersectInner: {
            // 替换文本内容包含标记信息, 取消当前文本标记（无论单个文本还是单行文本，均移除）
            // 在 manualUpdateAllMark() 函数处理会移除此标记
            qDebug() << "Replace next operation with mark info - intersect inner";
            info.start = 0;
            info.end = 0;
            break;
        }
        default:
            qDebug() << "Replace next operation with mark info - default";
            break;
        }
    }

    QString strSelection(cursor.selectedText());
    if (!strSelection.compare(replaceText, caseFlag) || replaceText.contains("\n")) {
        qDebug() << "Replace next operation with mark info - replace text";
        ChangeMarkCommand *pChangeMark = new ChangeMarkCommand(this, backupMarkList, replaceList);
        // 设置插入撤销项为颜色标记变更撤销项的子项
        new InsertTextUndoCommand(cursor, withText, this, pChangeMark);
        m_pUndoStack->push(pChangeMark);
        ensureCursorVisible();
    }

    // Update cursor.
    setTextCursor(cursor);
    highlightKeyword(replaceText, getPosition(), caseFlag);
    qDebug() << "Replace all operation completed for:" << replaceText.left(20);
}

void TextEdit::replaceRest(const QString &replaceText, const QString &withText, Qt::CaseSensitivity caseFlag)
{
    qDebug() << "Starting replace rest operation";
    
    if (m_readOnlyMode || m_bReadOnlyPermission) {
        qDebug() << "Replace cancelled - read only mode";
        return;
    }

    // If replace text is nothing, don't do replace action.
    if (replaceText.isEmpty()) {
        qDebug() << "Replace cancelled - empty search text";
        return;
    }

    // 替换文本相同，返回
    if (replaceText == withText) {
        qDebug() << "Replace cancelled - same replace text";
        return;
    }

    qDebug() << "Replacing rest occurrences of:" << replaceText.left(20) << "with:" << withText.left(20);

    QTextCursor cursor = textCursor();
    QTextCursor startCursor = textCursor();
    startCursor.beginEditBlock();

    int pos = cursor.position();
    QString oldText = this->toPlainText();
    QString newText = oldText.left(pos);
    QString right = oldText.right(oldText.size() - pos);

    // 保存旧的标记索引光标记录信息，只需要更新其坐标偏移信息即可
    QList<TextEdit::MarkReplaceInfo> backupMarkList = convertMarkToReplace(m_markOperations);
    auto replaceList = backupMarkList;
    // 计算替换颜色标记信息
    calcMarkReplaceList(replaceList, right, replaceText, withText, pos, caseFlag);

    right.replace(replaceText, withText, caseFlag);
    newText += right;

    if (oldText != newText) {
        qDebug() << "Replace rest operation with old text and new text";
        ChangeMarkCommand *pChangeMark = new ChangeMarkCommand(this, backupMarkList, replaceList);
        // 设置替换撤销项为颜色标记变更撤销项的子项
        new ReplaceAllCommand(oldText, newText, cursor, pChangeMark);
        m_pUndoStack->push(pChangeMark);
    }
    qDebug() << "Replace rest operation completed";
    startCursor.endEditBlock();
    setTextCursor(startCursor);
    qDebug() << "Replace rest operation completed for:" << replaceText.left(20);
}

void TextEdit::beforeReplace(const QString &strReplaceText, Qt::CaseSensitivity caseFlag)
{
    qDebug() << "Starting before replace operation";
    if (strReplaceText.isEmpty() || !m_findHighlightSelection.cursor.hasSelection()) {
        qDebug() << "Before replace operation with empty search text";
        highlightKeyword(strReplaceText, getPosition(), caseFlag);
    }
    qDebug() << "Before replace operation completed";
}

bool TextEdit::findKeywordForward(const QString &keyword)
{
    qDebug() << "Finding keyword forward:" << keyword.left(20);

    if (textCursor().hasSelection()) {
        qDebug() << "Finding keyword forward with selection";
        // Get selection bound.
        int startPos = textCursor().anchor();
        int endPos = textCursor().position();

        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
        //setTextCursor(cursor);

        QTextDocument::FindFlags options;
        if (Qt::CaseSensitive == defaultCaseSensitive) {
            qDebug() << "Finding keyword forward with case sensitive";
            options |= QTextDocument::FindCaseSensitively;
        }
        bool foundOne = find(keyword, options);

        cursor.setPosition(endPos, QTextCursor::MoveAnchor);
        cursor.setPosition(startPos, QTextCursor::KeepAnchor);
        //setTextCursor(cursor);

        qDebug() << "Finding keyword forward with selection successfully: " << foundOne;
        return foundOne;
    } else {
        qDebug() << "Finding keyword forward without selection";
        QTextCursor recordCursor = textCursor();

        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
        //setTextCursor(cursor);

        QTextDocument::FindFlags options;
        if (Qt::CaseSensitive == defaultCaseSensitive) {
            qDebug() << "Finding keyword forward with case sensitive";
            options |= QTextDocument::FindCaseSensitively;
        }
        bool foundOne = find(keyword, options);

        //setTextCursor(recordCursor);

        qDebug() << "Finding keyword forward without selection successfully: " << foundOne;
        return foundOne;
    }
}

void TextEdit::removeKeywords()
{
    qDebug() << "Removing keywords";
    m_findHighlightSelection.cursor = textCursor();
    m_findHighlightSelection.cursor.clearSelection();

    m_findMatchSelections.clear();

    updateHighlightLineSelection();

    renderAllSelections();

    qDebug() << "Removing keywords successfully";
    //setFocus();
}

bool TextEdit::highlightKeyword(const QString &keyword, int position, Qt::CaseSensitivity caseFlag)
{
    qDebug() << "Highlighting keyword";
    Q_UNUSED(position)
    m_findMatchSelections.clear();
    updateHighlightLineSelection();
    updateCursorKeywordSelection(keyword, true);
    bool bRet = updateKeywordSelectionsInView(keyword, m_findMatchFormat, &m_findMatchSelections, caseFlag);
    renderAllSelections();

    qDebug() << "Highlighting keyword successfully: " << bRet;
    return bRet;
}

bool TextEdit::highlightKeywordInView(const QString &keyword, Qt::CaseSensitivity caseFlag)
{
    qDebug() << "Highlighting keyword in view";
    m_findMatchSelections.clear();
    bool bRet = updateKeywordSelectionsInView(keyword, m_findMatchFormat, &m_findMatchSelections, caseFlag);
    // 直接设置 setExtraSelections 会导致无法显示颜色标记，调用 renderAllSelections 进行显示更新
    // setExtraSelections(m_findMatchSelections);
    renderAllSelections();

    qDebug() << "Highlighting keyword in view successfully: " << bRet;
    return bRet;
}

void TextEdit::clearFindMatchSelections()
{
    qDebug() << "Clearing find match selections";
    m_findMatchSelections.clear();
}

void TextEdit::setFindHighlightSelection(const QTextCursor &cursor)
{
    qDebug() << "Setting find highlight selection";
    m_findHighlightSelection.cursor = cursor;
    qDebug() << "Find highlight selection set to position:" << cursor.position() << "with selection:" << cursor.hasSelection();
}

void TextEdit::updateCursorKeywordSelection(QString keyword, bool findNext)
{
    qDebug() << "Updating cursor keyword selection";
    bool findOne = searchKeywordSeletion(keyword, textCursor(), findNext);

    qDebug() << "Updating cursor keyword selection successfully: " << findOne;
    if (!findOne) {
        qDebug() << "Updating cursor keyword selection with no find";
        QTextCursor cursor = textCursor();
        cursor.movePosition(findNext ? QTextCursor::Start : QTextCursor::End, QTextCursor::MoveAnchor);
        if (!searchKeywordSeletion(keyword, cursor, findNext)) {
            m_findHighlightSelection.cursor = textCursor();
            m_findMatchSelections.clear();
            renderAllSelections();
            qDebug() << "Updating cursor keyword selection with no find successfully";
        }
    }
    qDebug() << "Updating cursor keyword selection completed";
}

void TextEdit::updateHighlightLineSelection()
{
    qDebug() << "Updating highlight line selection";
    if (m_gestureAction == GA_slide) {
        QTextCursor textCursor = QPlainTextEdit::textCursor();
        qDebug() << "Updating highlight line selection with gesture action slide";
        return;
    }

    QTextEdit::ExtraSelection selection;

    selection.format.setBackground(m_currentLineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();
    m_currentLineSelection = selection;
    qDebug() << "Updating highlight line selection completed";
}

bool TextEdit::updateKeywordSelections(QString keyword, QTextCharFormat charFormat, QList<QTextEdit::ExtraSelection> &listSelection)
{
    qDebug() << "Updating keyword selections";
    // Clear keyword selections first.
    listSelection.clear();

    // Update selections with keyword.
    if (!keyword.isEmpty()) {
        qDebug() << "Updating keyword selections with keyword";
        QTextCursor cursor(document());
        QTextDocument::FindFlags flags;
        if (Qt::CaseSensitive == defaultCaseSensitive) {
            flags |= QTextDocument::FindCaseSensitively;
        }
        QTextEdit::ExtraSelection extra;
        extra.format = charFormat;
        cursor = document()->find(keyword, cursor, flags);

        if (cursor.isNull()) {
            qDebug() << "Updating keyword selections with no find";
            return false;
        }

        while (!cursor.isNull()) {
            extra.cursor = cursor;
            listSelection.append(extra);
            cursor = document()->find(keyword, cursor, flags);
        }

        qDebug() << "Updating keyword selections with keyword successfully";
        return true;
    }

    qDebug() << "Updating keyword selections with no keyword";
    return false;
}

bool TextEdit::updateKeywordSelectionsInView(QString keyword, QTextCharFormat charFormat,
                                             QList<QTextEdit::ExtraSelection> *listSelection, Qt::CaseSensitivity caseFlag)
{
    qDebug() << "Updating keyword selections in view";
    // Clear keyword selections first.
    listSelection->clear();

    // Update selections with keyword.
    if (!keyword.isEmpty()) {
        qDebug() << "Updating keyword selections in view with keyword";
        QTextCursor cursor(document());
        QTextEdit::ExtraSelection extra;
        extra.format = charFormat;

        QScrollBar *pScrollBar = verticalScrollBar();
        QPoint startPoint = QPointF(0, 0).toPoint();
        QTextBlock beginBlock = cursorForPosition(startPoint).block();
        int beginPos = beginBlock.position();
        QTextBlock endBlock;

        if (pScrollBar->maximum() > 0) {
            qDebug() << "Updating keyword selections in view with scroll bar maximum";
            QPoint endPoint = QPointF(0, 1.5 * height()).toPoint();
            endBlock = cursorForPosition(endPoint).block();
        } else {
            qDebug() << "Updating keyword selections in view with no scroll bar maximum";
            endBlock = document()->lastBlock();
        }
        int endPos = endBlock.position() + endBlock.length() - 1;

        // 内部计算时，均视为 \n 结尾
        QLatin1Char endLine('\n');
        QString multiLineText;
        QTextDocument::FindFlags flags;
        if (Qt::CaseSensitive == caseFlag) {
            qDebug() << "Updating keyword selections in view with case sensitive";
            flags |= QTextDocument::FindCaseSensitively;
        }
        if (keyword.contains(endLine)) {
            qDebug() << "Updating keyword selections in view with end line";
            auto temp = this->textCursor();
            temp.setPosition(beginPos);
            while (temp.position() < endPos) {
                temp.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
                multiLineText += temp.selectedText();
                multiLineText += endLine;
                temp.setPosition(temp.position() + 1);
            }
            cursor = findCursor(keyword, multiLineText, 0, false, beginPos, caseFlag);
        } else {
            qDebug() << "Updating keyword selections in view with no end line";
            cursor = document()->find(keyword, beginPos, flags);
        }

        if (cursor.isNull()) {
            qDebug() << "Updating keyword selections in view with no find";
            return false;
        }

        while (!cursor.isNull()) {
            extra.cursor = cursor;
            // 调整为不区分大小写
            Qt::CaseSensitivity option = defaultCaseSensitive;
            /* 查找字符时，查找到完全相等的时候才高亮，如查找小写f时，大写的F不高亮 */
            if (!extra.cursor.selectedText().compare(keyword, option) || keyword.contains(endLine, option)) {
                qDebug() << "Updating keyword selections in view with selected text";
                listSelection->append(extra);
            }

            if (keyword.contains(endLine)) {
                int pos = std::max(extra.cursor.position(), extra.cursor.anchor());
                qDebug() << "Updating keyword selections in view with max pos";
                cursor = findCursor(keyword, multiLineText, pos - beginPos, false, beginPos, caseFlag);
            } else {
                qDebug() << "Updating keyword selections in view with no end line";
                cursor = document()->find(keyword, cursor, flags);
            }

            if (cursor.position() > endPos) {
                qDebug() << "Updating keyword selections in view with position greater than end pos, break";
                break;
            }
        }

        qDebug() << "Updating keyword selections in view with keyword successfully";
        return true;
    }

    qDebug() << "Updating keyword selections in view with no keyword";
    return false;
}

bool TextEdit::searchKeywordSeletion(QString keyword, QTextCursor cursor, bool findNext)
{
    qDebug() << "Searching keyword seletion";
    if (keyword.isEmpty()) {
        qDebug() << "Searching keyword seletion with empty keyword";
        return false;
    }

    bool ret = false;
    int offsetLines = 3;

    if (findNext) {
        qDebug() << "Searching keyword seletion with find next";
        QTextDocument::FindFlags options;
        if (Qt::CaseSensitive == defaultCaseSensitive) {
            qDebug() << "Searching keyword seletion with find next and case sensitive";
            options |= QTextDocument::FindCaseSensitively;
        }

        QTextCursor next = document()->find(keyword, cursor, options);
        if (keyword.contains("\n")) {
            qDebug() << "Searching keyword seletion with find next and contains newline";
            int pos = std::max(cursor.position(), cursor.anchor());
            next = findCursor(keyword, this->toPlainText(), pos, false, 0, defaultCaseSensitive);
        }
        if (!next.isNull()) {
            qDebug() << "Searching keyword seletion with find next and not null";
            m_findHighlightSelection.cursor = next;
            jumpToLine(next.blockNumber() + offsetLines, false);
            setTextCursor(next);
            ret = true;
        }
    } else {
        qDebug() << "Searching keyword seletion with find previous";
        QTextDocument::FindFlags options = QTextDocument::FindBackward;
        if (Qt::CaseSensitive == defaultCaseSensitive) {
            qDebug() << "Searching keyword seletion with find previous and case sensitive";
            options |= QTextDocument::FindCaseSensitively;
        }

        QTextCursor prev = document()->find(keyword, cursor, options);
        if (keyword.contains("\n")) {
            int pos = std::min(cursor.position(), cursor.anchor());
            qDebug() << "Searching keyword seletion with find previous and contains newline";
            prev = findCursor(keyword, this->toPlainText().mid(0, pos), -1, true, defaultCaseSensitive);
        }
        if (!prev.isNull()) {
            qDebug() << "Searching keyword seletion with find previous and not null";
            m_findHighlightSelection.cursor = prev;
            jumpToLine(prev.blockNumber() + offsetLines, false);
            setTextCursor(prev);
            ret = true;
        }
    }

    qDebug() << "Searching keyword seletion completed: " << ret;
    return ret;
}

void TextEdit::renderAllSelections()
{
    qDebug() << "Rendering all selections";
    QList<QTextEdit::ExtraSelection> finalSelections;
    QList<QPair<QTextEdit::ExtraSelection, qint64>> selectionsSortList;

    // 标记当前行的浅灰色
    if (m_HightlightYes) {
        qDebug() << "Rendering all selections with highlight yes";
        finalSelections.append(m_currentLineSelection);
    }
    // 此处代码无作用，去除
    // else {
    //     selections.clear();
    // }

    // 按代码逻辑，推测m_markAllSelection 用于还原所有文本的背景颜色, 实际不需要
    // 考虑到各平台运行之间可能存在运行差异，暂不清除
    // finalSelections.append(m_markAllSelection);

    // 选中区域的颜色标记，先加入 selectionsSortList
    // 通过时间戳升序排序后，加入到 finalSelections 中
    selectionsSortList.append(m_wordMarkSelections);

    // Find 和 Replace 高亮选中，移动到最后放入到 finalSelections 中
    // 保证此高亮状态，若存在，一定可以被用户看到
    // selections.append(m_findMatchSelections);
    // selections.append(m_findHighlightSelection);

    // 不再使用，注释掉
    // selections.append(m_wordUnderCursorSelection);

    // 标记括号移动到处理完颜色标记后，插入到 finalSelections 中
    // selections.append(m_beginBracketSelection);
    // selections.append(m_endBracketSelection);

    // 仅对代码文件有效，标记代码中的代码段，例如函数内{}所有内容
    // 移动到处理完颜色标记后，插入到 finalSelections 中，插入晚于括号
    // selections.append(m_markFoldHighLightSelections);

    // Alt选中区域的高亮, 移动到排序后面加入
    // selections.append(m_altModSelections);

    // 将颜色标记，标记所有的 selections 加入到 selectionsSortList 中， 后边将进行排序
    QMap<QString, QList<QPair<QTextEdit::ExtraSelection, qint64>>>::Iterator it;
    for (it = m_mapKeywordMarkSelections.begin(); it != m_mapKeywordMarkSelections.end(); ++it) {
        selectionsSortList.append(it.value());
    }

    // 通过时间戳重新排序颜色标记功能的 selections
    std::sort(selectionsSortList.begin(), selectionsSortList.end(), [](const QPair<QTextEdit::ExtraSelection, qint64> &A, const QPair<QTextEdit::ExtraSelection, qint64> &B) {
        return (A.second < B.second);
    });

    // 将排序后的颜色标记 selections 加入到 finalSelections 中
    for (int i = 0; i < selectionsSortList.size(); i++) {
        finalSelections.append(selectionsSortList.at(i).first);
    }

    // 标记括号
    finalSelections.append(m_beginBracketSelection);
    finalSelections.append(m_endBracketSelection);

    // 标记代码段
    finalSelections.append(m_markFoldHighLightSelections);

    // Alt选中区域的高亮
    finalSelections.append(m_altModSelections);

    // 查找替换的高亮需要放在最后
    // Find 高亮
    finalSelections.append(m_findMatchSelections);
    // Replace 高亮
    finalSelections.append(m_findHighlightSelection);

    // 设置到 QPlainText 中进行渲染
    setExtraSelections(finalSelections);
    qDebug() << "Rendering all selections completed";
}

void TextEdit::updateMarkAllSelectColor()
{
    qDebug() << "Updating mark all select color";
    isMarkAllLine(m_bIsMarkAllLine, m_strMarkAllLineColorName);
    renderAllSelections();
    qDebug() << "Updating mark all select color completed";
}

DMenu *TextEdit::getHighlightMenu()
{
    qDebug() << "Getting highlight menu";
    return m_hlGroupMenu;
}

void TextEdit::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    qDebug() << "Painting line number area";
    QPainter painter(m_pLeftAreaWidget->m_pLineNumberArea);
    QColor lineNumberAreaBackgroundColor;

    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::ColorType::DarkType) {
        qDebug() << "Painting line number area with dark type";
        lineNumberAreaBackgroundColor = palette().brightText().color();
        lineNumberAreaBackgroundColor.setAlphaF(0.06);

        m_lineNumbersColor.setAlphaF(0.2);
    } else {
        qDebug() << "Painting line number area with light type";
        lineNumberAreaBackgroundColor = palette().brightText().color();
        lineNumberAreaBackgroundColor.setAlphaF(0.03);
        m_lineNumbersColor.setAlphaF(0.3);
    }
    //painter.fillRect(event->rect(), lineNumberAreaBackgroundColor);

    int blockNumber = getFirstVisibleBlockId();
    QTextBlock block = document()->findBlockByNumber(blockNumber);

    int top = this->viewport()->geometry().top() + verticalScrollBar()->value();
    int bottom = top + static_cast<int>(document()->documentLayout()->blockBoundingRect(block).height());

    Utils::setFontSize(painter, document()->defaultFont().pointSize() - 2);
    QPoint endPoint;

    if (verticalScrollBar()->maximum() > 0) {
        qDebug() << "Painting line number area with scroll bar maximum";
        endPoint = QPointF(0, height() + height() / verticalScrollBar()->maximum() * verticalScrollBar()->value()).toPoint();
    }

    QTextCursor cur = cursorForPosition(endPoint);
    QTextBlock endBlock = cur.block();
    int nPageLine = endBlock.blockNumber();
    int nStartLine = block.blockNumber();

    if (verticalScrollBar()->maximum() == 0) {
        nPageLine = blockCount() - 1;
        qDebug() << "Painting line number area with no scroll bar maximum";
    }

    auto currentCursor = this->textCursor();
    cur = textCursor();
    for (int i = nStartLine; i <= nPageLine; i++) {
        if (i + 1 == m_markStartLine) {
            qDebug() << "Painting line number area with mark start line";
            painter.setPen(m_regionMarkerColor);
        } else {
            qDebug() << "Painting line number area with no mark start line";
            painter.setPen(m_lineNumbersColor);
        }

        m_fontLineNumberArea.setPointSize(font().pointSize() - 1);
        painter.setFont(m_fontLineNumberArea);

        cur.setPosition(block.position(), QTextCursor::MoveAnchor);

        if (block.isVisible()) {
            qDebug() << "Painting line number area with block visible";
            int w = this->m_fontSize <= 15 ? 15 : m_fontSize;
            updateLeftWidgetWidth(w);
            int offset = 0;
            //the language currently set by the system is Tibetan.
            if ("bo_CN" == Utils::getSystemLan()) {
                offset = 2;
            }
            if (cur.blockNumber() == currentCursor.blockNumber()) {
                qDebug() << "Painting line number area with current cursor block number";
                painter.setPen(qApp->palette().highlight().color());
            }
            painter.drawText(0, cursorRect(cur).y() + offset,
                             m_pLeftAreaWidget->m_pLineNumberArea->width(), cursorRect(cur).height(),
                             Qt::AlignVCenter | Qt::AlignHCenter, QString::number(block.blockNumber() + 1));
        }

        block = block.next();
        top = bottom/* + document()->documentMargin()*/;
        bottom = top + static_cast<int>(document()->documentLayout()->blockBoundingRect(block).height());
    }
    qDebug() << "Painting line number area completed";
}

void TextEdit::codeFLodAreaPaintEvent(QPaintEvent *event)
{
    qDebug() << "Painting code flod area";
    m_listFlodIconPos.clear();
    QPainter painter(m_pLeftAreaWidget->m_pFlodArea);

    QColor codeFlodAreaBackgroundColor;
    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::ColorType::DarkType) {
        qDebug() << "Painting code flod area with dark type";
        codeFlodAreaBackgroundColor = palette().brightText().color();
        codeFlodAreaBackgroundColor.setAlphaF(0.06);

        m_lineNumbersColor.setAlphaF(0.2);
    } else {
        qDebug() << "Painting code flod area with light type";
        codeFlodAreaBackgroundColor = palette().brightText().color();
        codeFlodAreaBackgroundColor.setAlphaF(0.03);
        m_lineNumbersColor.setAlphaF(0.3);
    }

    int blockNumber = getFirstVisibleBlockId();
    QTextBlock block = document()->findBlockByNumber(blockNumber);

    DGuiApplicationHelper *guiAppHelp = DGuiApplicationHelper::instance();
    QString theme  = "";

    if (guiAppHelp->themeType() == DGuiApplicationHelper::ColorType::DarkType) {  //暗色主题
        qDebug() << "Painting code flod area with dark type";
        theme = "d";
    } else {  //浅色主题
        qDebug() << "Painting code flod area with light type";
        theme = "l";
    }

    // QString flodImagePath = QString(":/images/d-%1.svg").arg(theme);
    //QString unflodImagePath = QString(":/images/u-%1.svg").arg(theme);

    QPoint endPoint;

    if (verticalScrollBar()->maximum() > 0) {
        qDebug() << "Painting code flod area with scroll bar maximum";
        endPoint = QPointF(0, height() + height() / verticalScrollBar()->maximum() * verticalScrollBar()->value()).toPoint();
    }

    QTextCursor cur = cursorForPosition(endPoint);
    QTextBlock endBlock = cur.block();
    int nPageLine = endBlock.blockNumber();

    if (verticalScrollBar()->maximum() == 0) {
        nPageLine = blockCount() - 1;
        qDebug() << "Painting code flod area with no scroll bar maximum";
    }

    cur = textCursor();

    for (int iBlockCount = blockNumber ; iBlockCount <= nPageLine; ++iBlockCount) {
        if (block.isVisible()) {
            qDebug() << "Painting code flod area with block visible";
            //判定是否包含注释代码左括号、是否整行是注释，isNeedShowFoldIcon该函数是为了做判定当前行是否包含成对的括号，如果包括，则不显示折叠标志

            //获取行数文本块 出去字符串判断　梁卫东２０２０年０９月０１日１７：１６：１７
            QString text = block.text();
            //若存在字符串行，多个字符串中间的 '{' '}' 同样被忽略
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
            QRegExp regExp("\".*\"");
#else
            QRegularExpression regExp("\".*\"");
#endif
            QString curText = text.remove(regExp);

            //不同类型文件注释符号不同 梁卫东　２０２０－０９－０３　１７：２８：４５
            bool bHasCommnent = false;
            QString multiLineCommentMark;
            QString singleLineCommentMark;

            if (m_commentDefinition.isValid()) {
                qDebug() << "Painting code flod area with comment definition";
                multiLineCommentMark = m_commentDefinition.multiLineStart.trimmed();
                singleLineCommentMark = m_commentDefinition.singleLine.trimmed();
                //判断是否包含单行或多行注释
                if (!multiLineCommentMark.isEmpty()) bHasCommnent = block.text().trimmed().startsWith(multiLineCommentMark);
                if (!singleLineCommentMark.isEmpty()) bHasCommnent = block.text().trimmed().startsWith(singleLineCommentMark);
            } else {
                qDebug() << "Painting code flod area with no comment definition";
                bHasCommnent = false;
            }

            //添加注释判断 存在不显示折叠标志　不存在显示折叠标准　梁卫东　２０２０年０９月０３日１７：２８：５０
            if (curText.contains("{") && isNeedShowFoldIcon(block) && !bHasCommnent) {
                qDebug() << "Painting code flod area with contains {";
                cur.setPosition(block.position(), QTextCursor::MoveAnchor);

                painter.setRenderHint(QPainter::Antialiasing, true);
                painter.setRenderHints(QPainter::SmoothPixmapTransform);

                int w = this->m_fontSize <= 15 ? 15 : m_fontSize;
                updateLeftWidgetWidth(w);
                int h = cursorRect(cur).height();
                // 绘制行纵向居中
                int offset = qMax(0, (h - w) / 2);
                //the language currently set by the system is Tibetan.
                if ("bo_CN" == Utils::getSystemLan())
                    offset = h <= 20 ? 0 : h / 10;

                QRect rect(0, cursorRect(cur).y() + offset, w, w);
                if (block.next().isVisible()) {
                    qDebug() << "Painting code flod area with block next visible";
                    if (block.isVisible()) {
                        qDebug() << "Painting code flod area with block next visible";
                        paintCodeFlod(&painter, rect);
                    }
                } else {
                    qDebug() << "Painting code flod area with block next not visible";
                    if (block.isVisible()) {
                        qDebug() << "Painting code flod area with block next not visible";
                        paintCodeFlod(&painter, rect, true);
                    }
                }
                m_listFlodIconPos.append(block.blockNumber());
            }
        }

        block = block.next();
    }
    qDebug() << "Painting code flod area completed";
}

void TextEdit::setBookmarkFlagVisable(bool isVisable, bool bIsFirstOpen)
{
    qDebug() << "Setting bookmark flag visible";
    int w = this->m_fontSize <= 15 ? 15 : m_fontSize;
    updateLeftWidgetWidth(w);

    m_pIsShowBookmarkArea = isVisable;
    m_pLeftAreaWidget->m_pBookMarkArea->setVisible(isVisable);
    qDebug() << "Setting bookmark flag visible completed";
}

void TextEdit::setCodeFlodFlagVisable(bool isVisable, bool bIsFirstOpen)
{
    qDebug() << "Setting code flod flag visible";
    int w = this->m_fontSize <= 15 ? 15 : m_fontSize;
    updateLeftWidgetWidth(w);

    m_pIsShowCodeFoldArea = isVisable;
    m_pLeftAreaWidget->m_pFlodArea->setVisible(isVisable);
    qDebug() << "Setting code flod flag visible completed";
}

void TextEdit::setHighLineCurrentLine(bool ok)
{
    qDebug() << "Setting high line current line";
    m_HightlightYes = ok;
    renderAllSelections();
    qDebug() << "Setting high line current line completed";
}

void TextEdit::updateLeftAreaWidget()
{
    qDebug() << "Updating left area widget";
#if 0
// not used anymore
    int blockSize = QString::number(blockCount()).size();
    int leftAreaWidth = 0;

    //跟新左边框宽度
    if (m_pIsShowBookmarkArea) {
        qDebug() << "Updating left area widget with bookmark area";
        leftAreaWidth += m_pLeftAreaWidget->m_pBookMarkArea->width();
    }
    if (m_pIsShowCodeFoldArea) {
        qDebug() << "Updating left area widget with code fold area";
        leftAreaWidth += m_pLeftAreaWidget->m_pFlodArea->width();
    }

    if (bIsSetLineNumberWidth) {
        qDebug() << "Updating left area widget with line number width";
        leftAreaWidth += blockSize * fontMetrics().width('9') + 5;
    }
    // m_pLeftAreaWidget->setFixedWidth(leftAreaWidth);
#endif
    m_pLeftAreaWidget->updateAll();
    qDebug() << "Updating left area widget completed";
}


void TextEdit::handleScrollFinish()
{
    qDebug() << "Handling scroll finish";
    // Restore cursor postion.
    jumpToLine(m_restoreRow, false);

    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, m_restoreColumn);

    // Update cursor.
    setTextCursor(cursor);
    qDebug() << "Handling scroll finish completed";
}

void TextEdit::setSyntaxDefinition(KSyntaxHighlighting::Definition def)
{
    qDebug() << "Setting syntax definition";
    m_commentDefinition.setComments(def.singleLineCommentMarker(), def.multiLineCommentMarker().first,  def.multiLineCommentMarker().second);
}

bool TextEdit::setCursorKeywordSeletoin(int position, bool findNext)
{
    qDebug() << "Setting cursor keyword seletion";
    int offsetLines = 3;

    if (findNext) {
        qDebug() << "Setting cursor keyword seletion with find next";
        for (int i = 0; i < m_findMatchSelections.size(); i++) {
            if (m_findMatchSelections[i].cursor.position() > position) {
                qDebug() << "Setting cursor keyword seletion with find next and position greater than position";
                m_findHighlightSelection.cursor = m_findMatchSelections[i].cursor;

                jumpToLine(m_findMatchSelections[i].cursor.blockNumber() + offsetLines, false);

                QTextCursor cursor = textCursor();
                cursor.setPosition(m_findMatchSelections[i].cursor.position());

                // Update cursor.
                setTextCursor(cursor);

                qDebug() << "Setting cursor keyword seletion return true";
                return true;
            }
        }
    } else {
        qDebug() << "Setting cursor keyword seletion with find previous";
        for (int i = m_findMatchSelections.size() - 1; i >= 0; i--) {
            if (m_findMatchSelections[i].cursor.position() < position) {
                qDebug() << "Setting cursor keyword seletion with find previous and position less than position";
                m_findHighlightSelection.cursor = m_findMatchSelections[i].cursor;

                jumpToLine(m_findMatchSelections[i].cursor.blockNumber() + offsetLines, false);

                QTextCursor cursor = textCursor();
                cursor.setPosition(m_findMatchSelections[i].cursor.position());

                // Update cursor.
                setTextCursor(cursor);

                qDebug() << "Setting cursor keyword seletion return true";
                return true;
            }
        }
    }

    qDebug() << "Setting cursor keyword seletion return false";
    return false;
}

void TextEdit::cursorPositionChanged()
{
    qDebug() << "Cursor position changed";
    // 以赋值形式，清空 Bracket 括号的selection
    // m_beginBracketSelection 和 m_endBracketSelection 将在 updateHighlightBrackets 重新设置
    m_beginBracketSelection = QTextEdit::ExtraSelection();
    m_endBracketSelection = QTextEdit::ExtraSelection();

    updateHighlightLineSelection();
    updateHighlightBrackets('(', ')');
    updateHighlightBrackets('{', '}');
    updateHighlightBrackets('[', ']');
    renderAllSelections();

    QTextCursor cursor = textCursor();
    if (m_wrapper) {
        qDebug() << "Cursor position changed with wrapper";
        m_wrapper->bottomBar()->updatePosition(cursor.blockNumber() + 1,
                                               cursor.positionInBlock() + 1);
    }

    m_pLeftAreaWidget->m_pLineNumberArea->update();
    m_pLeftAreaWidget->m_pBookMarkArea->update();
    m_pLeftAreaWidget->m_pFlodArea->update();
    qDebug() << "Cursor position changed completed";
}

/**
 * @brief 剪切光标选中的文本
 * @param ignoreCheck 是否忽略权限判断(外部已进行)，默认false
 */
void TextEdit::cut(bool ignoreCheck)
{
    qDebug() << "Cutting text";
    // 添加权限判断是否允许拷贝，剪切；防止后续可能调用接口，冗余处理
    if (!ignoreCheck && !Utils::enableClipCopy(getFilePath())) {
        qDebug() << "Cutting text with no permission, return";
        return;
    }

    if (m_isSelectAll) {
        qDebug() << "Cutting text with select all";
        QPlainTextEdit::selectAll();
    }

    //列编辑添加撤销重做
    if (m_bIsAltMod && !m_altModSelections.isEmpty()) {
        qDebug() << "Cutting text with alt mod";
        QString data;
        for (auto it = m_altModSelections.begin(); it != m_altModSelections.end(); it++) {
            auto text = (*it).cursor.selectedText();
            data += text ;
            if (it != m_altModSelections.end() - 1)
                data += "\n";
        }
        // Record the column edit delete command
        QUndoCommand *pDeleteStack = new DeleteBackAltCommand(m_altModSelections, this);
        m_pUndoStack->push(pDeleteStack);
        
        //设置到剪切板
        QClipboard *clipboard = QApplication::clipboard();   //获取系统剪贴板指针
        clipboard->setText(data);
    } else {
        qDebug() << "Cutting text with no alt mod";
        QTextCursor cursor = textCursor();
        //有选择内容才剪切
        if (cursor.hasSelection()) {
            qDebug() << "Cutting text with selection";
            QString data = this->selectedText(true);
            QUndoCommand *pDeleteStack = new DeleteTextUndoCommand(cursor, this);
            m_pUndoStack->push(pDeleteStack);
            QClipboard *clipboard = QApplication::clipboard();   //获取系统剪贴板指针
            clipboard->setText(data);
        }
    }
    qDebug() << "Cutting text completed";
    unsetMark();
    qDebug() << "Cutting text unset mark completed";
}

/**
 * @brief 拷贝光标选中的文本
 * @param ignoreCheck 是否忽略权限判断(外部已进行)，默认false
 */
void TextEdit::copy(bool ignoreCheck)
{
    qDebug() << "Copying text";
    // 添加权限判断是否允许拷贝，剪切；防止后续可能调用接口，冗余处理
    if (!ignoreCheck && !Utils::enableClipCopy(getFilePath())) {
        qDebug() << "Copying text with no permission, return";
        return;
    }

    if (m_bIsAltMod && !m_altModSelections.isEmpty()) {
        qDebug() << "Copying text with alt mod";
        QString data;
        for (auto it = m_altModSelections.begin(); it != m_altModSelections.end(); it++) {
            auto text = (*it).cursor.selectedText();
            data += text ;
            if (it != m_altModSelections.end() - 1)
                data += "\n";
        }
        QClipboard *clipboard = QApplication::clipboard();   //获取系统剪贴板指针
        clipboard->setText(data);
        tryUnsetMark();
    } else {
        qDebug() << "Copying text with no alt mod";
        if (!m_isSelectAll) {
            qDebug() << "Copying text with no alt mod";
            QClipboard *clipboard = QApplication::clipboard();   //获取系统剪贴板指针
            if (textCursor().hasSelection()) {
                qDebug() << "Copying text with selection";
                //clipboard->setText(textCursor().selection().toPlainText());
                clipboard->setText(this->selectedText(true));
                tryUnsetMark();
            } else {
                qDebug() << "Copying text with no selection";
                clipboard->setText(m_highlightWordCacheCursor.selectedText());
            }
        } else {
            qDebug() << "Copying text with select all";
            QClipboard *clipboard = QApplication::clipboard();
            QString text = this->toPlainText();
            clipboard->setText(text);
        }
    }
    qDebug() << "Copying text completed";
}

void TextEdit::paste()
{
    qDebug() << "Pasting text";
#if 0
    //2021-05-25:为解决大文本粘贴卡顿而注释重写
    if (m_isSelectAll)
        QPlainTextEdit::selectAll();

    const QClipboard *clipboard = QApplication::clipboard(); //获取剪切版内容
    auto text = clipboard->text();
    if (text.isEmpty()) {
        qDebug() << "Pasting text with empty text, return";
        return;
    }
    if (!m_bIsAltMod) {
        qDebug() << "Pasting text with no alt mod";
        QTextCursor cursor = textCursor();
        insertSelectTextEx(cursor, text);
        unsetMark();
    } else {
        qDebug() << "Pasting text with alt mod";
        insertColumnEditTextEx(text);
    }

#endif


    //大文件粘贴-采用分块插入
    if (m_isSelectAll) {
        qDebug() << "Pasting text with select all";
        QPlainTextEdit::selectAll();
    }

    const QClipboard *clipboard = QApplication::clipboard(); //获取剪切版内容
    auto text = clipboard->text();

    if (text.isEmpty()) {
        qDebug() << "Pasting text with empty text, return";
        return;
    }
    if (!m_bIsAltMod) {
        qDebug() << "Pasting text with no alt mod";
        int block = 1 * 1024 * 1024;
        int size = text.size();
        if (size > block) {
            qDebug() << "Pasting text with size greater than block";
            InsertBlockByTextCommand *commond = new InsertBlockByTextCommand(text, this, m_wrapper);
            m_pUndoStack->push(commond);
        } else {
            qDebug() << "Pasting text with size less than block";
            QTextCursor cursor = textCursor();
            insertSelectTextEx(cursor, text);
            unsetMark();
        }
    } else {
        qDebug() << "Pasting text with alt mod";
        insertColumnEditTextEx(text);
    }

    m_isSelectAll = false;
    qDebug() << "Pasting text completed";
}

void TextEdit::highlight()
{
    QTimer::singleShot(0, this, [&]() {
        if (nullptr != m_wrapper) {
            qDebug() << "Highlighting with wrapper";
            m_wrapper->OnUpdateHighlighter();
        }
    });
    qDebug() << "Highlighting completed";
}

void TextEdit::selectTextInView()
{
    qDebug() << "Selecting text in view";
    int startPos = cursorForPosition(QPoint(0, 0)).position();
    QPoint endPoint = QPoint(this->viewport()->width(), this->viewport()->height());
    int endPos = cursorForPosition(endPoint).position();

    QTextCursor cursor = this->textCursor();
    cursor.setPosition(endPos);
    cursor.setPosition(startPos, QTextCursor::KeepAnchor);
    this->setTextCursor(cursor);
    this->horizontalScrollBar()->setValue(0);
    qDebug() << "Selecting text in view completed";
}

/**
 * @note Temporary workaround by overriding selectAll() to fix PMS-79951
 * Prevents redundant clipboard memory usage from repeated text copying
 * This issue has been fixed in Wayland/Treeland, but still exists in X11
 * @todo Consider using QPlainTextEdit::selectAll() only when migrating to Treeland
 */
void TextEdit::setSelectAll()
{
    qDebug() << "Setting select all";
    if (m_wrapper->getFileLoading()) {
        qDebug() << "Setting select all with file loading, return";
        return;
    }

    m_bIsAltMod = false;
    m_isSelectAll = true;
    selectTextInView();

    if (!document()->isEmpty() && IflytekAiAssistant::instance()->valid()) {
        qDebug() << "Setting select all with valid";
        if (auto clip = qApp->clipboard()) {
            qDebug() << "Setting select all with clipboard";
            // limit the size of the text to avoid performance issue
            static const int kMaxSelectCount = 200000;
            int charCount = document()->characterCount();

            if (charCount < kMaxSelectCount) {
                qDebug() << "Setting select all with clipboard and char count less than kMaxSelectCount";
                clip->setText(toPlainText(), QClipboard::Selection);
            } else {
                qDebug() << "Setting select all with clipboard and char count greater than kMaxSelectCount";
                auto selectCursor = textCursor();
                selectCursor.setPosition(0);
                selectCursor.setPosition(kMaxSelectCount, QTextCursor::KeepAnchor);
                clip->setText(selectCursor.selectedText(), QClipboard::Selection);
            }
        }
    }
    qDebug() << "Setting select all completed";
}

void TextEdit::slotSigColorSelected(bool bSelected, QColor color)
{
    qDebug() << "Slot sig color selected";
    if (m_isSelectAll) {
        qDebug() << "Slot sig color selected with select all";
        QPlainTextEdit::selectAll();
    }
    isMarkCurrentLine(bSelected, color.name());
    renderAllSelections();
    m_colorMarkMenu->close();
    m_rightMenu->close(); //选择颜色关闭菜单　梁卫东　２０２０－０８－２１　０９：３４：５３
    qDebug() << "Slot sig color selected completed";
}

void TextEdit::slotSigColorAllSelected(bool bSelected, QColor color)
{
    qDebug() << "Slot sig color all selected";
    if (m_isSelectAll) {
        qDebug() << "Slot sig color all selected with select all";
        QPlainTextEdit::selectAll();
    }
    isMarkAllLine(bSelected, color.name());
    renderAllSelections();
    m_colorMarkMenu->close();
    m_rightMenu->close(); //选择颜色关闭菜单　梁卫东　２０２０－０８－２１　０９：３４：５３
    qDebug() << "Slot sig color all selected completed";
}

void TextEdit::slotCutAction(bool checked)
{
    qDebug() << "Slot cut action";
    Q_UNUSED(checked);
    this->cut();
    qDebug() << "Slot cut action completed";
}

void TextEdit::slotCopyAction(bool checked)
{
    qDebug() << "Slot copy action";
    Q_UNUSED(checked);

    if (isAbleOperation(Utils::CopyOperation)) {
        qDebug() << "Slot copy action with able operation";
        copy();
    } else {
#ifdef DTKWIDGET_CLASS_DSizeMode
        qDebug() << "Slot copy action with dsize mode";
        Utils::sendFloatMessageFixedFont(this, QIcon(":/images/warning.svg"), tr("Copy failed: not enough memory"));
#else
        qDebug() << "Slot copy action with no dsize mode";
        DMessageManager::instance()->sendMessage(this, QIcon(":/images/warning.svg"), tr("Copy failed: not enough memory"));
#endif
    }
    qDebug() << "Slot copy action completed";
}

void TextEdit::slotPasteAction(bool checked)
{
    qDebug() << "Slot paste action";
    Q_UNUSED(checked);
    if (isAbleOperation(Utils::PasteOperation)) {
        qDebug() << "Slot paste action with able operation";
        paste();
    } else {
#ifdef DTKWIDGET_CLASS_DSizeMode
        qDebug() << "Slot paste action with dsize mode";
        Utils::sendFloatMessageFixedFont(this, QIcon(":/images/warning.svg"), tr("Paste failed: not enough memory"));
#else
        qDebug() << "Slot paste action with no dsize mode";
        DMessageManager::instance()->sendMessage(this, QIcon(":/images/warning.svg"), tr("Paste failed: not enough memory"));
#endif
    }
    qDebug() << "Slot paste action completed";
}

void TextEdit::slotDeleteAction(bool checked)
{
    qDebug() << "Slot delete action";
    Q_UNUSED(checked);
    if (m_isSelectAll) {
        qDebug() << "Slot delete action with select all";
        QPlainTextEdit::selectAll();
    }

    if (m_bIsAltMod && !m_altModSelections.isEmpty()) {
        qDebug() << "Slot delete action with alt mod";
        QUndoCommand *pDeleteStack = new DeleteBackAltCommand(m_altModSelections, this);
        m_pUndoStack->push(pDeleteStack);
    } else {
        if (textCursor().hasSelection()) {
            qDebug() << "Slot delete action with selection";
            QUndoCommand *pDeleteStack = new DeleteTextUndoCommand(textCursor(), this);
            m_pUndoStack->push(pDeleteStack);
        } else {
            qDebug() << "Slot delete action with no selection";
            setTextCursor(m_highlightWordCacheCursor);
        }
    }
    qDebug() << "Slot delete action completed";
}

void TextEdit::slotSelectAllAction(bool checked)
{
    qDebug() << "Slot select all action";
    Q_UNUSED(checked);
    setSelectAll();
    qDebug() << "Slot select all action completed";
}

bool TextEdit::slotOpenInFileManagerAction(bool checked)
{
    qDebug() << "Slot open in file manager action";
    Q_UNUSED(checked);
    return DDesktopServices::showFileItem(this->getTruePath());
}

void TextEdit::slotAddComment(bool checked)
{
    qDebug() << "Slot add comment action";
    Q_UNUSED(checked);
    toggleComment(true);
    qDebug() << "Slot add comment action completed";
}

void TextEdit::slotCancelComment(bool checked)
{
    qDebug() << "Slot cancel comment action";
    Q_UNUSED(checked);
    toggleComment(false);
    qDebug() << "Slot cancel comment action completed";
}

void TextEdit::slotVoiceReadingAction(bool checked)
{
    qDebug() << "Slot voice reading action";
    Q_UNUSED(checked);
    auto ret = IflytekAiAssistant::instance()->textToSpeech();
    if (IflytekAiAssistant::Success != ret) {
        qDebug() << "Slot voice reading action with error";
        Q_EMIT popupNotify(IflytekAiAssistant::instance()->errorString(ret), true);
    }

    emit signal_readingPath();
    qDebug() << "Slot voice reading action completed";
}

bool TextEdit::slotStopReadingAction(bool checked)
{
    qDebug() << "Slot stop reading action";
    Q_UNUSED(checked);
    bool ret = IflytekAiAssistant::instance()->stopTtsDirectly();
    qDebug() << "Slot stop reading action completed";
    return ret;
}

void TextEdit::slotdictationAction(bool checked)
{
    qDebug() << "Slot dictation action";
    Q_UNUSED(checked);
    auto ret = IflytekAiAssistant::instance()->speechToText();
    if (IflytekAiAssistant::Success != ret) {
        qDebug() << "Slot dictation action with error";
        Q_EMIT popupNotify(IflytekAiAssistant::instance()->errorString(ret), true);
    }
    qDebug() << "Slot dictation action completed";
}

void TextEdit::slotColumnEditAction(bool checked)
{
    Q_UNUSED(checked);
    qDebug() << "Slot column edit action";
#ifdef DTKWIDGET_CLASS_DSizeMode
    Utils::sendFloatMessageFixedFont(this, QIcon(":/images/ok.svg"), tr("Press ALT and click lines to edit in column mode"));
#else
    DMessageManager::instance()->sendMessage(this, QIcon(":/images/ok.svg"), tr("Press ALT and click lines to edit in column mode"));
#endif
    qDebug() << "Slot column edit action completed";
}

void TextEdit::slotPreBookMarkAction(bool checked)
{
    qDebug() << "Slot pre book mark action";
    Q_UNUSED(checked);
    int line = getLineFromPoint(m_mouseClickPos);
    int index = m_listBookmark.indexOf(line);

    if (index == 0) {
        qDebug() << "Slot pre book mark action with index 0";
        jumpToLine(m_listBookmark.last(), true);
    } else {
        qDebug() << "Slot pre book mark action with index not 0";
        jumpToLine(m_listBookmark.value(index - 1), true);
    }
    qDebug() << "Slot pre book mark action completed";
}

void TextEdit::slotNextBookMarkAction(bool checked)
{
    qDebug() << "Slot next book mark action";
    Q_UNUSED(checked);
    int line = getLineFromPoint(m_mouseClickPos);
    int index = m_listBookmark.indexOf(line);

    if (index == -1 && !m_listBookmark.isEmpty()) {
        qDebug() << "Slot next book mark action with index -1";
        jumpToLine(m_listBookmark.last(), false);
    }

    if (index == m_listBookmark.count() - 1) {
        qDebug() << "Slot next book mark action with index equal to count - 1";
        jumpToLine(m_listBookmark.first(), false);
    } else {
        qDebug() << "Slot next book mark action with index not equal to count - 1";
        jumpToLine(m_listBookmark.value(index + 1), false);
    }
    qDebug() << "Slot next book mark action completed";
}

void TextEdit::slotClearBookMarkAction(bool checked)
{
    qDebug() << "Slot clear book mark action";
    Q_UNUSED(checked);
    m_listBookmark.clear();
    qDebug() << "ClearBookMark:" << m_listBookmark;
    m_pLeftAreaWidget->m_pBookMarkArea->update();
    qDebug() << "Slot clear book mark action completed";
}

void TextEdit::slotFlodAllLevel(bool checked)
{
    qDebug() << "Slot flod all level action";
    Q_UNUSED(checked);
    flodOrUnflodAllLevel(true);
    qDebug() << "Slot flod all level action completed";
}

void TextEdit::slotUnflodAllLevel(bool checked)
{
    qDebug() << "Slot unflod all level action";
    Q_UNUSED(checked);
    flodOrUnflodAllLevel(false);
    qDebug() << "Slot unflod all level action completed";
}

void TextEdit::slotFlodCurrentLevel(bool checked)
{
    qDebug() << "Slot flod current level action";
    Q_UNUSED(checked);
    flodOrUnflodCurrentLevel(true);
    qDebug() << "Slot flod current level action completed";
}

void TextEdit::slotUnflodCurrentLevel(bool checked)
{
    qDebug() << "Slot unflod current level action";
    Q_UNUSED(checked);
    flodOrUnflodCurrentLevel(false);
    qDebug() << "Slot unflod current level action completed";
}

void TextEdit::slotCancleMarkAllLine(bool checked)
{
    qDebug() << "Slot cancle mark all line action";
    Q_UNUSED(checked);
    isMarkAllLine(false);
    qDebug() << "Slot cancle mark all line action completed";
}

void TextEdit::slotCancleLastMark(bool checked)
{
    qDebug() << "Slot cancle last mark action";
    Q_UNUSED(checked);
    cancelLastMark();
    qDebug() << "Slot cancle last mark action completed";
}

void TextEdit::slotUndoAvailable(bool undoIsAvailable)
{
    qDebug() << "Slot undo available action";
    m_canUndo = undoIsAvailable;
    qDebug() << "Slot undo available action completed";
}

void TextEdit::slotRedoAvailable(bool redoIsAvailable)
{
    qDebug() << "Slot redo available action";
    m_canRedo = redoIsAvailable;
    qDebug() << "Slot redo available action completed";
}

void TextEdit::redo_()
{
    qDebug() << "Starting redo operation";
    if (!m_pUndoStack->canRedo()) {
        qDebug() << "Redo operation skipped - nothing to redo";
        return;
    }

    // Get current stack info before redo
    const bool needUpdate = refreshUndoRedoColumnStatus();
    qDebug() << "Redo operation - need update selections:" << needUpdate;

    m_pUndoStack->redo();
    qDebug() << "Redo operation executed, new stack index:" << m_pUndoStack->index();

    if (needUpdate) {
        renderAllSelections();
        update();
        qDebug() << "Selections updated after redo";
    }

    if (m_pUndoStack->index() == m_lastSaveIndex) {
        this->m_wrapper->window()->updateModifyStatus(m_sFilePath, false);
        this->m_wrapper->setTemFile(false);
        this->document()->setModified(false);
        qDebug() << "File modification status reset after redo";
    }
    qDebug() << "Redo operation completed successfully";
}

void TextEdit::undo_()
{
    qDebug() << "Starting undo operation";
    qDebug() << "Starting undo operation";
    if (!m_pUndoStack->canUndo()) {
        qDebug() << "Undo operation skipped - nothing to undo";
        return;
    }

    m_pUndoStack->undo();
    qDebug() << "Undo operation executed, new stack index:" << m_pUndoStack->index();

    const bool needUpdate = refreshUndoRedoColumnStatus();
    if (needUpdate) {
        renderAllSelections();
        update();
        qDebug() << "Selections updated after undo";
    }

    // 对撤销栈清空的情况下，有两种文件仍需保留*号(重做无需如下判定)
    // 1. 备份文件，上次修改之后直接关闭时备份的文件，仍需要提示保存
    // 2. 临时文件，上次修改后关闭，撤销操作后文件内容不为空
    if (m_pUndoStack->index() == m_lastSaveIndex
            && !m_wrapper->isBackupFile()
            && !(m_wrapper->isDraftFile() && !m_wrapper->isPlainTextEmpty())) {
        this->m_wrapper->window()->updateModifyStatus(m_sFilePath, false);
        this->m_wrapper->setTemFile(false);
        this->document()->setModified(false);
        qDebug() << "File modification status reset after undo";
    }
    qDebug() << "Undo operation completed successfully";
}

/*!
   \brief 从索引位置 \a from 移动文本内容 \a text 到 \a to, 当仅拷贝文本时，
        \a copy 置为true.
   \param from 移动/拷贝文本位置
   \param to 目的文本位置
   \param text 文本内容
   \param copy 是否为拷贝文本，默认为移动
 */
void TextEdit::moveText(int from, int to, const QString &text, bool copy)
{
    qDebug() << "Moving text from" << from << "to" << to << "with text" << text << "and copy" << copy;
    auto cursor = this->textCursor();
    auto list = new UndoList;
    cursor.setPosition(from);
    QUndoCommand *delCommand = nullptr;
    // 拷贝模式下无需删除文本
    if (!copy) {
        qDebug() << "Moving text with delete";
        cursor.setPosition(from + text.size(), QTextCursor::KeepAnchor);
        delCommand = new DeleteBackCommand(cursor, this);
    }

    cursor.setPosition(to);
    auto insertCommand = new DragInsertTextUndoCommand(cursor, text, this);

    //the positon of 'from' is on the left of the position of 'to',
    //therefore,firstly do the insert operation.
    if (from < to) {
        qDebug() << "Moving text with from < to";
        list->appendCom(insertCommand);
        if (!copy) {
            qDebug() << "Moving text with delete and from < to";
            list->appendCom(delCommand);
        }
        m_pUndoStack->push(list);
    } else if (from > to) {
        qDebug() << "Moving text with from > to";
        if (!copy) {
            qDebug() << "Moving text with delete and from > to";
            list->appendCom(delCommand);
        }
        list->appendCom(insertCommand);
        m_pUndoStack->push(list);
    }
    qDebug() << "Moving text completed";
}

/**
 * @brief 在text中查找substr,并返回QTextCursor
 * @param substr:需要查找的串
 *        text:源字符串
 *        from:从text的from位置开始查找
 *        backword:是否反向查找
 *        cursorPos: text起始位置在全文本中的位置
 * @return
 */
QTextCursor TextEdit::findCursor(const QString &substr, const QString &text, int from, bool backward,
                                 int cursorPos, Qt::CaseSensitivity caseFlag)
{
    qDebug() << "Finding text:" << substr.left(20) << "from position:" << from
               << "direction:" << (backward ? "backward" : "forward");

    // 处理换行符为 \r\n (光标计算时被视为单个字符)的情况，移除多计算的字符数
    // text 均为 \n 结尾
    QString findSubStr = substr;
    if (BottomBar::Windows == m_wrapper->bottomBar()->getEndlineFormat()) {
        qDebug() << "Finding text with windows endline format";
        findSubStr.replace("\r\n", "\n");
    }

    int index = -1;
    if (backward) {
        qDebug() << "Finding text with backward direction";
        index = text.lastIndexOf(findSubStr, from, caseFlag);
    } else {
        qDebug() << "Finding text with forward direction";
        index = text.indexOf(findSubStr, from, caseFlag);
    }
    if (-1 != index) {
        qDebug() << "Find successful at position:" << index;
        auto cursor = this->textCursor();
        cursor.setPosition(index + cursorPos);
        cursor.setPosition(cursor.position() + findSubStr.size(), QTextCursor::KeepAnchor);
        return cursor;
    } else {
        qDebug() << "Find failed - text not found";
        return QTextCursor();
    }
    qDebug() << "Finding text completed";
}

/**
 * @brief 点击行号处理：选中当前行，光标置于下一行行首
   @param point 当前鼠标点击的位置
   @note 在 "bo_CN" 语言环境下，光标向上偏移2像素进行比较。
 */
void TextEdit::onPressedLineNumber(const QPoint &point)
{
    qDebug() << "On pressed line number";
    // 在执行大文件加载过程中，由于 cursorForPosition() 计算耗时，不响应点击处理
    if (TextEdit::FileOpenBegin == m_LeftAreaUpdateState) {
        qDebug() << "On pressed line number with file open begin, return";
        return;
    }

    if (point.x() < 0 || point.x() > m_pLeftAreaWidget->m_pLineNumberArea->width()) {
        qDebug() << "On pressed line number with point x out of range, return";
        return;
    }

    int offset = 0;
    //the language currently set by the system is Tibetan.
    if ("bo_CN" == Utils::getSystemLan()) {
        qDebug() << "On pressed line number with Tibetan language";
        offset = 2;
    }

    QPoint checkPoint(point.x(), qMax(0, point.y() - offset));
    QTextBlock possibleBlock = cursorForPosition(checkPoint).block();

    if (possibleBlock.isValid()) {
        qDebug() << "On pressed line number with possible block";
        QTextCursor cursor = textCursor();
        cursor.setPosition(possibleBlock.position(), QTextCursor::MoveAnchor);

        // 尾行光标不调整到下一行
        if (possibleBlock == document()->lastBlock()) {
            qDebug() << "On pressed line number with last block";
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        } else {
            qDebug() << "On pressed line number with not last block";
            cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
        }

        setTextCursor(cursor);
    }
    qDebug() << "On pressed line number completed";
}

/**
 * @return 返回当前光标选中的内容
 * @note 如果从编辑器获得的选中文本跨越换行符，则文本将包含 Unicode U+2029 段落分隔符而不是换行符 \n 字符。
 *      可使用 QString::replace() 将这些字符替换为换行符，为避免文本原有 Unicode U+2029 分割符影响，
 *      手动调整换行符插入位置。
 */
QString TextEdit::selectedText(bool checkCRLF)
{
    qDebug() << "Getting selected text";
    auto cursor = this->textCursor();
    auto temp = cursor;
    int startpos = std::min(cursor.anchor(), cursor.position());
    int endpos = std::max(cursor.anchor(), cursor.position());
    int startblock = 0, endblock = 0;
    cursor.setPosition(startpos);
    startblock = cursor.blockNumber();
    cursor.setPosition(endpos);
    endblock = cursor.blockNumber();
    // 仅单个文本块，直接提取文本数据
    if (startblock == endblock) {
        qDebug() << "Getting selected text with single block, return";
        return temp.selectedText();
    }

    QString text;
    cursor.setPosition(startpos);
    // 取得首个文本块内容
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    text += cursor.selectedText();
    cursor.setPosition(cursor.position() + 1);

    // 不同风格使用不同换行符，默认不替换，仅在剪贴板、保存文件等时设置
    QString endLine = "\n";
    if (checkCRLF && BottomBar::Windows == m_wrapper->bottomBar()->getEndlineFormat()) {
        qDebug() << "Getting selected text with windows endline format";
        endLine = "\r\n";
    }

    // 获取中间完整文本块内容
    while ((cursor.position() + cursor.block().length()) < endpos) {
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        text += endLine;
        text += cursor.selectedText();
        cursor.setPosition(cursor.position() + 1);
    }

    // 取得尾部文本块内容
    if (cursor.position() < endpos) {
        qDebug() << "Getting selected text with tail block";
        // 判断是否尾部文本块达到当前文本块末尾，到达末尾需要将 U+2029 替换为 \n
        bool needAdjustNewline = bool(cursor.position() + cursor.block().length() == endpos);
        cursor.setPosition(endpos, QTextCursor::KeepAnchor);
        text += endLine;
        text += cursor.selectedText();

        if (needAdjustNewline && text.endsWith("\u2029")) {
            text.replace("\u2029", endLine);
        }
    }

    qDebug() << "Getting selected text completed";
    return text;
}

/**
 * @brief 行尾符号改变后处理函数；
 * 1.这里并没有做"\n"和"\r\n"之间的替换，实际的替换动作在保存的时候才发生；
 * @param
 * @return
 */

void TextEdit::onEndlineFormatChanged(BottomBar::EndlineFormat from, BottomBar::EndlineFormat to)
{
    qDebug() << "On endline format changed";
    //auto endlineCom = new EndlineFormartCommand(this,m_wrapper->bottomBar(),from,to);
    //m_pUndoStack->push(endlineCom);
    m_wrapper->bottomBar()->setEndlineMenuText(to);
    qDebug() << "On endline format changed completed";
}

/**
 * @brief 系统调色板更新时重绘部分组件，例如：列选取项
 */
void TextEdit::onAppPaletteChanged()
{
    qDebug() << "On app palette changed";
    // 判断是否处于列选取状态
    if (m_bIsAltMod && !m_altModSelections.isEmpty()) {
        qDebug() << "On app palette changed with alt mod";
        QColor highlightBackground = DGuiApplicationHelper::instance()->applicationPalette().color(QPalette::Highlight);
        for (auto &selection : m_altModSelections) {
            selection.format.setBackground(highlightBackground);
        }
        // 更新高亮状态
        renderAllSelections();
    }
    qDebug() << "On app palette changed completed";
}

void TextEdit::updateHighlightBrackets(const QChar &openChar, const QChar &closeChar)
{
    qDebug() << "Updating highlight brackets";
    QTextDocument *doc = document();
    QTextCursor cursor = textCursor();
    int position = cursor.position();

    QTextCursor bracketBeginCursor;
    QTextCursor bracketEndCursor;
    cursor.clearSelection();

    if (!bracketBeginCursor.isNull() || !bracketEndCursor.isNull()) {
        qDebug() << "Updating highlight brackets with null";
        bracketBeginCursor.setCharFormat(QTextCharFormat());
        bracketEndCursor.setCharFormat(QTextCharFormat());
        bracketBeginCursor = bracketEndCursor = QTextCursor();
    }

    QChar begin, end;

    if (doc->characterAt(position) == openChar ||
            doc->characterAt(position) == closeChar ||
            doc->characterAt(position - 1) == openChar ||
            doc->characterAt(position - 1) == closeChar) {
        qDebug() << "Updating highlight brackets with character at position";
        bool forward = doc->characterAt(position) == openChar ||
                       doc->characterAt(position - 1) == openChar;

        if (forward) {
            qDebug() << "Updating highlight brackets with forward";
            if (doc->characterAt(position) == openChar) {
                qDebug() << "Updating highlight brackets with openChar";
                position++;
            } else {
                qDebug() << "Updating highlight brackets with closeChar";
                cursor.setPosition(position - 1);
            }

            begin = openChar;
            end = closeChar;
        } else {
            qDebug() << "Updating highlight brackets with backward";
            if (doc->characterAt(position) == closeChar) {
                qDebug() << "Updating highlight brackets with closeChar";
                cursor.setPosition(position + 1);
                position -= 1;
            } else {
                qDebug() << "Updating highlight brackets with openChar";
                position -= 2;
            }

            begin = closeChar;
            end = openChar;
        }

        bracketBeginCursor = cursor;
        bracketBeginCursor.movePosition(forward ? QTextCursor::NextCharacter : QTextCursor::PreviousCharacter,
                                        QTextCursor::KeepAnchor);

        int braceDepth = 1;
        // 判断当前是否处于字符串内，字符串内的 openChar closeChar  {} () [] 等不进行统计
        bool inCodeString = false;
        QChar c;
        while (!(c = doc->characterAt(position)).isNull()) {
            if (inCodeString) {
                qDebug() << "Updating highlight brackets with in code string";
                // 判断 " 是否存在转义字符，若不存在，则退出字符串模式
                // 注意回退(!forward)模式下，position = 0，characterAt(position - 1) 将返回空字符，同样符合判断，不会触发越界访问
                if ('"' == c && '\\' != doc->characterAt(position - 1)) {
                    qDebug() << "Updating highlight brackets with not in code string";
                    inCodeString = false;
                }
            } else {
                qDebug() << "Updating highlight brackets with not in code string";
                if (c == begin) {
                    qDebug() << "Updating highlight brackets with begin";
                    braceDepth++;
                } else if (c == end) {
                    qDebug() << "Updating highlight brackets with end";
                    braceDepth--;

                    if (!braceDepth) {
                        bracketEndCursor = QTextCursor(doc);
                        bracketEndCursor.setPosition(position);
                        bracketEndCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
                        break;
                    }
                } else if ('"' == c) {
                    qDebug() << "Updating highlight brackets with \"";
                    inCodeString = true;
                }
            }

            forward ? position++ : position--;
        }

        // cannot find the end bracket to not need to highlight.
        if (!bracketEndCursor.isNull()) {
            qDebug() << "Updating highlight brackets with not null";
            m_beginBracketSelection.cursor = bracketBeginCursor;
            m_beginBracketSelection.format = m_bracketMatchFormat;

            m_endBracketSelection.cursor = bracketEndCursor;
            m_endBracketSelection.format = m_bracketMatchFormat;
        }
    }
    qDebug() << "Updating highlight brackets completed";
}

int TextEdit::getFirstVisibleBlockId() const
{
    qDebug() << "Getting first visible block id";
    QTextCursor cur = QTextCursor(this->document());
    if (cur.isNull()) {
        qDebug() << "Getting first visible block id with null, return 0";
        return 0;
    }
    cur.movePosition(QTextCursor::Start);

    QPoint startPoint;
    QTextBlock startBlock, endBlock;

    if (verticalScrollBar()->maximum() > height()) {
        qDebug() << "Getting first visible block id with maximum > height";
        startPoint = QPointF(0, height() / verticalScrollBar()->maximum() * verticalScrollBar()->value()).toPoint();
        //endPoint = QPointF(0,height() + height()/verticalScrollBar()->maximum()*verticalScrollBar()->value()).toPoint();
    } else if (verticalScrollBar()->maximum() > 0 && verticalScrollBar()->maximum() <= height()) {
        qDebug() << "Getting first visible block id with maximum > 0 and maximum <= height";
        startPoint = QPointF(0, verticalScrollBar()->value() / verticalScrollBar()->maximum()).toPoint();
    }

    cur = cursorForPosition(startPoint);
    startBlock = document()->findBlock(cur.position());
    cur.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    if (startBlock.text() != cur.selection().toPlainText()) {
        qDebug() << "Getting first visible block id with text not equal to selection";
        return startBlock.blockNumber() + 1;
    }

    qDebug() << "Getting first visible block id completed";
    return startBlock.blockNumber();
}

void TextEdit::setLeftAreaUpdateState(TextEdit::UpdateOperationType statevalue)
{
    qDebug() << "Setting left area update state";
    if (statevalue != m_LeftAreaUpdateState) {
        qDebug() << "Setting left area update state with statevalue not equal to m_LeftAreaUpdateState";
        m_LeftAreaUpdateState = statevalue;

        // 当文件读取完成时，手动触发更新界面
        if (TextEdit::FileOpenEnd == m_LeftAreaUpdateState) {
            qDebug() << "Setting left area update state with FileOpenEnd";
            m_pLeftAreaWidget->updateAll();
        }
    }
    qDebug() << "Setting left area update state completed";
}

TextEdit::UpdateOperationType TextEdit::getLeftAreaUpdateState()
{
    qDebug() << "Getting left area update state";
    return m_LeftAreaUpdateState;
}
//line 开始处理的行号  isvisable是否折叠  iInitnum左括号默认开始计算的数量  isFirstLine是否是第一行，因为第一行默认不折叠
bool TextEdit::getNeedControlLine(int line, bool isVisable)
{
    qDebug() << "Processing code fold control for line" << line << "visibility:" << (isVisable ? "show" : "hide");
    // 查询折叠区域文本块范围
    QTextBlock beginBlock, endBlock, curBlock;
    bool bFoundBrace = findFoldBlock(line, beginBlock, endBlock, curBlock);

    qDebug() << "Found matching brace:" << bFoundBrace;
    //没有找到右括弧折叠左括弧后面所有行
    if (!bFoundBrace) {
        qDebug() << "No matching closing brace found, folding all subsequent lines";
        //遍历最后右括弧文本块 设置块隐藏或显示
        int blockCount = 0;
        while (beginBlock.isValid()) {
            beginBlock.setVisible(isVisable);
            viewport()->adjustSize();
            beginBlock = beginBlock.next();
            blockCount++;
        }
        qDebug() << "Processed" << blockCount << "blocks with visibility:" << (isVisable ? "shown" : "hidden");
        return true;
        //没有找到匹配左右括弧 //如果左右"{" "}"在同一行不折叠
    } else if (!bFoundBrace || endBlock == curBlock) {
        qDebug() << "Braces on same line or no proper match found, skipping fold operation";
        return false;
    } else {
        qDebug() << "Processing fold region from line" << line << "to line" << endBlock.blockNumber();
        //遍历最后右括弧文本块 设置块隐藏或显示
        int blockCount = 0;
        while (beginBlock != endBlock && beginBlock.isValid()) {
            if (beginBlock.isValid()) {
                beginBlock.setVisible(isVisable);
                blockCount++;
            }
            viewport()->adjustSize();
            beginBlock = beginBlock.next();
        }

        //最后一行显示或隐藏,或者下行就包含"}"
        if (beginBlock.isValid() && beginBlock == endBlock && endBlock.text().simplified() == "}") {
            endBlock.setVisible(isVisable);
            blockCount++;
            viewport()->adjustSize();
        }

        qDebug() << "Fold operation completed, processed" << blockCount << "blocks with visibility:" << (isVisable ? "shown" : "hidden");
        return true;
    }
}

bool TextEdit::event(QEvent *event)
{
    switch (event->type()) {
        case QEvent::Gesture:
            qDebug() << "Event gesture";
            gestureEvent(static_cast<QGestureEvent *>(event));
            break;
        case QEvent::PaletteChange:
            qDebug() << "Event palette change";
            // 调色板更新时更新选取高亮颜色
            onAppPaletteChanged();
            break;
        default:
            break;
    }

    return DPlainTextEdit::event(event);
}

bool TextEdit::gestureEvent(QGestureEvent *event)
{
    qDebug() << "Gesture event";
    if (QGesture *tap = event->gesture(Qt::TapGesture)) {
        qDebug() << "Gesture tap";
        tapGestureTriggered(static_cast<QTapGesture *>(tap));
    }
    if (QGesture *tapAndHold = event->gesture(Qt::TapAndHoldGesture)) {
        qDebug() << "Gesture tap and hold";
        tapAndHoldGestureTriggered(static_cast<QTapAndHoldGesture *>(tapAndHold));
    }
    if (QGesture *pan = event->gesture(Qt::PanGesture)) {
        qDebug() << "Gesture pan";
        panTriggered(static_cast<QPanGesture *>(pan));
    }
    if (QGesture *pinch = event->gesture(Qt::PinchGesture)) {
        qDebug() << "Gesture pinch";
        pinchTriggered(static_cast<QPinchGesture *>(pinch));
    }
    if (QGesture *swipe = event->gesture(Qt::SwipeGesture)) {
        qDebug() << "Gesture swipe";
        swipeTriggered(static_cast<QSwipeGesture *>(swipe));
    }
    qDebug() << "Gesture event completed, return true";
    return true;
}

void TextEdit::tapGestureTriggered(QTapGesture *tap)
{
    qDebug() << "Tap gesture triggered";
    //单指点击函数
    switch (tap->state()) {
    case Qt::GestureStarted: {
        m_gestureAction = GA_tap;
        m_tapBeginTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
        qDebug() << "Tap gesture triggered with started";
        break;
    }
    case Qt::GestureUpdated: {
        m_gestureAction = GA_slide;
        qDebug() << "Tap gesture triggered with updated";
        break;
    }
    case Qt::GestureCanceled: {
        qDebug() << "Tap gesture triggered with canceled";
        //根据时间长短区分轻触滑动
        qint64 timeSpace = QDateTime::currentDateTime().toMSecsSinceEpoch() - m_tapBeginTime;
        if (timeSpace < TAP_MOVE_DELAY || m_slideContinueX || m_slideContinueY) {
            m_slideContinueX = false;
            m_slideContinueY = false;
            m_gestureAction = GA_slide;
            qDebug() << "slide start" << timeSpace;
        } else {
            qDebug() << "null start" << timeSpace;
            m_gestureAction = GA_null;
        }
        break;
    }
    case Qt::GestureFinished: {
        qDebug() << "Tap gesture triggered with finished";
        m_gestureAction = GA_null;
        break;
    }
    default: {
        qDebug() << "Tap gesture triggered with default";
        Q_ASSERT(false);
        break;
    }
    }
    qDebug() << "Tap gesture triggered completed";
}

void TextEdit::tapAndHoldGestureTriggered(QTapAndHoldGesture *tapAndHold)
{
    qDebug() << "Tap and hold gesture triggered";
    //单指长按
    switch (tapAndHold->state()) {
    case Qt::GestureStarted:
        m_gestureAction = GA_hold;
        qDebug() << "Tap and hold gesture triggered with started";
        break;
    case Qt::GestureUpdated:
        qDebug() << "Tap and hold gesture triggered with updated";
        break;
    case Qt::GestureCanceled:
        qDebug() << "Tap and hold gesture triggered with canceled";
        break;
    case Qt::GestureFinished:
        qDebug() << "Tap and hold gesture triggered with finished";
        m_gestureAction = GA_null;
        break;
    default:
        break;
    }
    qDebug() << "Tap and hold gesture triggered completed";
}

void TextEdit::panTriggered(QPanGesture *pan)
{
    qDebug() << "Pan gesture triggered";
    //两指平移
    switch (pan->state()) {
    case Qt::GestureStarted:
        m_gestureAction = GA_pan;
        qDebug() << "Pan gesture triggered with started";
        break;
    case Qt::GestureUpdated:
        qDebug() << "Pan gesture triggered with updated";
        break;
    case Qt::GestureCanceled:
        qDebug() << "Pan gesture triggered with canceled";
        break;
    case Qt::GestureFinished:
        m_gestureAction = GA_null;
        qDebug() << "Pan gesture triggered with finished";
        break;
    default:
        qDebug() << "Pan gesture triggered with default";
        break;
    }
    qDebug() << "Pan gesture triggered completed";
}

void TextEdit::pinchTriggered(QPinchGesture *pinch)
{
    qDebug() << "Pinch gesture triggered";
    //两指拉伸   -----缩放or放大
    switch (pinch->state()) {
    case Qt::GestureStarted: {
        qDebug() << "Pinch gesture triggered with started";
        m_gestureAction = GA_pinch;
        if (static_cast<int>(m_scaleFactor) != m_fontSize) {
            m_scaleFactor = m_fontSize;
        }
        break;
    }
    case Qt::GestureUpdated: {
        qDebug() << "Pinch gesture triggered with updated";
        QPinchGesture::ChangeFlags changeFlags = pinch->changeFlags();
        if (changeFlags & QPinchGesture::ScaleFactorChanged) {
            m_currentStepScaleFactor = pinch->totalScaleFactor();
        }
        break;
    }
    case Qt::GestureCanceled: {
        qDebug() << "Pinch gesture triggered with canceled";
        break;
    }
    case Qt::GestureFinished: {
        qDebug() << "Pinch gesture triggered with finished";
        m_gestureAction = GA_null;
        m_scaleFactor *= m_currentStepScaleFactor;
        m_currentStepScaleFactor = 1;
        break;
    }
    default: {
        qDebug() << "Pinch gesture triggered with default";
        break;
    }
    }//switch

    //QFont font = getVTFont();
    int size = static_cast<int>(m_scaleFactor * m_currentStepScaleFactor);
    if (size < 8)
        size = 8;
    if (size > 50)
        size = 50;
    //根据移动距离设置字体大小
    setFontSize(size);
    //同步设置界面字体栏数值
    qobject_cast<Window *>(this->window())->changeSettingDialogComboxFontNumber(size);
    qDebug() << "Pinch gesture triggered completed, size:" << size;
}

void TextEdit::swipeTriggered(QSwipeGesture *swipe)
{
    qDebug() << "Swipe gesture triggered";
    //三指滑动
//    switch (swipe->state()) {
//    case Qt::GestureStarted:
//        m_gestureAction = GA_swipe;
//        break;
//    case Qt::GestureUpdated:
//        break;
//    case Qt::GestureCanceled:
//        m_gestureAction = GA_null;
//        break;
//    case Qt::GestureFinished:
//        Q_ASSERT(false);
//        break;
//    default:
//        Q_ASSERT(false);
//        break;
//    }

}

void TextEdit::slideGestureY(qreal diff)
{
    qDebug() << "Slide gesture y, diff:" << diff;
    static qreal delta = 0.0;
    int step = static_cast<int>(diff + delta);
    delta = diff + delta - step;

    verticalScrollBar()->setValue(verticalScrollBar()->value() + step);
    qDebug() << "Slide gesture y, verticalScrollBar()->value():" << verticalScrollBar()->value();
}

void TextEdit::slideGestureX(qreal diff)
{
    qDebug() << "Slide gesture x, diff:" << diff;
    static qreal delta = 0.0;
    int step = static_cast<int>(diff + delta);
    delta = diff + delta - step;

    horizontalScrollBar()->setValue(horizontalScrollBar()->value() + step * 30);
    qDebug() << "Slide gesture x, horizontalScrollBar()->value():" << horizontalScrollBar()->value();
}

void TextEdit::setTheme(const QString &path)
{
    qDebug() << "Set theme, path:" << path;
    QVariantMap jsonMap = Utils::getThemeMapFromPath(path);
    QVariantMap textStylesMap = jsonMap["text-styles"].toMap();
    QString themeCurrentLineColor = jsonMap["editor-colors"].toMap()["current-line"].toString();
    QString textColor = textStylesMap["Normal"].toMap()["text-color"].toString();

    m_backgroundColor = QColor(jsonMap["editor-colors"].toMap()["background-color"].toString());
    m_currentLineColor = QColor(themeCurrentLineColor);
    m_currentLineNumberColor = QColor(jsonMap["editor-colors"].toMap()["current-line-number"].toString());
    m_lineNumbersColor = QColor(jsonMap["editor-colors"].toMap()["line-numbers"].toString());
    m_regionMarkerColor = QColor(textStylesMap["RegionMarker"].toMap()["selected-text-color"].toString());
    m_selectionColor = QColor(textStylesMap["Normal"].toMap()["selected-text-color"].toString());
    m_selectionBgColor = QColor(textStylesMap["Normal"].toMap()["selected-bg-color"].toString());
    m_bracketMatchFormat = currentCharFormat();
    m_bracketMatchFormat.setForeground(QColor(jsonMap["editor-colors"].toMap()["bracket-match-fg"].toString()));
    m_bracketMatchFormat.setBackground(QColor(jsonMap["editor-colors"].toMap()["bracket-match-bg"].toString()));

    QString findMatchBgColor = jsonMap["editor-colors"].toMap()["find-match-background"].toString();
    QString findMatchFgColor = jsonMap["editor-colors"].toMap()["find-match-foreground"].toString();
    m_findMatchFormat = currentCharFormat();
    m_findMatchFormat.setBackground(QColor(findMatchBgColor));
    m_findMatchFormat.setForeground(QColor(findMatchFgColor));

    QString findHighlightBgColor = jsonMap["editor-colors"].toMap()["find-highlight-background"].toString();
    QString findHighlightFgColor = jsonMap["editor-colors"].toMap()["find-highlight-foreground"].toString();
    m_findHighlightSelection.format.setProperty(QTextFormat::BackgroundBrush, QBrush(QColor(findHighlightBgColor)));
    m_findHighlightSelection.format.setProperty(QTextFormat::ForegroundBrush, QBrush(QColor(findHighlightFgColor)));

    m_beginBracketSelection.format = m_bracketMatchFormat;
    m_endBracketSelection.format = m_bracketMatchFormat;

    int iVerticalScrollValue = getScrollOffset();
    int iHorizontalScrollVaule = horizontalScrollBar()->value();
    getScrollOffset();
    verticalScrollBar()->setSliderPosition(iVerticalScrollValue);
    horizontalScrollBar()->setSliderPosition(iHorizontalScrollVaule);

    m_pLeftAreaWidget->m_pLineNumberArea->update();
    m_pLeftAreaWidget->m_pBookMarkArea->update();

    updateHighlightLineSelection();
    renderAllSelections();
    qDebug() << "Set theme completed";
}

void TextEdit::removeHighlightWordUnderCursor()
{
    qDebug() << "Remove highlight word under cursor";
    //m_highlightWordCacheCursor = m_wordUnderCursorSelection.cursor;
    QTextEdit::ExtraSelection selection;
    //m_wordUnderCursorSelection = selection;

    renderAllSelections();
    m_nBookMarkHoverLine = -1;
    m_pLeftAreaWidget->m_pBookMarkArea->update();
    qDebug() << "Remove highlight word under cursor completed";
}

void TextEdit::setSettings(Settings *keySettings)
{
    qDebug() << "Set settings, keySettings:" << keySettings;
    m_settings = keySettings;
}

/**
 * @brief 拷贝选中的文本
 * @param ignoreCheck 是否忽略权限判断(外部已进行)，默认false
 */
void TextEdit::copySelectedText(bool ignoreCheck)
{
    qDebug() << "Copy selected text, ignoreCheck:" << ignoreCheck;
    // 添加权限判断是否允许拷贝，剪切；防止后续可能调用接口，冗余处理
    if (!ignoreCheck && !Utils::enableClipCopy(getFilePath())) {
        qDebug() << "Copy selected text, ignoreCheck is false and enableClipCopy is false";
        return;
    }

    if (m_bIsAltMod && !m_altModSelections.isEmpty()) {
        qDebug() << "Copy selected text, m_bIsAltMod is true and m_altModSelections is not empty";
        QString data;
        for (auto sel : m_altModSelections) {
            data.append(sel.cursor.selectedText());
        }
        QClipboard *clipboard = QApplication::clipboard();   //获取系统剪贴板指针
        clipboard->setText(data);
    } else {
        qDebug() << "Copy selected text, m_bIsAltMod is false or m_altModSelections is empty";
        QClipboard *clipboard = QApplication::clipboard();   //获取系统剪贴板指针
        if (textCursor().hasSelection()) {
            clipboard->setText(textCursor().selection().toPlainText());
            qDebug() << "Copy selected text, textCursor().hasSelection() is true";
            tryUnsetMark();
        } else {
            qDebug() << "Copy selected text, textCursor().hasSelection() is false";
            clipboard->setText(m_highlightWordCacheCursor.selectedText());
        }
    }
    tryUnsetMark();
    qDebug() << "Copy selected text completed";
}

/**
 * @brief 剪切选中的文本
 * @param ignoreCheck 是否忽略权限判断(外部已进行)，默认false
 */
void TextEdit::cutSelectedText(bool ignoreCheck)
{
    qDebug() << "Cut selected text, ignoreCheck:" << ignoreCheck;
    // 添加权限判断是否允许拷贝，剪切；防止后续可能调用接口，冗余处理
    if (!ignoreCheck && !Utils::enableClipCopy(getFilePath())) {
        qDebug() << "Cut selected text, ignoreCheck is false and enableClipCopy is false";
        return;
    }

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(textCursor().selection().toPlainText());

    QTextCursor cursor = textCursor();
    cursor.removeSelectedText();
    setTextCursor(cursor);

    unsetMark();
    qDebug() << "Cut selected text completed";
}

void TextEdit::pasteText()
{
    qDebug() << "Paste text";
    QPlainTextEdit::paste();
    unsetMark();
    qDebug() << "Paste text unsetMark completed";
}

void TextEdit::setMark()
{
    qDebug() << "Set mark";
    bool currentMark = m_cursorMark;
    bool markCursorChanged = false;
    if (m_cursorMark) {
        qDebug() << "Set mark, m_cursorMark is true";
        if (textCursor().hasSelection()) {
            qDebug() << "Set mark, textCursor().hasSelection() is true";
            markCursorChanged = true;

            QTextCursor cursor = textCursor();
            cursor.clearSelection();
            setTextCursor(cursor);
        } else {
            qDebug() << "Set mark, textCursor().hasSelection() is false";
            m_cursorMark = false;
        }
    } else {
        qDebug() << "Set mark, m_cursorMark is false";
        m_cursorMark = true;
    }

    if (m_cursorMark != currentMark || markCursorChanged) {
        qDebug() << "Set mark, m_cursorMark != currentMark || markCursorChanged";
        cursorMarkChanged(m_cursorMark, textCursor());
    }
    qDebug() << "Set mark completed";
}

void TextEdit::unsetMark()
{
    qDebug() << "Unset mark";
    bool currentMark = m_cursorMark;

    m_cursorMark = false;

    if (m_cursorMark != currentMark) {
        qDebug() << "Unset mark, m_cursorMark != currentMark";
        cursorMarkChanged(m_cursorMark, textCursor());
    }
    qDebug() << "Unset mark completed";
}

bool TextEdit::tryUnsetMark()
{
    qDebug() << "Try unset mark";
    if (m_cursorMark) {
        QTextCursor cursor = textCursor();
        cursor.clearSelection();
        setTextCursor(cursor);

        unsetMark();

        qDebug() << "Try unset mark, m_cursorMark is true, return true";
        return true;
    } else {
        qDebug() << "Try unset mark, m_cursorMark is false";
        return false;
    }
}

void TextEdit::exchangeMark()
{
    qDebug() << "Exchange mark";
    unsetMark();

    qDebug() << "Exchange mark, textCursor().hasSelection():" << textCursor().hasSelection();
    if (textCursor().hasSelection()) {
        qDebug() << "Exchange mark, textCursor().hasSelection() is true";
        // Record cursor and seleciton position before move cursor.
        int actionStartPos = textCursor().position();
        int selectionStartPos = textCursor().selectionStart();
        int selectionEndPos = textCursor().selectionEnd();

        QTextCursor cursor = textCursor();
        if (actionStartPos == selectionStartPos) {
            qDebug() << "Exchange mark, actionStartPos == selectionStartPos";
            cursor.setPosition(selectionStartPos, QTextCursor::MoveAnchor);
            cursor.setPosition(selectionEndPos, QTextCursor::KeepAnchor);
        } else {
            qDebug() << "Exchange mark, actionStartPos != selectionStartPos";
            cursor.setPosition(selectionEndPos, QTextCursor::MoveAnchor);
            cursor.setPosition(selectionStartPos, QTextCursor::KeepAnchor);
        }

        setTextCursor(cursor);
    }
    qDebug() << "Exchange mark completed";
}

void TextEdit::saveMarkStatus()
{
    qDebug() << "Save mark status";
    m_cursorMarkStatus = m_cursorMark;
    m_cursorMarkPosition = textCursor().anchor();
    qDebug() << "Save mark status completed";
}

void TextEdit::restoreMarkStatus()
{
    qDebug() << "Restore mark status";
    if (m_cursorMarkStatus) {
        qDebug() << "Restore mark status, m_cursorMarkStatus is true";
        QTextCursor currentCursor = textCursor();

        QTextCursor cursor = textCursor();
        cursor.setPosition(m_cursorMarkPosition, QTextCursor::MoveAnchor);
        cursor.setPosition(currentCursor.position(), QTextCursor::KeepAnchor);

        setTextCursor(cursor);
    }
    qDebug() << "Restore mark status completed";
}


void TextEdit::slot_translate()
{
    qDebug() << "Translate";
    auto ret = IflytekAiAssistant::instance()->textToTranslate();
    if (IflytekAiAssistant::Success != ret) {
        qDebug() << "Translate, ret is not Success";
        Q_EMIT popupNotify(IflytekAiAssistant::instance()->errorString(ret), true);
    }
    qDebug() << "Translate completed";
}

QString TextEdit::getWordAtCursor()
{
    qDebug() << "Get word at cursor";
    if (!characterCount()) {
        qDebug() << "Get word at cursor, characterCount() is 0";
        return "";
    } else {
        qDebug() << "Get word at cursor, characterCount() is not 0";
        QTextCursor cursor = textCursor();
        QChar currentChar = toPlainText().at(std::max(cursor.position() - 1, 0));

        cursor.movePosition(QTextCursor::NoMove, QTextCursor::MoveAnchor);
        while (!currentChar.isSpace() && cursor.position() != 0) {
            // while (!currentChar.isSpace() && cursor.position() != 0) {
            cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
            currentChar = toPlainText().at(std::max(cursor.position() - 1, 0));

            if (currentChar == '-') {
                break;
            }
        }

        qDebug() << "Get word at cursor, cursor.selectedText():" << cursor.selectedText();
        return cursor.selectedText();
    }
    qDebug() << "Get word at cursor completed";
}

QString TextEdit::getWordAtMouse()
{
    qDebug() << "Get word at mouse";
    if (!characterCount()) {
        qDebug() << "Get word at mouse, characterCount() is 0";
        return "";
    } else {
        qDebug() << "Get word at mouse, characterCount() is not 0";
        auto pos = mapFromGlobal(QCursor::pos());
        QTextCursor cursor(cursorForPosition(pos));

        // Get cursor rectangle.
        auto rect = cursorRect(cursor);
        int widthOffset = 10;
        rect.setX(std::max(rect.x() - widthOffset / 2, 0));
        rect.setWidth(rect.width() + widthOffset);

        // Just highlight word under pointer when cursor rectangle contain mouse pointer coordinate.
        if ((rect.x() <= pos.x()) &&
                (pos.x() <= rect.x() + rect.width()) &&
                (rect.y() <= pos.y()) &&
                (pos.y() <= rect.y() + rect.height())) {
            cursor.select(QTextCursor::WordUnderCursor);

            qDebug() << "Get word at mouse, cursor.selectedText():" << cursor.selectedText();
            return cursor.selectedText();
        } else {
            qDebug() << "Get word at mouse, cursor.selectedText() is empty";
            return "";
        }
    }
    qDebug() << "Get word at mouse completed";
}

void TextEdit::toggleReadOnlyMode(bool notNotify)
{
    qDebug() << "Toggle read only mode, notNotify:" << notNotify;
    if (m_readOnlyMode) {
        qDebug() << "Toggle read only mode, m_readOnlyMode is true";
        if (m_cursorMode == Overwrite) {
            qDebug() << "Toggle read only mode, m_cursorMode is Overwrite";
            emit cursorModeChanged(Overwrite);
        } else {
            qDebug() << "Toggle read only mode, m_cursorMode is Insert";
            emit cursorModeChanged(Insert);
        }
        setReadOnly(false);
        m_readOnlyMode = false;
        setCursorWidth(1);
        updateHighlightLineSelection();

        if (!notNotify) {
            qDebug() << "Toggle read only mode, notNotify is false";
            popupNotify(tr("Read-Only mode is off"));
        }
    } else {
        qDebug() << "Toggle read only mode, m_readOnlyMode is false";
        m_readOnlyMode = true;
        setReadOnly(true);
        setCursorWidth(0); //隐藏光标
        document()->clearUndoRedoStacks();
        updateHighlightLineSelection();

        if (!notNotify) {
            qDebug() << "Toggle read only mode, notNotify is false";
            popupNotify(tr("Read-Only mode is on"));
        }
        emit cursorModeChanged(Readonly);
    }
    qDebug() << "Toggle read only mode completed";
}

void TextEdit::toggleComment(bool bValue)
{
    qDebug() << "Toggle comment, bValue:" << bValue;
    const auto def = m_repository.definitionForFileName(QFileInfo(m_sFilePath).fileName());
    QTextCursor selectionCursor = textCursor();
    selectionCursor.movePosition(QTextCursor::StartOfBlock);
    selectionCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    QString text = selectionCursor.selectedText();
    // init base.
    bool isBlankLine = text.trimmed().isEmpty();
    if (characterCount() == 0 || isBlankLine || def.filePath().isEmpty()) {
        qDebug() << "Toggle comment, characterCount() is 0 or isBlankLine or def.filePath().isEmpty()";
        return;
    }

    if (m_readOnlyMode) {
        qDebug() << "Toggle comment, m_readOnlyMode is true";
        popupNotify(tr("Read-Only mode is on"));
        return;
    }

    //不同类型文件注释符号不同 梁卫东　２０２０－０９－０３　１７：２８：４５
    bool bHasCommnent = false;
    if (m_commentDefinition.isValid()) {
        QString  multiLineCommentMark = m_commentDefinition.multiLineStart.simplified();
        QString  singleLineCommentMark = m_commentDefinition.singleLine.simplified();
        if (multiLineCommentMark.isEmpty() && singleLineCommentMark.isEmpty()) {
            bHasCommnent = false;
        } else {
            bHasCommnent = true;
        }
    }
    if (!bHasCommnent) {
        qDebug() << "Toggle comment, !bHasCommnent";
        return;
    }

    QString name = def.name();
    if (name == "Markdown") {
        qDebug() << "Toggle comment, name is Markdown";
        return;
    }

    if (!def.filePath().isEmpty()) {
        qDebug() << "Toggle comment, def.filePath() is not empty";
        if (bValue) {
            qDebug() << "Toggle comment, bValue is true";
            setComment();
        } else {
            qDebug() << "Toggle comment, bValue is false";
            removeComment();
            //unCommentSelection();
        }
    } else {
        qDebug() << "Toggle comment, def.filePath() is empty";
        // do not need to prompt the user.
        // popupNotify(tr("File does not support syntax comments"));
    }
    qDebug() << "Toggle comment completed";
}

int TextEdit::getNextWordPosition(QTextCursor &cursor, QTextCursor::MoveMode moveMode)
{
    qDebug() << "Get next word position, cursor:" << cursor.position();
    // FIXME(rekols): if is empty text, it will crash.
    if (!characterCount()) {
        qDebug() << "Get next word position, characterCount() is 0";
        return 0;
    }

    // Move next char first.
    QTextCursor copyCursor = cursor;
    copyCursor.movePosition(QTextCursor::NextCharacter, moveMode);
    QString currentChar = copyCursor.selection().toPlainText();

    // Just to next non-space char if current char is space.
    if (currentChar.data()->isSpace()) {
        qDebug() << "Get next word position, currentChar is space";
        while (copyCursor.position() < characterCount() - 1 && currentChar.data()->isSpace()) {
            copyCursor.movePosition(QTextCursor::NextCharacter, moveMode);
            currentChar = copyCursor.selection().toPlainText();
        }
        while (copyCursor.position() < characterCount() - 1 && !atWordSeparator(copyCursor.position())) {
            copyCursor.movePosition(QTextCursor::NextCharacter, moveMode);

            //cursor.movePosition(QTextCursor::NextCharacter, moveMode);
        }
    }
    // Just to next word-separator char.
    else {
        qDebug() << "Get next word position, currentChar is not space";
        while (copyCursor.position() < characterCount() - 1 && !atWordSeparator(copyCursor.position())) {
            copyCursor.movePosition(QTextCursor::NextCharacter, moveMode);
        }
    }

    qDebug() << "Get next word position, copyCursor.position():" << copyCursor.position();
    return copyCursor.position();
}

int TextEdit::getPrevWordPosition(QTextCursor cursor, QTextCursor::MoveMode moveMode)
{
    qDebug() << "Get prev word position, cursor:" << cursor.position();
    if (!characterCount()) {
        qDebug() << "Get prev word position, characterCount() is 0";
        return 0;
    }

    // Move prev char first.
    QTextCursor copyCursor = cursor;
    copyCursor.movePosition(QTextCursor::PreviousCharacter, moveMode);
    QString currentChar = copyCursor.selection().toPlainText();

    // Just to next non-space char if current char is space.
    if (currentChar.data()->isSpace()) {
        qDebug() << "Get prev word position, currentChar is space";
        while (copyCursor.position() > 0 && currentChar.data()->isSpace()) {
            copyCursor.movePosition(QTextCursor::PreviousCharacter, moveMode);
            currentChar = copyCursor.selection().toPlainText();
        }
    }
    // Just to next word-separator char.
    else {
        qDebug() << "Get prev word position, currentChar is not space";
        while (copyCursor.position() > 0 && !atWordSeparator(copyCursor.position())) {
            copyCursor.movePosition(QTextCursor::PreviousCharacter, moveMode);
        }
    }

    qDebug() << "Get prev word position, copyCursor.position():" << copyCursor.position();
    return copyCursor.position();
}

bool TextEdit::atWordSeparator(int position)
{
    qDebug() << "At word separator, position:" << position;
    QTextCursor copyCursor = textCursor();
    copyCursor.setPosition(position, QTextCursor::MoveAnchor);
    copyCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
    QString currentChar = copyCursor.selection().toPlainText();
    qDebug() << "At word separator, currentChar:" << currentChar;
    return m_wordSepartors.contains(currentChar);
}

void TextEdit::showCursorBlink()
{
    qDebug() << "Show cursor blink";
    // -1 表示恢复Qt的默认值
    QApplication::setCursorFlashTime(-1);
}

void TextEdit::hideCursorBlink()
{
    qDebug() << "Hide cursor blink";
    QApplication::setCursorFlashTime(0);
}

void TextEdit::setReadOnlyPermission(bool permission)
{
    qDebug() << "Set read only permission, permission:" << permission;
    m_bReadOnlyPermission = permission; //true为不可读
    if (permission) {
        qDebug() << "Set read only permission, permission is true";
        m_Permission2 = true;
        setReadOnly(true);
        emit cursorModeChanged(Readonly);
    } else {
        qDebug() << "Set read only permission, permission is false";
        m_Permission = false;
        if (!m_readOnlyMode) {
            qDebug() << "Set read only permission, m_readOnlyMode is false";
            setReadOnly(false);
            //emit cursorModeChanged(Insert);
        } else {
            qDebug() << "Set read only permission, m_readOnlyMode is true";
            setReadOnly(true);
            emit cursorModeChanged(Readonly);
        }
    }
    SendtoggleReadOnlyMode();
    qDebug() << "Set read only permission completed";
}

void TextEdit::SendtoggleReadmessage()
{
    qDebug() << "Send toggle read only message";
    if (!m_bReadOnlyPermission) {
        qDebug() << "Send toggle read only message, m_bReadOnlyPermission is false";
        if (m_cursorMode == Overwrite) {
            qDebug() << "Send toggle read only message, m_cursorMode is Overwrite";
            emit cursorModeChanged(Overwrite);
        } else {
            qDebug() << "Send toggle read only message, m_cursorMode is Insert";
            emit cursorModeChanged(Insert);
        }
        setReadOnly(false);
        setCursorWidth(1);
        updateHighlightLineSelection();
    } else {
        qDebug() << "Send toggle read only message, m_bReadOnlyPermission is true";
        setReadOnly(true);
        setCursorWidth(0); //隐藏光标
        document()->clearUndoRedoStacks();
        updateHighlightLineSelection();
        emit cursorModeChanged(Readonly);
    }
    qDebug() << "Send toggle read only message completed";
}

bool TextEdit::isAbleOperation(Utils::OperationType iOperationType)
{
    qDebug() << "Is able operation, iOperationType:" << iOperationType;
    bool bRet = true;
    qlonglong operationDataSize = 0;
    qlonglong currentDocSize = document()->characterCount();

    if (iOperationType == Utils::CopyOperation) {
        qDebug() << "Is able operation, iOperationType is CopyOperation";
        if (m_isSelectAll) {
            qDebug() << "Is able operation, m_isSelectAll is true";
            operationDataSize = currentDocSize;
        } else if (m_bIsAltMod && !m_altModSelections.isEmpty()) {
            qDebug() << "Is able operation, m_bIsAltMod is true and m_altModSelections is not empty";
            for (auto it = m_altModSelections.begin(); it != m_altModSelections.end(); it++) {
                auto &itCursor = (*it).cursor;
                operationDataSize += (itCursor.selectionEnd() - itCursor.selectionStart());
                if (it != m_altModSelections.end() - 1)
                    operationDataSize++;
            }
        } else if (textCursor().hasSelection()) {
            qDebug() << "Is able operation, textCursor().hasSelection() is true";
            operationDataSize = textCursor().selectionEnd() - textCursor().selectionStart();
        }
        qDebug() << "Is able operation, operationDataSize:" << operationDataSize;
        bRet = Utils::isMemorySufficientForOperation(iOperationType, operationDataSize, currentDocSize);
        qDebug() << "Is able operation, bRet:" << bRet;

    } else if (iOperationType == Utils::PasteOperation) {
        qDebug() << "Is able operation, iOperationType is PasteOperation";
        const QClipboard *clipboard = QApplication::clipboard();
        QString strClipboardText = clipboard->text();
        operationDataSize = strClipboardText.size();
        qDebug() << "Is able operation, operationDataSize:" << operationDataSize;
        bRet = Utils::isMemorySufficientForOperation(iOperationType, operationDataSize, currentDocSize);
        qDebug() << "Is able operation, bRet:" << bRet;
    }

    qDebug() << "Is able operation completed";
    return bRet;
}

void TextEdit::SendtoggleReadOnlyMode()
{
    qDebug() << "Send toggle read only mode";
    if (m_bReadOnlyPermission && !m_Permission) {
        qDebug() << "Send toggle read only mode, m_bReadOnlyPermission is true and m_Permission is false";
        m_Permission = m_bReadOnlyPermission;
        SendtoggleReadmessage();
    } else if (m_Permission2 && !m_bReadOnlyPermission) {
        qDebug() << "Send toggle read only mode, m_Permission2 is true and m_bReadOnlyPermission is false";
        m_Permission2 = m_bReadOnlyPermission;
        SendtoggleReadmessage();
    }
    qDebug() << "Send toggle read only mode completed";
}

bool TextEdit::getReadOnlyPermission()
{
    qDebug() << "Get read only permission";
    return m_bReadOnlyPermission;
}

bool TextEdit::getReadOnlyMode()
{
    qDebug() << "Get read only mode";
    return m_readOnlyMode;
}

void TextEdit::hideRightMenu()
{
    qDebug() << "Hide right menu";
    //arm平台全屏然后恢复窗口，右键菜单不会消失，所以加了这个函数
    if (m_rightMenu) {
        qDebug() << "Send toggle read only mode, m_rightMenu is not null";
        m_rightMenu->hide();
    }
    qDebug() << "Hide right menu completed";
}

void TextEdit::bookMarkAreaPaintEvent(QPaintEvent *event)
{
    qDebug() << "Book mark area paint event";
    BookMarkWidget *bookMarkArea = m_pLeftAreaWidget->m_pBookMarkArea;
    QPainter painter(bookMarkArea);
    QColor lineNumberAreaBackgroundColor;
    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::ColorType::DarkType) {
        qDebug() << "Book mark area paint event, DGuiApplicationHelper::ColorType::DarkType";
        lineNumberAreaBackgroundColor = palette().brightText().color();
        lineNumberAreaBackgroundColor.setAlphaF(0.06);

        m_lineNumbersColor.setAlphaF(0.2);
    } else {
        qDebug() << "Book mark area paint event, DGuiApplicationHelper::ColorType::LightType";
        lineNumberAreaBackgroundColor = palette().brightText().color();
        lineNumberAreaBackgroundColor.setAlphaF(0.03);
        m_lineNumbersColor.setAlphaF(0.3);
    }

    QTextBlock lineBlock;//第几行文本块
    QImage image;
    QString pixmapPath;
    QList<int> list = m_listBookmark;
    bool bIsContains = false;

    if (!m_listBookmark.contains(m_nBookMarkHoverLine) && m_nBookMarkHoverLine != -1 && m_nBookMarkHoverLine <= blockCount()) {
        qDebug() << "Book mark area paint event, m_listBookmark does not contain m_nBookMarkHoverLine";
        list << m_nBookMarkHoverLine;
    } else {
        qDebug() << "Book mark area paint event, m_listBookmark contains m_nBookMarkHoverLine";
        bIsContains = true;
    }

    foreach (auto line, list) {
        lineBlock = document()->findBlockByNumber(line - 1);
        QTextCursor cur = textCursor();
        cur.setPosition(lineBlock.position(), QTextCursor::MoveAnchor);
        if (line == m_nBookMarkHoverLine && !bIsContains) {
            qDebug() << "Book mark area paint event, line == m_nBookMarkHoverLine and !bIsContains";
            if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::ColorType::DarkType) {
                pixmapPath = ":/images/like_hover_dark.svg";
                image = QImage(":/images/like_hover_dark.svg");
            } else {
                image = QImage(":/images/like_hover_light.svg");
                pixmapPath = ":/images/like_hover_light.svg";
            }
        } else {
            qDebug() << "Book mark area paint event, line != m_nBookMarkHoverLine or bIsContains";
            image = QImage(":/images/bookmark.svg");
            pixmapPath = ":/images/bookmark.svg";
        }

        if (line > 0) {
            qDebug() << "Book mark area paint event, line > 0";
            lineBlock = document()->findBlockByNumber(line - 1);
            if (!lineBlock.isVisible()) {
                qDebug() << "Book mark area paint event, lineBlock is not visible";
                continue;
            }

            int w = this->m_fontSize <= 15 ? 15 : m_fontSize;
            updateLeftWidgetWidth(w);
            int h = cursorRect(cur).height();
            // 绘制行纵向居中
            int offset = qMax(0, (h - w) / 2);
            //the language currently set by the system is Tibetan.
            if ("bo_CN" == Utils::getSystemLan())
                offset = h <= 20 ? 0 : h / 10;

            QRect rect(0, cursorRect(cur).y() + offset, w, w);
            QSvgRenderer render;
            render.load(pixmapPath);
            render.render(&painter, rect);
        }
    }
    qDebug() << "Book mark area paint event completed";
}

int TextEdit::getLineFromPoint(const QPoint &point)
{
    qDebug() << "Get line from point, point:" << point;
    QTextCursor cursor = cursorForPosition(point);
    QTextBlock block = cursor.block();
    qDebug() << "Get line from point, block.blockNumber():" << block.blockNumber();
    return block.blockNumber() + 1;
}

void TextEdit::addOrDeleteBookMark()
{
    qDebug() << "Add or delete book mark";
    int line = 0;
    if (m_bIsShortCut) {
        line = getCurrentLine();
        m_bIsShortCut = false;
    } else {
        line = getLineFromPoint(m_mouseClickPos);
    }

    if (line > blockCount()) {
        qDebug() << "Add or delete book mark, line > blockCount()";
        return;
    }

    if (m_listBookmark.contains(line)) {
        qDebug() << "Add or delete book mark, m_listBookmark contains line";
        m_listBookmark.removeOne(line);
        m_nBookMarkHoverLine = -1;
        qDebug() << "DeleteBookMark:" << line << m_listBookmark;
    } else {
        qDebug() << "Add or delete book mark, m_listBookmark does not contain line";
        m_listBookmark.push_back(line);
        qDebug() << "AddBookMark:" << line << m_listBookmark;
    }

    m_pLeftAreaWidget->m_pBookMarkArea->update();
    qDebug() << "Add or delete book mark completed";
}

void TextEdit::moveToPreviousBookMark()
{
    qDebug() << "Move to previous book mark";
    int line = getCurrentLine();
    int index = m_listBookmark.indexOf(line);

    if (index == -1 && !m_listBookmark.isEmpty()) {
        jumpToLine(m_listBookmark.last(), false);
        qDebug() << "Move to previous book mark, index == -1 and !m_listBookmark.isEmpty(), return";
        return;
    }

    if (index == 0) {
        qDebug() << "Move to previous book mark, index == 0";
        jumpToLine(m_listBookmark.last(), false);
    } else {
        qDebug() << "index != 0";
        jumpToLine(m_listBookmark.value(index - 1), false);
    }
    qDebug() << "Move to previous book mark completed";
}

void TextEdit::moveToNextBookMark()
{
    qDebug() << "Move to next book mark";
    int line = getCurrentLine();
    int index = m_listBookmark.indexOf(line);

    if (index == -1 && !m_listBookmark.isEmpty()) {
        jumpToLine(m_listBookmark.first(), false);
        qDebug() << "Move to next book mark, index == -1 and !m_listBookmark.isEmpty(), return";
        return;
    }

    if (index == m_listBookmark.count() - 1) {
        qDebug() << "Move to next book mark, index == m_listBookmark.count() - 1";
        jumpToLine(m_listBookmark.first(), false);
    } else {
        qDebug() << "Move to next book mark, index != m_listBookmark.count() - 1";
        jumpToLine(m_listBookmark.value(index + 1), false);
    }
    qDebug() << "Move to next book mark completed";
}

void TextEdit::checkBookmarkLineMove(int from, int charsRemoved, int charsAdded)
{
    qDebug() << "Check bookmark line move, from:" << from << "charsRemoved:" << charsRemoved << "charsAdded:" << charsAdded;
    Q_UNUSED(charsRemoved);
    Q_UNUSED(charsAdded);

    if (m_bIsFileOpen) {
        qDebug() << "Check bookmark line move, m_bIsFileOpen is true, return";
        return;
    }

    if (m_nLines != blockCount()) {
        qDebug() << "Check bookmark line move, m_nLines != blockCount()";
        QTextCursor cursor = textCursor();
        int nAddorDeleteLine = document()->findBlock(from).blockNumber() + 1;
        int currLine = textCursor().blockNumber() + 1;

        if (m_nLines > blockCount()) {
            qDebug() << "Check bookmark line move, m_nLines > blockCount()";
            foreach (const auto line, m_listBookmark) {
                qDebug() << "Check bookmark line move, foreach line:" << line;
                if (m_nSelectEndLine != -1) {
                    qDebug() << "Check bookmark line move, m_nSelectEndLine != -1";
                    if (nAddorDeleteLine < line && line <= m_nSelectEndLine) {
                        qDebug() << "Check bookmark line move, nAddorDeleteLine < line && line <= m_nSelectEndLine";
                        m_listBookmark.removeOne(line);
                    } else if (line > m_nSelectEndLine) {
                        qDebug() << "Check bookmark line move, line > m_nSelectEndLine";
                        m_listBookmark.replace(m_listBookmark.indexOf(line), line - m_nSelectEndLine + nAddorDeleteLine);
                    }
                } else {
                    qDebug() << "Check bookmark line move, m_nSelectEndLine == -1";
                    if (line == currLine + 1) {
                        qDebug() << "Check bookmark line move, line == currLine + 1";
                        m_listBookmark.removeOne(currLine + 1);
                    } else if (line > currLine + 1) {
                        qDebug() << "Check bookmark line move, line > currLine + 1";
                        m_listBookmark.replace(m_listBookmark.indexOf(line), line  - m_nLines + blockCount());
                    }
                }
            }
        } else {
            foreach (const auto line, m_listBookmark) {
                if (nAddorDeleteLine < line) {
                    qDebug() << "Check bookmark line move, nAddorDeleteLine < line";
                    m_listBookmark.replace(m_listBookmark.indexOf(line), line + blockCount() - m_nLines);
                }
            }
        }
    }
    m_nLines = blockCount();
    qDebug() << "Check bookmark line move completed";
}

void TextEdit::flodOrUnflodAllLevel(bool isFlod)
{
    qDebug() << "Processing" << (isFlod ? "fold" : "unfold") << "all code blocks in document";
    m_listMainFlodAllPos.clear();
    
    int totalBlocks = document()->blockCount();
    int processedBlocks = 0;
    int foldPointsFound = 0;
    
    //折叠
    if (isFlod) {
        qDebug() << "Searching for visible blocks with opening braces to fold";
        for (int line = 0; line < totalBlocks; line++) {
            if (blockContainStrBrackets(line)
                    && document()->findBlockByNumber(line).isVisible()
                    && !document()->findBlockByNumber(line).text().trimmed().startsWith("//")) {
                processedBlocks++;
                if (getNeedControlLine(line, false)) {
                    m_listMainFlodAllPos.append(line);
                    foldPointsFound++;
                    
                    if (foldPointsFound % 20 == 0) {
                        qDebug() << "Found" << foldPointsFound << "fold points so far";
                    }
                }
            }
        }
        //展开
    } else {
        qDebug() << "Searching for hidden blocks with opening braces to unfold";
        for (int line = 0; line < totalBlocks; line++) {
            if (blockContainStrBrackets(line)
                    && !document()->findBlockByNumber(line + 1).isVisible()
                    && !document()->findBlockByNumber(line).text().trimmed().startsWith("//")) {
                processedBlocks++;
                if (getNeedControlLine(line, true)) {
                    m_listMainFlodAllPos.append(line);
                    foldPointsFound++;
                    
                    if (foldPointsFound % 20 == 0) {
                        qDebug() << "Found" << foldPointsFound << "unfold points so far";
                    }
                }
            }
        }
    }
    
    qDebug() << "Found" << foldPointsFound << "fold points after processing" << processedBlocks 
             << "blocks out of" << totalBlocks << "total blocks";

    //折叠时出现点击光标选择行变短
    QPlainTextEdit::LineWrapMode curMode = this->lineWrapMode();
    QPlainTextEdit::LineWrapMode WrapMode = curMode ==  QPlainTextEdit::NoWrap ?  QPlainTextEdit::WidgetWidth :  QPlainTextEdit::NoWrap;
    this->setWordWrapMode(QTextOption::WrapAnywhere);
    this->setLineWrapMode(WrapMode);

    qDebug() << "Updating UI components after fold/unfold operation";
    m_pLeftAreaWidget->m_pFlodArea->update();
    m_pLeftAreaWidget->m_pLineNumberArea->update();
    m_pLeftAreaWidget->m_pBookMarkArea->update();

    viewport()->update();
    document()->adjustSize();

    this->setLineWrapMode(curMode);
    viewport()->update();
    qDebug() << (isFlod ? "Fold" : "Unfold") << "all operation completed with" << foldPointsFound << "points processed";
}

void TextEdit::flodOrUnflodCurrentLevel(bool isFlod)
{
    qDebug() << "Processing" << (isFlod ? "fold" : "unfold") << "operation at cursor position";
    int line = getLineFromPoint(m_mouseClickPos);
    qDebug() << "Target line for fold/unfold:" << (line - 1) << "based on mouse position at line" << line;
    
    bool success = getNeedControlLine(line - 1, !isFlod);
    if (success) {
        qDebug() << "Successfully" << (!isFlod ? "unfolded" : "folded") << "code block at line" << (line - 1);
    } else {
        qDebug() << "Could not" << (!isFlod ? "unfold" : "fold") << "code block at line" << (line - 1) << "- no valid fold point found";
    }
    
    qDebug() << "Updating UI components after fold/unfold operation";
    m_pLeftAreaWidget->m_pFlodArea->update();
    m_pLeftAreaWidget->m_pLineNumberArea->update();
    m_pLeftAreaWidget->m_pBookMarkArea->update();
    viewport()->update();
    document()->adjustSize();
    qDebug() << (isFlod ? "Fold" : "Unfold") << "operation completed at line" << (line - 1);
}

void TextEdit::getHideRowContent(int iLine)
{
    qDebug() << "Preparing fold preview content for line" << iLine;
    // 预览文本块没有必要读取所有文本数据，调整为仅读取部分
    // Note:需要注意单个文本块(一般为一行数据)过长的情况
    // 最大显示的预览文本块数量，不超过1000
    static int s_MaxDisplayBlockCount = 1000;

    // 查询折叠区域文本块范围
    QTextBlock beginBlock, endBlock, curBlock;
    bool bFoundBrace = findFoldBlock(iLine, beginBlock, endBlock, curBlock);

    // 根据背景色亮度设置高亮模式
    bool isDarkTheme = QColor(m_backgroundColor).lightness() < 128;
    qDebug() << "Setting highlight mode based on theme:" << (isDarkTheme ? "dark" : "light");
    m_foldCodeShow->initHighLight(m_sFilePath, !isDarkTheme);

    m_foldCodeShow->appendText("{", width());
    
    //左右括弧没有匹配到
    if (!bFoundBrace) {
        qDebug() << "No matching braces found, showing all subsequent content";
        // 读取文本块索引
        int curIndex = 0;
        int blocksAdded = 0;
        //遍历最后右括弧文本块 设置块隐藏或显示,显示文本块不超过1000
        while (beginBlock.isValid()
                && (curIndex++ < s_MaxDisplayBlockCount)) {
            m_foldCodeShow->appendText(beginBlock.text(), width());
            beginBlock = beginBlock.next();
            blocksAdded++;
        }
        qDebug() << "Added" << blocksAdded << "blocks to fold preview";

        //如果左右"{" "}"在同一行不折叠
    } else if (endBlock == curBlock) {
        qDebug() << "Braces on same line, no fold preview needed";
        return;
    } else {
        qDebug() << "Found matching braces from line" << beginBlock.blockNumber() << "to" << endBlock.blockNumber();
        // 读取文本块索引
        int curIndex = 0;
        int blocksAdded = 0;
        //遍历最后右括弧文本块 设置块隐藏或显示,显示文本块不超过1000
        while (beginBlock != endBlock
                && beginBlock.isValid()
                && (curIndex++ < s_MaxDisplayBlockCount)) {
            if (beginBlock.isValid()) {
                m_foldCodeShow->appendText(beginBlock.text(), width());
                blocksAdded++;
            }

            beginBlock = beginBlock.next();
        }

        if (endBlock.text().simplified() == "}") {
            qDebug() << "Adding closing brace from end block to preview";
            m_foldCodeShow->appendText("}", width());
        }

        m_foldCodeShow->appendText("}", width());
        qDebug() << "Added" << blocksAdded << "blocks to fold preview";
    }
    qDebug() << "Fold preview content preparation completed for line" << iLine;
}

bool TextEdit::isNeedShowFoldIcon(QTextBlock block)
{
    qDebug() << "Checking if fold icon needed for block at line" << block.blockNumber();
    QString blockText = block.text();
    bool hasFindLeft = false; // 是否已经找到当前行第一个左括号
    int rightNum = 0, leftNum = 0;//右括号数目、左括号数目
    
    // 分析代码行中的括号
    for (int i = 0 ; i < blockText.size(); ++i) {
        if (blockText.at(i) == "}" && hasFindLeft) {
            rightNum++;
        } else if (blockText.at(i) == "{") {
            if (!hasFindLeft) {
                hasFindLeft = true;
            }
            leftNum++;
        }
    }

    qDebug() << "Block analysis: found" << leftNum << "opening and" << rightNum << "closing braces";
    
    // 判断是否需要显示折叠图标
    bool needFoldIcon = (rightNum != leftNum);
    
    if (needFoldIcon) {
        qDebug() << "Fold icon needed - unbalanced braces in line" << block.blockNumber();
    } else {
        qDebug() << "No fold icon needed - balanced braces in line" << block.blockNumber();
    }
    
    return needFoldIcon;
}

int TextEdit::getHighLightRowContentLineNum(int iLine)
{
    qDebug() << "Calculating highlight range end line for fold starting at line" << iLine;
    // 查询折叠区域文本块范围
    QTextBlock beginBlock, endBlock, curBlock;
    bool bFoundBrace = findFoldBlock(iLine, beginBlock, endBlock, curBlock);

    int initialLine = iLine;
    int resultLine = iLine;
    
    //左右括弧没有匹配到
    if (!bFoundBrace) {
        qDebug() << "No matching closing brace found, counting all subsequent blocks";
        //遍历最后右括弧文本块 设置块隐藏或显示
        int blockCount = 0;
        while (beginBlock.isValid()) {
            resultLine++;
            beginBlock = beginBlock.next();
            blockCount++;
        }
        qDebug() << "Found" << blockCount << "blocks after line" << iLine << ", resulting end line:" << resultLine;
        return resultLine;
        //如果左右"{" "}"在同一行不折叠
    } else if (!bFoundBrace || endBlock == curBlock) {
        qDebug() << "Braces on same line or invalid fold point, returning original line" << iLine;
        return resultLine;
    } else {
        qDebug() << "Found matching brace at line" << endBlock.blockNumber() << ", counting blocks in between";
        int blockCount = 0;
        while (beginBlock != endBlock && beginBlock.isValid()) {
            resultLine++;
            beginBlock = beginBlock.next();
            blockCount++;
        }

        resultLine++;
        qDebug() << "Found" << blockCount << "blocks between braces, resulting end line:" << resultLine 
                 << "(+" << (resultLine - initialLine) << " lines)";
        return resultLine;
    }
}

void TextEdit::paintCodeFlod(QPainter *painter, QRect rect, bool flod)
{
    qDebug() << "Paint code flod, painter:" << painter << "rect:" << rect << "flod:" << flod;
    painter->save();

    painter->translate(rect.width() / 2.0, rect.height() / 2.0);
    painter->translate(0, rect.y());
    QPainterPath path;
    //QPointF p1(rect.x()+rect.width() * (1.0/3),rect.y()+rect.height()*(1.0/3));
    //QPointF p2(rect.x()+rect.width() * (2.0/3),rect.y()+rect.height()*(1.0/3));
    //QPointF p3(rect.x()+rect.width() * (1.0/2),rect.y()+rect.height()*(1.5/3));
    QPointF p1(-1 * rect.width() * (1.0 / 6), -1 * rect.height() * (1.0 / 6));
    QPointF p2(rect.width() * (1.0 / 6), -1 * rect.height() * (1.0 / 6));
    QPointF p3(0, 0);
    path.moveTo(p1);
    path.lineTo(p3);
    path.lineTo(p2);

    if (flod) {
        qDebug() << "Paint code flod, flod is true";
        painter->rotate(-90);
    }

    QPen pen(this->palette().windowText(), 2);
    painter->setPen(pen);
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->drawPath(path);

    painter->restore();
    qDebug() << "Paint code flod completed";
}

QColor TextEdit::getBackColor()
{
    qDebug() << "Get back color";
    return m_backgroundColor;
}

int TextEdit::lineNumberAreaWidth()
{
    qDebug() << "Line number area width";
    int digits = 1;
    int max = qMax(1, this->document()->blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    // 行号使用单独字体
    QFontMetrics fm(m_fontLineNumberArea);
    int w = fm.horizontalAdvance(QLatin1Char('9')) * digits;

    qDebug() << "Line number area width completed";
    return w > 15 ? w : 15;
}

void TextEdit::updateLeftWidgetWidth(int width)
{
    qDebug() << "Update left widget width, width:" << width;
    if (m_LeftAreaUpdateState != TextEdit::FileOpenBegin) {
        m_pLeftAreaWidget->m_pFlodArea->setFixedWidth(width);
        m_pLeftAreaWidget->m_pLineNumberArea->setFixedWidth(lineNumberAreaWidth());
        m_pLeftAreaWidget->m_pBookMarkArea->setFixedWidth(width);
        setLeftAreaUpdateState(TextEdit::Normal);
    }
    qDebug() << "Update left widget width completed";
}

int TextEdit::getLinePosYByLineNum(int iLine)
{
    qDebug() << "Get line pos y by line num, iLine:" << iLine;
    QTextBlock block = document()->findBlockByNumber(iLine);
    QTextCursor cur = textCursor();

    while (!block.isVisible()) {
        block = block.next();
    }

    cur.setPosition(block.position(), QTextCursor::MoveAnchor);
    qDebug() << "Get line pos y by line num completed";
    return cursorRect(cur).y();
}

bool TextEdit::ifHasHighlight()
{
    qDebug() << "If has highlight";
    if (!m_findHighlightSelection.cursor.isNull()) {
        qDebug() << "If has highlight, m_findHighlightSelection.cursor is not null";
        return m_findHighlightSelection.cursor.hasSelection();
    } else {
        qDebug() << "If has highlight, m_findHighlightSelection.cursor is null";
        return  false;
    }
    qDebug() << "If has highlight completed";
}

void TextEdit::setIsFileOpen()
{
    qDebug() << "Set is file open";
    m_bIsFileOpen = true;
}

void TextEdit::setTextFinished()
{
    qDebug() << "Set text finished";
    m_bIsFileOpen = false;
    m_nLines = blockCount();

    if (!m_listBookmark.isEmpty()) {
        qDebug() << "Set text finished, m_listBookmark is not empty";
        return;
    }

    QStringList bookmarkList = readHistoryRecordofBookmark();
    QStringList filePathList = readHistoryRecordofFilePath("advance.editor.browsing_history_file");
    QList<int> linesList;

    QString qstrPath = m_sFilePath;

    if (filePathList.contains(qstrPath)) {
        qDebug() << "Set text finished, filePathList contains qstrPath";
        int index = 2;
        QString qstrLines = bookmarkList.value(filePathList.indexOf(qstrPath));
        QString sign;

        for (int i = 0; i < qstrLines.size() - 1; i++) {
            sign = qstrLines.at(i);
            sign.append(qstrLines.at(i + 1));

            if (sign == ",*" || sign == ")*") {
                qDebug() << "Set text finished, sign == \",*\" || sign == \")\"";
                linesList << qstrLines.mid(index, i - index).toInt();
                index = i + 2;
            }
        }
        qDebug() << "Set text finished, filePathList does not contain qstrPath";
    }

    foreach (const auto line, linesList) {
        if (line <= document()->blockCount()) {
            if (!m_listBookmark.contains(line)) {
                qDebug() << "Set text finished, line <= document()->blockCount()";
                m_listBookmark << line;
            }
        }
    }
    qDebug() << "Set text finished, completed";
//    qDebug() << m_listBookmark << document()->blockCount();
}

QStringList TextEdit::readHistoryRecord(QString key)
{
    qDebug() << "Read history record, key:" << key;
    QString history = m_settings->settings->option(key)->value().toString();
    QStringList historyList;
    int nLeftPosition = history.indexOf("*{");
    int nRightPosition = history.indexOf("}*");

    while (nLeftPosition != -1) {
        historyList << history.mid(nLeftPosition, nRightPosition + 2 - nLeftPosition);
        nLeftPosition = history.indexOf("*{", nLeftPosition + 2);
        nRightPosition = history.indexOf("}*", nRightPosition + 2);
    }

    qDebug() << "Read history record completed";
    return historyList;
}

QStringList TextEdit::readHistoryRecordofBookmark()
{
    qDebug() << "Read history record of bookmark";
    QString history = m_settings->settings->option("advance.editor.browsing_history_file")->value().toString();
    QStringList bookmarkList;
    int nLeftPosition = history.indexOf("*(");
    int nRightPosition = history.indexOf(")*");

    while (nLeftPosition != -1) {
        bookmarkList << history.mid(nLeftPosition, nRightPosition + 2 - nLeftPosition);
        nLeftPosition = history.indexOf("*(", nLeftPosition + 2);
        nRightPosition = history.indexOf(")*", nRightPosition + 2);
    }

    qDebug() << "Read history record of bookmark completed";
    return bookmarkList;
}

QStringList TextEdit::readHistoryRecordofFilePath(QString key)
{
    qDebug() << "Read history record of file path, key:" << key;
    QString history = m_settings->settings->option(key)->value().toString();
    QStringList filePathList;
    int nLeftPosition = history.indexOf("*[");
    int nRightPosition = history.indexOf("]*");

    while (nLeftPosition != -1) {
        filePathList << history.mid(nLeftPosition + 2, nRightPosition - 2 - nLeftPosition);
        nLeftPosition = history.indexOf("*[", nLeftPosition + 2);
        nRightPosition = history.indexOf("]*", nRightPosition + 2);
    }

    qDebug() << "Read history record of file path completed";
    return filePathList;
}

void TextEdit::writeEncodeHistoryRecord()
{
    qDebug() << "Write encode history record";
    QString history = m_settings->settings->option("advance.editor.browsing_encode_history")->value().toString();

    QStringList pathList = readHistoryRecordofFilePath("advance.editor.browsing_encode_history");

    foreach (auto path, pathList) {
        QFileInfo f(path);
        if (!f.isFile()) {
            qDebug() << "Write encode history record, path is not a file";
            int nLeftPosition = history.indexOf(path);
            int nRightPosition = history.indexOf("}*", nLeftPosition);
            history.remove(nLeftPosition - 4, nRightPosition + 6 - nLeftPosition);
        }
    }

    int nLeftPosition = history.indexOf(m_sFilePath);
    int nRightPosition = history.indexOf("}*", nLeftPosition);

    if (history.contains(m_sFilePath)) {
        qDebug() << "Write encode history record, history contains m_sFilePath";
        history.remove(nLeftPosition - 4, nRightPosition + 6 - nLeftPosition);
    }

    QString encodeHistory = history + "*{*[" + m_sFilePath + "]*" + m_textEncode + "}*";
    m_settings->settings->option("advance.editor.browsing_encode_history")->setValue(encodeHistory);
    qDebug() << "Write encode history record completed";
}

QStringList TextEdit::readEncodeHistoryRecord()
{
    qDebug() << "Read encode history record";
    QString history = m_settings->settings->option("advance.editor.browsing_encode_history")->value().toString();
    QStringList filePathList;
    int nLeftPosition = history.indexOf("]*");
    int nRightPosition = history.indexOf("}*");

    while (nLeftPosition != -1) {
        filePathList << history.mid(nLeftPosition + 2, nRightPosition - 2 - nLeftPosition);
        nLeftPosition = history.indexOf("]*", nLeftPosition + 2);
        nRightPosition = history.indexOf("}*", nRightPosition + 2);
    }

    qDebug() << "Read encode history record completed";
    return filePathList;
}

void TextEdit::tellFindBarClose()
{
    qDebug() << "Tell find bar close";
    m_bIsFindClose = true;
}

void TextEdit::setEditPalette(const QString &activeColor, const QString &inactiveColor)
{
    qDebug() << "Set edit palette, activeColor:" << activeColor << "inactiveColor:" << inactiveColor;
    // Not recommend manually setPalette()
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    Q_UNUSED(activeColor)
    Q_UNUSED(inactiveColor)
#else
    QPalette pa = this->palette();
    pa.setColor(QPalette::Inactive, QPalette::Text, QColor(inactiveColor));
    pa.setColor(QPalette::Active, QPalette::Text, QColor(activeColor));
    setPalette(pa);
#endif
    qDebug() << "Set edit palette completed";
}

void TextEdit::setCodeFoldWidgetHide(bool isHidden)
{
    qDebug() << "Set code fold widget hide, isHidden:" << isHidden;
    if (m_foldCodeShow) {
        qDebug() << "Set code fold widget hide, m_foldCodeShow is not null";
        m_foldCodeShow->setHidden(isHidden);
    }
    qDebug() << "Set code fold widget hide completed";
}


void TextEdit::setTruePath(QString qstrTruePath)
{
    qDebug() << "Set true path, qstrTruePath:" << qstrTruePath;
    m_qstrTruePath = qstrTruePath;
    qDebug() << "Set true path completed";
}

QString TextEdit::getTruePath()
{
    qDebug() << "Get true path";
    if (m_qstrTruePath.isEmpty()) {
        qDebug() << "Get true path, m_qstrTruePath is empty";
        return m_sFilePath;
    }
    qDebug() << "Get true path completed";
    return  m_qstrTruePath;
}

QList<int> TextEdit::getBookmarkInfo()
{
    qDebug() << "Get bookmark info";
    return m_listBookmark;
}

void TextEdit::setBookMarkList(QList<int> bookMarkList)
{
    m_listBookmark = bookMarkList;
    qDebug() << "Set bookmark list completed";
}

void TextEdit::updateSaveIndex()
{
    qDebug() << "Update save index";
    m_lastSaveIndex = m_pUndoStack->index();
    qDebug() << "Update save index completed";
}

void TextEdit::isMarkCurrentLine(bool isMark, QString strColor,  qint64 timeStamp)
{
    qDebug() << "Is mark current line, isMark:" << isMark << "strColor:" << strColor << "timeStamp:" << timeStamp;
    qint64 operationTimeStamp = timeStamp;
    if (operationTimeStamp < 0) {
        qDebug() << "Is mark current line, timeStamp is less than 0";
        operationTimeStamp = QDateTime::currentDateTime().toMSecsSinceEpoch();
    }

    if (isMark) {
        qDebug() << "Is mark current line, isMark is true";
        QTextEdit::ExtraSelection selection;
        selection.cursor = textCursor();
        selection.format.setBackground(QColor(strColor));

        TextEdit::MarkOperation markOperation;
        markOperation.color = strColor;

        if (textCursor().hasSelection()) {
            markOperation.type = MarkOnce;
            qDebug() << "Is mark current line, textCursor().hasSelection() is true";
        } else {
            qDebug() << "Is mark current line, textCursor().hasSelection() is false";
            markOperation.type = MarkLine;
            int beginPos = textCursor().block().position();
            int endPos = beginPos + textCursor().block().length() - 1;
            selection.cursor.setPosition(beginPos, QTextCursor::MoveAnchor);
            selection.cursor.setPosition(endPos, QTextCursor::KeepAnchor);
        }
        //alt选中光标单独处理
        if (m_bIsAltMod) {
            qDebug() << "Is mark current line, m_bIsAltMod is true";
            for (int i = 0; i < m_altModSelections.size(); ++i) {
                markOperation.cursor = m_altModSelections[i].cursor;
                selection.cursor = m_altModSelections[i].cursor;
                m_markOperations.append(QPair<TextEdit::MarkOperation, qint64>(markOperation, operationTimeStamp));
                m_wordMarkSelections.append(
                    QPair<QTextEdit::ExtraSelection, qint64>
                    (selection, operationTimeStamp));
            }
        } else {
            qDebug() << "Is mark current line, m_bIsAltMod is false";
            markOperation.cursor = selection.cursor;
            m_markOperations.append(QPair<TextEdit::MarkOperation, qint64>(markOperation, operationTimeStamp));
            m_wordMarkSelections.append(
                QPair<QTextEdit::ExtraSelection, qint64>
                (selection, operationTimeStamp));
        }
    } else {
        qDebug() << "Is mark current line, isMark is false";
        clearMarksForTextCursor();
    }
    qDebug() << "Is mark current line completed";
}

void TextEdit::markAllKeywordInView()
{
    qDebug() << "Mark all keyword in view";
    if (m_markOperations.isEmpty()) {
        qDebug() << "Mark all keyword in view, m_markOperations is empty, return";
        return;
    }

    QList<QPair<TextEdit::MarkOperation, qint64>>::iterator it;

    for (it = m_markOperations.begin(); it != m_markOperations.end(); ++it) {
        if (MarkAllMatch == it->first.type) {
            qDebug() << "Mark all keyword in view, type is MarkAllMatch";
            // 标记当前视图根据匹配文本查找的所有文本
            markKeywordInView(it->first.matchText, it->first.color, it->second);
        } else if (MarkAll == it->first.type) {
            qDebug() << "Mark all keyword in view, type is MarkAll";
            markAllInView(it->first.color, it->second);
        }
    }

    renderAllSelections();
    qDebug() << "Mark all keyword in view completed";
}

bool TextEdit::markKeywordInView(QString keyword, QString color, qint64 timeStamp)
{
    qDebug() << "Mark keyword in view, keyword:" << keyword << "color:" << color << "timeStamp:" << timeStamp;
    qint64 operationTimeStamp = timeStamp;
    if (operationTimeStamp < 0) {
        qDebug() << "Mark keyword in view, timeStamp is less than 0";
        operationTimeStamp = QDateTime::currentDateTime().toMSecsSinceEpoch();
    }

    if (keyword.isEmpty()) {
        qDebug() << "Mark keyword in view, keyword is empty, retun false";
        return false;
    }

    QList<QTextEdit::ExtraSelection> listExtraSelection;
    QTextCharFormat format;
    bool ret = false;

    format.setBackground(QColor(color));
    ret = updateKeywordSelectionsInView(keyword, format, &listExtraSelection);

    // 构建带有时间戳的 listExtraSelectionWithTimeStamp
    QList<QPair<QTextEdit::ExtraSelection, qint64>> listExtraSelectionWithTimeStamp;
    for (int i = 0; i < listExtraSelection.size(); i++) {
        listExtraSelectionWithTimeStamp.append(QPair<QTextEdit::ExtraSelection, qint64>
                                               (listExtraSelection.at(i), operationTimeStamp));
    }

    if (ret) {
        qDebug() << "Mark keyword in view, ret is true";
        m_mapKeywordMarkSelections[keyword] = listExtraSelectionWithTimeStamp;
    }

    qDebug() << "Mark keyword in view completed";
    return ret;
}

void TextEdit::markAllInView(QString color, qint64 timeStamp)
{
    qDebug() << "Mark all keyword in view, color:" << color << "timeStamp:" << timeStamp;
    // 增加时间戳
    qint64 operationTimeStamp = timeStamp;
    if (operationTimeStamp < 0) {
        qDebug() << "Mark all keyword in view, timeStamp is less than 0";
        operationTimeStamp = QDateTime::currentDateTime().toMSecsSinceEpoch();
    }

    QTextEdit::ExtraSelection selection;
    QList<QPair<QTextEdit::ExtraSelection, qint64>> listSelections;

    // 此处选中操作存在错误
    // QScrollBar *pScrollBar = verticalScrollBar();
    // QPoint startPoint = QPointF(0, 0).toPoint();
    // QTextBlock beginBlock = cursorForPosition(startPoint).block();
    // QTextBlock endBlock;
    //
    // if (pScrollBar->maximum() > 0) {
    //     QPoint endPoint = QPointF(0, 1.5 * height()).toPoint();
    //     endBlock = cursorForPosition(endPoint).block();
    // } else {
    //     endBlock = document()->lastBlock();
    // }

    // selection.cursor = textCursor();
    // selection.cursor.setPosition(beginBlock.position(), QTextCursor::MoveAnchor);
    // selection.cursor.setPosition(endBlock.position() + endBlock.length() - 1, QTextCursor::KeepAnchor);

    // 标准 QTextCursor 选中操作
    QTextCursor textCursor(document());
    textCursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
    textCursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);

    selection.format.setBackground(QColor(color));
    selection.cursor = textCursor;

    listSelections.append(QPair<QTextEdit::ExtraSelection, qint64>
                          (selection, operationTimeStamp));

    m_mapKeywordMarkSelections[TEXT_EIDT_MARK_ALL] = listSelections;
    qDebug() << "Mark all keyword in view completed";
}

void TextEdit::isMarkAllLine(bool isMark, QString strColor)
{
    qDebug() << "Is mark all line, isMark:" << isMark << "strColor:" << strColor;
    // 增加时间戳
    qint64 timeStamp = QDateTime::currentDateTime().toMSecsSinceEpoch();

    if (isMark) {
        qDebug() << "Is mark all line, isMark is true";
        QString selectionText = textCursor().selectedText();
        if (selectionText.length() != 0 && selectionText.length() < (document()->characterCount() - 1)) {
            qDebug() << "Is mark all line, selectionText is not empty";
            // 没有使用的变量，去除
            // QList<QTextEdit::ExtraSelection> wordMarkSelections = m_wordMarkSelections;
            QList<QTextEdit::ExtraSelection> listExtraSelection;
            // 没有使用的变量，去除
            // QList<QTextEdit::ExtraSelection> listSelections;
            QTextEdit::ExtraSelection  extraSelection;
            QTextCharFormat format;
            format.setBackground(QColor(strColor));
            extraSelection.cursor = textCursor();
            extraSelection.format = format;

            TextEdit::MarkOperation markOperation;
            markOperation.type = MarkAllMatch;
            markOperation.cursor = textCursor();
            markOperation.color = strColor;
            markOperation.matchText = selectionText;
            m_markOperations.append(QPair<TextEdit::MarkOperation, qint64>(markOperation, timeStamp));

            if (updateKeywordSelectionsInView(selectionText, format, &listExtraSelection)) {
                qDebug() << "Is mark all line, updateKeywordSelectionsInView is true";

                QList<QPair<QTextEdit::ExtraSelection, qint64>> listExtraSelectionWithTimeStamp;
                for (int i = 0; i < listExtraSelection.size(); i++) {
                    listExtraSelectionWithTimeStamp.append(QPair<QTextEdit::ExtraSelection, qint64>
                                                           (listExtraSelection.at(i), timeStamp));
                }

                m_mapKeywordMarkSelections[selectionText] = listExtraSelectionWithTimeStamp;
            } else {
                qDebug() << "Is mark all line, updateKeywordSelectionsInView is false";
                // 选中部分在文档中仅此一个，查找标记失败了，进行补充处理
                QTextEdit::ExtraSelection extraSelect;
                listExtraSelection.append(extraSelection);
                QList<QPair<QTextEdit::ExtraSelection, qint64>> listExtraSelectionWithTimeStamp;
                for (int i = 0; i < listExtraSelection.size(); i++) {
                    listExtraSelectionWithTimeStamp.append(QPair<QTextEdit::ExtraSelection, qint64>
                                                           (listExtraSelection.at(i), timeStamp));
                }

                m_mapKeywordMarkSelections.insert(selectionText, listExtraSelectionWithTimeStamp);
            }

        } else if (!textCursor().hasSelection() || selectionText.length() == (document()->characterCount() - 1)) {
            qDebug() << "Is mark all line, selectionText is empty or equal to document length";
            TextEdit::MarkOperation markOperation;
            markOperation.type = MarkAll;
            markOperation.color = strColor;
            m_markOperations.append(QPair<TextEdit::MarkOperation, qint64>(markOperation, timeStamp));
            markAllInView(strColor, timeStamp);
        }
    } else {
        qDebug() << "Is mark all line, isMark is false";
        m_markOperations.clear();
        m_wordMarkSelections.clear();
        m_mapKeywordMarkSelections.clear();

        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(QColor(strColor));
        selection.cursor = textCursor();
        selection.cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        selection.cursor.clearSelection();
        m_markAllSelection = selection;
    }

    renderAllSelections();
    qDebug() << "Cancel last mark completed";
}

void TextEdit::cancelLastMark()
{
    qDebug() << "Cancel last mark";
    if (m_markOperations.isEmpty()) {
        qDebug() << "Cancel last mark, m_markOperations is empty";
        return;
    }

    switch (m_markOperations.last().first.type) {
    case MarkOnce:
    case MarkLine: {
        qDebug() << "Cancel last mark, type is MarkOnce or MarkLine";
        if (!m_wordMarkSelections.isEmpty()) {
            qDebug() << "Cancel last mark, m_wordMarkSelections is not empty";
            // m_wordMarkSelections.removeLast();
            // 考虑到可能的插入操作，需要同步清理时间戳一样的selection
            const qint64 operationTimeStamp = m_markOperations.last().second;
            for (int i = 0; i < m_wordMarkSelections.size(); i++) {
                if (operationTimeStamp == m_wordMarkSelections.at(i).second) {
                    qDebug() << "Cancel last mark, find same time stamp selection";
                    m_wordMarkSelections.removeAt(i);
                    i--;
                }
            }
        }
        break;
    }

    case MarkAllMatch: {
        qDebug() << "Cancel last mark, type is MarkAllMatch";
        // QString keyword = m_markOperations.last().first.cursor.selectedText();
        QString keyword;
        qint64 timeStamp = m_markOperations.last().second;
        // 使用时间戳查找 keyword
        QMap<QString, QList<QPair<QTextEdit::ExtraSelection, qint64>>>::Iterator it;
        for (it = m_mapKeywordMarkSelections.begin(); it != m_mapKeywordMarkSelections.end(); ++it) {
            if (it.value().size() > 0) {
                qDebug() << "Cancel last mark, find keyword in map";
                qint64 itsTimeStamp = it.value().first().second;
                if (itsTimeStamp == timeStamp) {
                    qDebug() << "Cancel last mark, find keyword in map, time stamp is same";
                    keyword = it.key();
                    break;
                }
            }
        }

        if (m_mapKeywordMarkSelections.contains(keyword)) {
            qDebug() << "Cancel last mark, find keyword in map, remove";
            m_mapKeywordMarkSelections.remove(keyword);
        }
        break;
    }

    case MarkAll: {
        qDebug() << "Cancel last mark, type is MarkAll";
        if (m_mapKeywordMarkSelections.contains(TEXT_EIDT_MARK_ALL)) {
            m_mapKeywordMarkSelections.remove(TEXT_EIDT_MARK_ALL);
        }
        break;
    }

    }

    m_markOperations.removeLast();

    // 如果在标记颜色操作后，更改文本内容，如果存在残留，补充一个清除处理
    if (m_markOperations.isEmpty() &&
            (m_wordMarkSelections.size() > 0 || m_mapKeywordMarkSelections.size() > 0)) {
        qWarning() << __FUNCTION__ << __LINE__ << " cancle mark color operation,"
                   << "find exist remain selections, will clear!";
        m_wordMarkSelections.clear();
        m_mapKeywordMarkSelections.clear();
    }

    renderAllSelections();
    qDebug() << "Cancel last mark completed";
}

bool TextEdit::clearMarkOperationForCursor(QTextCursor cursor)
{
    qDebug() << "Cancel last mark, find cursor in m_markOperations";
    bool bRet = false;
    for (int i = m_markOperations.size() - 1; i >= 0; --i) {
        if (m_markOperations.at(i).first.cursor == cursor) {
            m_markOperations.removeAt(i);
            bRet = true;
            qDebug() << "Cancel last mark, find cursor in m_markOperations, break!";
            break;
        }
    }

    qDebug() << "Cancel last mark, find cursor in m_markOperations, return: " << bRet;
    return bRet;
}

bool TextEdit::clearMarksForTextCursor()
{
    qDebug() << "Cancel last mark, find cursor in m_wordMarkSelections";
    bool bFind = false;
    QTextCursor cursor;
    QTextCursor textcursor = textCursor();

    for (int i = m_wordMarkSelections.size() - 1; i >= 0; --i) {
        cursor = m_wordMarkSelections.at(i).first.cursor;
        if (textcursor.hasSelection()) {
            qDebug() << "Cancel last mark, find cursor in m_wordMarkSelections, has selection";
            if (textcursor == cursor) {
                bFind = true;
                clearMarkOperationForCursor(cursor);
                m_wordMarkSelections.removeAt(i);
                qDebug() << "Cancel last mark, find cursor in m_wordMarkSelections, has selection, equal, break";
                break;
            }

        } else {
            qDebug() << "Cancel last mark, find cursor in m_wordMarkSelections, has no selection";
            if (textcursor.position() >= cursor.selectionStart() && textcursor.position() <= cursor.selectionEnd()) {
                bFind = true;
                clearMarkOperationForCursor(cursor);
                m_wordMarkSelections.removeAt(i);
            }
        }
    }

    qDebug() << "Cancel last mark, find cursor in m_wordMarkSelections, return: " << bFind;
    return bFind;
}

void TextEdit::toggleMarkSelections()
{
    qDebug() << "Cancel last mark, find cursor in m_wordMarkSelections, toggle";
    if (!clearMarksForTextCursor()) {
        qDebug() << "Cancel last mark, find cursor in m_wordMarkSelections, toggle, no find";
        ColorSelectWdg *pColorSelectWdg = static_cast<ColorSelectWdg *>(m_actionColorStyles->defaultWidget());
        isMarkCurrentLine(true, pColorSelectWdg->getDefaultColor().name());
    }

    renderAllSelections();
    qDebug() << "Cancel last mark, find cursor in m_wordMarkSelections, toggle end";
}

/**
 * @brief 转换标记项替换信息 \a replaceInfo 为标记项信息，标记项替换信息包含了光标的绝对位置，
 *      在转换过程中，光标会更新当前选中区域为绝对位置
 * @param replaceInfo 标记替换信息
 * @return 转换后的标记操作项列表
 */
QList<QPair<TextEdit::MarkOperation, qint64> > TextEdit::convertReplaceToMark(const QList<TextEdit::MarkReplaceInfo> &replaceInfo)
{
    qDebug() << "Cancel last mark, find cursor in m_wordMarkSelections, toggle, convert";
    QList<QPair<TextEdit::MarkOperation, qint64> > markList;
    for (auto info : replaceInfo) {
        MarkOperation markOpt = info.opt;

        // 更新当前光标选中信息
        markOpt.cursor.setPosition(info.start);
        markOpt.cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, info.end - info.start);

        markList.append(qMakePair(markOpt, info.time));
    }

    qDebug() << "Cancel last mark, find cursor in m_wordMarkSelections, toggle, convert end";
    return markList;
}

/**
 * @brief 转换标记操作项 \a markInfo 为标记项替换信息，标记项替换信息包含了光标的绝对位置，
 *      在转换过程中，会记录当前选中区域为绝对位置
 * @param markInfo 标记信息
 * @return 转换后的标记替换信息列表
 */
QList<TextEdit::MarkReplaceInfo> TextEdit::convertMarkToReplace(const QList<QPair<TextEdit::MarkOperation, qint64> > &markInfo)
{
    qDebug() << "Cancel last mark, find cursor in m_wordMarkSelections, toggle, convert";
    QList<TextEdit::MarkReplaceInfo> replaceMarkList;
    for (auto info : markInfo) {
        MarkReplaceInfo replace;
        replace.opt = info.first;
        replace.time = info.second;

        // 记录当前光标位置，QTextDocument后续操作文本到QTextCursor光标范围时，会导致QTextCursor记录的光标位置失效
        replace.start = info.first.cursor.selectionStart();
        replace.end = info.first.cursor.selectionEnd();

        replaceMarkList.append(replace);
    }

    qDebug() << "Cancel last mark, find cursor in m_wordMarkSelections, toggle, convert end";
    return replaceMarkList;
}

/**
 * @brief 更新所有的标记信息为 \a markInfo , 用于撤销项处理颜色标记变更
 * @param markInfo 颜色标记信息列表
 */
void TextEdit::manualUpdateAllMark(const QList<QPair<MarkOperation, qint64> > &markInfo)
{
    qDebug() << "Cancel last mark, find cursor in m_wordMarkSelections, manualUpdateAllMark";
    m_markOperations = markInfo;

    // 全部标记更新后，修改手动标记文本部分
    m_wordMarkSelections.clear();
    // 用于将跨行的单行颜色标记拓展为多行处理
    QList<QPair<TextEdit::MarkOperation, qint64>> multiLineSelections;

    for (auto itr = m_markOperations.begin(); itr != m_markOperations.end();) {
        if (MarkAll == (*itr).first.type
                || MarkAllMatch == (*itr).first.type) {
            ++itr;
            qDebug() << "Cancel last mark, find cursor in m_wordMarkSelections, manualUpdateAllMark, all mark, continue";
            continue;
        }

        // 若无选中项，则过滤此颜色标记
        if (!(*itr).first.cursor.hasSelection()) {
            qDebug() << "Cancel last mark, find cursor in m_wordMarkSelections, manualUpdateAllMark, no selection, erase";
            itr = m_markOperations.erase(itr);
            continue;
        }

        auto &info = *itr;
        if (MarkOnce == info.first.type) {
            qDebug() << "Cancel last mark, find cursor in m_wordMarkSelections, manualUpdateAllMark, once mark";
            QTextEdit::ExtraSelection selection;
            selection.format.setBackground(QColor(info.first.color));
            selection.cursor = info.first.cursor;

            // 更新单独文本标记
            m_wordMarkSelections.append(qMakePair(selection, info.second));

        } else if (MarkLine == info.first.type) {
            qDebug() << "Cancel last mark, find cursor in m_wordMarkSelections, manualUpdateAllMark, line mark";
            int selectStart = info.first.cursor.selectionStart();
            int selectEnd = info.first.cursor.selectionEnd();
            QTextBlock startBlock = document()->findBlock(selectStart);
            QTextBlock endBlock = document()->findBlock(selectEnd);

            // 判断是否为多行颜色标记
            bool isMultiLine = startBlock < endBlock;

            while (startBlock.isValid()
                    && endBlock.isValid()
                    && !(endBlock < startBlock)) {
                QTextEdit::ExtraSelection selection;
                selection.format.setBackground(QColor(info.first.color));
                selection.cursor = info.first.cursor;
                // 更新单行标记的索引区间, 注意标记单行颜色不包括换行符
                selection.cursor.setPosition(startBlock.position());
                selection.cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, startBlock.length() - 1);

                // 更新单行文本标记，允许使用一样的时间戳
                m_wordMarkSelections.append(qMakePair(selection, info.second));

                // 追加拆解的多行颜色标记信息，允许使用一样的时间戳
                if (isMultiLine) {
                    auto copyInfo = info;
                    copyInfo.first.cursor = selection.cursor;
                    multiLineSelections.append(copyInfo);
                }

                // 继续下一文本块处理
                startBlock = startBlock.next();
            }

            // 若为多行颜色标记，会将当前文本颜色标记移除，后续添加为多行颜色标记
            if (isMultiLine) {
                itr = m_markOperations.erase(itr);
                qDebug() << "Cancel last mark, find cursor in m_wordMarkSelections, manualUpdateAllMark, multi line, erase";
                continue;
            }
        }

        // 继续下一遍历
        ++itr;
    }

    // 追加多行颜色标记处理记录
    m_markOperations.append(multiLineSelections);

    // 对修改后的颜色标记进行排序
    std::sort(m_markOperations.begin(), m_markOperations.end(),
    [](const QPair<TextEdit::MarkOperation, qint64> &a, const QPair<TextEdit::MarkOperation, qint64> &b) {
        return a.second < b.second;
    });
    std::sort(m_wordMarkSelections.begin(), m_wordMarkSelections.end(),
    [](const QPair<QTextEdit::ExtraSelection, qint64> &a, const QPair<QTextEdit::ExtraSelection, qint64> &b) {
        return a.second < b.second;
    });

    // 计算全文标记部分并刷新界面颜色标记
    markAllKeywordInView();
    qDebug() << "Cancel last mark, find cursor in m_wordMarkSelections, manualUpdateAllMark, end";
}

/**
 * @brief 查询颜色标记范围内存在多少组替换文本的函数，对 \a markStart ~ \a markEnd 区间进行搜索，
 *      判断 \a posList 中是否存在已查询到的替换文本位置索引，并将这些位置索引转换为索引区间列表返回。
 * @param posList       已查询的替换文本位置索引
 * @param markStart     颜色标记开始位置索引
 * @param markEnd       颜色标记结束位置索引
 * @param replaceText   替换的文本
 * @return 查找到和颜色标记位置有交叉的文本位置索引区间
 */
static QPair<int, int> findMatchRange(const QList<int> &posList, int markStart, int markEnd, const QString &replaceText)
{
    qDebug() << "findMatchRange, markStart" << markStart << "markEnd" << markEnd << "replaceText" << replaceText;
    // 判断颜色标记 info 范围内是否包含替换文本索引信息
    QPair<int, int> foundPosRange {-1, -1};
    if (posList.isEmpty()) {
        qDebug() << "findMatchRange, posList is empty, return";
        return foundPosRange;
    }

    // 将颜色标记搜索范围向左延伸 replaceText.size() - 1 位置()，若此区间出现replaceText, 那么必定和 markStart ~ markEnd 相交
    int adjustMarkStart = markStart - replaceText.size();

    // 获取最近的左侧查找文本位置索引，使用 qUpperBound, 索引必须大于 adjustMarkStart
    auto leftfindItr = std::upper_bound(posList.begin(), posList.end(), adjustMarkStart);
    if (leftfindItr != posList.end()
            && (*leftfindItr) < markEnd) {
        qDebug() << "findMatchRange, leftfindItr" << (*leftfindItr) << "markEnd" << markEnd;
        // 设置查询的左边界
        foundPosRange.first = static_cast<int>(std::distance(posList.begin(), leftfindItr));
    }

    // 获取最近的右侧查找文本位置索引（小于右边界 markEnd）
    auto rightFindItr = std::lower_bound(posList.rbegin(), posList.rend(), markEnd - 1, std::greater<int>());
    if (rightFindItr != posList.rend()
            && markStart < (*rightFindItr)) {
        qDebug() << "findMatchRange, rightFindItr" << (*rightFindItr) << "markStart" << markStart;
        // 设置右边界
        foundPosRange.second = static_cast<int>(std::distance(rightFindItr, posList.rend() - 1));
    } else if (-1 != foundPosRange.first) {
        qDebug() << "findMatchRange, foundPosRange.first" << foundPosRange.first;
        foundPosRange.second = foundPosRange.first;
    }

    qDebug() << "findMatchRange, foundPosRange" << foundPosRange;
    return foundPosRange;
}

/**
 * @brief 根据当前已查找到的替换文本偏移位置和颜色标记的交叉范围，调整颜色标记的位置
 * @param foundPosList  已查询的替换文本位置索引
 * @param info          需要计算的颜色标记
 * @param replaceText   被替换的文本
 * @param withText      替换后的文本
 */
static void updateMarkReplaceRange(const QList<int> &foundPosList, TextEdit::MarkReplaceInfo &info, const QString &replaceText, const QString &withText)
{
    qDebug() << "updateMarkReplaceRange, info";
    // 文本替换长度变更调整量
    int adjustlen = withText.size() - replaceText.size();
    // 获取替换文本位置索引列表
    QPair<int, int> posIndexRange = findMatchRange(foundPosList, info.start, info.end, replaceText);

    if (-1 == posIndexRange.first
            && -1 == posIndexRange.second) {
        qDebug() << "updateMarkReplaceRange, posIndexRange, first" << posIndexRange.first << "second" << posIndexRange.second;
        // 颜色标记内不包含替换文本，和替换文本无交集
        // 根据当前查询位置判断 foundPosList[findCount - 1] 方向
        if (!foundPosList.isEmpty()
                && info.end <= foundPosList.last()) {
            // 处于颜色标记右侧，减去右侧替换文本的偏移量
            info.start += (foundPosList.size() - 1) * adjustlen;
            info.end += (foundPosList.size() - 1) * adjustlen;
        } else {
            // 处于颜色标记左侧
            info.start += foundPosList.size() * adjustlen;
            info.end += foundPosList.size() * adjustlen;
        }
    } else {
        qDebug() << "updateMarkReplaceRange, posIndexRange, first" << posIndexRange.first << "second" << posIndexRange.second;
        // 判断左侧是否有交集，拓展颜色标记到替换文本
        if (-1 != posIndexRange.first
                && foundPosList[posIndexRange.first] <= info.start) {
            // 存在交叉，调整位置到左侧替换位置起始边界
            info.start = foundPosList[posIndexRange.first] + (posIndexRange.first * adjustlen);
        } else {
            info.start += posIndexRange.first * adjustlen;
        }

        // 判断右侧是否有交集，拓展颜色标记到替换文本
        if (-1 != posIndexRange.second
                && info.end < (foundPosList[posIndexRange.second] + replaceText.size())) {
            // 存在交叉，调整位置到右侧替换后文本的边界
            info.end = foundPosList[posIndexRange.second] + (posIndexRange.second * adjustlen) + withText.size();
        } else {
            info.end += (posIndexRange.second + 1) * adjustlen;
        }
    }
    qDebug() << "updateMarkReplaceRange exit";
}

/**
 * @brief 根据传入的颜色标记信息 \a replaceList 计算文本 \a oldText 替换后颜色标记的位置变化
 *      在文本替换过程前，记录当前标记光标位置，遍历需要替换的文本位置，记录需要修改的颜色标记位置变更。
 * @param replaceList   颜色标记替换信息
 * @param oldText       需要替换的文本内容
 * @param replaceText   被替换的文本
 * @param withText      替换后的文本
 * @param offset        非全文替换时使用，用于补充文本缺失部分偏移量
 *
 * @note 文本替换颜色标记处理
 * @li MarkOnce 单个文本颜色标记
 *      1. 替换文本覆盖了颜色标记内容，移除颜色标记
 *      2. 替换文本和单个颜色标记内容存在交叉，包括替换文本处于颜色标记内，将颜色标记拓展到替换文本，替换文本的背景色显示为颜色标记背景色
 *      3. 若替换文本内包含多个颜色标记，每个颜色标记均拓展到替换文本，颜色标记显示层级按设置时间排列
 * @li MarkLine 单行文本颜色标记
 *      1. 同单个颜色标记处理
 * @li MarkAllMatch 单个文本全文查找颜色标记
 *      1. 替换前后不进行特殊处理
 * @li MarkAll 全文颜色标记
 *      1. 替换前后不进行特殊处理
 */
void TextEdit::calcMarkReplaceList(QList<TextEdit::MarkReplaceInfo> &replaceList, const QString &oldText,
                                   const QString &replaceText, const QString &withText, int offset, Qt::CaseSensitivity caseFlag) const
{
    qDebug() << "calcMarkReplaceList, replaceList, size=" << replaceList.size();
    // 当前替换项为空或相同，退出
    if (replaceList.isEmpty()
            || replaceText == withText) {
        qDebug() << "calcMarkReplaceList, replaceList is empty or replaceText == withText, return";
        return;
    }

    // 将传入的替换列表排序，按光标位置先后顺序进行排列
    std::sort(replaceList.begin(), replaceList.end(), [](const MarkReplaceInfo & a, const MarkReplaceInfo & b) {
        return a.start < b.start;
    });

    // 当前标记的索引，后续使用 updateMarkRange() 初始化，设置为-1
    int currentMarkIndex = -1;
    // 临时缓存的当前标记的索引范围
    QPair<int, int> curMarkRange {0, 0};
    // 更新当前标记替换项索引和其对应的标记范围
    auto updateMarkIndexAndRange = [&]() {
        qDebug() << "updateMarkIndexAndRange, currentMarkIndex" << currentMarkIndex;
        currentMarkIndex++;
        while (currentMarkIndex >= 0 && currentMarkIndex < replaceList.size()) {
            auto &markInfo = replaceList.at(currentMarkIndex);
            qDebug() << "updateMarkIndexAndRange, markInfo, start" << markInfo.start << "end" << markInfo.end;
            // 标记类型为标记全文或文本全文标记(使用文本查找而非光标位置)，不进行替换处理
            if (MarkAllMatch == markInfo.opt.type
                    || MarkAll == markInfo.opt.type) {
                qDebug() << "updateMarkIndexAndRange, markInfo, type is MarkAllMatch or MarkAll, continue";
                currentMarkIndex++;
                continue;
            }

            curMarkRange = qMakePair(markInfo.start, markInfo.end);
            qDebug() << "updateMarkIndexAndRange, curMarkRange" << curMarkRange;
            break;
        }
    };
    qDebug() << "updateMarkIndexAndRange, before updateMarkIndexAndRange";
    updateMarkIndexAndRange();
    qDebug() << "updateMarkIndexAndRange, after updateMarkIndexAndRange";

    // 查找统计及已查找偏移量
    int findOffset = 0;
    QList<int> foundPosList;

    // 查找替换位置，遍历查找替换文本出现位置
    int findPos = oldText.indexOf(replaceText, findOffset, caseFlag);
    // 需要取得左侧所有的变更相对偏移，从文本左侧开始循环遍历
    while (-1 != findPos
            && currentMarkIndex < replaceList.size()) {
        qDebug() << "calcMarkReplaceList, findPos" << findPos << "offset" << offset;
        // 转换为相对全文的偏移
        int realPos = findPos + offset;
        // 记录已查询的索引位置
        foundPosList.append(realPos);

        // 获取替换文本区域和颜色标记区域的交叉关系
        Utils::RegionIntersectType type = Utils::checkRegionIntersect(realPos, realPos + replaceText.size(),
                                                                      curMarkRange.first, curMarkRange.second);

        // 判断是否处于查询范围内，处于右侧区间则后续处理
        while (Utils::ERight != type) {
            auto &info = replaceList[currentMarkIndex];
            qDebug() << "calcMarkReplaceList, info, start" << info.start << "end" << info.end;
            bool checkNext = false;
            switch (type) {
            case Utils::EIntersectInner: {
                qDebug() << "calcMarkReplaceList, type is EIntersectInner";
                // 替换文本内容包含标记信息, 取消当前文本标记（无论单个文本还是单行文本，均移除）
                // 在 manualUpdateAllMark() 函数处理会移除此标记
                info.start = 0;
                info.end = 0;
                checkNext = true;
                break;
            }
            case Utils::ELeft: {
                qDebug() << "calcMarkReplaceList, type is ELeft";
                // 当前无交集，颜色标记在替换文本左侧，表示当前颜色标记已经经过
                updateMarkReplaceRange(foundPosList, info, replaceText, withText);

                checkNext = true;
                break;
            }
            // 其它交叉类型需要等待颜色标记区间内所有替换文本均查找后处理，单个颜色标记中可能含多个替换文本
            default:
                break;
            }

            if (checkNext) {
                qDebug() << "calcMarkReplaceList, checkNext is true";
                // 更新当前计算的颜色标记和范围
                updateMarkIndexAndRange();
                // 判断颜色标记是否计算完成
                if (currentMarkIndex == replaceList.size()) {
                    qDebug() << "calcMarkReplaceList, currentMarkIndex == replaceList.size(), break";
                    break;
                }

                // 继续查找下一颜色标记和当前查询位置的交叉范围
                type = Utils::checkRegionIntersect(realPos, realPos + replaceText.size(), curMarkRange.first, curMarkRange.second);
            } else {
                qDebug() << "calcMarkReplaceList, checkNext is false, break";
                break;
            }
        }

        if (currentMarkIndex == replaceList.size()) {
            qDebug() << "calcMarkReplaceList, currentMarkIndex == replaceList.size(), break";
            break;
        }

        // 继续查找替换文本位置
        findOffset = findPos + replaceText.size();
        findPos = oldText.indexOf(replaceText, findOffset, caseFlag);
    }

    // 继续处理剩余颜色标记偏移
    while (currentMarkIndex != replaceList.size()) {
        qDebug() << "calcMarkReplaceList, currentMarkIndex != replaceList.size(), while";
        // 将后续未处理到的颜色标记调整偏移量
        auto &info = replaceList[currentMarkIndex];
        updateMarkReplaceRange(foundPosList, info, replaceText, withText);

        updateMarkIndexAndRange();
    }
    qDebug() << "calcMarkReplaceList, completed";
}

void TextEdit::markSelectWord()
{
    qDebug() << "markSelectWord";
    bool isFind  = false;
    for (int i = 0 ; i < m_wordMarkSelections.size(); ++i) {
        QTextCursor curson = m_wordMarkSelections.at(i).first.cursor;
        curson.movePosition(QTextCursor::EndOfLine);
        QTextCursor currentCurson = textCursor();
        currentCurson.movePosition(QTextCursor::EndOfLine);
        //if (m_wordMarkSelections.at(i).cursor == textCursor()) {
        if (curson == currentCurson) {
            qDebug() << "markSelectWord, curson == currentCurson";
            isFind = true;
            m_wordMarkSelections.removeAt(i);
            renderAllSelections();
            break;
        }
    }
    if (!isFind) {
        qDebug() << "markSelectWord, isFind is false";
        //添加快捷键标记颜色
        ColorSelectWdg *pColorSelectWdg = static_cast<ColorSelectWdg *>(m_actionColorStyles->defaultWidget());
        isMarkCurrentLine(true, pColorSelectWdg->getDefaultColor().name());
        renderAllSelections();
    }
    qDebug() << "markSelectWord, completed";
}

void TextEdit::updateMark(int from, int charsRemoved, int charsAdded)
{
    qDebug() << "updateMark, from" << from << "charsRemoved" << charsRemoved << "charsAdded" << charsAdded;
    //只读模式下实现禁止语音输入的效果
    if (m_readOnlyMode) {
        //undo();
        qDebug() << "updateMark, readOnlyMode, return";
        return;
    }

    //如果是读取文件导致的文本改变
    if (m_bIsFileOpen) {
        return;
    }

    qDebug() << "updateMark, charsRemoved" << charsRemoved << "charsAdded" << charsAdded;
    //更新标记
    int nStartPos = 0,///< 标记的起始位置
        nEndPos = 0,///< 标记的结束位置
        nCurrentPos = 0;///< 当前光标位置
    QTextEdit::ExtraSelection selection;///< 指定文本格式
    QList<QTextEdit::ExtraSelection> listSelections;///< 指定文本格式列表
    QList<QPair<QTextEdit::ExtraSelection, qint64>> wordMarkSelections = m_wordMarkSelections;///< 标记列表
    QColor strColor;///< 指定文本颜色格式
    nCurrentPos = textCursor().position();

    //如果是删除字符
    if (charsRemoved > 0) {
        qDebug() << "updateMark, charsRemoved > 0";
        QList<int> listRemoveItem;///< 要移除标记的indexs

        //寻找要移除标记的index
        for (int i = 0; i < wordMarkSelections.count(); i++) {

            nEndPos = wordMarkSelections.value(i).first.cursor.selectionEnd();
            nStartPos = wordMarkSelections.value(i).first.cursor.selectionStart();
            strColor = wordMarkSelections.value(i).first.format.background().color();

            //如果有文字被选择
            if (m_nSelectEndLine != -1) {
                qDebug() << "updateMark, charsRemoved > 0, m_nSelectEndLine != -1";
                //如果删除的内容，完全包含标记内容
                if (m_nSelectStart <= nStartPos && m_nSelectEnd >= nEndPos) {
                    listRemoveItem.append(i);
                }
            } else {
                qDebug() << "updateMark, charsRemoved > 0, m_nSelectEndLine == -1";
                //如果标记内容全部被删除
                if (wordMarkSelections.value(i).first.cursor.selection().toPlainText().isEmpty()) {
                    // 此时 remove m_wordMarkSelections中的元素
                    // 会造成 m_wordMarkSelections 和 wordMarkSelections的长度不一致
                    // m_wordMarkSelections.removeAt(i);
                    listRemoveItem.append(i);
                }
            }
        }

        //从标记列表中移除标记
        // 修复：使用removeIf避免索引管理问题
        if (!listRemoveItem.isEmpty()) {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
            QSet<int> removeSet = listRemoveItem.toSet();
            for (int i = m_wordMarkSelections.size() - 1; i >= 0; --i) {
                if (removeSet.contains(i)) {
                    m_wordMarkSelections.removeAt(i);
                }
            }
            qDebug() << "updateMark: Removed marks, remaining:" << m_wordMarkSelections.size();
#else
            QSet<int> removeSet(listRemoveItem.begin(), listRemoveItem.end());
            int index = 0;
            int removedCount = m_wordMarkSelections.removeIf([&removeSet, &index](const auto&) {
                return removeSet.contains(index++);
            });
            qDebug() << "updateMark: Removed" << removedCount << "marks, remaining:" << m_wordMarkSelections.size();
#endif
        }
    }

    //如果是添加字符
    if (charsAdded > 0) {
        qDebug() << "updateMark, charsAdded > 0";
        for (int i = 0; i < wordMarkSelections.count(); i++) {
            nEndPos = wordMarkSelections.value(i).first.cursor.selectionEnd();
            nStartPos = wordMarkSelections.value(i).first.cursor.selectionStart();
            strColor = wordMarkSelections.value(i).first.format.background().color();
            qint64 timeStamp = wordMarkSelections.value(i).second;

            //如果字符添加在标记中
            if (nCurrentPos > nStartPos && nCurrentPos < nEndPos) {
                qDebug() << "updateMark, charsAdded > 0, nCurrentPos > nStartPos && nCurrentPos < nEndPos";
                m_wordMarkSelections.removeAt(i);
                selection.format.setBackground(strColor);
                selection.cursor = textCursor();

                QTextEdit::ExtraSelection preSelection;

                //如果是输入法输入
                if (m_bIsInputMethod) {
                    qDebug() << "updateMark, charsAdded > 0, m_bIsInputMethod";
                    //添加第一段标记
                    selection.cursor.setPosition(nStartPos, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(nCurrentPos - m_qstrCommitString.size(), QTextCursor::KeepAnchor);
                    m_wordMarkSelections.insert(i, QPair<QTextEdit::ExtraSelection, qint64>
                                                (selection, timeStamp));

                    preSelection.cursor = selection.cursor;
                    preSelection.format = selection.format;

                    //添加第二段标记
                    selection.cursor.setPosition(nCurrentPos, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(nEndPos, QTextCursor::KeepAnchor);
                    m_wordMarkSelections.insert(i + 1,  QPair<QTextEdit::ExtraSelection, qint64>
                                                (selection, timeStamp));

                    m_bIsInputMethod = false;
                } else {
                    qDebug() << "updateMark, charsAdded > 0, !m_bIsInputMethod";
                    //添加第一段标记
                    selection.cursor.setPosition(nStartPos, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(from, QTextCursor::KeepAnchor);
                    m_wordMarkSelections.insert(i, QPair<QTextEdit::ExtraSelection, qint64>
                                                (selection, timeStamp));

                    preSelection.cursor = selection.cursor;
                    preSelection.format = selection.format;

                    //添加第二段标记
                    selection.cursor.setPosition(nCurrentPos, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(nEndPos, QTextCursor::KeepAnchor);
                    m_wordMarkSelections.insert(i + 1, QPair<QTextEdit::ExtraSelection, qint64>
                                                (selection, timeStamp));
                }

                bool bIsFind = false;
                qDebug() << "updateMark, charsAdded > 0, bIsFind";
                //在记录标记的表中替换（按标记动作记录）
                for (int j = 0; j < m_mapWordMarkSelections.count(); j++) {
                    auto list = m_mapWordMarkSelections.value(j);
                    for (int k = 0; k < list.count(); k++) {
                        if (list.value(k).cursor == wordMarkSelections.value(i).first.cursor
                                && list.value(k).format == wordMarkSelections.value(i).first.format) {
                            list.removeAt(k);
                            listSelections = list;
                            listSelections.insert(k, preSelection);
                            listSelections.insert(k + 1, selection);
                            bIsFind = true;
                            break;
                        }
                    }

                    if (bIsFind) {
                        m_mapWordMarkSelections.remove(j);
                        m_mapWordMarkSelections.insert(j, listSelections);
                        break;
                    }
                }
                break;

            } else if (nCurrentPos == nEndPos) { //如果字符添加在标记后
                qDebug() << "updateMark, charsAdded > 0, nCurrentPos == nEndPos";
                m_wordMarkSelections.removeAt(i);
                selection.format.setBackground(strColor);
                selection.cursor = textCursor();

                if (m_bIsInputMethod) {
                    qDebug() << "updateMark, charsAdded > 0, m_bIsInputMethod";
                    selection.cursor.setPosition(nStartPos, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(nEndPos - m_qstrCommitString.size(), QTextCursor::KeepAnchor);
                    m_bIsInputMethod = false;
                } else {
                    qDebug() << "updateMark, charsAdded > 0, !m_bIsInputMethod";
                    selection.cursor.setPosition(nStartPos, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(from + charsAdded, QTextCursor::KeepAnchor);
                }

                m_wordMarkSelections.insert(i, QPair<QTextEdit::ExtraSelection, qint64>
                                            (selection, timeStamp));

                bool bIsFind = false;
                for (int j = 0; j < m_mapWordMarkSelections.count(); j++) {
                    auto list = m_mapWordMarkSelections.value(j);
                    for (int k = 0; k < list.count(); k++) {
                        if (list.value(k).cursor == wordMarkSelections.value(i).first.cursor
                                && list.value(k).format == wordMarkSelections.value(i).first.format) {
                            list.removeAt(k);
                            listSelections = list;
                            listSelections.insert(k, selection);
                            bIsFind = true;
                            qDebug() << "updateMark, charsAdded > 0, bIsFind, break";
                            break;
                        }
                    }

                    if (bIsFind) {
                        m_mapWordMarkSelections.remove(j);
                        m_mapWordMarkSelections.insert(j, listSelections);
                        qDebug() << "updateMark, charsAdded > 0, bIsFind, break";
                        break;
                    }
                }
                qDebug() << "updateMark, charsAdded > 0, break";
                break;
            }
        }
    }

    //渲染所有的指定字符格式
    renderAllSelections();

    highlight();
    qDebug() << "updateMark, completed";
}

void TextEdit::setCursorStart(int pos)
{
    qDebug() << "setCursorStart, pos" << pos;
    m_cursorStart = pos;
}


void TextEdit::completionWord(QString word)
{
    qDebug() << "completionWord, word" << word;
    QString wordAtCursor = getWordAtCursor();
    QTextCursor cursor = textCursor();

    QString completionString = word.remove(0, wordAtCursor.size());
    if (completionString.size() > 0) {
        qDebug() << "completionWord, completionString" << completionString;
        cursor = textCursor();
        cursor.insertText(completionString);
        setTextCursor(cursor);
    }
    qDebug() << "completionWord, completed";
}

bool TextEdit::eventFilter(QObject *object, QEvent *event)
{
    // qDebug() << "eventFilter, event->type()" << event->type();
    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd: {
        qDebug() << "eventFilter, case QEvent::TouchBegin, QEvent::TouchUpdate, QEvent::TouchEnd";
        QTouchEvent *touchEvent = static_cast<QTouchEvent *>(event);
        QList<QTouchEvent::TouchPoint> touchPoints = touchEvent->touchPoints();
        QMouseEvent *event2 = static_cast<QMouseEvent *>(event);
        DPlainTextEdit::mousePressEvent(event2);
        if (touchPoints.count() == 3) {
            slot_translate();
        }
        break;
    }
    default: break;
    }
    if (event->type() == QEvent::MouseButtonPress) {
        qDebug() << "eventFilter, case QEvent::MouseButtonPress";
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        m_mouseClickPos = mouseEvent->pos();

        if (object == m_pLeftAreaWidget->m_pBookMarkArea) {
            qDebug() << "eventFilter, case QEvent::MouseButtonPress, object == m_pLeftAreaWidget->m_pBookMarkArea";
            m_mouseClickPos = mouseEvent->pos();
            if (mouseEvent->button() == Qt::RightButton) {
                m_rightMenu->clear();
                m_rightMenu->deleteLater();
                m_rightMenu = nullptr;
                m_rightMenu = new DMenu(this);
                connect(m_rightMenu, &DMenu::destroyed, this, &TextEdit::removeHighlightWordUnderCursor);
                int line = getLineFromPoint(mouseEvent->pos());
                if (m_listBookmark.contains(line)) {
                    m_rightMenu->addAction(m_cancelBookMarkAction);
                    if (m_listBookmark.count() > 1) {
                        qDebug() << "eventFilter, case QEvent::MouseButtonPress, object == m_pLeftAreaWidget->m_pBookMarkArea, m_listBookmark.count() > 1";
                        m_rightMenu->addAction(m_preBookMarkAction);
                        m_rightMenu->addAction(m_nextBookMarkAction);
                    }
                } else {
                    qDebug() << "eventFilter, case QEvent::MouseButtonPress, object == m_pLeftAreaWidget->m_pBookMarkArea, m_listBookmark.isEmpty()";
                    m_rightMenu->addAction(m_addBookMarkAction);
                }

                if (!m_listBookmark.isEmpty()) {
                    qDebug() << "eventFilter, case QEvent::MouseButtonPress, object == m_pLeftAreaWidget->m_pBookMarkArea, !m_listBookmark.isEmpty()";
                    m_rightMenu->addAction(m_clearBookMarkAction);
                }

                m_rightMenu->exec(mouseEvent->globalPos());
            } else {
                qDebug() << "eventFilter, case QEvent::MouseButtonPress, object == m_pLeftAreaWidget->m_pBookMarkArea, else";
                addOrDeleteBookMark();
            }
            return true;
        } else if (object == m_pLeftAreaWidget->m_pFlodArea) {
            qDebug() << "eventFilter, case QEvent::MouseButtonPress, object == m_pLeftAreaWidget->m_pFlodArea";
            m_foldCodeShow->hide();
            if (mouseEvent->button() == Qt::LeftButton) {
                qDebug() << "eventFilter, case QEvent::MouseButtonPress, object == m_pLeftAreaWidget->m_pFlodArea, mouseEvent->button() == Qt::LeftButton";
                int line = getLineFromPoint(mouseEvent->pos());
                m_markFoldHighLightSelections.clear();
                renderAllSelections();

                //不同类型文件注释符号不同 梁卫东　２０２０－０９－０３　１７：２８：４５
                bool bHasCommnent = false;
                QString multiLineCommentMark;
                QString singleLineCommentMark;

                if (m_commentDefinition.isValid()) {
                    multiLineCommentMark = m_commentDefinition.multiLineStart.trimmed();
                    singleLineCommentMark = m_commentDefinition.singleLine.trimmed();
                    //qDebug()<<"CommentMark:"<<multiLineCommentMark<<singleLineCommentMark;
                    //判断是否包含单行或多行注释
                    if (!multiLineCommentMark.isEmpty()) bHasCommnent = document()->findBlockByNumber(line - 1).text().trimmed().startsWith(multiLineCommentMark);
                    if (!singleLineCommentMark.isEmpty()) bHasCommnent = document()->findBlockByNumber(line - 1).text().trimmed().startsWith(singleLineCommentMark);
                } else {
                    bHasCommnent = false;
                }

                // 当前行line-1 判断下行line是否隐藏
                if (document()->findBlockByNumber(line).isVisible() && document()->findBlockByNumber(line - 1).text().contains("{") && !bHasCommnent) {
                    qDebug() << "Processing visible block at line" << line << "with opening brace, folding code";
                    getNeedControlLine(line - 1, false);
                    document()->adjustSize();

                    //折叠时出现点击光标选择行变短
                    QPlainTextEdit::LineWrapMode curMode = this->lineWrapMode();
                    QPlainTextEdit::LineWrapMode WrapMode = curMode ==  QPlainTextEdit::NoWrap ?  QPlainTextEdit::WidgetWidth :  QPlainTextEdit::NoWrap;          this->setWordWrapMode(QTextOption::WrapAnywhere);
                    this->setLineWrapMode(WrapMode);
                    viewport()->update();
                    m_pLeftAreaWidget->updateBookMark();
                    m_pLeftAreaWidget->updateCodeFlod();
                    m_pLeftAreaWidget->updateLineNumber();
                    this->setLineWrapMode(curMode);
                    qDebug() << "Code fold completed for visible block at line" << line;
                    viewport()->update();

                } else if (!document()->findBlockByNumber(line).isVisible() && document()->findBlockByNumber(line - 1).text().contains("{") && !bHasCommnent) {
                    qDebug() << "Processing hidden block at line" << line << "with opening brace, unfolding code";
                    getNeedControlLine(line - 1, true);
                    document()->adjustSize();

                    //折叠时出现点击光标选择行变短
                    QPlainTextEdit::LineWrapMode curMode = this->lineWrapMode();
                    QPlainTextEdit::LineWrapMode WrapMode = curMode ==  QPlainTextEdit::NoWrap ?  QPlainTextEdit::WidgetWidth :  QPlainTextEdit::NoWrap;          this->setWordWrapMode(QTextOption::WrapAnywhere);
                    this->setLineWrapMode(WrapMode);
                    viewport()->update();
                    m_pLeftAreaWidget->updateBookMark();
                    m_pLeftAreaWidget->updateCodeFlod();
                    m_pLeftAreaWidget->updateLineNumber();
                    this->setLineWrapMode(curMode);
                    qDebug() << "Code unfold completed for hidden block at line" << line;
                    viewport()->update();
                } else {
                    qDebug() << "No fold/unfold action needed for line" << line;
                    //其他不做处理
                }

            } else {
                m_mouseClickPos = mouseEvent->pos();
                m_rightMenu->clear();
                m_rightMenu->deleteLater();
                m_rightMenu = nullptr;
                m_rightMenu = new DMenu(this);
                connect(m_rightMenu, &DMenu::destroyed, this, &TextEdit::removeHighlightWordUnderCursor);
                int line = getLineFromPoint(mouseEvent->pos());

                if (m_listFlodIconPos.contains(line - 1)) {
                    m_rightMenu->addAction(m_flodAllLevel);
                    m_rightMenu->addAction(m_unflodAllLevel);
                    m_rightMenu->addAction(m_flodCurrentLevel);
                    m_rightMenu->addAction(m_unflodCurrentLevel);
                }
                if (document()->findBlockByNumber(line).isVisible()) {
                    m_unflodCurrentLevel->setEnabled(false);
                    m_flodCurrentLevel->setEnabled(true);
                } else {
                    m_unflodCurrentLevel->setEnabled(true);
                    m_flodCurrentLevel->setEnabled(false);
                }

                //展开/折叠所有层次后，对应的菜单项置灰
                if (m_listMainFlodAllPos.count() == 0) {
                    m_unflodAllLevel->setEnabled(false);
                } else {
                    bool bExistVisible = false;
                    bool bExistInVisble = false;
                    for (int iLine : m_listMainFlodAllPos) {
                        if (document()->findBlockByNumber(iLine + 1).isVisible()) {
                            m_flodAllLevel->setEnabled(true);
                            bExistVisible = true;
                        } else {
                            m_unflodAllLevel->setEnabled(true);
                            bExistInVisble = true;
                        }

                        if (bExistVisible && bExistInVisble) {
                            break;
                        }
                    }
                    if (!bExistVisible) {
                        m_flodAllLevel->setEnabled(false);
                        m_unflodAllLevel->setEnabled(true);
                    }
                    if (!bExistInVisble) {
                        m_unflodAllLevel->setEnabled(false);
                        m_flodAllLevel->setEnabled(true);
                    }
                }

                m_rightMenu->exec(mouseEvent->globalPos());
            }

            return true;
        }

    } else if (event->type() == QEvent::HoverMove) {
        QHoverEvent *hoverEvent = static_cast<QHoverEvent *>(event);
        int line = getLineFromPoint(hoverEvent->pos());

        if (object == m_pLeftAreaWidget->m_pBookMarkArea) {
            m_nBookMarkHoverLine = line;
            m_pLeftAreaWidget->m_pBookMarkArea->update();
        } else if (object == m_pLeftAreaWidget->m_pFlodArea) {
            m_markFoldHighLightSelections.clear();
            renderAllSelections();

            if (m_listFlodIconPos.contains(line - 1)) {
                if (!document()->findBlockByNumber(line).isVisible()) {
                    m_foldCodeShow->clear();
                    m_foldCodeShow->setStyle(lineWrapMode());//enum LineWrapMode {NoWrap,WidgetWidth};
                    getHideRowContent(line - 1);
                    m_foldCodeShow->show();
                    m_foldCodeShow->hideFirstBlock();
                    m_foldCodeShow->move(5, getLinePosYByLineNum(line));
                } else {
                    QTextCursor previousCursor = textCursor();
                    int ivalue = this->verticalScrollBar()->value();
                    int iLine = getHighLightRowContentLineNum(line - 1);

                    if (line - 1 == iLine)   return false;

                    QTextEdit::ExtraSelection selection;
                    selection.format.setProperty(QTextFormat::FullWidthSelection, true);

                    QTextBlock startblock;
                    QTextBlock endblock = document()->findBlockByNumber(iLine);
                    startblock = document()->findBlockByNumber(line - 1);

                    QTextCursor beginCursor(startblock);
                    beginCursor.setPosition(startblock.position() + startblock.text().indexOf("{"), QTextCursor::MoveAnchor);


                    beginCursor.setPosition(endblock.position() + endblock.text().indexOf("}") + 1, QTextCursor::KeepAnchor);

                    if (iLine == document()->blockCount() - 1)
                        beginCursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);

                    setTextCursor(beginCursor);
                    selection.cursor = textCursor();

                    QColor highlightBackground = DGuiApplicationHelper::instance()->applicationPalette().color(QPalette::Highlight);
                    highlightBackground.setAlphaF(0.4);
                    selection.format.setBackground(highlightBackground);
                    m_markFoldHighLightSelections.push_back(selection);

                    renderAllSelections();
                    setTextCursor(QTextCursor(document()->findBlockByNumber(line - 1)));
                    this->verticalScrollBar()->setValue(ivalue);
                }
            } else {
                m_foldCodeShow->hide();
            }

            return true;
        }

    } else if (event->type() == QEvent::HoverLeave) {
        if (object == m_pLeftAreaWidget->m_pBookMarkArea) {
            m_nBookMarkHoverLine = -1;
            m_pLeftAreaWidget->m_pBookMarkArea->update();
            return true;
        } else if (object == m_pLeftAreaWidget->m_pFlodArea) {
            m_markFoldHighLightSelections.clear();
            m_foldCodeShow->hide();
            renderAllSelections();
            return true;
        }
    } else if (event->type() == QEvent::MouseButtonDblClick) {
        m_bIsDoubleClick = true;
        m_bBeforeIsDoubleClick = true;
    } else if (object == m_colorMarkMenu) {
        // 进行对于 color mark menu 的特殊按键处理
        if (event->type() == QEvent::KeyRelease) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

            // 当前仅处理 Tab 键 ， 保留后续其他按键的处理
            if (keyEvent->key() == Qt::Key_Tab) {
                // handleKey 作用是保留后续增加方向键处理
                int handleKey = keyEvent->key();

                // 使用Timer进行后续处理，避免在此处处理后，和基类的处理造成混淆
                QTimer::singleShot(0, this, [ = ]() {
                    // 仅当 menu 可见时进行处理，当前仅处理Tab键
                    if (m_colorMarkMenu->isVisible() && handleKey == Qt::Key_Tab) {
                        QAction *currentAction = m_colorMarkMenu->activeAction();
                        QAction *nextAction = nullptr;
                        int currentIndex = -1;

                        // 如果没有当前 focus item ， 不需要进行处理
                        if (currentAction == nullptr) {
                            return;
                        }

                        // 遍历 Order List， 如果不符合 order list 规则，变更focus item
                        for (int i = 0; i < m_MarkColorMenuTabOrder.size(); i++) {
                            QPair<QAction *, bool> orderItem = m_MarkColorMenuTabOrder.at(i);

                            if (orderItem.first == currentAction) {
                                // 设置当前 item 位置
                                currentIndex = i;

                                if (orderItem.second == true) {
                                    // 允许 focus， 不进行处理，退出
                                    return;
                                } else {
                                    // 退出当前查找循环，进行后续处理
                                    break;
                                }
                            }
                        }

                        if (currentIndex == -1) {
                            qWarning() << " can not find current item in tab order list , need check 'Mark Color Menu' tab order setting!";
                            return;
                        }

                        // 在 order list中，寻找下一个item
                        for (int j = currentIndex + 1; j < m_MarkColorMenuTabOrder.size(); j++) {
                            QPair<QAction *, bool> newOrderItem = m_MarkColorMenuTabOrder.at(j);

                            if (newOrderItem.second == true) {
                                nextAction = newOrderItem.first;
                                break;
                            }
                        }

                        // 原 order list 中不存在后一个可用item， 从列表头开始头查找
                        if (nextAction == nullptr) {
                            // 从头进行查找，因为i为当前位置，查找到i就足够
                            for (int j = 0; j < currentIndex; j++) {
                                QPair<QAction *, bool> newOrderItem = m_MarkColorMenuTabOrder.at(j);

                                if (newOrderItem.second == true) {
                                    nextAction = newOrderItem.first;
                                    break;
                                }
                            }
                        }

                        // 为找到的新item设置focus
                        if (nextAction != nullptr) {
                            m_colorMarkMenu->setActiveAction(nextAction);
                        } else {
                            qWarning() << " can not find valid item , need check 'Mark Color Menu' tab order setting!";
                        }
                    }
                });
            }
        }
    }

    return QPlainTextEdit::eventFilter(object, event);
}

void TextEdit::adjustScrollbarMargins()
{
    QEvent event(QEvent::LayoutRequest);
    QApplication::sendEvent(this, &event);

    QMargins margins = viewportMargins();
    setViewportMargins(0, 0, 5, 0);
    setViewportMargins(margins);
    if (!verticalScrollBar()->visibleRegion().isEmpty()) {
        setViewportMargins(0, 0, 5, 0);        //-verticalScrollBar()->sizeHint().width()  原本的第三个参数
        //setViewportMargins(0, 0, -verticalScrollBar()->sizeHint().width(), 0);
    } else {
        setViewportMargins(0, 0, 5, 0);
    }
}

void TextEdit::slotSelectionChanged()
{
    if (textCursor().hasSelection()) {
        hideCursorBlink();
    } else {
        showCursorBlink();
    }
}

void TextEdit::slotCanRedoChanged(bool bCanRedo)
{
    Q_UNUSED(bCanRedo)
    bool isModified = this->m_wrapper->isTemFile() | (m_pUndoStack->canUndo() || m_pUndoStack->index() != m_lastSaveIndex);
    this->m_wrapper->window()->updateModifyStatus(m_sFilePath, isModified);
    this->m_wrapper->OnUpdateHighlighter();
}

void TextEdit::slotCanUndoChanged(bool bCanUndo)
{
    bool isModified = this->m_wrapper->isTemFile() | (bCanUndo || m_pUndoStack->index() != m_lastSaveIndex);
    this->m_wrapper->window()->updateModifyStatus(m_sFilePath, isModified);
    this->m_wrapper->OnUpdateHighlighter();
}

bool TextEdit::containsExtraSelection(QList<QTextEdit::ExtraSelection> listSelections, QTextEdit::ExtraSelection selection)
{
    for (int i = 0; i < listSelections.count(); i++) {
        if (listSelections.value(i).cursor == selection.cursor
                && listSelections.value(i).format == selection.format) {
            return true;
        }
    }
    return false;
}

void TextEdit::appendExtraSelection(QList<QTextEdit::ExtraSelection> wordMarkSelections
                                    , QTextEdit::ExtraSelection selection, QString strColor
                                    , QList<QTextEdit::ExtraSelection> *listSelections)
{
// 没有使用的方法，应该去除，降低维护成本
// 由于需要处理对应的单元测试，暂时不完全移除此函数，后期将统一进行处理
#if 1
    // 去除参数未使用警告
    Q_UNUSED(wordMarkSelections)
    Q_UNUSED(selection)
    Q_UNUSED(strColor)
    Q_UNUSED(listSelections)
#else
    //如果文档中有标记
    if (wordMarkSelections.count() > 0) {
        bool bIsContains = false;///< 是否占用已有标记
        int nWordMarkSelectionStart = 0,///< 已有标记起始位置
            nSelectionStart = 0,///< 标记起始位置
            nWordMarkSelectionEnd = 0,///< 已有标记结束位置
            nSelectionEnd = 0;///< 标记结束位置

        //按大小确定标记的起始和结束位置
        if (selection.cursor.selectionStart() > selection.cursor.selectionEnd()) {
            nSelectionStart = selection.cursor.selectionEnd();
            nSelectionEnd = selection.cursor.selectionStart();
        } else {
            nSelectionStart = selection.cursor.selectionStart();
            nSelectionEnd = selection.cursor.selectionEnd();
        }

        for (int i = 0; i < wordMarkSelections.count(); i++) {

            //按大小确定已有标记的起始和结束位置
            if (wordMarkSelections.value(i).cursor.selectionStart() > wordMarkSelections.value(i).cursor.selectionEnd()) {
                nWordMarkSelectionStart = wordMarkSelections.value(i).cursor.selectionEnd();
                nWordMarkSelectionEnd = wordMarkSelections.value(i).cursor.selectionStart();
            } else {
                nWordMarkSelectionStart = wordMarkSelections.value(i).cursor.selectionStart();
                nWordMarkSelectionEnd = wordMarkSelections.value(i).cursor.selectionEnd();
            }

            //如果被已有标记包含
            if ((nWordMarkSelectionStart <= nSelectionStart && nWordMarkSelectionEnd > nSelectionEnd)
                    || (nWordMarkSelectionStart < nSelectionStart && nWordMarkSelectionEnd >= nSelectionEnd)) {

                bIsContains = true;
                selection.format.setBackground(QColor(strColor));

                //如果标记格式不相同
                if (wordMarkSelections.value(i).format != selection.format) {
                    int nRemPos = 0;///< 标记插入位置

                    //移除已有标记
                    for (int j = 0; j < wordMarkSelections.count(); j++) {
                        if (m_wordMarkSelections.value(j).cursor == wordMarkSelections.value(i).cursor
                                && m_wordMarkSelections.value(j).format == wordMarkSelections.value(i).format) {

                            m_wordMarkSelections.removeAt(j);
                            nRemPos = j;
                            break;
                        }
                    }

                    //重新记录标记
                    selection.cursor.setPosition(nWordMarkSelectionStart, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(nSelectionStart, QTextCursor::KeepAnchor);
                    selection.format.setBackground(wordMarkSelections.value(i).format.background());

                    bool bIsInsert = false;///< 标记是否将原有标记分成两段

                    //如果第一段存在
                    if (selection.cursor.selectedText() != "") {
                        bIsInsert = true;
                        m_wordMarkSelections.insert(nRemPos, selection);
                    }

                    QTextEdit::ExtraSelection preSelection;
                    preSelection.format = selection.format;
                    preSelection.cursor = selection.cursor;

                    selection.cursor.setPosition(nSelectionEnd, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(nWordMarkSelectionEnd, QTextCursor::KeepAnchor);

                    //如果第二段存在
                    if (selection.cursor.selectedText() != "") {
                        if (bIsInsert) {
                            m_wordMarkSelections.insert(nRemPos + 1, selection);
                        } else {
                            m_wordMarkSelections.insert(nRemPos, selection);
                        }
                    }

                    //从记录标记的表中替换原有标记（按标记动作记录）
                    QList<QTextEdit::ExtraSelection> selecList;
                    bool bIsFind = false;///< 是否有包含该标记的标记动作

                    for (int j = 0; j < m_mapWordMarkSelections.count(); j++) {
                        auto list = m_mapWordMarkSelections.value(j);

                        for (int k = 0; k < list.count(); k++) {
                            if (list.value(k).cursor == wordMarkSelections.value(i).cursor
                                    && list.value(k).format == wordMarkSelections.value(i).format) {

                                list.removeAt(k);
                                selecList = list;
                                bIsInsert = false;

                                if (preSelection.cursor.selectedText() != "") {
                                    bIsInsert = true;
                                    selecList.insert(k, preSelection);
                                }

                                if (selection.cursor.selectedText() != "") {
                                    if (bIsInsert) {
                                        selecList.insert(k + 1, selection);
                                    } else {
                                        selecList.insert(k, selection);
                                    }
                                }

                                bIsFind = true;
                                break;
                            }
                        }

                        if (bIsFind) {
                            m_mapWordMarkSelections.remove(j);
                            m_mapWordMarkSelections.insert(j, selecList);
                            break;
                        }
                    }

                    //记录新添加的标记
                    selection.cursor.setPosition(nSelectionStart, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(nSelectionEnd, QTextCursor::KeepAnchor);
                    selection.format.setBackground(QColor(strColor));
                    m_wordMarkSelections.append(selection);
                    listSelections->append(selection);
                }
            } else if (nWordMarkSelectionStart >= nSelectionStart && nWordMarkSelectionEnd <= nSelectionEnd) { //如果标记包含已有标记
                bIsContains = true;
                selection.format.setBackground(QColor(strColor));

                //移除已有标记
                for (int j = 0; j < wordMarkSelections.count(); j++) {
                    if (m_wordMarkSelections.value(j).cursor == wordMarkSelections.value(i).cursor
                            && m_wordMarkSelections.value(j).format == wordMarkSelections.value(i).format) {
                        m_wordMarkSelections.removeAt(j);
                        break;
                    }
                }

                //记录新添加的标记
                m_wordMarkSelections.append(selection);

                //如果标记格式不相同
                if (wordMarkSelections.value(i).format != selection.format) {

                    QList<QTextEdit::ExtraSelection> selecList;
                    bool bIsFind = false;///< 是否有包含该标记的标记动作

                    //从记录标记的表中替换原有标记（按标记动作记录）
                    for (int j = 0; j < m_mapWordMarkSelections.count(); j++) {
                        auto list = m_mapWordMarkSelections.value(j);
                        for (int k = 0; k < list.count(); k++) {
                            if (list.value(k).cursor == wordMarkSelections.value(i).cursor
                                    && list.value(k).format == wordMarkSelections.value(i).format) {

                                list.removeAt(k);
                                selecList = list;
                                bIsFind = true;
                                break;
                            }
                        }

                        if (bIsFind) {
                            m_mapWordMarkSelections.remove(j);
                            m_mapWordMarkSelections.insert(j, selecList);
                            break;
                        }
                    }
                }

                listSelections->append(selection);
            } else if (nWordMarkSelectionEnd < nSelectionEnd && nWordMarkSelectionStart < nSelectionStart
                       && nWordMarkSelectionEnd > nSelectionStart) { //如果添加的标记占有原有标记的后段部分

                selection.format.setBackground(QColor(strColor));
                int nRemPos = 0;///< 标记插入位置

                //移除已有标记
                for (int j = 0; j < wordMarkSelections.count(); j++) {
                    if (m_wordMarkSelections.value(j).cursor == wordMarkSelections.value(i).cursor
                            && m_wordMarkSelections.value(j).format == wordMarkSelections.value(i).format) {
                        m_wordMarkSelections.removeAt(j);
                        nRemPos = j;
                        break;
                    }
                }

                //如果标记格式不相同
                if (wordMarkSelections.value(i).format != selection.format) {

                    //从记录标记的表中替换原有标记（分行记录）
                    selection.cursor.setPosition(nWordMarkSelectionStart, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(nSelectionStart, QTextCursor::KeepAnchor);
                    selection.format.setBackground(wordMarkSelections.value(i).format.background());
                    m_wordMarkSelections.insert(nRemPos, selection);

                    QList<QTextEdit::ExtraSelection> selecList;
                    bool bIsFind = false;///< 是否有包含该标记的标记动作

                    //从记录标记的表中替换原有标记（按标记动作记录）
                    for (int j = 0; j < m_mapWordMarkSelections.count(); j++) {
                        auto list = m_mapWordMarkSelections.value(j);
                        for (int k = 0; k < list.count(); k++) {
                            if (list.value(k).cursor == wordMarkSelections.value(i).cursor
                                    && list.value(k).format == wordMarkSelections.value(i).format) {
                                list.removeAt(k);
                                selecList = list;
                                selecList.insert(k, selection);
                                bIsFind = true;
                                break;
                            }
                        }

                        if (bIsFind) {
                            m_mapWordMarkSelections.remove(j);
                            m_mapWordMarkSelections.insert(j, selecList);
                            break;
                        }
                    }

                    //记录新添加的标记
                    selection.cursor.setPosition(nSelectionStart, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(nSelectionEnd, QTextCursor::KeepAnchor);
                    selection.format.setBackground(QColor(strColor));
                    m_wordMarkSelections.append(selection);
                } else { //如果标记格式相同
                    selection.cursor.setPosition(nWordMarkSelectionStart, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(nSelectionEnd, QTextCursor::KeepAnchor);
                    m_wordMarkSelections.insert(nRemPos, selection);
                }

                if (!bIsContains) {
                    listSelections->append(selection);
                }

                bIsContains = true;
            } else if (nWordMarkSelectionEnd > nSelectionEnd && nWordMarkSelectionStart > nSelectionStart
                       && nWordMarkSelectionStart < nSelectionEnd) { //如果添加的标记占有原有标记的前段部分

                selection.format.setBackground(QColor(strColor));
                int nRemPos = 0;///< 标记插入位置

                //移除已有标记
                for (int j = 0; j < wordMarkSelections.count(); j++) {
                    if (m_wordMarkSelections.value(j).cursor == wordMarkSelections.value(i).cursor
                            && m_wordMarkSelections.value(j).format == wordMarkSelections.value(i).format) {
                        m_wordMarkSelections.removeAt(j);
                        nRemPos = j;
                        break;
                    }
                }

                //如果标记格式不相同
                if (wordMarkSelections.value(i).format != selection.format) {

                    //从记录标记的表中替换原有标记（分行记录）
                    selection.cursor.setPosition(nSelectionEnd, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(nWordMarkSelectionEnd, QTextCursor::KeepAnchor);
                    selection.format.setBackground(wordMarkSelections.value(i).format.background());
                    m_wordMarkSelections.insert(nRemPos, selection);

                    QList<QTextEdit::ExtraSelection> selecList;
                    bool bIsFind = false;

                    //从记录标记的表中替换原有标记（按标记动作记录）
                    for (int j = 0; j < m_mapWordMarkSelections.count(); j++) {
                        auto list = m_mapWordMarkSelections.value(j);
                        for (int k = 0; k < list.count(); k++) {
                            if (list.value(k).cursor == wordMarkSelections.value(i).cursor
                                    && list.value(k).format == wordMarkSelections.value(i).format) {
                                list.removeAt(k);
                                selecList = list;
                                selecList.insert(k, selection);
                                bIsFind = true;
                                break;
                            }
                        }

                        if (bIsFind) {
                            m_mapWordMarkSelections.remove(j);
                            m_mapWordMarkSelections.insert(j, selecList);
                            break;
                        }
                    }

                    //记录新添加的标记
                    selection.cursor.setPosition(nSelectionStart, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(nSelectionEnd, QTextCursor::KeepAnchor);
                    selection.format.setBackground(QColor(strColor));
                    m_wordMarkSelections.append(selection);
                } else { //如果标记格式相同
                    selection.cursor.setPosition(nSelectionStart, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(nWordMarkSelectionEnd, QTextCursor::KeepAnchor);
                    m_wordMarkSelections.insert(nRemPos, selection);
                }

                if (!bIsContains) {
                    listSelections->append(selection);
                }
                bIsContains = true;
            }
        }

        if (!bIsContains) {
            selection.format.setBackground(QColor(strColor));
            m_wordMarkSelections.append(selection);
            listSelections->append(selection);
        }

    } else { //如果文档中没有标记
        selection.format.setBackground(QColor(strColor));
        m_wordMarkSelections.append(selection);
        listSelections->append(selection);
    }
#endif
}

void TextEdit::onSelectionArea()
{
    if (textCursor().hasSelection()) {
        m_nSelectStart = textCursor().selectionStart();
        m_nSelectEnd = textCursor().selectionEnd();
        m_nSelectEndLine = document()->findBlock(textCursor().selectionEnd()).blockNumber() + 1;
    } else {
        m_nSelectEndLine = -1;
    }

    //防止三次点击选中一整行的功能失效
    if (m_bIsDoubleClick == true) {
        m_bIsDoubleClick = false;
        return;
    } else if (m_bBeforeIsDoubleClick == true && textCursor().selectedText() != "") {
        m_bBeforeIsDoubleClick = false;
        return;
    }

    if (m_gestureAction != GA_null) {
        QTextCursor cursor = textCursor();
        if (cursor.selectedText() != "") {
            cursor.clearSelection();
            setTextCursor(cursor);
        }
    }
}

void TextEdit::fingerZoom(QString name, QString direction, int fingers)
{
    if (name == "tap" && fingers == 3) {
        slot_translate();
    }
    // 当前窗口被激活,且有焦点
    if (hasFocus()) {
        if (name == "pinch" && fingers == 2) {
            if (direction == "in") {
                // 捏合 in是手指捏合的方向 向内缩小
                qobject_cast<Window *>(this->window())->decrementFontSize();
            } else if (direction == "out") {
                // 捏合 out是手指捏合的方向 向外放大
                qobject_cast<Window *>(this->window())->incrementFontSize();
            }
        }
    }
}

void TextEdit::dragEnterEvent(QDragEnterEvent *event)
{
    QPlainTextEdit::dragEnterEvent(event);
    qobject_cast<Window *>(this->window())->requestDragEnterEvent(event);
}

void TextEdit::dragMoveEvent(QDragMoveEvent *event)
{
    if (m_readOnlyMode) {
        return;
    }

    const QMimeData *data = event->mimeData();

    if (data->hasUrls()) {
        event->acceptProposedAction();
    } else {
        QPlainTextEdit::dragMoveEvent(event);
    }
}

void TextEdit::dropEvent(QDropEvent *event)
{
    const QMimeData *data = event->mimeData();

    // 判断是否存在url信息，需要注意即使hasUrls()为true, urls()仍可能返回空，使用urls().first()可能越界
    QList<QUrl> dataUrls;
    if (data->hasUrls()) {
        dataUrls = data->urls();
    }

    if (!dataUrls.isEmpty() && dataUrls.first().isLocalFile()) {
        qobject_cast<Window *>(this->window())->requestDropEvent(event);
    } else if (data->hasText() && !m_readOnlyMode) {
        //drag text in the same editor
        if (event->source() && event->source()->parent() == this) {
            auto cursor = this->textCursor();
            int srcpos = std::min(cursor.position(), cursor.anchor());
            //use default behavior to make the cursor blink normally
            QPlainTextEdit::dropEvent(event);
            cursor = this->textCursor();
            int dstpos = std::min(cursor.position(), cursor.anchor()) - data->text().size();

            // 检测是否为拷贝文件而不是移动文件，使用Ctrl仅拷贝
            bool copyText = event->keyboardModifiers().testFlag(Qt::ControlModifier);

            //fall back to the original state
            if (srcpos != dstpos) {
                cursor.setPosition(dstpos);
                cursor.setPosition(dstpos + data->text().size(), QTextCursor::KeepAnchor);
                cursor.deleteChar();

                cursor.setPosition(srcpos);
                // 拷贝模式下不对原文本操作
                if (!copyText) {
                    cursor.insertText(data->text());
                }
            }
            if (srcpos < dstpos && !copyText) {
                dstpos += data->text().size();
            }

            //perform moveText operation
            moveText(srcpos, dstpos, data->text(), copyText);

            //drag text to another editor
        } else if (event->source() && event->source()->parent() != this) {

            //use default behavior to make the cursor blink normally
            QPlainTextEdit::dropEvent(event);

            //operations in the destination editor.
            //firstly do the restore operation,
            //then, insert the text
            auto cursor = this->textCursor();
            int dstpos = std::min(cursor.position(), cursor.anchor()) - data->text().size();
            cursor.setPosition(dstpos);
            cursor.setPosition(dstpos + data->text().size(), QTextCursor::KeepAnchor);
            cursor.deleteChar();
            auto com = new DragInsertTextUndoCommand(cursor, data->text(), this);
            m_pUndoStack->push(com);

            //operations in the source editor.
            //firstly do the restore operation,
            //then, delete the text
            auto another = qobject_cast<TextEdit *>(event->source()->parent());
            auto cursor2 = another->textCursor();
            cursor2.insertText(data->text());
            cursor2.setPosition(cursor2.position() - data->text().size(), QTextCursor::KeepAnchor);
            auto com2 = new DeleteBackCommand(cursor2, another);
            another->m_pUndoStack->push(com2);
        } else if (!data->text().isEmpty()) {
            if (m_bReadOnlyPermission || m_readOnlyMode) {
                return;
            }

            QPlainTextEdit::dropEvent(event);
            auto cursor = this->textCursor();
            int dstpos = std::min(cursor.position(), cursor.anchor()) - data->text().size();
            cursor.setPosition(dstpos);
            cursor.setPosition(dstpos + data->text().size(), QTextCursor::KeepAnchor);
            cursor.deleteChar();
            auto com = new DragInsertTextUndoCommand(cursor, data->text(), this);
            m_pUndoStack->push(com);
        }
    } else {
        QPlainTextEdit::dropEvent(event);
    }
}

void TextEdit::inputMethodEvent(QInputMethodEvent *e)
{
    m_bIsInputMethod = true;

    if (m_isSelectAll)
        QPlainTextEdit::selectAll();

    if (m_readOnlyMode || m_bReadOnlyPermission) {
        return;
    }

    if (m_isPreeditBefore) {
        // 每次 preedit 都是完整的字符串，所以撤销上次的 preedit
        undo_();

        // 将光标移回原位
        if (Overwrite == m_cursorMode) {
            auto cursor = textCursor();
            cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::MoveAnchor, m_preeditLengthBefore);
            setTextCursor(cursor);
        }
    }
    bool isPreedit = !e->preeditString().isEmpty();
    if (isPreedit || !e->commitString().isEmpty()) {
        const QString &text = isPreedit ? e->preeditString() : e->commitString();

        //列编辑添加撤销重做
        if (m_bIsAltMod && !m_altModSelections.isEmpty()) {
            insertColumnEditTextEx(text);
        } else {
            // 覆盖模式下输入法输入时，单独处理，模拟选中替换处理
            if (Overwrite == m_cursorMode) {
                auto cursor = this->textCursor();
                // 设置光标选中后续与当前输入法输入的文本相同长度
                cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, text.size());
                // 使用此光标信息插入将输入的文本替换选中的长度
                insertSelectTextEx(cursor, text);
            } else {
                insertSelectTextEx(textCursor(), text);
            }
        }

        m_isSelectAll = false;
    }

    m_isPreeditBefore = isPreedit;
    m_preeditLengthBefore = e->preeditString().length();

    // check if del operation, e.g.: for AI speech to text
    if (e->preeditString().isEmpty() && e->commitString().isEmpty()
        && e->replacementLength() && e->replacementStart() < 0) {
        if (m_bIsAltMod && !m_altModSelections.isEmpty()) {
            // check position
            if (e->replacementLength() > 1) {
                for (auto &seleciton : m_altModSelections) {
                    seleciton.cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, e->replacementLength());
                }
            }

            QUndoCommand *pDeleteStack = new DeleteBackAltCommand(m_altModSelections, this);
            m_pUndoStack->push(pDeleteStack);
        } else {
            auto cursor = textCursor();
            cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, e->replacementLength());
            QUndoCommand *pDeleteStack = new DeleteTextUndoCommand(cursor, this);
            m_pUndoStack->push(pDeleteStack);
        }
    }
}

void TextEdit::mousePressEvent(QMouseEvent *e)
{
    if (m_bIsFindClose)
    {
        m_bIsFindClose = false;
        removeKeywords();
    }
    if (e->button() != Qt::RightButton)
        m_isSelectAll = false;

    if (Qt::MouseEventSynthesizedByQt == e->source())
    {
        m_startY = e->y();
        m_startX = e->x();
    }
    if (e->source() == Qt::MouseEventSynthesizedByQt)
    {
        m_lastTouchBeginPos = e->pos();

        if (QScroller::hasScroller(this))
        {
            QScroller::scroller(this)->deleteLater();
        }

        if (m_updateEnableSelectionByMouseTimer)
        {
            m_updateEnableSelectionByMouseTimer->stop();
        }
        else
        {
            m_updateEnableSelectionByMouseTimer = new QTimer(this);
            m_updateEnableSelectionByMouseTimer->setSingleShot(true);

            static QObject *theme_settings = reinterpret_cast<QObject *>(qvariant_cast<quintptr>(qApp->property("_d_theme_settings_object")));
            QVariant touchFlickBeginMoveDelay;

            if (theme_settings)
            {
                touchFlickBeginMoveDelay = theme_settings->property("touchFlickBeginMoveDelay");
            }

            m_updateEnableSelectionByMouseTimer->setInterval(touchFlickBeginMoveDelay.isValid() ? touchFlickBeginMoveDelay.toInt() : 300);
            connect(m_updateEnableSelectionByMouseTimer, &QTimer::timeout, m_updateEnableSelectionByMouseTimer, &QTimer::deleteLater);
        }

        m_updateEnableSelectionByMouseTimer->start();
    }

    // add for single refers to the sliding
    if (e->type() == QEvent::MouseButtonPress && e->source() == Qt::MouseEventSynthesizedByQt)
    {
        m_lastMouseTimeX = e->timestamp();
        m_lastMouseTimeY = e->timestamp();
        m_lastMouseYpos = e->pos().y();
        m_lastMouseXpos = e->pos().x();

        if (tweenY.activeY())
        {
            m_slideContinueY = true;
            tweenY.stopY();
        }

        if (tweenX.activeX())
        {
            m_slideContinueX = true;
            tweenX.stopX();
        }
    }

    if (e->modifiers() == Qt::AltModifier)
    {
        m_bIsAltMod = true;
        // 鼠标点击位置为光标位置 　获取光标行列位置
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(e);
        m_altStartTextCursor = this->cursorForPosition(mouseEvent->pos());
        m_altStartTextCursor.clearSelection();
        this->setTextCursor(m_altStartTextCursor);
        m_altModSelections.clear();
    }
    else
    {
        if (e->button() != 2)
        { // 右键,调用右键菜单时候不能清空
            m_bIsAltMod = false;
            m_altModSelections.clear();
        }
    }

    QPlainTextEdit::mousePressEvent(e);
}

void TextEdit::mouseMoveEvent(QMouseEvent *e)
{
    if (Qt::MouseEventSynthesizedByQt == e->source())
    {
        m_endY = e->y();
        m_endX = e->x();
    }

    // add for single refers to the sliding
    if (e->type() == QEvent::MouseMove && e->source() == Qt::MouseEventSynthesizedByQt)
    {
        const ulong diffTimeX = e->timestamp() - m_lastMouseTimeX;
        const ulong diffTimeY = e->timestamp() - m_lastMouseTimeY;
        const int diffYpos = e->pos().y() - m_lastMouseYpos;
        const int diffXpos = e->pos().x() - m_lastMouseXpos;
        m_lastMouseTimeX = e->timestamp();
        m_lastMouseTimeY = e->timestamp();
        m_lastMouseYpos = e->pos().y();
        m_lastMouseXpos = e->pos().x();

        if (m_gestureAction == GA_slide)
        {
            QFont font = this->font();

            /*开根号时数值越大衰减比例越大*/
            qreal direction = diffYpos > 0 ? 1.0 : -1.0;
            slideGestureY(-direction * sqrt(abs(diffYpos)) / font.pointSize());
            qreal directionX = diffXpos > 0 ? 1.0 : -1.0;
            slideGestureX(-directionX * sqrt(abs(diffXpos)) / font.pointSize());

            /*预算惯性滑动时间*/
            m_stepSpeedY = static_cast<qreal>(diffYpos) / static_cast<qreal>(diffTimeY + 0.000001);
            durationY = sqrt(abs(m_stepSpeedY)) * 1000;
            m_stepSpeedX = static_cast<qreal>(diffXpos) / static_cast<qreal>(diffTimeX + 0.000001);
            durationX = sqrt(abs(m_stepSpeedX)) * 1000;

            /*预算惯性滑动距离,4.0为调优数值*/
            m_stepSpeedY /= sqrt(font.pointSize() * 4.0);
            changeY = m_stepSpeedY * sqrt(abs(m_stepSpeedY)) * 100;
            m_stepSpeedX /= sqrt(font.pointSize() * 4.0);
            changeX = m_stepSpeedX * sqrt(abs(m_stepSpeedX)) * 100;

            // return true;
        }

        if (m_gestureAction != GA_null)
        {
            // return true;
        }
    }

    // other apps will override their own cursor when opened
    // so they need to be restored.
    QApplication::restoreOverrideCursor();

    if (viewport()->cursor().shape() != Qt::IBeamCursor)
    {
        viewport()->setCursor(Qt::IBeamCursor);
    }

    QPlainTextEdit::mouseMoveEvent(e);
    if (e->modifiers() == Qt::AltModifier && m_bIsAltMod)
    {
        m_altModSelections.clear();
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(e);

        QPoint curPos = mouseEvent->pos();
        m_altEndTextCursor = this->cursorForPosition(curPos);
        int row = m_altEndTextCursor.blockNumber();
        int startRow = m_altStartTextCursor.blockNumber();
        int minRow = startRow < row ? startRow : row;
        int maxRow = startRow > row ? startRow : row;
        QTextCharFormat format;
        QPalette palette;
        QColor highlightBackground = DGuiApplicationHelper::instance()->applicationPalette().color(QPalette::Highlight);
        format.setBackground(highlightBackground);
        format.setForeground(palette.highlightedText());
        int judgeAncherPosX;
        int judgeStartLength;
        int startLineIdx;
        {
            QTextBlock block = document()->findBlockByNumber(startRow);
            int startCurPos = m_altStartTextCursor.positionInBlock();
            QTextLine startLine = block.layout()->lineForTextPosition(startCurPos);
            judgeAncherPosX = startLine.cursorToX(startCurPos);
            judgeStartLength = startCurPos - startLine.textStart();
            startLineIdx = startLine.lineNumber();
        }
        int judgeCursorPosX;
        int judgeEndLength;
        int endLineIdx;
        {
            QTextBlock block = document()->findBlockByNumber(row);
            int endCurPos = m_altEndTextCursor.positionInBlock();
            QTextLine endLine = block.layout()->lineForTextPosition(endCurPos);
            judgeCursorPosX = curPos.x();
            judgeEndLength = endCurPos - endLine.textStart();
            endLineIdx = endLine.lineNumber();
        }
        bool isDown = false;
        if (row > startRow)
        {
            isDown = true;
        }
        else if (row == startRow && endLineIdx >= startLineIdx)
        {
            isDown = true;
        }
        for (int iRow = minRow; iRow <= maxRow; iRow++)
        {
            QTextBlock block = document()->findBlockByNumber(iRow);
            int lineAt = 0;
            int lineCount = block.lineCount();
            // 对开始块和结束块的行数做判断
            if (iRow == minRow)
            {
                lineAt = isDown ? startLineIdx : endLineIdx;
            }
            if (iRow == maxRow)
            {
                lineCount = isDown ? endLineIdx + 1 : startLineIdx + 1;
            }
            for (; lineAt < lineCount; lineAt++)
            {
                // 引入行的判断
                QTextLine lineInBlock = block.layout()->lineAt(lineAt);
                int lineLength = lineInBlock.textLength();
                if (lineLength < judgeStartLength && lineLength < judgeEndLength)
                {
                    continue;
                }
                QTextCursor cursor = this->textCursor();
                cursor.clearSelection();
                setTextCursor(cursor);
                int properColumn;
                int blockPos = block.position();
                properColumn = lineInBlock.xToCursor(judgeAncherPosX);
                cursor.setPosition(blockPos + properColumn, QTextCursor::MoveAnchor);
                // 由于窗口大小小于块的最大长度的外部UI问题，这里要进行对鼠标pos进行若达到最大值无法对每行最后一个字符的覆盖的问题处理
                // 扩大了judgeCursorPosX的最大限度。
                int lineEndPosInBlock = lineInBlock.textStart() + lineInBlock.textLength();
                int lineEndPosToX = lineInBlock.cursorToX(lineEndPosInBlock);
                if (lineInBlock.width() <= lineEndPosToX)
                {
                    if (judgeCursorPosX > lineInBlock.cursorToX(lineEndPosInBlock - 1))
                    {
                        judgeCursorPosX = lineEndPosToX;
                    }
                }
                properColumn = lineInBlock.xToCursor(judgeCursorPosX);
                cursor.setPosition(blockPos + properColumn, QTextCursor::KeepAnchor);
                QTextEdit::ExtraSelection selection;
                selection.cursor = cursor;
                selection.format = format;
                m_altModSelections << selection;
            }
        }
        renderAllSelections();
        update();
    }
}

void TextEdit::mouseReleaseEvent(QMouseEvent *e)
{
    //add for single refers to the sliding
    if (e->type() == QEvent::MouseButtonRelease && e->source() == Qt::MouseEventSynthesizedByQt) {
        if (m_gestureAction == GA_slide) {

            tweenX.startX(0, 0, changeX, durationX, std::bind(&TextEdit::slideGestureX, this, std::placeholders::_1));
            tweenY.startY(0, 0, changeY, durationY, std::bind(&TextEdit::slideGestureY, this, std::placeholders::_1));
        }

        m_gestureAction = GA_null;
    }

    int i = m_endY - m_startY;
    int k = m_endX - m_startX;
    if (Qt::MouseEventSynthesizedByQt == e->source()
            && (i > 10 && this->verticalScrollBar()->value() != 0)
            && (k > 10 && this->horizontalScrollBar()->value() != 0)) {
        e->accept();
        return;
    }

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    bool midClick = (e->button() == Qt::MidButton);
#else
    bool midClick = (e->button() == Qt::MiddleButton);
#endif

    if (midClick) {
        bool midButtonPaste = m_settings->settings->option("advance.editor.allow_midbutton_paste")->value().toBool();
        if (midButtonPaste) {
            // 只读模式过滤鼠标中间黏贴
            if (m_readOnlyMode || m_bReadOnlyPermission) {
                popupNotify(tr("Read-Only mode is on"));
                return;
            }

            slotPasteAction();
        }
        return;
    }

    return QPlainTextEdit::mouseReleaseEvent(e);
}

void TextEdit::keyPressEvent(QKeyEvent *e)
{
    Qt::KeyboardModifiers modifiers = e->modifiers();
    QString key = Utils::getKeyshortcut(e);
    //没有修改键　插入文件
    //按下esc的时候,光标退出编辑区，切换至标题栏
    if (modifiers == Qt::NoModifier && e->key() == Qt::Key_Escape) {
        emit signal_setTitleFocus();
        return;
    }

    if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "copy")) {
        slotCopyAction();
        return;
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "selectall")) {
#if 0
        //2021-2-25:setSelectAll()替换
        if (m_wrapper->getFileLoading()) {
            return;
        }
        m_bIsAltMod = false;
        selectAll();
#endif
        setSelectAll();
        return;
    }

    if (m_readOnlyMode || m_bReadOnlyPermission) {
        if (key == "J") {
            nextLine();
            return;
        } else if (key == "K") {
            prevLine();
            return;
        } else if (key == ",") {
            moveToEnd();
            return;
        } else if (key == ".") {
            moveToStart();
            return;
        } else if (key == "H") {
            backwardChar();
            return;
        } else if (key == "L") {
            forwardChar();
            return;
        } else if (key == "Space") {
            scrollUp();
            return;
        } else if (key == "V") {
            scrollDown();
            return;
        } else if (key == "F") {
            forwardWord();
            return;
        } else if (key == "B") {
            backwardWord();
            return;
        } else if (key == "A") {
            moveToStartOfLine();
            return;
        } else if (key == "E") {
            moveToEndOfLine();
            return;
        } else if (key == "M") {
            moveToLineIndentation();
            return;
        } else if (key == "Q" && m_bReadOnlyPermission == false) {
//            setReadOnly(false);
            toggleReadOnlyMode();
            return;
        } else if (key == "Shfit+J") {
            scrollLineUp();
            return;
        } else if (key == "Shift+K") {
            scrollLineDown();
            return;
        } else if (key == "P") {
            forwardPair();
            return;
        } else if (key == "N") {
            backwardPair();
            return;
        } else if (key == "Shift+:") {
            copyLines();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "mark")) {
            toggleMarkSelections();
            return;
        } else if ((key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "togglereadonlymode")/* || key=="Alt+Meta+L"*/)
                   && m_bReadOnlyPermission == false) {
//            setReadOnly(false);
            toggleReadOnlyMode();
            return;
        } else if (key == "Shift+/" && e->modifiers() == Qt::ControlModifier) {
            e->ignore();
        } else if (e->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
            if (e->key() == Qt::Key_Return || e->key() == Qt::Key_D || e->key() == Qt::Key_K
                    || e->key() == Qt::Key_Up || e->key() == Qt::Key_Down) {
                popupNotify(tr("Read-Only mode is on"));
                return;
            } else {
                e->ignore();
            }
        } else if (e->modifiers() == Qt::ControlModifier) {
            if (e->key() == Qt::Key_Return || e->key() == Qt::Key_K || e->key() == Qt::Key_X ||
                    e->key() == Qt::Key_V || e->key() == Qt::Key_J || e->key() == Qt::Key_Z || e->key() == Qt::Key_Y) {
                popupNotify(tr("Read-Only mode is on"));
                return;
            } else {
                e->ignore();
            }
        } else if (e->key() == Qt::Key_Control || e->key() == Qt::Key_Shift) {
            e->ignore();
        } else if (e->key() == Qt::Key_F11 || e->key() == Qt::Key_F5) {
            e->ignore();
            return;
        } else if (e->modifiers() == Qt::NoModifier || e->modifiers() == Qt::KeypadModifier) {
            popupNotify(tr("Read-Only mode is on"));
            return;
        } else {
            // If press another key
            // the main window does not receive
            e->ignore();
            return;
        }
    } else {
        if (isReadOnly()) { //原生接口setReadOnly不生效,在这里拦截模拟ReadOnly
            return;
        }

        // 左右移动光标后退出全选状态
        if (Qt::Key_Left == e->key()
                || Qt::Key_Right == e->key()) {
            m_isSelectAll = false;
            // 继续后续判断
        }

        //插入键盘可现实字符
        if (modifiers == Qt::NoModifier && (e->key() <= Qt::Key_ydiaeresis && e->key() >= Qt::Key_Space) && !e->text().isEmpty()) {

            if (m_isSelectAll)
                QPlainTextEdit::selectAll();

            //非修改键盘按键加撤销重做栈
            if (m_bIsAltMod && !m_altModSelections.isEmpty()) {
                insertColumnEditTextEx(e->text());
            } else {
                if (m_cursorMode == Overwrite) {
                    auto cursor = this->textCursor();
                    cursor.clearSelection();
                    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
                    this->setTextCursor(cursor);
                }
                insertSelectTextEx(textCursor(), e->text());
            }

            m_isSelectAll = false;
            return;
        }

        //键盘右边数字键
        if (modifiers == Qt::KeypadModifier && (e->key() <= Qt::Key_9 && e->key() >= Qt::Key_Asterisk) && !e->text().isEmpty()) {

            if (m_isSelectAll)
                QPlainTextEdit::selectAll();

            //非修改键盘按键加撤销重做栈
            if (m_bIsAltMod && !m_altModSelections.isEmpty()) {
                insertColumnEditTextEx(e->text());
            } else {
                if (m_cursorMode == Overwrite) {
                    auto cursor = this->textCursor();
                    cursor.clearSelection();
                    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
                    this->setTextCursor(cursor);
                }
                insertSelectTextEx(textCursor(), e->text());
            }
            m_isSelectAll = false;
            return;
        }

        //插入空白字符
        if (modifiers == Qt::NoModifier && (e->key() == Qt::Key_Tab || e->key() == Qt::Key_Return) ||
                modifiers == Qt::KeypadModifier && (e->key() == Qt::Key_Enter)) {

            if (m_isSelectAll)
                QPlainTextEdit::selectAll();

            //列编辑添加撤销重做
            if (m_bIsAltMod && !m_altModSelections.isEmpty()) {
                insertColumnEditTextEx(e->text());
            } else {
                auto cursor = this->textCursor();
                bool hassel = cursor.hasSelection();
                auto selectedtext = cursor.selectedText();

                //calculate the startline and endline.
                auto anchor = cursor.anchor();
                auto pos = cursor.position();
                cursor.setPosition(anchor);
                auto line1 = cursor.blockNumber();
                cursor.setPosition(pos);
                auto line2 = cursor.blockNumber();

                //get the text of current line.
                cursor.movePosition(QTextCursor::StartOfBlock);
                cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                auto currentline = cursor.selectedText();

                if (hassel && e->key() == Qt::Key_Tab && (line1 != line2 || selectedtext == currentline)) {
                    indentText();
                } else {
                    insertSelectTextEx(textCursor(), e->text());
                }
            }
            m_isSelectAll = false;
            return;
        }

        //列编辑 删除撤销重做
        if (modifiers == Qt::NoModifier && (e->key() == Qt::Key_Backspace)) {
            if (m_isSelectAll)
                QPlainTextEdit::selectAll();

            if (m_bIsAltMod && !m_altModSelections.isEmpty()) {
                QUndoCommand *pDeleteStack = new DeleteBackAltCommand(m_altModSelections, this);
                m_pUndoStack->push(pDeleteStack);
            } else {
                //修改backspace删除，在文档最末尾点击backspace,引起标签栏*出现问题
                QTextCursor cursor = textCursor();
                if (!cursor.hasSelection()) {
                    cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
                }
                QString m_delText = cursor.selectedText();
                if (m_delText.size() <= 0) return;
                QUndoCommand *pDeleteStack = new DeleteTextUndoCommand(cursor, this);
                m_pUndoStack->push(pDeleteStack);
            }
            m_isSelectAll = false;
            return;
        }

        //列编辑 向后删除撤销重做
        if (modifiers == Qt::NoModifier && (e->key() == Qt::Key_Delete)) {
            if (m_isSelectAll)
                QPlainTextEdit::selectAll();

            if (m_bIsAltMod && !m_altModSelections.isEmpty()) {
                DeleteBackAltCommand *commond = new DeleteBackAltCommand(m_altModSelections, this, true);
                m_pUndoStack->push(commond);
            } else {
                //修改delete删除，在文档最末尾点击delete,引起标签栏*出现问题
                QTextCursor cursor = textCursor();
                if (!cursor.hasSelection()) {
                    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
                }
                QString m_delText = cursor.selectedText();
                if (m_delText.size() <= 0) return;

                DeleteBackCommand *commond = new DeleteBackCommand(cursor, this);
                m_pUndoStack->push(commond);
            }
            m_isSelectAll = false;
            return;
        }

        //fix 66710 输入的内容为英文符号时，文本编辑器未识别为临时文件
        //fix 75313  lxp 2021.4.22
        if ((modifiers == Qt::ShiftModifier || e->key() == Qt::Key_Shift) && !e->text().isEmpty()) {

            if (m_isSelectAll)
                QPlainTextEdit::selectAll();

            if (m_bIsAltMod) {
                insertColumnEditTextEx(e->text());
            } else {
                insertSelectTextEx(textCursor(), e->text());
            }
            m_isSelectAll = false;
            return;
        }

        // alt+m 弹出编辑器右键菜单
        if (e->modifiers() == Qt::AltModifier && !e->text().compare(QString("m"), Qt::CaseInsensitive)) {
            popRightMenu();
            return;
        }

        //快捷建处理
        if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "undo")) {
            this->undo_();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "redo")) {
            this->redo_();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "cut")) {
#if 0
            //列编辑添加撤销重做
            if (m_bIsAltMod && !m_altModSelections.isEmpty()) {
                QString data;
                for (auto sel : m_altModSelections) data.append(sel.cursor.selectedText());
                //删除有选择
                for (int i = 0; i < m_altModSelections.size(); i++) {
                    if (m_altModSelections[i].cursor.hasSelection()) {
                        QUndoCommand *pDeleteStack = new DeleteTextUndoCommand(m_altModSelections[i].cursor);
                        m_pUndoStack->push(pDeleteStack);
                    }
                }
                //设置到剪切板
                QClipboard *clipboard = QApplication::clipboard();   //获取系统剪贴板指针
                clipboard->setText(data);
            } else {
                QTextCursor cursor = textCursor();
                //有选择内容才剪切
                if (cursor.hasSelection()) {
                    QString data = cursor.selectedText();
                    deleteTextEx(cursor);
                    QClipboard *clipboard = QApplication::clipboard();   //获取系统剪贴板指针
                    clipboard->setText(data);
                }
            }
#endif
            this->cut();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "paste")) {
#if 0
            //添加剪切板内容到撤销重做栈
            const QClipboard *clipboard = QApplication::clipboard(); //获取剪切版内容

            if (m_bIsAltMod && !m_altModSelections.isEmpty()) {
                insertColumnEditTextEx(clipboard->text());
            } else {
                insertSelectTextEx(textCursor(), clipboard->text());
            }
#endif
            slotPasteAction();
            return;
        }  else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "scrollup")) {
            //向上翻页
            scrollUp();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "scrolldown")) {
            //向下翻页
            scrollDown();
            return;
        }  else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "copylines")) {
            copyLines();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "cutlines")) {
            cutlines();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "indentline")) {
            QPlainTextEdit::keyPressEvent(e);
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "backindentline")) {
            unindentText();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "forwardchar")) {
            forwardChar();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "backwardchar")) {
            backwardChar();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "forwardword")) {
            forwardWord();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "backwardword")) {
            backwardWord();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "nextline")) {
            nextLine();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "prevline")) {
            prevLine();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "newline")/* || key == "Return"*/) {
            newline();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "opennewlineabove")) {
            openNewlineAbove();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "opennewlinebelow")) {
            openNewlineBelow();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "duplicateline")) {
            duplicateLine();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "killline")) {
            killLine();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "killcurrentline")) {
            killCurrentLine();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "swaplineup")) {
            moveLineDownUp(true);
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "swaplinedown")) {
            moveLineDownUp(false);
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "scrolllineup")) {
            scrollLineUp();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "scrolllinedown")) {
            scrollLineDown();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "scrollup")) {
            scrollUp();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "scrolldown")) {
            scrollDown();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "movetoendofline")) {
            moveToEndOfLine();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "movetostartofline")) {
            moveToStartOfLine();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "movetostart")) {
            moveToStart();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "movetoend")) {
            moveToEnd();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "movetolineindentation")) {
            moveToLineIndentation();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "upcaseword")) {
            upcaseWord();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "downcaseword")) {
            downcaseWord();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "capitalizeword")) {
            capitalizeWord();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "killbackwardword")) {
            killBackwardWord();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "killforwardword")) {
            killForwardWord();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "forwardpair")) {
            forwardPair();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "backwardpair")) {
            backwardPair();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "transposechar")) {
            transposeChar();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "setmark")) {
            setMark();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "exchangemark")) {
            exchangeMark();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "joinlines")) {
            joinLines();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "togglereadonlymode")/*|| key=="Alt+Meta+L"*/) {
            //setReadOnly(false);
            toggleReadOnlyMode();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "togglecomment")) {
            toggleComment(true);
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "removecomment")) {
            toggleComment(false);
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "switchbookmark")) {
            m_bIsShortCut = true;
            addOrDeleteBookMark();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "movetoprebookmark")) {
            moveToPreviousBookMark();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "movetonextbookmark")) {
            moveToNextBookMark();
            return;
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "mark")) {
            toggleMarkSelections();
            return;
        } else if (e->key() == Qt::Key_Insert && key != "Shift+Ins" && e->modifiers() == Qt::NoModifier) {
            setOverwriteMode(!overwriteMode());
            //update();
            if (!overwriteMode()) {
                auto cursor = this->textCursor();
                cursor.clearSelection();
                cursor.movePosition(QTextCursor::Right);
                this->setTextCursor(cursor);

                cursor = this->textCursor();
                cursor.movePosition(QTextCursor::Left);
                this->setTextCursor(cursor);
            }

            m_cursorMode = overwriteMode() ? Overwrite : Insert;
            emit cursorModeChanged(m_cursorMode);
            e->accept();
        } else {
            // Post event to window widget if key match window key list.
            for (auto option : m_settings->settings->group("shortcuts.window")->options()) {
                if (key == m_settings->settings->option(option->key())->value().toString()) {
                    e->ignore();
                    return;
                }
            }

            // Post event to window widget if match Alt+0 ~ Alt+9
            QRegularExpression re("^Alt\\+\\d");
            QRegularExpressionMatch match = re.match(key);
            if (match.hasMatch()) {
                e->ignore();
                return;
            }

            /* qt原生控件QPlainTextEdit对Alt+Tab快捷键有接收响应，需求里无定义Alt+Tab快捷键响应功能，遇到该快捷键直接return即可 */
            if (key.contains(QString("Alt+Tab"))) {
                return;
            }

            // Text editor handle key self.
            QPlainTextEdit::keyPressEvent(e);
        }

        //return QPlainTextEdit::keyPressEvent(e);
    }
}

void TextEdit::wheelEvent(QWheelEvent *e)
{
    if (e->modifiers() & Qt::ControlModifier) {
        const int deltaY = e->angleDelta().y();

        if (deltaY < 0) {
            qobject_cast<Window *>(this->window())->decrementFontSize();
        } else {
            qobject_cast<Window *>(this->window())->incrementFontSize();
        }

        return;
    }

    QPlainTextEdit::wheelEvent(e);
}

void TextEdit::contextMenuEvent(QContextMenuEvent *event)
{
    popRightMenu(event->globalPos());
}

void TextEdit::paintEvent(QPaintEvent *e)
{
    DPlainTextEdit::paintEvent(e);

    if (m_altModSelections.length() > 0) {
        for (auto sel : m_altModSelections) {
            if (sel.cursor.hasSelection()) {
                m_hasColumnSelection = true;
                break;
            } else {
                m_hasColumnSelection = false;
            }
        }
    }
    QColor lineColor = palette().text().color();

    if (m_bIsAltMod && !m_altModSelections.isEmpty()) {

        QTextCursor textCursor = this->textCursor();
        int cursorWidth = this->cursorWidth();
        QPainter painter(viewport());
        QPen pen;
        pen.setColor(lineColor);
        pen.setWidth(cursorWidth);
        painter.setPen(pen);

        for (int i = 0; i < m_altModSelections.size(); i++)
        {
            QRect textCursorRect = this->cursorRect(m_altModSelections[i].cursor);
            painter.drawRect(textCursorRect);
        }
    }
}

void TextEdit::resizeEvent(QResizeEvent *e)
{
    if (m_isSelectAll)
        selectTextInView();

    // 显示区域变化时同时更新视图
    markAllKeywordInView();

    // 当前处于文档页面尾部时，缩放后保持焦点在文档页面尾部
    if (e->oldSize().width() < e->size().width() && verticalScrollBar()->maximum() == verticalScrollBar()->value()) {
        QTimer::singleShot(0, [this]() {
            // 宽度变大时文档布局大小变更信号未触发，手动通知
            auto docLayout = this->document()->documentLayout();
            Q_EMIT docLayout->documentSizeChanged(docLayout->documentSize());

            verticalScrollBar()->setValue(verticalScrollBar()->maximum());
        });
    }

    QPlainTextEdit::resizeEvent(e);
}

bool TextEdit::isComment(const QString &text, int index, const QString &commentType)
{
    int length = commentType.length();

    Q_ASSERT(text.length() - index >= length);

    int i = 0;
    while (i < length) {
        if (text.at(index + i) != commentType.at(i))
            return false;
        ++i;
    }
    return true;
}

void TextEdit::restoreColumnEditSelection(const QList<QTextEdit::ExtraSelection> &selections)
{
    m_altModSelections = selections;
}

void TextEdit::unCommentSelection()
{
    if (!m_commentDefinition.isValid())
        return;

    if (m_isSelectAll)
        QPlainTextEdit::selectAll();

    QTextCursor cursor = textCursor();
    QTextDocument *doc = cursor.document();

    int pos = cursor.position();
    int anchor = cursor.anchor();
    int start = qMin(anchor, pos);
    int end = qMax(anchor, pos);
    bool anchorIsStart = (anchor == start);

    QTextBlock startBlock = doc->findBlock(start);
    QTextBlock endBlock = doc->findBlock(end);

    if (end > start && endBlock.position() == end) {
        --end;
        endBlock = endBlock.previous();
    }

    bool doMultiLineStyleUncomment = false;
    bool doMultiLineStyleComment = false;
    bool doSingleLineStyleUncomment = false;

    bool hasSelection = cursor.hasSelection();

    if (hasSelection && m_commentDefinition.hasMultiLineStyle()) {

        QString startText = startBlock.text();
        int startPos = start - startBlock.position();
        const int multiLineStartLength = m_commentDefinition.multiLineStart.length();
        bool hasLeadingCharacters = !startText.left(startPos).trimmed().isEmpty();

        if (startPos >= multiLineStartLength
                && isComment(startText,
                             startPos - multiLineStartLength,
                             m_commentDefinition.multiLineStart)) {
            startPos -= multiLineStartLength;
            start -= multiLineStartLength;
        }

        bool hasSelStart = startPos <= startText.length() - multiLineStartLength
                           && isComment(startText, startPos, m_commentDefinition.multiLineStart);

        QString endText = endBlock.text();
        int endPos = end - endBlock.position();
        const int multiLineEndLength = m_commentDefinition.multiLineEnd.length();
        bool hasTrailingCharacters =
            !endText.left(endPos).remove(m_commentDefinition.singleLine).trimmed().isEmpty()
            && !endText.mid(endPos).trimmed().isEmpty();

        if (endPos <= endText.length() - multiLineEndLength
                && isComment(endText, endPos, m_commentDefinition.multiLineEnd)) {
            endPos += multiLineEndLength;
            end += multiLineEndLength;
        }

        bool hasSelEnd = endPos >= multiLineEndLength
                         && isComment(endText, endPos - multiLineEndLength, m_commentDefinition.multiLineEnd);

        doMultiLineStyleUncomment = hasSelStart && hasSelEnd;
        doMultiLineStyleComment = !doMultiLineStyleUncomment
                                  && (hasLeadingCharacters
                                      || hasTrailingCharacters
                                      || !m_commentDefinition.hasSingleLineStyle());
    } else if (!hasSelection && !m_commentDefinition.hasSingleLineStyle()) {

        QString text = startBlock.text().trimmed();
        doMultiLineStyleUncomment = text.startsWith(m_commentDefinition.multiLineStart)
                                    && text.endsWith(m_commentDefinition.multiLineEnd);
        doMultiLineStyleComment = !doMultiLineStyleUncomment && !text.isEmpty();

        start = startBlock.position();
        end = endBlock.position() + endBlock.length() - 1;

        if (doMultiLineStyleUncomment) {
            int offset = 0;
            text = startBlock.text();
            const int length = text.length();
            while (offset < length && text.at(offset).isSpace())
                ++offset;
            start += offset;
        }
    }

    if (doMultiLineStyleUncomment) {
        cursor.setPosition(end);
        cursor.movePosition(QTextCursor::PreviousCharacter,
                            QTextCursor::KeepAnchor,
                            m_commentDefinition.multiLineEnd.length());
        //cursor.removeSelectedText();
        deleteTextEx(cursor);
        cursor.setPosition(start);
        cursor.movePosition(QTextCursor::NextCharacter,
                            QTextCursor::KeepAnchor,
                            m_commentDefinition.multiLineStart.length());
        //cursor.removeSelectedText();
        deleteTextEx(cursor);
    } else if (doMultiLineStyleComment) {
        cursor.setPosition(end);
        insertTextEx(cursor, m_commentDefinition.multiLineEnd);
        //cursor.insertText(m_commentDefinition.multiLineEnd);
        cursor.setPosition(start);
        insertTextEx(cursor, m_commentDefinition.multiLineStart);
        //cursor.insertText(m_commentDefinition.multiLineStart);
    } else {
        endBlock = endBlock.next();
        doSingleLineStyleUncomment = true;
        for (QTextBlock block = startBlock; block != endBlock; block = block.next()) {
            QString text = block.text().trimmed();
            if (!text.isEmpty() && !text.startsWith(m_commentDefinition.singleLine)) {
                doSingleLineStyleUncomment = false;
                break;
            }
        }

        const int singleLineLength = m_commentDefinition.singleLine.length();
        for (QTextBlock block = startBlock; block != endBlock; block = block.next()) {
            if (doSingleLineStyleUncomment) {
                QString text = block.text();
                int i = 0;
                while (i <= text.size() - singleLineLength) {
                    if (isComment(text, i, m_commentDefinition.singleLine)) {
                        cursor.setPosition(block.position() + i);
                        cursor.movePosition(QTextCursor::NextCharacter,
                                            QTextCursor::KeepAnchor,
                                            singleLineLength);
                        //cursor.removeSelectedText();
                        deleteTextEx(cursor);
                        break;
                    }
                    if (i < text.size()) {
                        if (!text.at(i).isSpace())
                            break;
                    }

                    ++i;
                }
            } else {
                const QString text = block.text();
                foreach (QChar c, text) {
                    if (!c.isSpace()) {
                        if (m_commentDefinition.isAfterWhiteSpaces)
                            cursor.setPosition(block.position() + text.indexOf(c));
                        else
                            cursor.setPosition(block.position());
                        insertTextEx(cursor, m_commentDefinition.singleLine);
                        break;
                    }
                }
            }
        }
    }

    // adjust selection when commenting out
    if (hasSelection && !doMultiLineStyleUncomment && !doSingleLineStyleUncomment) {
        cursor = textCursor();
        if (!doMultiLineStyleComment)
            start = startBlock.position(); // move the comment into the selection
        int lastSelPos = anchorIsStart ? cursor.position() : cursor.anchor();
        if (anchorIsStart) {
            cursor.setPosition(start);
            cursor.setPosition(lastSelPos, QTextCursor::KeepAnchor);
        } else {
            cursor.setPosition(lastSelPos);
            cursor.setPosition(start, QTextCursor::KeepAnchor);
        }
        setTextCursor(cursor);
    }
}

void TextEdit::setComment()
{
    //此函数是删除了unCommentSelection()的if-else的uncomment分支得来的
    if (!m_commentDefinition.isValid())
        return;
    if (m_isSelectAll)
        QPlainTextEdit::selectAll();

    QTextCursor cursor = textCursor();
    if (cursor.isNull()) return;

    QTextDocument *doc = cursor.document();

    int pos = cursor.position();
    int anchor = cursor.anchor();
    int start = qMin(anchor, pos);
    int end = qMax(anchor, pos);
    bool anchorIsStart = (anchor == start);

    QTextBlock startBlock = doc->findBlock(start);
    QTextBlock endBlock = doc->findBlock(end);
    if (end > start && endBlock.position() == end) {
        --end;
        endBlock = endBlock.previous();
    }
    bool doMultiLineStyleUncomment = false;
    bool doMultiLineStyleComment = false;
    bool doSingleLineStyleUncomment = false;

    bool hasSelection = cursor.hasSelection();

    if (hasSelection && m_commentDefinition.hasMultiLineStyle()) {

        QString startText = startBlock.text();
        int startPos = start - startBlock.position();
        const int multiLineStartLength = m_commentDefinition.multiLineStart.length();
        bool hasLeadingCharacters = !startText.left(startPos).trimmed().isEmpty();

        if (startPos >= multiLineStartLength
                && isComment(startText,
                             startPos - multiLineStartLength,
                             m_commentDefinition.multiLineStart)) {
            startPos -= multiLineStartLength;
            start -= multiLineStartLength;
        }

        bool hasSelStart = startPos <= startText.length() - multiLineStartLength
                           && isComment(startText, startPos, m_commentDefinition.multiLineStart);

        QString endText = endBlock.text();
        int endPos = end - endBlock.position();
        const int multiLineEndLength = m_commentDefinition.multiLineEnd.length();
        bool hasTrailingCharacters =
            !endText.left(endPos).remove(m_commentDefinition.singleLine).trimmed().isEmpty()
            && !endText.mid(endPos).trimmed().isEmpty();

        if (endPos <= endText.length() - multiLineEndLength
                && isComment(endText, endPos, m_commentDefinition.multiLineEnd)) {
            endPos += multiLineEndLength;
            end += multiLineEndLength;
        }

        bool hasSelEnd = endPos >= multiLineEndLength
                         && isComment(endText, endPos - multiLineEndLength, m_commentDefinition.multiLineEnd);

        doMultiLineStyleUncomment = hasSelStart && hasSelEnd;
        doMultiLineStyleComment = !doMultiLineStyleUncomment
                                  && (hasLeadingCharacters
                                      || hasTrailingCharacters
                                      || m_commentDefinition.hasMultiLineStyle());
    } else if (!hasSelection && !m_commentDefinition.hasSingleLineStyle()) {
        QString text = startBlock.text().trimmed();
        doMultiLineStyleUncomment = text.startsWith(m_commentDefinition.multiLineStart)
                                    && text.endsWith(m_commentDefinition.multiLineEnd);
        doMultiLineStyleComment = !doMultiLineStyleUncomment && !text.isEmpty();

        start = startBlock.position();
        end = endBlock.position() + endBlock.length() - 1;

        if (doMultiLineStyleUncomment) {
            int offset = 0;
            text = startBlock.text();
            const int length = text.length();
            while (offset < length && text.at(offset).isSpace())
                ++offset;
            start += offset;
        }
    }

    if (doMultiLineStyleComment) {
        // 压入添加信息，注意光标索引顺序不能调换
        QList<QPair<QTextCursor, QString> > multiText;
        cursor.setPosition(end);
        multiText.append(qMakePair(cursor, m_commentDefinition.multiLineEnd));
        cursor.setPosition(start);
        multiText.append(qMakePair(cursor, m_commentDefinition.multiLineStart));

        // 同时将两组插入信息压入撤销栈
        insertMultiTextEx(multiText);
    } else {
        endBlock = endBlock.next();
        doSingleLineStyleUncomment = true;
        for (QTextBlock block = startBlock; block != endBlock; block = block.next()) {
            QString text = block.text().trimmed();
            if (!text.isEmpty() && !text.startsWith(m_commentDefinition.singleLine)) {
                doSingleLineStyleUncomment = false;
                break;
            }
        }

        QList<QPair<QTextCursor, QString> > multiText;
        for (QTextBlock block = startBlock; block != endBlock; block = block.next()) {
            cursor.setPosition(block.position());
            multiText.append(qMakePair(cursor, m_commentDefinition.singleLine));
        }
        // 同时将多组组插入信息压入撤销栈
        insertMultiTextEx(multiText);
    }

    // adjust selection when commenting out
    if (hasSelection && !doMultiLineStyleUncomment && !doSingleLineStyleUncomment) {
        cursor = textCursor();
        if (!doMultiLineStyleComment)
            start = startBlock.position(); // move the comment into the selection
        int lastSelPos = anchorIsStart ? cursor.position() : cursor.anchor();
        if (anchorIsStart) {
            cursor.setPosition(start);
            cursor.setPosition(lastSelPos, QTextCursor::KeepAnchor);
        } else {
            cursor.setPosition(lastSelPos);
            cursor.setPosition(start, QTextCursor::KeepAnchor);
        }
        setTextCursor(cursor);
    }
}

void TextEdit::removeComment()
{
    //此函数是删除了unCommentSelection()的if-else的comment分支得来的
    if (!m_commentDefinition.isValid()) {
        return;
    }

    QString tep = m_commentDefinition.singleLine;
    if (tep.isEmpty()) {
        tep = m_commentDefinition.multiLineStart;
    }
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QRegExp s("\\s");
#else
    QRegularExpression s("\\s");
#endif
    QString abb = tep.remove(s);

    if (m_isSelectAll) {
        QPlainTextEdit::selectAll();
    }

    QTextCursor cursor = textCursor();
    QTextDocument *doc = cursor.document();

    int pos = cursor.position();
    int anchor = cursor.anchor();
    int start = qMin(anchor, pos);
    int end = qMax(anchor, pos);
    bool anchorIsStart = (anchor == start);

    QTextBlock startBlock = doc->findBlock(start);
    QTextBlock endBlock = doc->findBlock(end);

    if (end > start && endBlock.position() == end) {
        --end;
        endBlock = endBlock.previous();
    }

    bool doMultiLineStyleUncomment = false;
    bool doMultiLineStyleComment = false;
    bool doSingleLineStyleUncomment = false;

    bool hasSelection = cursor.hasSelection();

    if (hasSelection && m_commentDefinition.hasMultiLineStyle()) {
        QString startText = startBlock.text();
        int startPos = start - startBlock.position();
        const int multiLineStartLength = m_commentDefinition.multiLineStart.length();
        bool hasLeadingCharacters = !startText.left(startPos).trimmed().isEmpty();

        if (startPos >= multiLineStartLength
                && isComment(startText,
                             startPos - multiLineStartLength,
                             m_commentDefinition.multiLineStart)) {
            startPos -= multiLineStartLength;
            start -= multiLineStartLength;
        }

        bool hasSelStart = startPos <= startText.length() - multiLineStartLength
                           && isComment(startText, startPos, m_commentDefinition.multiLineStart);

        QString endText = endBlock.text();
        int endPos = end - endBlock.position();
        const int multiLineEndLength = m_commentDefinition.multiLineEnd.length();
        bool hasTrailingCharacters =
            !endText.left(endPos).remove(m_commentDefinition.singleLine).trimmed().isEmpty()
            && !endText.mid(endPos).trimmed().isEmpty();

        if (endPos <= endText.length() - multiLineEndLength
                && isComment(endText, endPos, m_commentDefinition.multiLineEnd)) {
            endPos += multiLineEndLength;
            end += multiLineEndLength;
        }

        bool hasSelEnd = endPos >= multiLineEndLength
                         && isComment(endText, endPos - multiLineEndLength, m_commentDefinition.multiLineEnd);
        doMultiLineStyleUncomment = hasSelStart && hasSelEnd;
        doMultiLineStyleComment = !doMultiLineStyleUncomment
                                  && (hasLeadingCharacters
                                      || hasTrailingCharacters
                                      || !m_commentDefinition.hasSingleLineStyle());
    } else if (!hasSelection && !m_commentDefinition.hasSingleLineStyle()) {

        QString text = startBlock.text().trimmed();
        doMultiLineStyleUncomment = text.startsWith(m_commentDefinition.multiLineStart)
                                    && text.endsWith(m_commentDefinition.multiLineEnd);
        doMultiLineStyleComment = !doMultiLineStyleUncomment && !text.isEmpty();
        start = startBlock.position();
        end = endBlock.position() + endBlock.length() - 1;

        if (doMultiLineStyleUncomment) {
            int offset = 0;
            text = startBlock.text();
            const int length = text.length();
            while (offset < length && text.at(offset).isSpace())
                ++offset;
            start += offset;
        }
    }

    if (doMultiLineStyleUncomment) {
        // 注意删除先后顺序不能调换
        QList<QTextCursor> multiText;
        cursor.setPosition(end);
        cursor.movePosition(QTextCursor::PreviousCharacter,
                            QTextCursor::KeepAnchor,
                            m_commentDefinition.multiLineEnd.length());
        multiText.append(cursor);
        cursor.setPosition(start);
        cursor.movePosition(QTextCursor::NextCharacter,
                            QTextCursor::KeepAnchor,
                            m_commentDefinition.multiLineStart.length());
        multiText.append(cursor);

        // 同时删除多组注释文本
        deleteMultiTextEx(multiText);
    } else if (textCursor().hasSelection() && m_commentDefinition.singleLine.isEmpty()) {
        doSingleLineStyleUncomment = false;
    } else {
        int tmp = 0;//备注偏移量，判断备注标记后面有没有空格
        endBlock = endBlock.next();
        doSingleLineStyleUncomment = true;
        for (QTextBlock block = startBlock; block != endBlock; block = block.next()) {
            QString text = block.text().trimmed();
            if (!text.isEmpty() && m_commentDefinition.singleLine.isEmpty()) {
                if (text.startsWith(abb)) {
                    doSingleLineStyleUncomment = false;
                    break;
                }
            }

            if (!text.isEmpty() && (!text.startsWith(m_commentDefinition.singleLine) || (!text.startsWith(m_commentDefinition.multiLineStart)))) {
                if (!text.startsWith(abb)) {
                    doSingleLineStyleUncomment = false;
                    break;
                }
            }
        }

        int singleLineLength = m_commentDefinition.singleLine.length();
        QString text = startBlock.text().trimmed();

        if (text.startsWith(m_commentDefinition.singleLine)) {
            tmp = 0;
        } else if (text.startsWith(abb)) {
            tmp = 1;
        }
        QString check = "";

        QList<QTextCursor> multiText;
        for (QTextBlock block = startBlock; block != endBlock; block = block.next()) {
            if (doSingleLineStyleUncomment) {
                text = block.text();
                int i = 0;
                if (tmp == 1)
                    check = abb;
                else {
                    check = m_commentDefinition.singleLine;
                }
                while (i <= text.size() - singleLineLength) {
                    if (isComment(text, i, check)) {
                        cursor.setPosition(block.position() + i);
                        cursor.movePosition(QTextCursor::NextCharacter,
                                            QTextCursor::KeepAnchor,
                                            singleLineLength - tmp);
                        // 添加待删除的文本
                        multiText.append(cursor);
                        break;
                    }
                    if (i < text.size()) {
                        if (!text.at(i).isSpace())
                            break;
                    }

                    ++i;
                }
            }
        }

        if (!multiText.isEmpty()) {
            // 同时删除多组注释文本
            deleteMultiTextEx(multiText);
        }
    }

    // adjust selection when commenting out
    if (hasSelection && !doMultiLineStyleUncomment && !doSingleLineStyleUncomment) {
        cursor = textCursor();
        if (!doMultiLineStyleComment)
            start = startBlock.position(); // move the comment into the selection
        int lastSelPos = anchorIsStart ? cursor.position() : cursor.anchor();
        if (anchorIsStart) {
            cursor.setPosition(start);
            cursor.setPosition(lastSelPos, QTextCursor::KeepAnchor);
        } else {
            cursor.setPosition(lastSelPos);
            cursor.setPosition(start, QTextCursor::KeepAnchor);
        }
        setTextCursor(cursor);
    }
}

bool TextEdit::blockContainStrBrackets(int line)
{
    //获取行数文本块
    QTextBlock curBlock = document()->findBlockByNumber(line);
    QString text = curBlock.text();
    //若存在字符串行，多个字符串中间的 '{' '}' 同样被忽略
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QRegExp regExp("\".*\"");
#else
    QRegularExpression regExp("\".*\"");
#endif
    if (text.contains(regExp)) {
        QString curText = text.remove(regExp);
        return curText.contains("{");
    } else {
        return text.contains("{");
    }
}

/**
 * @brief 根据传入的起始行号 \a line ，查找在此行号下的折叠区域，查找后将返回查询过程中的折叠区域起始文本块
 *      \a beginBlock 和结束文本块 \a endBlock 。
 * @param line          查找起始行号
 * @param beginBlock    起始文本块
 * @param endBlock      结束文本框
 * @param curBlock      行号对应文本块
 * @return 是否查找到折叠区域
 */
bool TextEdit::findFoldBlock(int line, QTextBlock &beginBlock, QTextBlock &endBlock, QTextBlock &curBlock)
{
    qDebug() << "Finding fold block for line" << line;
    //使用统一 折叠判断算法 根据左右"{""}"高亮算法
    QTextDocument *doc = document();
    //获取行号对应文本块
    curBlock = doc->findBlockByNumber(line);

    //开始本文块 结束文本块
    beginBlock = curBlock.next();
    endBlock = curBlock.next();

    //如果是第一行不包括左括弧"{"
    if (line == 0 && !curBlock.text().contains("{")) {
        qDebug() << "First line without opening brace, moving to next block";
        curBlock = curBlock.next();
    }

    //获取当前块文本左括弧所在光标
    int position = curBlock.position();
    int offset = curBlock.text().lastIndexOf('{');
    position += offset;

    //获取当前文本块第一个字符光标
    QChar begin = '{', end = '}';
    QTextCursor cursor = textCursor();
    cursor.setPosition(position, QTextCursor::MoveAnchor);
    cursor.clearSelection();

    //左括弧光标 右括弧光标
    QTextCursor bracketBeginCursor = cursor;
    QTextCursor bracketEndCursor = cursor;
    bracketBeginCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);

    //获取最后右括弧光标
    int braceDepth = 0;
    // 判断当前是否处于字符串内，字符串内的 { } 不进行统计
    bool inCodeString = false;
    QChar c;
    while (!(c = doc->characterAt(position)).isNull()) {
        // 判断是否处于字符串内，已在字符串内将不处理其它字符，只判断结束处理
        if (inCodeString) {
            // 判断 " 前是否存在转义字符，若不存在，则退出字符串
            if ('"' == c
                    && '\\' != doc->characterAt(position - 1)) {
                inCodeString = false;
            }
        } else {
            if (c == begin) {
                braceDepth++;
                qDebug() << "Found opening brace at position" << position << ", depth now" << braceDepth;
            } else if (c == end) {
                braceDepth--;
                qDebug() << "Found closing brace at position" << position << ", depth now" << braceDepth;

                if (0 == braceDepth) {
                    bracketEndCursor = QTextCursor(doc);
                    bracketEndCursor.setPosition(position);
                    bracketEndCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
                    endBlock = bracketEndCursor.block();
                    qDebug() << "Matching closing brace found at line" << endBlock.blockNumber() << "position" << position;
                    break;
                }
            } else if ('"' == c) {
                inCodeString = true;
            }
        }

        position++;
    }

    bool result = (0 == braceDepth);
    qDebug() << "Fold block search result:" << result << "for line" << line << "ending at line" << endBlock.blockNumber();
    return result;
}

/**
   @brief Call this function after undo/redo , refresh column edit status.
   @return Ture if need update seletion status.
 */
bool TextEdit::refreshUndoRedoColumnStatus()
{
    if (const QUndoCommand *cmd = m_pUndoStack->command(m_pUndoStack->index())) {
        const int id = cmd->id();
        const bool columnEdit = (id != Utils::IdDefault) && (Utils::IdColumnEdit & id);
        if (columnEdit != m_bIsAltMod) {
            // update selection
            m_bIsAltMod = columnEdit;
            if (!m_bIsAltMod) {
                m_altModSelections.clear();
            }

            return true;
        }

        // in column editing, update every time.
        if (columnEdit) {
            return true;
        }
    }

    return false;
}

/**
 * @brief 文档内容变更时触发
 *      当前用于记录中键更新前后的动作并压入撤销栈，需要注意鼠标中键不会移除字符仅会插入选中的字符。
 * @param from          更新的光标位置
 * @param charsRemoved  移除的字符数
 * @param charsAdded    插入的字符数
 */
void TextEdit::onTextContentChanged(int from, int charsRemoved, int charsAdded)
{
    Q_UNUSED(charsRemoved)

    // 判断是否正在执行中键黏贴动作
    if (m_MidButtonPatse) {
        QUndoCommand *undo = new QUndoCommand;
        m_pUndoStack->push(undo);

        // 记录鼠标中键变更，滞后插入撤销栈，在push()时不执行redo()
        QTextCursor cursor = textCursor();
        cursor.setPosition(from);
        cursor.setPosition(from + charsAdded, QTextCursor::KeepAnchor);
        // 取得已插入的文本信息
        QString insertText = cursor.selectedText();
        cursor.setPosition(from);
        (void)new MidButtonInsertTextUndoCommand(cursor, insertText, this, undo);

        m_MidButtonPatse = false;
    }
}

void TextEdit::onAudioPortEnabledChanged(quint32 cardId, const QString &portName, bool enabled)
{
    Q_UNUSED(cardId)
    Q_UNUSED(portName)

    // 只处理设备被禁用的情况
    if (!enabled) {
        // 检查是否还有可用的输出设备和输入设备
        bool hasOutputDevice = IflytekAiAssistant::instance()->hasAudioOutputDevice();
        bool hasInputDevice = IflytekAiAssistant::instance()->hasAudioInputDevice();

        // 如果所有输出设备都被禁用，显示输出设备提示
        if (!hasOutputDevice) {
#ifdef DTKWIDGET_CLASS_DSizeMode
            Utils::sendFloatMessageFixedFont(this, QIcon(":/images/warning.svg"), tr("No audio output device was detected. Please ensure your speakers or headphones are properly connected and try again."));
#else
            DMessageManager::instance()->sendMessage(this, QIcon(":/images/warning.svg"), tr("No audio output device was detected. Please ensure your speakers or headphones are properly connected and try again."));
#endif
        }

        // 如果所有输入设备都被禁用，显示输入设备提示
        if (!hasInputDevice) {
#ifdef DTKWIDGET_CLASS_DSizeMode
            Utils::sendFloatMessageFixedFont(this, QIcon(":/images/warning.svg"), tr("No audio input device was detected. Please ensure your speakers or headphones are properly connected and try again."));
#else
            DMessageManager::instance()->sendMessage(this, QIcon(":/images/warning.svg"), tr("No audio input device was detected. Please ensure your speakers or headphones are properly connected and try again."));
#endif
        }
    }
}
