/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Rekols    <rekols@foxmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dtextedit.h"
#include "utils.h"
#include "window.h"
#include "editwrapper.h"
#include "widgets/bottombar.h"
#include "leftareaoftextedit.h"

#include "showflodcodewidget.h"


#include <KF5/KSyntaxHighlighting/definition.h>
#include <KF5/KSyntaxHighlighting/syntaxhighlighter.h>
#include <KF5/KSyntaxHighlighting/theme.h>

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
#include <DSysInfo>

#include <private/qguiapplication_p.h>
#include <qpa/qplatformtheme.h>

#define STYLE_COLOR_1 "#FFA503"
#define STYLE_COLOR_2 "#FF1C49"
#define STYLE_COLOR_3 "#9023FC"
#define STYLE_COLOR_4 "#3468FF"
#define FOLD_HIGHLIGHT_COLOR "#0081FF"

static inline bool isModifier(QKeyEvent *e)
{
    if (!e) {
        return false;
    }

    switch (e->key()) {
    case Qt::Key_Shift:
    case Qt::Key_Control:
    case Qt::Key_Meta:
    case Qt::Key_Alt:
        return true;
    default:
        return false;
    }
}

TextEdit::TextEdit(QWidget *parent)
    : DTextEdit(parent),
      m_wrapper(nullptr),
     m_highlighter(new KSyntaxHighlighting::SyntaxHighlighter(document()))
{
    m_nLines = 0;
    m_nBookMarkHoverLine = -1;
    m_bIsFileOpen = false;
    m_qstrCommitString = "";
    m_bIsShortCut = false;
    m_bIsInputMethod = false;
    //lineNumberArea = new LineNumberArea(m_pLeftAreaWidget);
    m_pLeftAreaWidget = new leftareaoftextedit(this);
    lineNumberArea = m_pLeftAreaWidget->m_linenumberarea;

#if QT_VERSION < QT_VERSION_CHECK(5,9,0)
    m_touchTapDistance = 15;
#else
    m_touchTapDistance = QGuiApplicationPrivate::platformTheme()->themeHint(QPlatformTheme::TouchDoubleTapDistance).toInt();
#endif
    m_fontLineNumberArea.setFamily("SourceHanSansSC-Normal");

    m_pLeftAreaWidget->m_flodArea->setAttribute(Qt::WA_Hover, true); //开启悬停事件
    m_pLeftAreaWidget->m_bookMarkArea->setAttribute(Qt::WA_Hover, true);
    viewport()->installEventFilter(this);
    m_pLeftAreaWidget->m_bookMarkArea->installEventFilter(this);
    m_pLeftAreaWidget->m_flodArea->installEventFilter(this);
    viewport()->setCursor(Qt::IBeamCursor);

    // Don't draw frame around editor widget.
    setFrameShape(QFrame::NoFrame);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAcceptRichText(false);

    // Init widgets.
    connect(this->verticalScrollBar(), &QScrollBar::valueChanged, this, &TextEdit::updateLineNumber);
    connect(this, &QTextEdit::textChanged, this, [this]() {

//        if (!m_bIsFileOpen) {
//            checkBookmarkLineMove();
//        }
        updateLineNumber();
        updateWordCount();
    });
    connect(this, &QTextEdit::cursorPositionChanged, this, &TextEdit::cursorPositionChanged);
    connect(this, &QTextEdit::selectionChanged, this, &TextEdit::onSelectionArea);
    connect(document(), &QTextDocument::modificationChanged, this, &TextEdit::setModified);
    connect(document(), &QTextDocument::contentsChange, this, &TextEdit::updateMark);
    connect(document(), &QTextDocument::contentsChange, this, &TextEdit::checkBookmarkLineMove);

    m_foldCodeShow = new ShowFlodCodeWidget(this);
    m_foldCodeShow->setVisible(false);

    // Init menu.
    m_rightMenu = new DMenu();
    //m_rightMenu->setStyle(QStyleFactory::create("dlight"));
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
    m_toggleCommentAction = new QAction(tr("Toggle comment"), this);
    m_voiceReadingAction = new QAction(tr("Text to Speech"),this);
    m_stopReadingAction = new QAction(tr("Stop reading"),this);
    m_dictationAction = new QAction(tr("Speech to Text"),this);
    m_translateAction = new QAction(tr("Translate"),this);
    m_addBookMarkAction = new QAction(tr("Add bookmark"),this);
    m_cancelBookMarkAction = new QAction(tr("Cancel bookmark"),this);
    m_preBookMarkAction = new QAction(tr("Previous bookmark"),this);
    m_nextBookMarkAction = new QAction(tr("Next bookmark"),this);
    m_clearBookMarkAction = new QAction(tr("Clear bookmark"),this);
    m_flodAllLevel = new QAction(tr("Flod all Level"), this);
    m_flodCurrentLevel = new QAction(tr("Flod current Level"), this);
    m_unflodAllLevel = new QAction(tr("Unflod all Level"), this);
    m_unflodCurrentLevel = new QAction(tr("Unflod Current Level"), this);

//    setAddRigetMenu();
    //yanyuhan
    //颜色标记、折叠/展开、书签、列编辑、设置注释、取消注释;
    //点击颜色标记菜单，显示二级菜单，包括：标记、清除上次标记、清除标记、标记所有;
    m_colorMarkMenu = new DMenu(tr("Color Mark"),this);
    m_markAllLine = new DMenu(tr("Mark All Line"), this);
    m_markCurrentLine = new DMenu(tr("Mark"), this);
    m_cancleMarkAllLine = new QAction(tr("Cancle Mark All Line"), this);
    m_cancleMarkCurrentLine = new QAction(tr("Cancle Mark Current Line"), this);
    m_cancleLastMark = new QAction(tr("Cancle last Mark"), this);

    m_actionStyleOne = new QAction(tr("Style One"), this);
    m_actionStyleTwo = new QAction(tr("Style Two"), this);
    m_actionStyleThree = new QAction(tr("Style Three"), this);
    m_actionStyleFour = new QAction(tr("Style Four"), this);

    m_actionAllStyleOne = new QAction(tr("Style One"), this);
    m_actionAllStyleTwo = new QAction(tr("Style Two"), this);
    m_actionAllStyleThree = new QAction(tr("Style Three"), this);
    m_actionAllStyleFour = new QAction(tr("Style Four"), this);

    m_markAllLine->addAction(m_actionAllStyleOne);
    m_markAllLine->addAction(m_actionAllStyleTwo);
    m_markAllLine->addAction(m_actionAllStyleThree);
    m_markAllLine->addAction(m_actionAllStyleFour);

    m_markCurrentLine->addAction(m_actionStyleOne);
    m_markCurrentLine->addAction(m_actionStyleTwo);
    m_markCurrentLine->addAction(m_actionStyleThree);
    m_markCurrentLine->addAction(m_actionStyleFour);

    //点击折叠/展开菜单，显示二级菜单;包括：折叠所有层次、展开所有层次、折叠当前层次、展开当前层次。
    m_collapseExpandMenu = new DMenu(tr("Collapse/Expand"),this);
    QAction *collapseAll = new QAction(tr("Collapse all"));
    QAction *expandAll = new QAction(tr("Expand all"));
    QAction *collapseThis = new QAction(tr("Collapse this"));
    QAction *expandThis = new QAction(tr("Expand this"));
    m_collapseExpandMenu->addAction(collapseAll);
    m_collapseExpandMenu->addAction(expandAll);
    m_collapseExpandMenu->addAction(collapseThis);
    m_collapseExpandMenu->addAction(expandThis);

    m_columnEditACtion = new QAction(tr("Column edit"),this);
    m_columnEditACtion->setCheckable(true);
    m_addComment = new QAction(tr("Add comment"),this);
    m_cancelComment = new QAction(tr("Cancel comment"),this);

    connect(m_rightMenu, &DMenu::aboutToHide, this, &TextEdit::removeHighlightWordUnderCursor);
    connect(m_undoAction, &QAction::triggered, this, &TextEdit::undo);
    connect(m_redoAction, &QAction::triggered, this, &TextEdit::redo);
    connect(m_cutAction, &QAction::triggered, this, &TextEdit::clickCutAction);
    connect(m_copyAction, &QAction::triggered, this, &TextEdit::clickCopyAction);
    connect(m_pasteAction, &QAction::triggered, this, &TextEdit::clickPasteAction);
    connect(m_deleteAction, &QAction::triggered, this, &TextEdit::clickDeleteAction);
    connect(m_selectAllAction, &QAction::triggered, this, &TextEdit::selectAll);
    connect(m_findAction, &QAction::triggered, this, &TextEdit::clickFindAction);
    connect(m_replaceAction, &QAction::triggered, this, &TextEdit::clickReplaceAction);
    connect(m_jumpLineAction, &QAction::triggered, this, &TextEdit::clickJumpLineAction);
    connect(m_fullscreenAction, &QAction::triggered, this, &TextEdit::clickFullscreenAction);
    connect(m_exitFullscreenAction, &QAction::triggered, this, &TextEdit::clickFullscreenAction);
    connect(m_enableReadOnlyModeAction, &QAction::triggered, this, &TextEdit::toggleReadOnlyMode);
    connect(m_disableReadOnlyModeAction, &QAction::triggered, this, &TextEdit::toggleReadOnlyMode);
    connect(m_openInFileManagerAction, &QAction::triggered, this, &TextEdit::clickOpenInFileManagerAction);
 //   connect(m_toggleCommentAction, &QAction::triggered, this, &TextEdit::toggleComment);
    connect(m_addComment,&QAction::triggered,this,[=] {
        toggleComment(true);
    });
    connect(m_cancelComment,&QAction::triggered,this,[=] {
        toggleComment(false);
    });
    connect(m_voiceReadingAction, &QAction::triggered, this, &TextEdit::slot_voiceReading);
    connect(m_stopReadingAction, &QAction::triggered, this, &TextEdit::slot_stopReading);
    connect(m_dictationAction, &QAction::triggered, this, &TextEdit::slot_dictation);
    connect(m_translateAction, &QAction::triggered, this, &TextEdit::slot_translate);
    connect(m_addBookMarkAction, &QAction::triggered, this, &TextEdit::onAddBookMark);
    connect(m_cancelBookMarkAction, &QAction::triggered, this, &TextEdit::onCancelBookMark);
    connect(m_preBookMarkAction, &QAction::triggered, this, &TextEdit::onMoveToPreviousBookMark);
    connect(m_nextBookMarkAction, &QAction::triggered, this, &TextEdit::onMoveToNextBookMark);
    connect(m_clearBookMarkAction, &QAction::triggered, this, &TextEdit::onClearBookMark);
    connect(m_flodAllLevel, &QAction::triggered, this, [ = ] {
        flodOrUnflodAllLevel(true);
    });
    connect(m_unflodAllLevel, &QAction::triggered, this, [ = ] {
        flodOrUnflodAllLevel(false);
    });
    connect(m_flodCurrentLevel, &QAction::triggered, this, [ = ] {
        flodOrUnflodCurrentLevel(true);
    });
    connect(m_unflodCurrentLevel, &QAction::triggered, this, [ = ] {
        flodOrUnflodCurrentLevel(false);
    });
    connect(m_markCurrentLine, &DMenu::triggered, this, [ = ](QAction * pAction) {
        QString strColor;
        if (pAction == m_actionStyleOne) {
            strColor = STYLE_COLOR_1;
        } else if (pAction == m_actionStyleTwo) {
            strColor = STYLE_COLOR_2;
        } else if (pAction == m_actionStyleThree) {
            strColor = STYLE_COLOR_3;
        } else if (pAction == m_actionStyleFour) {
            strColor = STYLE_COLOR_4;
        }
        isMarkCurrentLine(true, strColor);
        renderAllSelections();
    });
    connect(m_cancleMarkCurrentLine, &QAction::triggered, this, [ = ] {
        isMarkCurrentLine(false);
        renderAllSelections();
    });

    connect(m_markAllLine, &DMenu::triggered, this, [ = ](QAction * pAction) {
        QString strColor;
        if (pAction == m_actionAllStyleOne) {
            strColor = STYLE_COLOR_1;
        } else if (pAction == m_actionAllStyleTwo) {
            strColor = STYLE_COLOR_2;
        } else if (pAction == m_actionAllStyleThree) {
            strColor = STYLE_COLOR_3;
        } else if (pAction == m_actionAllStyleFour) {
            strColor = STYLE_COLOR_4;
        }
        isMarkAllLine(true, strColor);
        renderAllSelections();
    });
    connect(m_cancleMarkAllLine, &QAction::triggered, this, [ = ] {
        isMarkAllLine(false);
        renderAllSelections();
    });
    connect(m_cancleLastMark, &QAction::triggered, this, [ = ] {
        cancelLastMark();
        renderAllSelections();
    });


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

    connect(this, &TextEdit::undoAvailable, this, [=] (bool undoIsAvailable) {
        m_canUndo = undoIsAvailable;
    });
    connect(this, &TextEdit::redoAvailable, this, [=] (bool redoIsAvailable) {
        m_canRedo = redoIsAvailable;
    });

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
    connect(this, &TextEdit::selectionChanged, this, [=] {
        if (textCursor().hasSelection()) {
            hideCursorBlink();
        } else {
            showCursorBlink();
        }
    });

    // syntax selection

    m_hlGroupMenu = new DMenu;
    QAction *noHlAction = m_hlGroupMenu->addAction(tr("None"));

    m_hlActionGroup = new QActionGroup(m_hlGroupMenu);
    m_hlActionGroup->setExclusive(true);

    noHlAction->setCheckable(true);
    m_hlActionGroup->addAction(noHlAction);
    noHlAction->setChecked(!m_highlighter->definition().isValid());

    DMenu *hlSubMenu = nullptr;
    QString currentGroup;

    for (const auto &def : m_repository.definitions()) {
        if (def.isHidden()) {
            continue;
        }

        if (currentGroup != def.section()) {
            currentGroup = def.section();
            hlSubMenu = m_hlGroupMenu->addMenu(def.translatedSection());
        }

        if (!hlSubMenu) {
            continue;
        }

        auto action = hlSubMenu->addAction(def.translatedName());
        action->setCheckable(true);
        action->setData(def.name());
        m_hlActionGroup->addAction(action);

        if (def.name() == m_highlighter->definition().name()) {
            action->setChecked(true);
        }
    }

    connect(m_hlActionGroup, &QActionGroup::triggered, this, [this] (QAction *action) {
        const auto defName = action->data().toString();
        const auto def = m_repository.definitionForName(defName);
        if (m_highlighter == nullptr) {
            m_highlighter = new KSyntaxHighlighting::SyntaxHighlighter(document());
        }
        if (!def.isValid()) {
            emit hightlightChanged(action->text());
            delete m_highlighter;
            m_highlighter = nullptr;
        } else {
            m_highlighter->setDefinition(def);
            emit hightlightChanged(action->text());
        }

    });
}

TextEdit::~TextEdit()
{
    writeHistoryRecord();
    delete m_highlighter;
}

void TextEdit::setWrapper(EditWrapper *w)
{
    m_wrapper = w;
}

int TextEdit::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, this->document()->blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    return 13 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits + 40;
}

int TextEdit::getCurrentLine()
{
    return textCursor().blockNumber() + 1;
}

int TextEdit::getCurrentColumn()
{
    return textCursor().columnNumber();
}

int TextEdit::getPosition()
{
    return textCursor().position();
}

int TextEdit::getScrollOffset()
{
    QScrollBar *scrollbar = verticalScrollBar();

    return scrollbar->value();
}

void TextEdit::forwardChar()
{
    if (m_cursorMark) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        moveCursorNoBlink(QTextCursor::NextCharacter);
    }
}

void TextEdit::backwardChar()
{
    if (m_cursorMark) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        moveCursorNoBlink(QTextCursor::PreviousCharacter);
    }
}

void TextEdit::forwardWord()
{
    QTextCursor cursor = textCursor();

    if (m_cursorMark) {
        // cursor.setPosition(getNextWordPosition(cursor, QTextCursor::KeepAnchor), QTextCursor::KeepAnchor);
        cursor.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor);
    } else {
        // cursor.setPosition(getNextWordPosition(cursor, QTextCursor::MoveAnchor), QTextCursor::MoveAnchor);
        cursor.movePosition(QTextCursor::NextWord, QTextCursor::MoveAnchor);
    }

    setTextCursor(cursor);
}

void TextEdit::backwardWord()
{
    QTextCursor cursor = textCursor();

    if (m_cursorMark) {
        // cursor.setPosition(getPrevWordPosition(cursor, QTextCursor::KeepAnchor), QTextCursor::KeepAnchor);
        cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
    } else {
        // cursor.setPosition(getPrevWordPosition(cursor, QTextCursor::MoveAnchor), QTextCursor::MoveAnchor);
        cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::MoveAnchor);
    }

    setTextCursor(cursor);
}

void TextEdit::forwardPair()
{
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
    if (find(QRegExp("[\"'>)}]"))) {
        int findPos = textCursor().position();

        QTextCursor cursor = textCursor();
        auto moveMode = m_cursorMark ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;

        if (actionStartPos == selectionStartPos) {
            cursor.setPosition(selectionEndPos, QTextCursor::MoveAnchor);
            cursor.setPosition(findPos, moveMode);
        } else {
            cursor.setPosition(selectionStartPos, QTextCursor::MoveAnchor);
            cursor.setPosition(findPos, moveMode);
        }

        setTextCursor(cursor);
    }
}

void TextEdit::backwardPair()
{
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

    if (find(QRegExp("[\"'<({]"), options)) {
        QTextCursor cursor = textCursor();
        auto moveMode = m_cursorMark ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;

        cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor);

        int findPos = cursor.position();

        if (actionStartPos == selectionStartPos) {
            cursor.setPosition(selectionEndPos, QTextCursor::MoveAnchor);
            cursor.setPosition(findPos, moveMode);
            setTextCursor(cursor);
        } else {
            cursor.setPosition(selectionStartPos, QTextCursor::MoveAnchor);
            cursor.setPosition(findPos, moveMode);
            setTextCursor(cursor);
        }
    }
}

int TextEdit::blockCount() const
{
    return document()->blockCount();
}

int TextEdit::characterCount() const
{
    return document()->characterCount();
}

QTextBlock TextEdit::firstVisibleBlock()
{
    return document()->findBlockByLineNumber(getFirstVisibleBlockId());
}

void TextEdit::moveToStart()
{
    verticalScrollBar()->setValue(0);
    if (m_cursorMark) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Start, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        moveCursorNoBlink(QTextCursor::Start);
    }
}

void TextEdit::moveToEnd()
{
    if (m_cursorMark) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        moveCursorNoBlink(QTextCursor::End);
    }
}

void TextEdit::moveToStartOfLine()
{
    if (m_cursorMark) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        moveCursorNoBlink(QTextCursor::StartOfBlock);
    }
}

void TextEdit::moveToEndOfLine()
{
    if (m_cursorMark) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        moveCursorNoBlink(QTextCursor::EndOfBlock);
    }
}

void TextEdit::moveToLineIndentation()
{
    // Init cursor and move type.
    QTextCursor cursor = textCursor();
    auto moveMode = m_cursorMark ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;

    // Get line start position.
    cursor.movePosition(QTextCursor::StartOfBlock, moveMode);
    int startColumn = cursor.columnNumber();

    // Get line end position.
    cursor.movePosition(QTextCursor::EndOfBlock, moveMode);
    int endColumn = cursor.columnNumber();

    // Move to line start first.
    cursor.movePosition(QTextCursor::StartOfBlock, moveMode);

    // Move to first non-blank char of line.
    int column = startColumn;
    while (column < endColumn) {
        QChar currentChar = toPlainText().at(std::max(cursor.position() - 1, 0));

        if (!currentChar.isSpace()) {
            cursor.movePosition(QTextCursor::PreviousCharacter, moveMode);
            break;
        } else {
            cursor.movePosition(QTextCursor::NextCharacter, moveMode);
        }

        column++;
    }

    setTextCursor(cursor);
}

void TextEdit::nextLine()
{
    if (toPlainText().isEmpty())
        return;

    if (m_cursorMark) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        moveCursorNoBlink(QTextCursor::Down);
    }
}

void TextEdit::prevLine()
{
    if (toPlainText().isEmpty())
        return;

    if (m_cursorMark) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Up, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        moveCursorNoBlink(QTextCursor::Up);
    }
}

void TextEdit::moveCursorNoBlink(QTextCursor::MoveOperation operation, QTextCursor::MoveMode mode)
{
    // Function moveCursorNoBlink will blink cursor when move cursor.
    // But function movePosition won't, so we use movePosition to avoid that cursor link when moving cursor.
    QTextCursor cursor = textCursor();
    cursor.movePosition(operation, mode);
    setTextCursor(cursor);
}

void TextEdit::jumpToLine(int line, bool keepLineAtCenter)
{
    QTextCursor cursor(document()->findBlockByNumber(line - 1)); // line - 1 because line number starts from 0
    verticalScrollBar()->setValue(fontMetrics().height() * line - height());
    // Update cursor.
    setTextCursor(cursor);

    if (keepLineAtCenter) {
        keepCurrentLineAtCenter();
    }
}

void TextEdit::newline()
{
    // Stop mark if mark is set.
    tryUnsetMark();

    QTextCursor cursor = textCursor();
    cursor.insertText("\n");
    setTextCursor(cursor);
}

void TextEdit::openNewlineAbove()
{
    // Stop mark if mark is set.
    tryUnsetMark();

    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
    cursor.insertText("\n");
    cursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor);

    setTextCursor(cursor);
}

void TextEdit::openNewlineBelow()
{
    // Stop mark if mark is set.
    tryUnsetMark();

    moveCursorNoBlink(QTextCursor::EndOfBlock);
    textCursor().insertText("\n");
}

void TextEdit::moveLineDownUp(bool up)
{
    QTextCursor cursor = textCursor();
    QTextCursor move = cursor;
    bool hasSelection = cursor.hasSelection();

    // this opens folded items instead of destroying them
    move.setVisualNavigation(false);
    move.beginEditBlock();

    if (hasSelection) {
        move.setPosition(cursor.selectionStart());
        move.movePosition(QTextCursor::StartOfBlock);
        move.setPosition(cursor.selectionEnd(), QTextCursor::KeepAnchor);
        move.movePosition(move.atBlockStart() ? QTextCursor::Left: QTextCursor::EndOfBlock,
                          QTextCursor::KeepAnchor);
    } else {
        move.movePosition(QTextCursor::StartOfBlock);
        move.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    }
    const QString text = move.selectedText();

    move.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
    move.removeSelectedText();

    if (up) {
        move.movePosition(QTextCursor::PreviousBlock);
        move.insertBlock();
        move.movePosition(QTextCursor::Left);
    } else {
        move.movePosition(QTextCursor::EndOfBlock);
        if (move.atBlockStart()) { // empty block
            move.movePosition(QTextCursor::NextBlock);
            move.insertBlock();
            move.movePosition(QTextCursor::Left);
        } else {
            move.insertBlock();
        }
    }

    int start = move.position();
    move.clearSelection();
    move.insertText(text);
    int end = move.position();

    if (hasSelection) {
        move.setPosition(end);
        move.setPosition(start, QTextCursor::KeepAnchor);
    } else {
        move.setPosition(start);
    }

    move.endEditBlock();
    setTextCursor(move);
}

void TextEdit::scrollLineUp()
{
    QScrollBar *scrollbar = verticalScrollBar();

    scrollbar->setValue(scrollbar->value() - 1);

    if (cursorRect().y() > rect().height() - fontMetrics().height()) {
        auto moveMode = m_cursorMark ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;

        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Up, moveMode);
        setTextCursor(cursor);
    }
}

void TextEdit::scrollLineDown()
{
    QScrollBar *scrollbar = verticalScrollBar();

    scrollbar->setValue(scrollbar->value() + 1);

    if (cursorRect().y() < 0) {
        auto moveMode = m_cursorMark ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;

        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Down, moveMode);
        setTextCursor(cursor);
    }
}

void TextEdit::scrollUp()
{
    QScrollBar *scrollbar = verticalScrollBar();
    scrollbar->setValue(scrollbar->value()-scrollbar->pageStep());
    auto moveMode = m_cursorMark ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;
    QTextCursor lineCursor(document()->findBlockByLineNumber(this->getFirstVisibleBlockId()));
    QTextCursor cursor = textCursor();
    cursor.setPosition(lineCursor.position(), moveMode);
    setTextCursor(cursor);
}

void TextEdit::scrollDown()
{
    QScrollBar *scrollbar = verticalScrollBar();
    scrollbar->setValue(scrollbar->value()+scrollbar->pageStep());
    int lines = this->height() / fontMetrics().height();

    auto moveMode = m_cursorMark ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;
    int tem = document()->blockCount();

    if(this->getFirstVisibleBlockId() + lines <tem ){
    QTextCursor lineCursor(document()->findBlockByLineNumber(this->getFirstVisibleBlockId()-1));
    QTextCursor cursor = textCursor();
    cursor.setPosition(lineCursor.position(), moveMode);
    setTextCursor(cursor);
    }
    else {              //如果文本结尾部分不足一页
        if((getCurrentLine()+lines)>tem)
        {
            QTextCursor lineCursor(document()->findBlockByLineNumber(tem-1));
            QTextCursor cursor = textCursor();
            cursor.setPosition(lineCursor.position(), moveMode);
            setTextCursor(cursor);
        }
        else {
            QTextCursor lineCursor(document()->findBlockByLineNumber(getCurrentLine()+lines-1));
            QTextCursor cursor = textCursor();
            cursor.setPosition(lineCursor.position(), moveMode);
            setTextCursor(cursor);
        }
    }
}

void TextEdit::duplicateLine()
{
    if (textCursor().hasSelection()) {
        bool cursorAtSelectionStart = (textCursor().position() == textCursor().selectionStart());

        // Rember current line's column number.
        int column = textCursor().columnNumber();

        int startPos = textCursor().selectionStart();
        int endPos = textCursor().selectionEnd();

        // Expand selection to lines bound.
        QTextCursor startCursor = textCursor();
        startCursor.setPosition(startPos, QTextCursor::MoveAnchor);
        startCursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);

        QTextCursor endCursor = textCursor();
        endCursor.setPosition(endPos, QTextCursor::MoveAnchor);
        endCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::MoveAnchor);

        QTextCursor cursor = textCursor();
        cursor.setPosition(startCursor.position(), QTextCursor::MoveAnchor);
        cursor.setPosition(endCursor.position(), QTextCursor::KeepAnchor);

        setTextCursor(cursor);

        // Get selection lines content.
        QString selectionLines = cursor.selectedText();

        // Duplicate copy lines.
        if (cursorAtSelectionStart) {
            // Insert duplicate lines.
            cursor.setPosition(startCursor.position(), QTextCursor::MoveAnchor);
            cursor.insertText("\n");
            cursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor);
            cursor.insertText(selectionLines);

            // Restore cursor's column.
            cursor.setPosition(startCursor.position(), QTextCursor::MoveAnchor);
            cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, column);
        } else {
            // Insert duplicate lines.
            cursor.setPosition(endCursor.position(), QTextCursor::MoveAnchor);
            cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor);
            cursor.insertText(selectionLines);
            cursor.insertText("\n");

            // Restore cursor's column.
            cursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor);
            cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
            cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, column);
        }

        // Update cursor.
        setTextCursor(cursor);

        // Clean mark flag anyway.
        unsetMark();
    } else {
        // Rember current line's column number.
        int column = textCursor().columnNumber();

        // Get current line's content.
        QTextCursor cursor(textCursor().block());
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        QString text = cursor.selectedText();

        // Copy current line.
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::MoveAnchor);
        cursor.insertText("\n");
        cursor.insertText(text);

        // Restore cursor's column.
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, column);

        // Update cursor.
        setTextCursor(cursor);
    }
}

void TextEdit::copyLines()
{
    // Record current cursor and build copy cursor.
    QTextCursor currentCursor = textCursor();
    QTextCursor copyCursor = textCursor();

    if (textCursor().hasSelection()) {
        // Sort selection bound cursors.
        int startPos = textCursor().anchor();
        int endPos = textCursor().position();

        if (startPos > endPos) {
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
        // Selection current line.
        copyCursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        copyCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

        popupNotify(tr("Current line copied"));
    }

    // Copy lines to system clipboard.
    setTextCursor(copyCursor);
    copySelectedText();

    // Reset cursor before copy lines.
    copyCursor.setPosition(currentCursor.position(), QTextCursor::MoveAnchor);
    setTextCursor(copyCursor);
}

void TextEdit::cutlines()
{
    // Record current cursor and build copy cursor.
    QTextCursor currentCursor = textCursor();
    QTextCursor copyCursor = textCursor();

    if (textCursor().hasSelection()) {
        // Sort selection bound cursors.
        int startPos = textCursor().anchor();
        int endPos = textCursor().position();

        if (startPos > endPos) {
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

        popupNotify(tr("Selected line(s) clipped"));
    } else {
        // Selection current line.
        copyCursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        copyCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

        popupNotify(tr("Current line clipped"));
    }

    // Copy lines to system clipboard.
    setTextCursor(copyCursor);
    cutSelectedText();

    // Reset cursor before copy lines.
    copyCursor.setPosition(currentCursor.position(), QTextCursor::MoveAnchor);
    setTextCursor(copyCursor);
}

void TextEdit::joinLines()
{
    if (textCursor().hasSelection()) {
        // Get selection bound.
        int startPos = textCursor().anchor();
        int endPos = textCursor().position();

        if (startPos > endPos) {
            std::swap(startPos, endPos);
        }

        // Expand selection to multi-lines bound.
        QTextCursor startCursor = textCursor();
        startCursor.setPosition(startPos, QTextCursor::MoveAnchor);
        startCursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);

        QTextCursor endCursor = textCursor();
        endCursor.setPosition(endPos, QTextCursor::MoveAnchor);
        endCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::MoveAnchor);

        // Select multi-lines.
        QTextCursor cursor = textCursor();
        cursor.setPosition(startCursor.position(), QTextCursor::MoveAnchor);
        cursor.setPosition(endCursor.position(), QTextCursor::KeepAnchor);

        // Remove selected lines.
        QString selectedLines = cursor.selectedText();
        cursor.removeSelectedText();

        // Insert line with join actoin.
        // Because function `selectedText' will use Unicode char U+2029 instead \n,
        // so we need replace Unicode char U+2029, not replace char '\n'.
        cursor.insertText(selectedLines.replace(QChar(0x2029), " "));

        setTextCursor(cursor);
    } else {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::MoveAnchor);
        cursor.insertText(" ");
        cursor.deleteChar();
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::MoveAnchor);

        setTextCursor(cursor);
    }

    tryUnsetMark();
}

void TextEdit::killLine()
{
    if (tryUnsetMark()) {
        return;
    }

    // Remove selection content if has selection.
    if (textCursor().hasSelection()) {
        textCursor().removeSelectedText();
    } else {
        // Get current line content.
        QTextCursor selectionCursor = textCursor();
        selectionCursor.movePosition(QTextCursor::StartOfBlock);
        selectionCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        QString text = selectionCursor.selectedText();

        bool isEmptyLine = text.size() == 0;
        bool isBlankLine = text.trimmed().size() == 0;

        QTextCursor cursor = textCursor();
        // Join next line if current line is empty or cursor at end of line.
        if (isEmptyLine || textCursor().atBlockEnd()) {
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::MoveAnchor);
            cursor.deleteChar();
        }
        // Kill whole line if current line is blank line.
        else if (isBlankLine && textCursor().atBlockStart()) {
            cursor.movePosition(QTextCursor::StartOfBlock);
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
            cursor.deleteChar();
        }
        // Otherwise kill rest content of line.
        else {
            cursor.movePosition(QTextCursor::NoMove, QTextCursor::MoveAnchor);
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
        }

        // Update cursor.
        setTextCursor(cursor);
    }
}

void TextEdit::killCurrentLine()
{
    if (tryUnsetMark()) {
        return;
    }

    QTextCursor cursor = textCursor();

    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

    QString text = cursor.selectedText();
    bool isBlankLine = text.trimmed().size() == 0;

    cursor.removeSelectedText();
    if (isBlankLine) {
        cursor.deleteChar();
    }

    setTextCursor(cursor);
}

void TextEdit::killBackwardWord()
{
    tryUnsetMark();

    if (textCursor().hasSelection()) {
        //textCursor().removeSelectedText();
    } else {
        //  QTextCursor cursor = textCursor();
        //  cursor.movePosition(QTextCursor::NoMove, QTextCursor::MoveAnchor);
        //  cursor.setPosition(getPrevWordPosition(cursor, QTextCursor::KeepAnchor), QTextCursor::KeepAnchor);
        //  cursor.removeSelectedText();
        //  setTextCursor(cursor);

        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        setTextCursor(cursor);
    }
}

void TextEdit::killForwardWord()
{
    tryUnsetMark();

    if (textCursor().hasSelection()) {
        //textCursor().removeSelectedText();
    } else {
        //  QTextCursor cursor = textCursor();
        //  cursor.movePosition(QTextCursor::NoMove, QTextCursor::MoveAnchor);
        //  cursor.setPosition(getNextWordPosition(cursor, QTextCursor::KeepAnchor), QTextCursor::KeepAnchor);
        //  cursor.removeSelectedText();
        //  setTextCursor(cursor);

        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        setTextCursor(cursor);
    }
}

void TextEdit::escape()
{
    emit pressEsc();

    tryUnsetMark();
}

void TextEdit::indentText()
{
    // Stop mark if mark is set.
    tryUnsetMark();
    hideCursorBlink();

    QTextCursor cursor = textCursor();

    if (cursor.hasSelection()) {
        QTextBlock block = document()->findBlock(cursor.selectionStart());
        QTextBlock end = document()->findBlock(cursor.selectionEnd()).next();

        cursor.beginEditBlock();

        while (block != end) {
            QString speaces(m_tabSpaceNumber, ' ');
            cursor.setPosition(block.position());
            cursor.insertText(speaces);
            block = block.next();
        }

        cursor.endEditBlock();
    } else {
        cursor.beginEditBlock();

        int indent = m_tabSpaceNumber - (cursor.positionInBlock() % m_tabSpaceNumber);
        QString spaces(indent, ' ');
        cursor.insertText(spaces);

        cursor.endEditBlock();
    }

    showCursorBlink();
}

void TextEdit::unindentText()
{
    // Stop mark if mark is set.
    tryUnsetMark();
    hideCursorBlink();

    QTextCursor cursor = textCursor();
    QTextBlock block;
    QTextBlock end;

    if (cursor.hasSelection()) {
        block = document()->findBlock(cursor.selectionStart());
        end = document()->findBlock(cursor.selectionEnd()).next();
    } else {
        block = cursor.block();
        end = block.next();
    }

    cursor.beginEditBlock();

    while (block != end) {
        cursor.setPosition(block.position());

        if (document()->characterAt(cursor.position()) == '\t') {
            cursor.deleteChar();
        } else {
            int pos = 0;

            while (document()->characterAt(cursor.position()) == ' ' &&
                   pos < m_tabSpaceNumber){
                pos++;
                cursor.deleteChar();
            }
        }

        block = block.next();
    }

    cursor.endEditBlock();
    showCursorBlink();
}

void TextEdit::setTabSpaceNumber(int number)
{
    m_tabSpaceNumber = number;
    updateFont();
    updateLineNumber();
}

void TextEdit::upcaseWord()
{
    tryUnsetMark();

    convertWordCase(UPPER);
}

void TextEdit::downcaseWord()
{
    tryUnsetMark();

    convertWordCase(LOWER);
}

void TextEdit::capitalizeWord()
{
    tryUnsetMark();

    convertWordCase(CAPITALIZE);
}

void TextEdit::transposeChar()
{
    tryUnsetMark();

    QTextCursor cursor = textCursor();
    cursor.clearSelection();

    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
    QString nextChar = cursor.selectedText();
    cursor.removeSelectedText();

    cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
    QString prevChar = cursor.selectedText();
    cursor.removeSelectedText();

    cursor.insertText(nextChar);
    cursor.insertText(prevChar);

    if (!nextChar.isEmpty()) {
        cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::MoveAnchor);
    }

    setTextCursor(cursor);
}

void TextEdit::handleCursorMarkChanged(bool mark, QTextCursor cursor)
{
    if (mark) {
        m_markStartLine = cursor.blockNumber() + 1;
    } else {
        m_markStartLine = -1;
    }

    lineNumberArea->update();
    m_pLeftAreaWidget->m_bookMarkArea->update();
}

void TextEdit::convertWordCase(ConvertCase convertCase)
{
    if (textCursor().hasSelection()) {
        QString text = textCursor().selectedText();

        if (convertCase == UPPER) {
            textCursor().insertText(text.toUpper());
        } else if (convertCase == LOWER) {
            textCursor().insertText(text.toLower());
        } else {
            textCursor().insertText(capitalizeText(text));
        }
    } else {
        QTextCursor cursor;

        // Move cursor to mouse position first. if have word under mouse pointer.
        if (m_haveWordUnderCursor) {
            setTextCursor(m_wordUnderPointerCursor);
        }

        cursor = textCursor();
        cursor.movePosition(QTextCursor::NoMove, QTextCursor::MoveAnchor);
        cursor.setPosition(getNextWordPosition(cursor, QTextCursor::KeepAnchor), QTextCursor::KeepAnchor);

        QString text = cursor.selectedText();
        if (convertCase == UPPER) {
            cursor.insertText(text.toUpper());
        } else if (convertCase == LOWER) {
            cursor.insertText(text.toLower());
        } else {
            cursor.insertText(capitalizeText(text));
        }

        setTextCursor(cursor);

        m_haveWordUnderCursor = false;
    }
}

QString TextEdit::capitalizeText(QString text)
{
    QString newText = text.toLower();
    QChar currentChar;
    QChar nextChar;
    if(!newText.at(0).isSpace())
    {
        newText.replace(0, 1, newText.at(0).toUpper());
    }

    for (int i = 0; i < newText.size(); i++) {
        currentChar = newText.at(i);
        if(i+1<newText.size())
        nextChar = newText.at(i+1);
//        if (!currentChar.isSpace() && !m_wordSepartors.contains(QString(currentChar))&&nextChar.isSpace()) {
//            newText.replace(i, 1, currentChar.toUpper());
//            //break;
//        }
        if(currentChar.isSpace()&&!nextChar.isSpace())
        {
            newText.replace(i+1, 1, nextChar.toUpper());
        }
    }

    return newText;
}

void TextEdit::keepCurrentLineAtCenter()
{
    QScrollBar *scrollbar = verticalScrollBar();

    int currentLine = cursorRect().top() / cursorRect().height();
    int halfEditorLines = rect().height() / 2 / cursorRect().height();
    scrollbar->setValue(scrollbar->value() + currentLine - halfEditorLines);
}

void TextEdit::scrollToLine(int scrollOffset, int row, int column)
{
    // Save cursor postion.
    m_restoreRow = row;
    m_restoreColumn = column;

    // Start scroll animation.
    m_scrollAnimation->setStartValue(verticalScrollBar()->value());
    m_scrollAnimation->setEndValue(scrollOffset);
    m_scrollAnimation->start();
}

void TextEdit::setLineWrapMode(bool enable)
{
    this->setWordWrapMode(QTextOption::WrapAnywhere);
    QTextEdit::setLineWrapMode(enable ? QTextEdit::WidgetWidth : QTextEdit::NoWrap);
    m_pLeftAreaWidget->m_linenumberarea->update();
    m_pLeftAreaWidget->m_flodArea->update();
    m_pLeftAreaWidget->m_bookMarkArea->update();
}

void TextEdit::setFontFamily(QString name)
{
    // Update font.
    m_fontName = name;
    updateFont();
    updateLineNumber();
}

void TextEdit::setFontSize(int size)
{
    // Update font.
    m_fontSize = size;
    updateFont();

    // Update line number after adjust font size.
    updateLineNumber();
}

void TextEdit::updateFont()
{
    QFont font = document()->defaultFont();
    font.setFixedPitch(true);
    font.setPointSize(m_fontSize);
    font.setFamily(m_fontName);
    setFont(font);
    setTabStopWidth(m_tabSpaceNumber * QFontMetrics(font).width(' '));
}

void TextEdit::replaceAll(const QString &replaceText, const QString &withText)
{
    if (m_readOnlyMode){
        return;
    }

    if (replaceText.isEmpty()) {
        return;
    }

    QTextDocument::FindFlags flags;
    flags &= QTextDocument::FindCaseSensitively;

    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::Start);

    QTextCursor startCursor = textCursor();
    startCursor.beginEditBlock();

    while (1) {
        cursor = document()->find(replaceText, cursor, flags);

        if (!cursor.isNull()) {
            cursor.insertText(withText);
        } else {
            break;
        }
    }

    startCursor.endEditBlock();
    setTextCursor(startCursor);
}

void TextEdit::replaceNext(const QString &replaceText, const QString &withText)
{
    if (m_readOnlyMode){
        return;
    }

    if (replaceText.isEmpty() ||
        !m_findHighlightSelection.cursor.hasSelection()) {
        // 无限替换的根源
        return;
    }
    QTextCursor cursor = textCursor();
    //cursor.setPosition(m_findHighlightSelection.cursor.position() - replaceText.size());
    if(m_cursorStart!=-1)
    {
        cursor.setPosition(m_cursorStart);
        m_cursorStart=-1;
    }
    else {
        cursor.setPosition(m_findHighlightSelection.cursor.selectionStart());
    }
    cursor.movePosition(QTextCursor::NoMove, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, replaceText.size());
    cursor.insertText(withText);

    // Update cursor.
    setTextCursor(cursor);
    highlightKeyword(replaceText, getPosition());
}

void TextEdit::replaceRest(const QString &replaceText, const QString &withText)
{
    if (m_readOnlyMode){
        return;
    }

    // If replace text is nothing, don't do replace action.
    if (replaceText.isEmpty()) {
        return;
    }

    QTextDocument::FindFlags flags;
    flags &= QTextDocument::FindCaseSensitively;

    QTextCursor cursor = textCursor();

    QTextCursor startCursor = textCursor();
    startCursor.beginEditBlock();

    while (1) {
        cursor = document()->find(replaceText, cursor, flags);

        if (!cursor.isNull()) {
            cursor.insertText(withText);
        } else {
            break;
        }
    }

    startCursor.endEditBlock();
    setTextCursor(startCursor);
}

void TextEdit::beforeReplace(QString _)
{
    if (_.isEmpty() ||
        !m_findHighlightSelection.cursor.hasSelection()) {
        highlightKeyword(_, getPosition());
    }
}

bool TextEdit::findKeywordForward(const QString &keyword)
{
    if (textCursor().hasSelection()) {
        // Get selection bound.
        int startPos = textCursor().anchor();
        int endPos = textCursor().position();

        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
        //setTextCursor(cursor);

        QTextDocument::FindFlags options;
        options &= QTextDocument::FindCaseSensitively;

        bool foundOne = find(keyword, options);

        cursor.setPosition(endPos, QTextCursor::MoveAnchor);
        cursor.setPosition(startPos, QTextCursor::KeepAnchor);
        //setTextCursor(cursor);

        return foundOne;
    } else {
        QTextCursor recordCursor = textCursor();

        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
        //setTextCursor(cursor);

        QTextDocument::FindFlags options;
        options &= QTextDocument::FindCaseSensitively;

        bool foundOne = find(keyword, options);

        //setTextCursor(recordCursor);

        return foundOne;
    }
}

void TextEdit::removeKeywords()
{
    m_findHighlightSelection.cursor = textCursor();
    m_findHighlightSelection.cursor.clearSelection();

    m_findMatchSelections.clear();

    updateHighlightLineSelection();

    renderAllSelections();

    setFocus();
}

bool TextEdit::highlightKeyword(QString keyword, int position)
{
    m_findMatchSelections.clear();
    bool yes = updateKeywordSelections(keyword,m_findMatchFormat,&m_findMatchSelections);
    setExtraSelections(m_findMatchSelections);
    updateCursorKeywordSelection(position, true);
    updateHighlightLineSelection();
    renderAllSelections();
    return yes;
}

void TextEdit::updateCursorKeywordSelection(int position, bool findNext)
{
    bool findOne = setCursorKeywordSeletoin(position, findNext);

    if (!findOne) {
        if (findNext) {
            // Clear keyword if keyword not match anything.
            if (!setCursorKeywordSeletoin(0, findNext)) {
                m_findHighlightSelection.cursor = textCursor();

                m_findMatchSelections.clear();
                renderAllSelections();
            }
        } else {
            QTextCursor cursor = textCursor();
            cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);

            m_findHighlightSelection.cursor.clearSelection();
            setCursorKeywordSeletoin(cursor.position(), findNext);
        }
    }
}

void TextEdit::updateHighlightLineSelection()
{
    if(m_readOnlyMode){
        //selection.format.setProperty(QTextFormat::FullWidthSelection, false);
    }
    else {
        QTextEdit::ExtraSelection selection;

        selection.format.setBackground(m_currentLineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        m_currentLineSelection = selection;
    }
}

bool TextEdit::updateKeywordSelections(QString keyword,QTextCharFormat charFormat,QList<QTextEdit::ExtraSelection> *listSelection)
{
    // Clear keyword selections first.
    listSelection->clear();

    // Update selections with keyword.
    if (!keyword.isEmpty()) {
        QTextCursor cursor(document());
        QTextDocument::FindFlags flags;
        flags = QTextDocument::FindCaseSensitively;
        QTextEdit::ExtraSelection extra;
        extra.format = charFormat;
        cursor = document()->find(keyword, cursor, flags);
        if(cursor.isNull())
        {
            return false;
        }
        while (!cursor.isNull()) { 
            extra.cursor = cursor;
            listSelection->append(extra);
            cursor = document()->find(keyword, cursor, flags);
        }
        return true;
    }
    return false;
}

void TextEdit::renderAllSelections()
{
    QList<QTextEdit::ExtraSelection> selections;
//    setExtraSelections(selections);

//    for (auto findMatch : m_findMatchSelections) {
//        findMatch.format = m_findMatchFormat;
//        selections.append(findMatch);
//    }
    selections.append(m_currentLineSelection);
    selections.append(m_markAllSelection);
    selections.append(m_wordMarkSelections);
    selections.append(m_findMatchSelections);
    selections.append(m_findHighlightSelection);
    selections.append(m_wordUnderCursorSelection);
    selections.append(m_beginBracketSelection);
    selections.append(m_endBracketSelection);
    selections.append(m_markFoldHighLightSelections);
//    selections.append(m_markAllSelection);

    setExtraSelections(selections);
}

DMenu *TextEdit::getHighlightMenu()
{
    return m_hlGroupMenu;
}

void TextEdit::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    //painter.fillRect(event->rect(), m_backgroundColor);

    QColor lineNumberAreaBackgroundColor;
    if (QColor(m_backgroundColor).lightness() < 128) {
        lineNumberAreaBackgroundColor = palette().brightText().color();
        lineNumberAreaBackgroundColor.setAlphaF(0.01);

        m_lineNumbersColor.setAlphaF(0.2);
    } else {
        lineNumberAreaBackgroundColor = palette().brightText().color();
        lineNumberAreaBackgroundColor.setAlphaF(0.03);
        m_lineNumbersColor.setAlphaF(0.3);
    }
    painter.fillRect(event->rect(), lineNumberAreaBackgroundColor);

    int blockNumber = getFirstVisibleBlockId();
    if(blockNumber > 0) {        //多绘制上面一行的行号
        blockNumber--;
    }
//    blockNumber = 0;
    QTextBlock block = document()->findBlockByNumber(blockNumber);

    QTextBlock prev_block = (blockNumber > 0) ? document()->findBlockByNumber(blockNumber-1) : block;
    int translate_y = (blockNumber > 0) ? -verticalScrollBar()->sliderPosition() : 0;

    int top = this->viewport()->geometry().top();

    // Adjust text position according to the previous "non entirely visible" block
    // if applicable. Also takes in consideration the document's margin offset.
    int additional_margin;
    if (blockNumber == 0) {
        // Simply adjust to document's margin
  //      additional_margin = document()->documentMargin() -1 - this->verticalScrollBar()->sliderPosition();
        additional_margin = document()->documentMargin() - this->verticalScrollBar()->sliderPosition();
    }
    else {
        // Getting the height of the visible part of the previous "non entirely visible" block
//        additional_margin = document()->documentLayout()->blockBoundingRect(prev_block).toRect()
//                .translated(0, translate_y).intersected(this->viewport()->geometry()).height();
        additional_margin = document()->documentLayout()->blockBoundingRect(prev_block).toRect()
                .translated(0, translate_y).bottom();          //不比较，直接从上一行的底边开始绘制，即多绘制一行的行号
    }

    // Shift the starting point
    //additional_margin -= 2;

    top += additional_margin;

    int bottom = top + document()->documentLayout()->blockBoundingRect(block).height();


//    int bottom1 = top + document()->documentLayout()->blockBoundingRect(block).toRect().height();
//    qDebug()<<"bottom1 = "<<bottom1;

    Utils::setFontSize(painter, document()->defaultFont().pointSize() - 2);
    // Draw the numbers (displaying the current line number in green)
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            if (blockNumber + 1 == m_markStartLine) {
                painter.setPen(m_regionMarkerColor);
            } else {
                painter.setPen(m_lineNumbersColor);
            }

            m_fontLineNumberArea.setPointSize(currentFont().pointSize() - 1);
            painter.setFont(m_fontLineNumberArea);

            if (fontMetrics().height() *1.5 > document()->documentLayout()->blockBoundingRect(block).height()) {

                painter.drawText(0, top,
                                 lineNumberArea->width(), document()->documentLayout()->blockBoundingRect(block).height(),
                                 Qt::AlignVCenter | Qt::AlignHCenter, QString::number(blockNumber + 1));
               // qDebug() << ".........." << document()->documentLayout()->blockBoundingRect(block).width();
            } else {
                painter.drawText(0, top,
                                 lineNumberArea->width(), document()->documentLayout()->blockBoundingRect(block).height(),
                                 /*Qt::AlignVCenter |*/ Qt::AlignHCenter, QString::number(blockNumber + 1));
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + document()->documentLayout()->blockBoundingRect(block).height();
        ++blockNumber;
    }
}

void TextEdit::codeFLodAreaPaintEvent(QPaintEvent *event)
{

    m_listFlodFlag.clear();
    m_listFlodIconPos.clear();
    QPainter painter(m_pLeftAreaWidget->m_flodArea);

    QColor codeFlodAreaBackgroundColor;
    if (QColor(m_backgroundColor).lightness() < 128) {
        codeFlodAreaBackgroundColor = palette().brightText().color();
        codeFlodAreaBackgroundColor.setAlphaF(0.01);

        m_lineNumbersColor.setAlphaF(0.2);
    } else {
        codeFlodAreaBackgroundColor = palette().brightText().color();
        codeFlodAreaBackgroundColor.setAlphaF(0.03);
        m_lineNumbersColor.setAlphaF(0.3);
    }
    painter.fillRect(event->rect(), codeFlodAreaBackgroundColor);

    int blockNumber = getFirstVisibleBlockId();
    QTextBlock block = document()->findBlockByNumber(blockNumber);
    QTextBlock prev_block = (blockNumber > 0) ? document()->findBlockByNumber(blockNumber - 1) : block;
    int translate_y = (blockNumber > 0) ? -verticalScrollBar()->sliderPosition() : 0;

    int top = this->viewport()->geometry().top();
    int additional_margin;
    if (blockNumber == 0)
        // Simply adjust to document's margin
   //     additional_margin = document()->documentMargin() - 1 - this->verticalScrollBar()->sliderPosition();
        additional_margin = document()->documentMargin() - this->verticalScrollBar()->sliderPosition();
    else
        // Getting the height of the visible part of the previous "non entirely visible" block
        additional_margin = document()->documentLayout()->blockBoundingRect(prev_block).toRect()
                            .translated(0, translate_y).intersected(this->viewport()->geometry()).height();

    // Shift the starting point
    additional_margin += 3;
    top += additional_margin;

    DGuiApplicationHelper *guiAppHelp = DGuiApplicationHelper::instance();

    QString theme  = "";
    if (guiAppHelp->themeType() == DGuiApplicationHelper::ColorType::DarkType) {  //暗色主题
        theme = "d";
    } else {  //浅色主题
        theme = "l";
    }
    QString unflodImagePath = ":/images/d-" + theme + ".svg";
    QString flodImagePath = ":/images/u-" + theme + ".svg";
    QImage Unfoldimage(unflodImagePath);
    QImage foldimage(flodImagePath);
    QImage scaleFoldImage;
    QImage scaleunFoldImage;
    int imageTop = 0;//图片绘制位置
    int fontHeight = fontMetrics().height();
    double nfoldImageHeight = fontHeight;

    int bottom = top + document()->documentLayout()->blockBoundingRect(block).height();

    for (int iBlockCount = blockNumber ; iBlockCount < document()->blockCount(); ++iBlockCount) {
        if (block.text().trimmed().startsWith("{") && blockNumber != 0) {
            m_listFlodFlag.append(blockNumber - 1);
        } else {
            m_listFlodFlag.append(blockNumber);
        }
        if (block.isValid() && top <= event->rect().bottom()) {
            if (/*block.isVisible() &&*/ bottom >= event->rect().top()) {
                //判定是否包含左括号、是否整行是注释，isNeedShowFoldIcon该函数是为了做判定当前行是否包含成对的括号，如果包括，则不显示折叠标志
                if (block.text().contains("{") &&
                        !block.text().trimmed().startsWith("//") &&
                        isNeedShowFoldIcon(block)) {
                    int blockHeight = 0;
                    QTextBlock tmpblock = document()->findBlockByNumber(0);;
                    while (tmpblock.isValid()) {
                        if (tmpblock.isVisible()) {
                            blockHeight = document()->documentLayout()->blockBoundingRect(tmpblock).height();
                            break;
                        }
                        tmpblock = tmpblock.next();
                    }
                    if (fontHeight > foldimage.height()) {
                        if (blockHeight > 1.5 * fontHeight) {
                            imageTop = top + qFabs(fontHeight - foldimage.height()) / 2;
                        } else {

                            imageTop = top + qFabs(blockHeight - foldimage.height()) / 2;
                        }

                        scaleFoldImage = foldimage;
                        scaleunFoldImage = Unfoldimage;
                    } else {
                        if (blockHeight > 1.5 * fontHeight) {

                            imageTop = top - qFabs(fontHeight - foldimage.height()) / 2;
                        } else {

                            imageTop = top - qFabs(blockHeight - foldimage.height()) / 2;
                        }

                        //imageTop = startPoint + qFabs(document()->documentLayout()->blockBoundingRect(lineBlock).height() - image.height())/2;
                        double scale = nfoldImageHeight / foldimage.height();
                        double nScaleWidth = scale * foldimage.height() * foldimage.height() / foldimage.width();
                        scaleFoldImage = foldimage.scaled(scale * foldimage.height(), nScaleWidth);
                        scaleunFoldImage = Unfoldimage.scaled(scale * Unfoldimage.height(), nScaleWidth);
                    }
                    if (block.text().trimmed().startsWith("{") && blockNumber != 0) {

                        if (block.isVisible()) {
                            painter.drawImage(5, imageTop - blockHeight, scaleFoldImage);
                        } else {
                            painter.drawImage(5, imageTop - blockHeight, scaleunFoldImage);
                        }

                        m_listFlodIconPos.append(blockNumber - 1);
                    } else {
                        if (block.next().isVisible()) {
                            if (block.isVisible())
                                painter.drawImage(5, imageTop, scaleFoldImage);
                        } else {
                            if (block.isVisible())
                                painter.drawImage(5, imageTop, scaleunFoldImage);
                        }
                        m_listFlodIconPos.append(blockNumber);
                    }

                }
            }
            block = document()->findBlockByNumber(iBlockCount + 1);
            top = bottom;
            bottom = top + document()->documentLayout()->blockBoundingRect(block).height();
        }
        ++blockNumber;
    }
}

void TextEdit::setCodeFlodFlagVisable(bool isVisable,bool bIsFirstOpen)
{
   int leftAreaWidth = m_pLeftAreaWidget->width();
   int flodAreaWidth = m_pLeftAreaWidget->m_flodArea->width();
//    qDebug()<<"leftAreaWidth = "<<leftAreaWidth;
//    qDebug()<<"flodAreaWidth = "<<flodAreaWidth;
    if(!bIsFirstOpen) {
       if(isVisable) {
           m_pLeftAreaWidget->setFixedWidth(leftAreaWidth+flodAreaWidth);
//            qDebug()<<"isVisable = "<<isVisable;
//            qDebug()<<"-----------------------leftAreaWidth = "<<m_pLeftAreaWidget->width();
       } else {
           m_pLeftAreaWidget->setFixedWidth(leftAreaWidth - flodAreaWidth);
//            qDebug()<<"isVisable = "<<isVisable;
//            qDebug()<<"-----------------------leftAreaWidth = "<<m_pLeftAreaWidget->width();
       }
    }
    m_pIsShowCodeFoldArea = isVisable;
    m_pLeftAreaWidget->m_flodArea->setVisible(isVisable);
}

void TextEdit::updateLineNumber()
{
    // Update line number painter.

    int blockSize = QString::number(blockCount()).size();

//    m_pLeftAreaWidget->setFixedWidth(23 + blockSize * fontMetrics().width('9') + m_lineNumberPaddingX * 4);

//    m_pLeftAreaWidget->setFixedWidth(blockSize * fontMetrics().width('9') + m_lineNumberPaddingX * 4);


    if(m_pIsShowCodeFoldArea) {
//        m_pLeftAreaWidget->setFixedWidth(23 + blockSize * fontMetrics().width('9') + m_lineNumberPaddingX * 4);
        m_pLeftAreaWidget->setFixedWidth(23 + blockSize * fontMetrics().width('9') + m_pLeftAreaWidget->m_bookMarkArea->width());
    } else {
//        m_pLeftAreaWidget->setFixedWidth(blockSize * fontMetrics().width('9') + m_lineNumberPaddingX * 4);
        m_pLeftAreaWidget->setFixedWidth(blockSize * fontMetrics().width('9') + m_pLeftAreaWidget->m_bookMarkArea->width());
    }

    if(!bIsSetLineNumberWidth) {
        int leftAreaWidth = m_pLeftAreaWidget->width();
        m_pLeftAreaWidget->setFixedWidth(leftAreaWidth - blockSize * fontMetrics().width('9'));
    }
    m_pLeftAreaWidget->m_bookMarkArea->update();
    lineNumberArea->update();
    m_pLeftAreaWidget->m_flodArea->update();
}

void TextEdit::updateWordCount()
{
    m_wrapper->bottomBar()->updateWordCount(characterCount());
}

void TextEdit::handleScrollFinish()
{
    // Restore cursor postion.
    jumpToLine(m_restoreRow, false);

    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, m_restoreColumn);

    // Update cursor.
    setTextCursor(cursor);
}

bool TextEdit::setCursorKeywordSeletoin(int position, bool findNext)
{
    int offsetLines = 3;

    if (findNext) {
        for (int i = 0; i < m_findMatchSelections.size(); i++) {
            if (m_findMatchSelections[i].cursor.position() > position) {
                m_findHighlightSelection.cursor = m_findMatchSelections[i].cursor;

                jumpToLine(m_findMatchSelections[i].cursor.blockNumber() + offsetLines, false);

                QTextCursor cursor = textCursor();
                cursor.setPosition(m_findMatchSelections[i].cursor.position());

                // Update cursor.
                setTextCursor(cursor);

                return true;
            }
        }
    } else {
        for (int i = m_findMatchSelections.size() - 1; i >= 0; i--) {
            if (m_findMatchSelections[i].cursor.position() < position) {
                m_findHighlightSelection.cursor = m_findMatchSelections[i].cursor;

                jumpToLine(m_findMatchSelections[i].cursor.blockNumber() + offsetLines, false);

                QTextCursor cursor = textCursor();
                cursor.setPosition(m_findMatchSelections[i].cursor.position());

                // Update cursor.
                setTextCursor(cursor);

                return true;
            }
        }
    }

    return false;
}

void TextEdit::cursorPositionChanged()
{
    m_beginBracketSelection = QTextEdit::ExtraSelection();
    m_endBracketSelection = QTextEdit::ExtraSelection();

    updateHighlightLineSelection();
    updateHighlightBrackets('(', ')');
    updateHighlightBrackets('{', '}');
    updateHighlightBrackets('[', ']');
    renderAllSelections();

    QTextCursor cursor = textCursor();
    if (m_wrapper) {
        m_wrapper->bottomBar()->updatePosition(cursor.blockNumber() + 1,
                                               cursor.columnNumber() + 1);
    }
}

void TextEdit::updateHighlightBrackets(const QChar &openChar, const QChar &closeChar)
{
    QTextDocument *doc = document();
    QTextCursor cursor = textCursor();
    int position = cursor.position();

    QTextCursor bracketBeginCursor;
    QTextCursor bracketEndCursor;
    cursor.clearSelection();

    if (!bracketBeginCursor.isNull() || !bracketEndCursor.isNull()) {
        bracketBeginCursor.setCharFormat(QTextCharFormat());
        bracketEndCursor.setCharFormat(QTextCharFormat());
        bracketBeginCursor = bracketEndCursor = QTextCursor();
    }

    bool forward;
    QChar begin, end;

    if (doc->characterAt(position) == openChar ||
        doc->characterAt(position) == closeChar ||
        doc->characterAt(position - 1) == openChar ||
        doc->characterAt(position - 1) == closeChar)
    {
        forward = doc->characterAt(position) == openChar ||
                  doc->characterAt(position - 1) == openChar;

        if (forward) {
            if (doc->characterAt(position) == openChar) {
                position++;
            } else {
                cursor.setPosition(position - 1);
            }

            begin = openChar;
            end = closeChar;
        } else {
            if (doc->characterAt(position) == closeChar) {
                cursor.setPosition(position + 1);
                position -= 1;
            } else {
                position -= 2;
            }

            begin = closeChar;
            end = openChar;
        }

        bracketBeginCursor = cursor;
        bracketBeginCursor.movePosition(forward ? QTextCursor::NextCharacter : QTextCursor::PreviousCharacter,
                                        QTextCursor::KeepAnchor);

        int braceDepth = 1;
        QChar c;

        while (!(c = doc->characterAt(position)).isNull()) {
            if (c == begin) {
                braceDepth++;
            } else if (c == end) {
                braceDepth--;

                if (!braceDepth) {
                    bracketEndCursor = QTextCursor(doc);
                    bracketEndCursor.setPosition(position);
                    bracketEndCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
                    break;
                }
            }

            forward ? position++ : position--;
        }

        // cannot find the end bracket to not need to highlight.
        if (!bracketEndCursor.isNull()) {
            m_beginBracketSelection.cursor = bracketBeginCursor;
            m_beginBracketSelection.format = m_bracketMatchFormat;

            m_endBracketSelection.cursor = bracketEndCursor;
            m_endBracketSelection.format = m_bracketMatchFormat;
        }
    }
}

int TextEdit::getFirstVisibleBlockId() const
{
    // Detect the first block for which bounding rect - once translated
    // in absolute coordinated - is contained by the editor's text area

    // Costly way of doing but since "blockBoundingGeometry(...)" doesn't
    // exists for "QTextEdit"...

    QTextCursor curs = QTextCursor(this->document());
    curs.movePosition(QTextCursor::Start);
    for (int i=0; i < this->document()->blockCount(); ++i) {
        QTextBlock block = curs.block();
        if (block.isVisible()) {
            QRect r1 = this->viewport()->geometry();
            QRect r2 = this->document()->documentLayout()->blockBoundingRect(block).translated(
                           this->viewport()->geometry().x(), this->viewport()->geometry().y() - (
                               this->verticalScrollBar()->sliderPosition())).toRect();

            r2.setWidth(0);        //只通过高度判断是否包含在当前界面
            if (r1.contains(r2, true)) {
                return i;
            }
            curs.movePosition(QTextCursor::NextBlock);
        }
    }

    return 0;
}

//line 开始处理的行号  isvisable是否折叠  iInitnum左括号默认开始计算的数量  isFirstLine是否是第一行，因为第一行默认不折叠
void TextEdit::getNeedControlLine(int line, bool isVisable, int iInitnum, bool isFirstLine)
{
    int iLine = line;
    QTextBlock block = document()->findBlockByNumber(iLine);
    int existLeftSubbrackets = iInitnum, existRightSubbrackets = 0;

    while (block.isValid()) {
        //去掉注释里面的括号
        if (block.text().contains("//")) {
            QString tmpText = block.text().split("//")[1];
            for (int i = 0 ; i < tmpText.size(); ++i) {
                if (tmpText[i] == "}")
                    existRightSubbrackets--;
                if (tmpText[i] == "{")
                    existLeftSubbrackets--;
            }
        }
        //去掉字符串里面的括号
//        if (block.text().contains("\"")) {
//            QString tmpText = block.text().split("\"")[1];
//            qDebug() << ">>>>>>>>>>>>>>>>size" << block.text().split("\"").size();
//            for (int i = 0 ; i < tmpText.size(); ++i) {
//                if (tmpText[i] == "}")
//                    existRightSubbrackets--;
//                if (tmpText[i] == "{")
//                    existLeftSubbrackets--;
//            }
//        }
        //计算左右括号对应的数量
        if (block.text().contains("{") && !block.text().contains("}")) {
            existLeftSubbrackets ++;
        } else if (block.text().contains("}") && !block.text().contains("{")) {
            existRightSubbrackets ++;
        } else if (block.text().contains("}") && block.text().contains("{")) {
            for (int i = 0 ; i < block.text().size() ; ++i) {
                if (block.text().at(i) == "{") {
                    existLeftSubbrackets++;
                } else if (block.text().at(i) == "}" && !isFirstLine) {
                    existRightSubbrackets++;
                }
                if (existLeftSubbrackets == existRightSubbrackets &&
                        existLeftSubbrackets != 0) {
                    break;
                }
            }
        }

        //左右括号数量相等时，则视为找到需要折叠的代码块
        if (existLeftSubbrackets == existRightSubbrackets &&
                existLeftSubbrackets != 0) {
            if (!block.text().contains("{")) {
                block.setVisible(isVisable);
                break;
            } else {
                block.setVisible(true);
                break;
            }
        }
        if (isFirstLine) {
            isFirstLine = false;
            block.setVisible(true);
        } else {
            block.setVisible(isVisable);
        }
        block = block.next();
        iLine++;
        viewport()->adjustSize();
    }
}

void TextEdit::setThemeWithPath(const QString &path)
{
    const KSyntaxHighlighting::Theme theme = m_repository.theme("");
    setTheme(theme, path);
}

void TextEdit::setTheme(const KSyntaxHighlighting::Theme &theme, const QString &path)
{
    QVariantMap jsonMap = Utils::getThemeMapFromPath(path);
    QVariantMap textStylesMap = jsonMap["text-styles"].toMap();
    const QString &themeCurrentLineColor = jsonMap["editor-colors"].toMap()["current-line"].toString();
    const QString textColor = textStylesMap["Normal"].toMap()["text-color"].toString();

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

    const QString findMatchBgColor = jsonMap["editor-colors"].toMap()["find-match-background"].toString();
    const QString findMatchFgColor = jsonMap["editor-colors"].toMap()["find-match-foreground"].toString();
    m_findMatchFormat = currentCharFormat();
    m_findMatchFormat.setBackground(QColor(findMatchBgColor));
    m_findMatchFormat.setForeground(QColor(findMatchFgColor));

    const QString findHighlightBgColor = jsonMap["editor-colors"].toMap()["find-highlight-background"].toString();
    const QString findHighlightFgColor = jsonMap["editor-colors"].toMap()["find-highlight-foreground"].toString();
    m_findHighlightSelection.format.setProperty(QTextFormat::BackgroundBrush, QBrush(QColor(findHighlightBgColor)));
    m_findHighlightSelection.format.setProperty(QTextFormat::ForegroundBrush, QBrush(QColor(findHighlightFgColor)));

    m_beginBracketSelection.format = m_bracketMatchFormat;
    m_endBracketSelection.format = m_bracketMatchFormat;

    const QString &styleSheet = QString("QTextEdit {"
                                        "background-color: %1;"
                                        "color: %2;"
                                        "selection-color: %3;"
                                        "selection-background-color: %4;"
                                        "}").arg(m_backgroundColor.name(), textColor,
                                                 m_selectionColor.name(), m_selectionBgColor.name());
    setStyleSheet(styleSheet);

    if (m_highlighter != nullptr) {
        if (m_backgroundColor.lightness() < 128) {

            m_highlighter->setTheme(m_repository.defaultTheme(KSyntaxHighlighting::Repository::DarkTheme));
        } else {
            m_highlighter->setTheme(m_repository.defaultTheme(KSyntaxHighlighting::Repository::LightTheme));
        }
    }

    // TODO
    if (m_wrapper) {
        QPalette palette = m_wrapper->bottomBar()->palette();
        palette.setColor(QPalette::Background, m_backgroundColor);
        palette.setColor(QPalette::Text, textColor);
        m_wrapper->bottomBar()->setPalette(palette);
    }

    // does not support highlight do not reload
    // when switching theme will be jammed or large files.
    int iVerticalScrollValue = getScrollOffset();
    int iHorizontalScrollVaule = horizontalScrollBar()->value();
    getScrollOffset();
//    QString backupTxt = toPlainText();
//    setPlainText("");
    if (m_highlighted) {
        m_highlighter->rehighlight();
    }

//    setPlainText(backupTxt);
    verticalScrollBar()->setSliderPosition(iVerticalScrollValue);
    horizontalScrollBar()->setSliderPosition(iHorizontalScrollVaule);

    lineNumberArea->update();
    m_pLeftAreaWidget->m_bookMarkArea->update();

    highlightCurrentLine();
}

void TextEdit::loadHighlighter()
{
    if (m_highlighter == nullptr) {
        m_highlighter = new KSyntaxHighlighting::SyntaxHighlighter(document());
    }
    const auto def = m_repository.definitionForFileName(QFileInfo(filepath).fileName());

    if (!def.filePath().isEmpty()) {
        const QString &syntaxFile = QFileInfo(QString(":/syntax/%1")
                                              .arg(QFileInfo(def.filePath()).fileName())).absoluteFilePath();

        QFile file(syntaxFile);
        if (!file.open(QFile::ReadOnly)) {
            qDebug() << "Can't open file" << syntaxFile;
        }

        QXmlStreamReader reader(&file);
        QString singleLineComment;
        QString multiLineCommentStart;
        QString multiLineCommentEnd;

        while (!reader.atEnd()) {
            const auto token = reader.readNext();
            if (token != QXmlStreamReader::StartElement) {
                continue;
            }

            if (reader.name() == "comment") {
                if (reader.attributes().hasAttribute(QStringLiteral("name"))) {
                    QString attrName = reader.attributes().value(QStringLiteral("name")).toString();

                    if (attrName == "singleLine") {
                        singleLineComment = reader.attributes().value(QStringLiteral("start")).toString();
                    } else if (attrName == "multiLine") {
                        multiLineCommentStart = reader.attributes().value(QStringLiteral("start")).toString();
                        multiLineCommentEnd = reader.attributes().value(QStringLiteral("end")).toString();
                    }
                }
            }
        }

        m_commentDefinition.setComments(QString("%1 ").arg(singleLineComment), multiLineCommentStart, multiLineCommentEnd);

        m_highlighter->setDefinition(def);

        file.close();

        m_highlighted = true;

        // init action.
        for (QAction *action : m_hlActionGroup->actions()) {
            if (action->text() == def.name()) {
                action->setChecked(true);
                emit hightlightChanged(action->text());
            }
        }

    } else {
        delete m_highlighter;
        m_highlighter = nullptr;
        m_highlighted = false;
    }
}

bool TextEdit::highlightWordUnderMouse(QPoint pos)
{
    // Get cursor match mouse pointer coordinate, but cursor maybe not under mouse pointer.
    QTextCursor cursor(cursorForPosition(pos));

    // Get cursor rectangle.
    auto rect = cursorRect(cursor);
    int widthOffset = 10;
    rect.setX(std::max(rect.x() - widthOffset / 2, 0));
    rect.setWidth(rect.width() + widthOffset);

    // Just highlight word under pointer when cursor rectangle contain moue pointer coordinate.
    if ((rect.x() <= pos.x()) &&
        (pos.x() <= rect.x() + rect.width()) &&
        (rect.y() <= pos.y()) &&
        (pos.y() <= rect.y() + rect.height())) {
        // Move back to word bound start postion, and save cursor for convert case.
        m_wordUnderPointerCursor = cursor;
        m_wordUnderPointerCursor.select(QTextCursor::WordUnderCursor);
        m_wordUnderPointerCursor.setPosition(m_wordUnderPointerCursor.anchor(), QTextCursor::MoveAnchor);

        // Update highlight cursor.
        QTextEdit::ExtraSelection selection;

        selection.format.setBackground(m_selectionBgColor);
        selection.format.setForeground(m_selectionColor);
        selection.cursor = cursor;
        selection.cursor.select(QTextCursor::WordUnderCursor);

        //m_wordUnderCursorSelection = selection;

        renderAllSelections();

        return true;
    } else {
        return false;
    }
}

void TextEdit::removeHighlightWordUnderCursor()
{
    //m_highlightWordCacheCursor = m_wordUnderCursorSelection.cursor;

    QTextEdit::ExtraSelection selection;
    //m_wordUnderCursorSelection = selection;

    renderAllSelections();
    m_nBookMarkHoverLine = -1;
    m_pLeftAreaWidget->m_bookMarkArea->update();
}

void TextEdit::setSettings(Settings *keySettings)
{
    m_settings = keySettings;
}

void TextEdit::setModified(bool modified)
{
    document()->setModified(modified);

    emit modificationChanged(filepath, modified);
}

void TextEdit::copySelectedText()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(textCursor().selection().toPlainText());
    tryUnsetMark();
}

void TextEdit::cutSelectedText()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(textCursor().selection().toPlainText());

    QTextCursor cursor = textCursor();
    cursor.removeSelectedText();
    setTextCursor(cursor);

    unsetMark();
}

void TextEdit::pasteText()
{
    QTextEdit::paste();

    unsetMark();
}

void TextEdit::setMark()
{
    bool currentMark = m_cursorMark;
    bool markCursorChanged = false;

    if (m_cursorMark) {
        if (textCursor().hasSelection()) {
            markCursorChanged = true;

            QTextCursor cursor = textCursor();
            cursor.clearSelection();
            setTextCursor(cursor);
        } else {
            m_cursorMark = false;
        }
    } else {
        m_cursorMark = true;
    }

    if (m_cursorMark != currentMark || markCursorChanged) {
        cursorMarkChanged(m_cursorMark, textCursor());
    }
}

void TextEdit::unsetMark()
{
    bool currentMark = m_cursorMark;

    m_cursorMark = false;

    if (m_cursorMark != currentMark) {
        cursorMarkChanged(m_cursorMark, textCursor());
    }
}

bool TextEdit::tryUnsetMark()
{
    if (m_cursorMark) {
        QTextCursor cursor = textCursor();
        cursor.clearSelection();
        setTextCursor(cursor);

        unsetMark();

        return true;
    } else {
        return false;
    }
}

void TextEdit::exchangeMark()
{
    unsetMark();

    if (textCursor().hasSelection()) {
        // Record cursor and seleciton position before move cursor.
        int actionStartPos = textCursor().position();
        int selectionStartPos = textCursor().selectionStart();
        int selectionEndPos = textCursor().selectionEnd();

        QTextCursor cursor = textCursor();
        if (actionStartPos == selectionStartPos) {
            cursor.setPosition(selectionStartPos, QTextCursor::MoveAnchor);
            cursor.setPosition(selectionEndPos, QTextCursor::KeepAnchor);
        } else {
            cursor.setPosition(selectionEndPos, QTextCursor::MoveAnchor);
            cursor.setPosition(selectionStartPos, QTextCursor::KeepAnchor);
        }

        setTextCursor(cursor);
    }
}

void TextEdit::saveMarkStatus()
{
    m_cursorMarkStatus = m_cursorMark;
    m_cursorMarkPosition = textCursor().anchor();
}

void TextEdit::restoreMarkStatus()
{
    if (m_cursorMarkStatus) {
        QTextCursor currentCursor = textCursor();

        QTextCursor cursor = textCursor();
        cursor.setPosition(m_cursorMarkPosition, QTextCursor::MoveAnchor);
        cursor.setPosition(currentCursor.position(), QTextCursor::KeepAnchor);

        setTextCursor(cursor);
    }
}

void TextEdit::clickCutAction()
{
    if (textCursor().hasSelection()) {
        cutSelectedText();
    } else {
        cutWordUnderCursor();
    }
}

void TextEdit::clickCopyAction()
{
    if (textCursor().hasSelection()) {
        copySelectedText();
    } else {
        copyWordUnderCursor();
    }
}

void TextEdit::clickPasteAction()
{
//    if (textCursor().hasSelection()) {
//        pasteText();
//    } else {
//        QTextCursor cursor;

//        // Move to word cursor if have word around mouse.
//        // Otherwise find nearest cursor with mouse click.
//        if (m_highlightWordCacheCursor.position() != -1) {
//            cursor = textCursor();
//            cursor.setPosition(m_highlightWordCacheCursor.position(), QTextCursor::MoveAnchor);
//        } else {
//            auto pos = mapFromGlobal(m_mouseClickPos);
//            cursor = cursorForPosition(pos);
//        }

//        setTextCursor(cursor);

//        pasteText();
//    }
    pasteText();
}

void TextEdit::clickDeleteAction()
{
    if (textCursor().hasSelection()) {
        textCursor().removeSelectedText();
    } else {
        setTextCursor(m_highlightWordCacheCursor);
        textCursor().removeSelectedText();
    }
}

void TextEdit::clickOpenInFileManagerAction()
{
    DDesktopServices::showFileItem(filepath);
}

void TextEdit::onAddBookMark()
{
    addOrDeleteBookMark();
}

void TextEdit::onCancelBookMark()
{
    addOrDeleteBookMark();
}

void TextEdit::onMoveToPreviousBookMark()
{
    int line = getLineFromPoint(m_mouseClickPos);
    int index = m_listBookmark.indexOf(line);

    if (index == 0)
    {
       jumpToLine(m_listBookmark.last(),true);
    } else {
       jumpToLine(m_listBookmark.value(index - 1),true);
    }
}

void TextEdit::onMoveToNextBookMark()
{
    int line = getLineFromPoint(m_mouseClickPos);
    int index = m_listBookmark.indexOf(line);

    if(index == -1 && !m_listBookmark.isEmpty())
    {
        jumpToLine(m_listBookmark.last(),false);
    }

    if (index == m_listBookmark.count() - 1)
    {
       jumpToLine(m_listBookmark.first(),false);
    } else {
       jumpToLine(m_listBookmark.value(index + 1),false);
    }
}

void TextEdit::onClearBookMark()
{
    m_listBookmark.clear();
    qDebug() << "ClearBookMark:" << m_listBookmark;
    m_pLeftAreaWidget->m_bookMarkArea->update();
}

void TextEdit::copyWordUnderCursor()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(m_highlightWordCacheCursor.selectedText());
}

void TextEdit::cutWordUnderCursor()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(m_highlightWordCacheCursor.selectedText());

    setTextCursor(m_highlightWordCacheCursor);
    textCursor().removeSelectedText();
}

void TextEdit::slot_voiceReading()
{
    QProcess::startDetached("dbus-send  --print-reply --dest=com.iflytek.aiassistant /aiassistant/deepinmain com.iflytek.aiassistant.mainWindow.TextToSpeech");
    emit signal_readingPath();
}

void TextEdit::slot_stopReading()
{

    QProcess::startDetached("dbus-send  --print-reply --dest=com.iflytek.aiassistant /aiassistant/tts com.iflytek.aiassistant.tts.stopTTSDirectly");
}


void TextEdit::slot_dictation()
{
    QProcess::startDetached("dbus-send  --print-reply --dest=com.iflytek.aiassistant /aiassistant/deepinmain com.iflytek.aiassistant.mainWindow.SpeechToText");
}

void TextEdit::slot_translate()
{
    QProcess::startDetached("dbus-send  --print-reply --dest=com.iflytek.aiassistant /aiassistant/deepinmain com.iflytek.aiassistant.mainWindow.TextToTranslate");
}

QString TextEdit::getWordAtCursor()
{
    if (toPlainText().isEmpty()) {
        return "";
    } else {
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

        return cursor.selectedText();
    }
}

QString TextEdit::getWordAtMouse()
{
    if (toPlainText().isEmpty()) {
        return "";
    } else {
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

            return cursor.selectedText();
        } else {
            return "";
        }
    }
}

void TextEdit::toggleReadOnlyMode()
{
    if (m_readOnlyMode) {
        if (m_cursorMode == Overwrite) {
            emit cursorModeChanged(Overwrite);
        } else {
            emit cursorModeChanged(Insert);
        }

        m_readOnlyMode = false;
        setReadOnly(false);
        //setSpeechToTextEnabled(true); //此函数在shuttle上编译报错
        popupNotify(tr("Read-Only mode is off"));
    } else {
        m_readOnlyMode = true;
        setReadOnly(true);
        //setSpeechToTextEnabled(false);
        document()->clearUndoRedoStacks();
        popupNotify(tr("Read-Only mode is on"));
        emit cursorModeChanged(Readonly);
    }
}

void TextEdit::toggleComment(bool sister)
{
    if (m_readOnlyMode) {
        popupNotify(tr("Read-Only mode is on"));
        return;
    }

    const auto def = m_repository.definitionForFileName(QFileInfo(filepath).fileName());
    qDebug()<<"文件的名字"<<def.name();                  //Java ,C++,HTML,
    QString name= def.name();

    if (!def.filePath().isEmpty()) {
        if(sister){
        Comment::setComment(this, m_commentDefinition,name);
        }
        else {
        Comment::removeComment(this, m_commentDefinition,name);
        }
    } else {
        // do not need to prompt the user.
        // popupNotify(tr("File does not support syntax comments"));
    }
}

int TextEdit::getNextWordPosition(QTextCursor cursor, QTextCursor::MoveMode moveMode)
{
    // FIXME(rekols): if is empty text, it will crash.
    if (toPlainText().isEmpty()) {
        return 0;
    }

    // Move next char first.
    cursor.movePosition(QTextCursor::NextCharacter, moveMode);

    QChar currentChar = toPlainText().at(cursor.position() - 1);

    // Just to next non-space char if current char is space.
    if (currentChar.isSpace()) {
        while (cursor.position() < toPlainText().length() && currentChar.isSpace()) {
            cursor.movePosition(QTextCursor::NextCharacter, moveMode);
            currentChar = toPlainText().at(cursor.position() - 1);
        }
        while (cursor.position() < toPlainText().length() && !atWordSeparator(cursor.position())) {
            cursor.movePosition(QTextCursor::NextCharacter, moveMode);
        }
    }
    // Just to next word-separator char.
    else {
        while (cursor.position() < toPlainText().length() && !atWordSeparator(cursor.position())) {
            cursor.movePosition(QTextCursor::NextCharacter, moveMode);
        }
    }

    return cursor.position();
}

int TextEdit::getPrevWordPosition(QTextCursor cursor, QTextCursor::MoveMode moveMode)
{
    if (toPlainText().isEmpty()) {
        return 0;
    }

    // Move prev char first.
    cursor.movePosition(QTextCursor::PreviousCharacter, moveMode);

    QChar currentChar = toPlainText().at(cursor.position());

    // Just to next non-space char if current char is space.
    if (currentChar.isSpace()) {
        while (cursor.position() > 0 && currentChar.isSpace()) {
            cursor.movePosition(QTextCursor::PreviousCharacter, moveMode);
            currentChar = toPlainText().at(cursor.position());
        }
    }
    // Just to next word-separator char.
    else {
        while (cursor.position() > 0 && !atWordSeparator(cursor.position())) {
            cursor.movePosition(QTextCursor::PreviousCharacter, moveMode);
        }
    }

    return cursor.position();
}

bool TextEdit::atWordSeparator(int position)
{
    return m_wordSepartors.contains(QString(toPlainText().at(position)));
}

void TextEdit::showCursorBlink()
{
    // -1 表示恢复Qt的默认值
    QApplication::setCursorFlashTime(-1);
}

void TextEdit::hideCursorBlink()
{
    QApplication::setCursorFlashTime(0);
}

void TextEdit::setReadOnlyPermission(bool permission)
{
    if(m_bReadOnlyPermission){
        m_bReadOnlyPermission = permission;
        m_readOnlyMode = permission;
    }
    else if(!m_bReadOnlyPermission){
        m_bReadOnlyPermission = permission;
        if(permission)
            m_readOnlyMode = permission;
    }

    if(permission){
        setReadOnly(permission);
        emit cursorModeChanged(Readonly);
    }
    else if(m_bReadOnlyPermission){
        emit cursorModeChanged(Insert);
    }

//    if (m_bReadOnlyPermission == true)
//    {
//        m_bReadOnlyPermission = permission;
//        if (m_bReadOnlyPermission == true)
//        {
//            if(m_readOnlyMode == false)
//            {
//                m_readOnlyMode = true;
//            }

//            emit cursorModeChanged(Readonly);
//        }
//        else
//        {
//            if(m_readOnlyMode == true)
//            {
//                m_readOnlyMode = false;
//            }
//            emit cursorModeChanged(Insert);
//        }
//    }
//    else if (m_bReadOnlyPermission == false)
//    {
//        m_bReadOnlyPermission = permission;
//        if (m_bReadOnlyPermission == true)
//        {
//            if(m_readOnlyMode == false)
//            {
//                m_readOnlyMode = true;
//            }
//            emit cursorModeChanged(Readonly);
//        }
//        else
//        {
//            //emit cursorModeChanged(Insert);
//        }
//    }


}

bool TextEdit::getReadOnlyPermission()
{
    return m_bReadOnlyPermission;
}

bool TextEdit::getReadOnlyMode()
{
    return m_readOnlyMode;
}

void TextEdit::hideRightMenu()
{
    //arm平台全屏然后恢复窗口，右键菜单不会消失，所以加了这个函数
    if(m_rightMenu)
    m_rightMenu->hide();
}

void TextEdit::clearBlack()
{
    emit signal_clearBlack();
}

void TextEdit::bookMarkAreaPaintEvent(QPaintEvent *event)
{
    bookmarkwidget *bookMarkArea = m_pLeftAreaWidget->m_bookMarkArea;
    QPainter painter(bookMarkArea);
    QColor lineNumberAreaBackgroundColor;
    if (QColor(m_backgroundColor).lightness() < 128) {
        lineNumberAreaBackgroundColor = palette().brightText().color();
        lineNumberAreaBackgroundColor.setAlphaF(0.01);

        m_lineNumbersColor.setAlphaF(0.2);
    } else {
        lineNumberAreaBackgroundColor = palette().brightText().color();
        lineNumberAreaBackgroundColor.setAlphaF(0.03);
        m_lineNumbersColor.setAlphaF(0.3);
    }
    painter.fillRect(event->rect(), lineNumberAreaBackgroundColor);

    int blockNumber = getFirstVisibleBlockId();
    if(blockNumber > 0) {
        blockNumber -= 1;
    }
    QTextBlock block = document()->findBlockByNumber(blockNumber);
    QTextBlock prev_block = (blockNumber > 0) ? document()->findBlockByNumber(blockNumber-1) : block;
    int translate_y = (blockNumber > 0) ? -verticalScrollBar()->sliderPosition() : 0;

    int top = this->viewport()->geometry().top();

    // Adjust text position according to the previous "non entirely visible" block
    // if applicable. Also takes in consideration the document's margin offset.
    int additional_margin;
    if (blockNumber == 0)
        // Simply adjust to document's margin
    //    additional_margin = document()->documentMargin() -1 - this->verticalScrollBar()->sliderPosition();
        additional_margin = document()->documentMargin() - this->verticalScrollBar()->sliderPosition();
    else
        // Getting the height of the visible part of the previous "non entirely visible" block
//        additional_margin = document()->documentLayout()->blockBoundingRect(prev_block).toRect()
//                .translated(0, translate_y).intersected(this->viewport()->geometry()).height();

        additional_margin = document()->documentLayout()->blockBoundingRect(prev_block).toRect().translated(0, translate_y).bottom();

    // Shift the starting point
    additional_margin += 3;
    top += additional_margin;

    QTextBlock lineBlock;//第几行文本块
    QImage image;
    QImage scaleImage;
    int startLine = 0;//当前可见区域开始行号
    int startPoint = 0;//当前可见区域开始位置
    int imageTop = 0;//图片绘制位置
    int fontHeight = fontMetrics().height();
    double nBookmarkLineHeight = fontHeight;
    QList<int> list = m_listBookmark;
    bool bIsContains = false;

    if (!m_listBookmark.contains(m_nBookMarkHoverLine) && m_nBookMarkHoverLine != -1 && m_nBookMarkHoverLine <= blockCount()) {
        list << m_nBookMarkHoverLine;
    } else {
        bIsContains = true;
    }

    foreach (auto line, list) {

        startLine = blockNumber + 1;

        if (line < startLine) {

        } else {
            startPoint = top;

            while (line - startLine > 0) {
                lineBlock = document()->findBlockByNumber(startLine - 1);
                startPoint += document()->documentLayout()->blockBoundingRect(lineBlock).height();
                startLine++;
            }

            if (line == m_nBookMarkHoverLine && !bIsContains) {
                if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::ColorType::DarkType) {
                    image = QImage(":/images/like_hover_dark.svg");
                } else {
                    image = QImage(":/images/like_hover_light.svg");
                }
            } else {
                image = QImage(":/images/bookmark.svg");
            }

            if(line > 0)
            {
                lineBlock = document()->findBlockByNumber(line - 1);
                if (!lineBlock.isVisible()) {
                    continue;
                }

                if(fontHeight > image.height()) {
                    scaleImage = image;
                } else {
                    double scale = nBookmarkLineHeight/image.height();
                    int nScaleWidth = static_cast<int>(scale*image.width());
                    scaleImage = image.scaled(static_cast<int>(scale*image.height()),nScaleWidth);
                }
                if(scaleImage.height() > 0.8 * image.height()) {
                    imageTop = startPoint - 2 + (document()->documentLayout()->blockBoundingRect(lineBlock).toRect().height() - scaleImage.height())/2;
                } else {
                    imageTop = startPoint - 3 + (document()->documentLayout()->blockBoundingRect(lineBlock).toRect().height() - scaleImage.height())/2;
                }
                if(document()->documentLayout()->blockBoundingRect(lineBlock).height() > 1.5*fontHeight) {
                    imageTop = startPoint + static_cast<int>(qFabs(fontHeight - scaleImage.height())/2);
                }

                painter.drawImage(5,imageTop,scaleImage);

            }
        }
    }
}

int TextEdit::getLineFromPoint(const QPoint &point)
{
    int blockNumber = getFirstVisibleBlockId();
    if(blockNumber > 0)
    {
        blockNumber -= 1;
    }
    QTextBlock block = document()->findBlockByNumber(blockNumber);
    QTextBlock prev_block = (blockNumber > 0) ? document()->findBlockByNumber(blockNumber-1) : block;
    int translate_y = (blockNumber > 0) ? -verticalScrollBar()->sliderPosition() : 0;

    int top = this->viewport()->geometry().top();

    // Adjust text position according to the previous "non entirely visible" block
    // if applicable. Also takes in consideration the document's margin offset.
    int additional_margin;
    if (blockNumber == 0)
        // Simply adjust to documeline - 1nt's margin
        additional_margin = document()->documentMargin() -1 - this->verticalScrollBar()->sliderPosition();
    else
        // Getting the height of the visible part of the previous "non entirely visible" block
//        additional_margin = document()->documentLayout()->blockBoundingRect(prev_block).toRect()
//                .translated(0, translate_y).intersected(this->viewport()->geometry()).height();
        additional_margin = document()->documentLayout()->blockBoundingRect(prev_block).toRect()
                .translated(0, translate_y).bottom();

    // Shift the starting point
    top += additional_margin;

    int cursorPoint = point.y() - top;//鼠标点击位置 - 起始位置
    int line = 0;//当前可见区域第几行
    block = document()->findBlockByNumber(blockNumber);//当前可见区域第一个文本块

    while (cursorPoint >= 0) {
        cursorPoint -= document()->documentLayout()->blockBoundingRect(block).height();
        block = block.next();
        if(line + blockNumber > blockCount())
        {
            //line++;
            return line + blockNumber;;
        }
        line++;
    }
    return line + blockNumber;
}

void TextEdit::addOrDeleteBookMark()
{
    int line = 0;
    if (m_bIsShortCut) {
        line = getCurrentLine();
        m_bIsShortCut = false;
    } else {
        line = getLineFromPoint(m_mouseClickPos);
    }

    if (line > blockCount()) {
         return;
    }

    if (m_listBookmark.contains(line)) {
        m_listBookmark.removeOne(line);
        m_nBookMarkHoverLine = -1;
        qDebug() << "DeleteBookMark:" << line << m_listBookmark;
    } else {
        m_listBookmark.push_back(line);
        qDebug() << "AddBookMark:" << line << m_listBookmark;
    }

    m_pLeftAreaWidget->m_bookMarkArea->update();
}

void TextEdit::moveToPreviousBookMark()
{
    int line = getCurrentLine();
    int index = m_listBookmark.indexOf(line);

    if(index == -1 && !m_listBookmark.isEmpty())
    {
        jumpToLine(m_listBookmark.last(),false);
        return;
    }

    if (index == 0)
    {
       jumpToLine(m_listBookmark.last(),false);
    } else {
       jumpToLine(m_listBookmark.value(index - 1),false);
    }
}

void TextEdit::moveToNextBookMark()
{
    int line = getCurrentLine();
    int index = m_listBookmark.indexOf(line);

    if(index == -1 && !m_listBookmark.isEmpty())
    {
        jumpToLine(m_listBookmark.first(),false);
        return;
    }

    if (index == m_listBookmark.count() - 1)
    {
       jumpToLine(m_listBookmark.first(),false);
    } else {
       jumpToLine(m_listBookmark.value(index + 1),false);
    }
}

void TextEdit::checkBookmarkLineMove(int from, int charsRemoved, int charsAdded)
{
    Q_UNUSED(charsRemoved);
    Q_UNUSED(charsAdded);

    if (m_bIsFileOpen) {
        return;
    }

    if(m_nLines != blockCount())
    {
        QTextCursor cursor = textCursor();
        int nAddorDeleteLine = document()->findBlock(from).blockNumber() + 1;
        int currLine = textCursor().blockNumber() + 1;

        if (m_nLines > blockCount()) {
            foreach (const auto line, m_listBookmark) {
                if (m_nSelectEndLine != -1) {
                    if (nAddorDeleteLine < line && line <= m_nSelectEndLine) {
                        m_listBookmark.removeOne(line);
                    } else if (line > m_nSelectEndLine) {
                        m_listBookmark.replace(m_listBookmark.indexOf(line),line - m_nSelectEndLine + nAddorDeleteLine);
                    }
                } else {
                    if (line == currLine + 1) {
                        m_listBookmark.removeOne(currLine + 1);
                    } else if (line > currLine + 1) {
                        m_listBookmark.replace(m_listBookmark.indexOf(line),line  - m_nLines + blockCount());
                    }
                }
            }
        } else {
            foreach (const auto line, m_listBookmark) {

                if (nAddorDeleteLine < line) {
                    m_listBookmark.replace(m_listBookmark.indexOf(line),line + blockCount() - m_nLines);
                }
            }
        }
    }
    m_nLines = blockCount();
}

void TextEdit::flodOrUnflodAllLevel(bool isFlod)
{
    foreach (auto line, m_listFlodFlag) {
        if (document()->findBlockByNumber(line + 1).isVisible() == isFlod) {

            if (document()->findBlockByNumber(line + 1).text().contains("{")
                    && document()->findBlockByNumber(line + 1).text().trimmed().startsWith("{")) {
                getNeedControlLine(line, !isFlod);
            }
            if (document()->findBlockByNumber(line).text().contains("{")
                    && !document()->findBlockByNumber(line).text().trimmed().startsWith("{")) {
                getNeedControlLine(line, !isFlod);

            }
            if (document()->findBlockByNumber(line).text().contains("{")
                    && document()->findBlockByNumber(line).text().trimmed().startsWith("{")) {
                getNeedControlLine(line + 1, !isFlod, 1, false);

            }
        }
    }
    m_pLeftAreaWidget->m_flodArea->update();
    lineNumberArea->update();
    m_pLeftAreaWidget->m_bookMarkArea->update();
    viewport()->update();
    document()->adjustSize();

}

void TextEdit::flodOrUnflodCurrentLevel(bool isFlod)
{
    int line = getLineFromPoint(m_mouseClickPos);
    getNeedControlLine(line - 1, !isFlod);
    m_pLeftAreaWidget->m_flodArea->update();
    lineNumberArea->update();
    m_pLeftAreaWidget->m_bookMarkArea->update();
    viewport()->update();
    document()->adjustSize();

}

void TextEdit::getHideRowContent(int iLine)
{
    bool isFirstLine = true;
    QTextBlock block = document()->findBlockByNumber(iLine);
    int existLeftSubbrackets = 0, existRightSubbrackets = 0;

    while (block.isValid()) {
        if (block.text().contains("{") && !block.text().contains("}")) {
            existLeftSubbrackets ++;
        } else if (block.text().contains("}") && !block.text().contains("{")) {
            existRightSubbrackets ++;
        } else if (block.text().contains("}") && block.text().contains("{")) {
            for (int i = 0 ; i < block.text().size() ; ++i) {
                if (block.text().at(i) == "{") {
                    existLeftSubbrackets++;
                } else if (block.text().at(i) == "}" && !isFirstLine) {
                    existRightSubbrackets++;
                }
                if (existLeftSubbrackets == existRightSubbrackets &&
                        existLeftSubbrackets != 0) {
                    break;
                }
            }
        }

        if (existLeftSubbrackets == existRightSubbrackets &&
                existLeftSubbrackets != 0) {
            if (!block.text().contains("{")) {
                m_foldCodeShow->appendText(block.text());
                break;
            } else {
                break;
            }
        }
        if (isFirstLine) {
            isFirstLine = false;
        } else {
            m_foldCodeShow->appendText(block.text());
        }
        block = block.next();
        iLine++;
    }
}

bool TextEdit::isNeedShowFoldIcon(QTextBlock block)
{
    QString blockText = block.text();
    bool hasFindLeft = false; // 是否已经找到当前行第一个左括号
    int rightNum = 0, leftNum = 0;//右括号数目、左括号数目
    for (int i = 0 ; i < blockText.count(); ++i) {
        if (blockText.at(i) == "}" && hasFindLeft) {
            rightNum++;
        } else if (blockText.at(i) == "{") {
            if (!hasFindLeft)
                hasFindLeft = true;
            leftNum++;
        }
    }

    if (rightNum == leftNum) {
        return  false;
    } else {
        return  true;
    }

}

int TextEdit::getHighLightRowContentLineNum(int iLine)
{
    bool isFirstLine = true;
    QTextBlock block = document()->findBlockByNumber(iLine);
    int existLeftSubbrackets = 0, existRightSubbrackets = 0;
    while (block.isValid()) {
        if (block.text().contains("{") && !block.text().contains("}")) {
            existLeftSubbrackets ++;
        } else if (block.text().contains("}") && !block.text().contains("{")) {
            existRightSubbrackets ++;
        } else if (block.text().contains("}") && block.text().contains("{")) {
            for (int i = 0 ; i < block.text().size() ; ++i) {
                if (block.text().at(i) == "{") {
                    existLeftSubbrackets++;
                } else if (block.text().at(i) == "}" && !isFirstLine) {
                    existRightSubbrackets++;
                }
                if (existLeftSubbrackets == existRightSubbrackets &&
                        existLeftSubbrackets != 0) {
                    break;
                }
            }
        }

        if (existLeftSubbrackets == existRightSubbrackets &&
                existLeftSubbrackets != 0) {
            if (!block.text().contains("{")) {
                break;
            } else {
                break;
            }
        }
        if (isFirstLine) {
            isFirstLine = false;
        }
        block = block.next();
        iLine++;
    }
    return  iLine;
}

int TextEdit::getLinePosByLineNum(int iLine)
{
    int blockNumber = getFirstVisibleBlockId();
    QTextBlock block = document()->findBlockByNumber(blockNumber);
    QTextBlock prev_block = (blockNumber > 0) ? document()->findBlockByNumber(blockNumber - 1) : block;
    int translate_y = (blockNumber > 0) ? -verticalScrollBar()->sliderPosition() : 0;

    int top = this->viewport()->geometry().top();
    int additional_margin;
    if (blockNumber == 0)
        // Simply adjust to document's margin
        //     additional_margin = document()->documentMargin() - 1 - this->verticalScrollBar()->sliderPosition();
        additional_margin = document()->documentMargin() - this->verticalScrollBar()->sliderPosition();
    else
        // Getting the height of the visible part of the previous "non entirely visible" block
        additional_margin = document()->documentLayout()->blockBoundingRect(prev_block).toRect()
                            .translated(0, translate_y).intersected(this->viewport()->geometry()).height();

    // Shift the starting point
    additional_margin += 3;
    top += additional_margin;

    int bottom = top + document()->documentLayout()->blockBoundingRect(block).height();
    while (block.isValid()) {
        if (blockNumber == iLine)
            return top;
        block = block.next();
        top = bottom;
        bottom = top + document()->documentLayout()->blockBoundingRect(block).height();
        ++blockNumber;
    }
    return -1;
}

void TextEdit::setIsFileOpen()
{
    m_bIsFileOpen = true;
}

void TextEdit::setTextFinished()
{
    m_bIsFileOpen = false;
    m_nLines = blockCount();

    QStringList bookmarkList = readHistoryRecordofBookmark();
    QStringList filePathList = readHistoryRecordofFilePath();
    QList<int> linesList;
    QString qstrPath = "*[" + filepath + "]*";

    if (filePathList.contains(qstrPath)) {
        int index = 2;
        QString qstrLines = bookmarkList.value(filePathList.indexOf(qstrPath));
        QString sign;
        for (int i = 0;i < qstrLines.count()-1;i++) {
            sign = qstrLines.at(i);
            sign.append(qstrLines.at(i + 1));
            if(sign == ",*" || sign ==")*") {
                linesList << qstrLines.mid(index,i - index).toInt();
                index = i + 2;
            }
        }
    }

    foreach (const auto line, linesList) {
        if (line <= document()->blockCount()) {
            m_listBookmark << line;
        }
    }
//    qDebug() << m_listBookmark << document()->blockCount();
}

QStringList TextEdit::readHistoryRecord()
{
    QString history = m_settings->settings->option("advance.editor.browsing_history_file")->value().toString();
    QStringList historyList;
    int nLeftPosition = history.indexOf("*{");
    int nRightPosition = history.indexOf("}*");

    while (nLeftPosition != -1) {
        historyList << history.mid(nLeftPosition,nRightPosition + 2 - nLeftPosition);
        nLeftPosition = history.indexOf("*{",nLeftPosition + 2);
        nRightPosition = history.indexOf("}*",nRightPosition + 2);
    }

    return historyList;
}

QStringList TextEdit::readHistoryRecordofBookmark()
{
    QString history = m_settings->settings->option("advance.editor.browsing_history_file")->value().toString();
    QStringList bookmarkList;
    int nLeftPosition = history.indexOf("*(");
    int nRightPosition = history.indexOf(")*");

    while (nLeftPosition != -1) {
        bookmarkList << history.mid(nLeftPosition,nRightPosition + 2 - nLeftPosition);
        nLeftPosition = history.indexOf("*(",nLeftPosition + 2);
        nRightPosition = history.indexOf(")*",nRightPosition + 2);
    }

    return bookmarkList;
}

QStringList TextEdit::readHistoryRecordofFilePath()
{
    QString history = m_settings->settings->option("advance.editor.browsing_history_file")->value().toString();
    QStringList filePathList;
    int nLeftPosition = history.indexOf("*[");
    int nRightPosition = history.indexOf("]*");

    while (nLeftPosition != -1) {
        filePathList << history.mid(nLeftPosition,nRightPosition + 2 - nLeftPosition);
        nLeftPosition = history.indexOf("*[",nLeftPosition + 2);
        nRightPosition = history.indexOf("]*",nRightPosition + 2);
    }

    return filePathList;
}

void TextEdit::writeHistoryRecord()
{
    qDebug() << "writeHistoryRecord";
    QString history = m_settings->settings->option("advance.editor.browsing_history_file")->value().toString();
    QStringList historyList = readHistoryRecord();

    int nLeftPosition = history.indexOf(filepath);
    int nRightPosition = history.indexOf("}*",nLeftPosition);
    if (history.contains(filepath)) {
        history.remove(nLeftPosition - 4,nRightPosition + 6 - nLeftPosition);
    }

    if (!m_listBookmark.isEmpty()) {

        QString filePathandBookmarkLine = "*{*[" + filepath + "]**(";

        foreach (auto line, m_listBookmark) {
            if (line <= 0) {
                line = 1;
            }

            filePathandBookmarkLine += QString::number(line) + ",*";
        }

        filePathandBookmarkLine.remove(filePathandBookmarkLine.count() - 2,2);

        if (historyList.count() <= 5) {
//            qDebug() << "writeHistoryRecord()" <<  filePathandBookmarkLine + ")}" + history;
            m_settings->settings->option("advance.editor.browsing_history_file")->setValue(filePathandBookmarkLine + ")*}*" + history);
        } else {
            history.remove(historyList.last());
            m_settings->settings->option("advance.editor.browsing_history_file")->setValue(filePathandBookmarkLine + ")*}*" + history);
        }
    } else {
        m_settings->settings->option("advance.editor.browsing_history_file")->setValue(history);
    }
}

void TextEdit::isMarkCurrentLine(bool isMark, QString strColor)
{
    if (isMark) {
        QTextEdit::ExtraSelection selection;
        QList<QTextEdit::ExtraSelection> listSelections;
        QList<QTextEdit::ExtraSelection> wordMarkSelections = m_wordMarkSelections;
        selection.cursor = textCursor();

        if(!this->textCursor().hasSelection()) {
            selection.cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
            selection.cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        }

        QString markText = "" ,currentMarkText = textCursor().selection().toPlainText();
        int pos = currentMarkText.indexOf("\n");

        if (pos != -1) {
            selection.cursor.setPosition(textCursor().selectionStart(), QTextCursor::MoveAnchor);
            selection.cursor.setPosition(textCursor().selectionStart() + pos, QTextCursor::KeepAnchor);

            if (selection.cursor.selectedText() != "") {
                appendExtraSelection(wordMarkSelections,selection,strColor,&listSelections);
            }

            while (pos != -1) {
                selection.cursor.setPosition(textCursor().selectionStart() + pos + 1, QTextCursor::MoveAnchor);
                pos = currentMarkText.indexOf("\n",pos + 1);

                if(pos == -1)
                {
                    break;
                }

                selection.cursor.setPosition(textCursor().selectionStart() + pos, QTextCursor::KeepAnchor);

                if (selection.cursor.selectedText() != "") {
                    wordMarkSelections = m_wordMarkSelections;
                    appendExtraSelection(wordMarkSelections,selection,strColor,&listSelections);
                }

            }
            selection.cursor.setPosition(textCursor().selectionEnd(), QTextCursor::KeepAnchor);
        }

        if (selection.cursor.selectedText() != "") {
            wordMarkSelections = m_wordMarkSelections;
            appendExtraSelection(wordMarkSelections,selection,strColor,&listSelections);
        }

        if (!listSelections.isEmpty()) {
            m_mapWordMarkSelections.insert(m_mapWordMarkSelections.count(),listSelections);
        }

    } else {
//        m_wordMarkSelections.removeLast();
//        updateHighlightLineSelection();
        for (int i = 0 ; i < m_wordMarkSelections.size(); ++i) {
            QTextCursor curson = m_wordMarkSelections.at(i).cursor;
            curson.movePosition(QTextCursor::EndOfLine);
            QTextCursor currentCurson = textCursor();
            currentCurson.movePosition(QTextCursor::EndOfLine);
     //       if (m_wordMarkSelections.at(i).cursor == textCursor()) {
            if(curson == currentCurson) {
                m_wordMarkSelections.removeAt(i);
//                renderAllSelections();
                break;
            }
        }
    }
}

void TextEdit::isMarkAllLine(bool isMark, QString strColor)
{
    if (isMark) {
        if (this->textCursor().hasSelection()) {
            QList<QTextEdit::ExtraSelection> wordMarkSelections = m_wordMarkSelections;
            QList<QTextEdit::ExtraSelection> listExtraSelection;
            QList<QTextEdit::ExtraSelection> listSelections;
            QTextEdit::ExtraSelection  extraSelection;
            QTextCharFormat format;
            format.setBackground(QColor(strColor));
            extraSelection.cursor = textCursor();
            extraSelection.format = format;
            if (!updateKeywordSelections(textCursor().selection().toPlainText(),format,&listExtraSelection)) {
                isMarkCurrentLine(true,strColor);
            } else {
                for (int j = 0;j < listExtraSelection.count();j++) {
                    wordMarkSelections = m_wordMarkSelections;
                    appendExtraSelection(wordMarkSelections,listExtraSelection.value(j),strColor,&listSelections);
                }

                if (!listSelections.isEmpty()) {
                    m_mapWordMarkSelections.insert(m_mapWordMarkSelections.count(),listSelections);
                }
            }
        } else {
//            m_wordMarkSelections.clear();
//            m_mapWordMarkSelections.clear();
            QTextEdit::ExtraSelection selection;
            QTextEdit::ExtraSelection currentSelection;
            QList<QTextEdit::ExtraSelection> listSelections;
            QList<QTextEdit::ExtraSelection> wordMarkSelections = m_wordMarkSelections;

            selection.format.setBackground(QColor(strColor));
            selection.cursor = textCursor();
            currentSelection.cursor = textCursor();
            currentSelection.cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
            currentSelection.cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
//            selection.cursor.select(QTextCursor::Document);

            QString markText = "" ,currentMarkText = currentSelection.cursor.selection().toPlainText();
            int pos = currentMarkText.indexOf("\n");

            if (pos != -1) {
                selection.cursor.setPosition(currentSelection.cursor.selectionStart(), QTextCursor::MoveAnchor);
                selection.cursor.setPosition(currentSelection.cursor.selectionStart() + pos, QTextCursor::KeepAnchor);

                if (selection.cursor.selectedText() != "") {
                    appendExtraSelection(wordMarkSelections,selection,strColor,&listSelections);
                }

                while (pos != -1) {
                       selection.cursor.setPosition(currentSelection.cursor.selectionStart() + pos + 1, QTextCursor::MoveAnchor);
                       pos = currentMarkText.indexOf("\n",pos + 1);
                       if(pos == -1)
                       {
                           break;
                       }

                       selection.cursor.setPosition(currentSelection.cursor.selectionStart() + pos, QTextCursor::KeepAnchor);

                       if (selection.cursor.selectedText() != "") {
                           wordMarkSelections = m_wordMarkSelections;
                           appendExtraSelection(wordMarkSelections,selection,strColor,&listSelections);
                       }

                }
                selection.cursor.setPosition(currentSelection.cursor.selectionEnd(), QTextCursor::KeepAnchor);
            }

            if (selection.cursor.selectedText() != "") {
                wordMarkSelections = m_wordMarkSelections;
                appendExtraSelection(wordMarkSelections,selection,strColor,&listSelections);
            }

            if (!listSelections.isEmpty()) {
                m_mapWordMarkSelections.insert(m_mapWordMarkSelections.count(),listSelections);
            }

//            m_markAllSelection = selection;
        }
    } else {
        m_wordMarkSelections.clear();
        m_mapWordMarkSelections.clear();

        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(QColor(strColor));
        selection.cursor = textCursor();
        selection.cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        selection.cursor.clearSelection();
        m_markAllSelection = selection;
    }
}

void TextEdit::cancelLastMark()
{
    if (m_wordMarkSelections.size() < 1) {
        return;
    }
    QList<QTextEdit::ExtraSelection> wordMarkSelections = m_wordMarkSelections;
    int nRemCount = 0;

    for (int j = 0;j < m_mapWordMarkSelections.count();j++) {
        auto listSelections = m_mapWordMarkSelections.value(j);

        if(containsExtraSelection(listSelections,m_wordMarkSelections.value(m_wordMarkSelections.count() - 1))) {
            for (int k = 0;k < listSelections.count();k++) {
                for (int i = 0;i < wordMarkSelections.count();i++) {
                    if (listSelections.value(k).cursor == wordMarkSelections.at(i).cursor
                            && listSelections.value(k).format ==  wordMarkSelections.at(i).format) {
                        m_wordMarkSelections.removeAt(i - nRemCount);
                        nRemCount++;
                    }
                }
            }

            m_mapWordMarkSelections.remove(j);
            break;
        }
    }
    updateHighlightLineSelection();
}

void TextEdit::markSelectWord()
{
    bool isFind  = false;
    for (int i = 0 ; i < m_wordMarkSelections.size(); ++i) {
        QTextCursor curson = m_wordMarkSelections.at(i).cursor;
        curson.movePosition(QTextCursor::EndOfLine);
        QTextCursor currentCurson = textCursor();
        currentCurson.movePosition(QTextCursor::EndOfLine);
 //       if (m_wordMarkSelections.at(i).cursor == textCursor()) {
        if(curson == currentCurson) {
            isFind = true;
            m_wordMarkSelections.removeAt(i);
            renderAllSelections();
            break;
        }
    }
    if (!isFind) {
        isMarkCurrentLine(true, STYLE_COLOR_1);
        renderAllSelections();
    }
}

void TextEdit::updateMark(int from, int charsRemoved, int charsAdded)
{
//    Q_UNUSED(charsRemoved);

    if (m_readOnlyMode) {
//         if(charsAdded > 0) {
//             textCursor().deletePreviousChar();
//
        undo();
        return;
    }

    if (m_bIsFileOpen) {
        return;
    }

    int nStartPos = 0,nEndPos = 0,nCurrentPos = 0;
    QTextEdit::ExtraSelection selection;
    QList<QTextEdit::ExtraSelection> listSelections;
    QList<QTextEdit::ExtraSelection> wordMarkSelections = m_wordMarkSelections;
    QColor strColor;
    nCurrentPos = textCursor().position();

    if (charsRemoved > 0) {
        QList<int> listRemoveItem;

        for (int i = 0;i < wordMarkSelections.count();i++) {

            nEndPos = wordMarkSelections.value(i).cursor.selectionEnd();
            nStartPos = wordMarkSelections.value(i).cursor.selectionStart();
            strColor = wordMarkSelections.value(i).format.background().color();
            if (m_nSelectEndLine != -1) {
                if (m_nSelectStart <= nStartPos && m_nSelectEnd >= nEndPos) {
                    listRemoveItem.append(i);
                }
            } else {
                if (wordMarkSelections.value(i).cursor.selection().toPlainText().isEmpty()) {
                    m_wordMarkSelections.removeAt(i);
                }
            }
        }

        for (int j = 0;j < listRemoveItem.count();j++) {
            m_wordMarkSelections.removeAt(listRemoveItem.value(j) - j);
        }
    }

    if (charsAdded > 0) {
        for (int i = 0;i < wordMarkSelections.count();i++) {
            nEndPos = wordMarkSelections.value(i).cursor.selectionEnd();
            nStartPos = wordMarkSelections.value(i).cursor.selectionStart();
            strColor = wordMarkSelections.value(i).format.background().color();

            if (nCurrentPos > nStartPos && nCurrentPos < nEndPos) {

                m_wordMarkSelections.removeAt(i);
                selection.format.setBackground(strColor);
                selection.cursor = textCursor();

                QTextEdit::ExtraSelection preSelection;
                if (m_bIsInputMethod) {
                    selection.cursor.setPosition(nStartPos, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(nCurrentPos - m_qstrCommitString.count(), QTextCursor::KeepAnchor);
                    m_wordMarkSelections.insert(i,selection);

                    preSelection.cursor = selection.cursor;
                    preSelection.format = selection.format;

                    selection.cursor.setPosition(nCurrentPos, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(nEndPos, QTextCursor::KeepAnchor);
                    m_wordMarkSelections.insert(i + 1,selection);

                    m_bIsInputMethod = false;
                } else {
                    selection.cursor.setPosition(nStartPos, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(from, QTextCursor::KeepAnchor);
                    m_wordMarkSelections.insert(i,selection);

                    preSelection.cursor = selection.cursor;
                    preSelection.format = selection.format;

                    selection.cursor.setPosition(nCurrentPos, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(nEndPos, QTextCursor::KeepAnchor);
                    m_wordMarkSelections.insert(i + 1,selection);
                }

                bool bIsFind = false;
                for (int j = 0;j < m_mapWordMarkSelections.count();j++) {
                    auto list = m_mapWordMarkSelections.value(j);
                    for (int k = 0;k < list.count();k++) {
                        if (list.value(k).cursor == wordMarkSelections.value(i).cursor
                                && list.value(k).format == wordMarkSelections.value(i).format) {
                            list.removeAt(k);
                            listSelections = list;
                            listSelections.insert(k,preSelection);
                            listSelections.insert(k + 1,selection);
                            bIsFind = true;
                            break;
                        }
                    }

                    if (bIsFind) {
                        m_mapWordMarkSelections.remove(j);
                        m_mapWordMarkSelections.insert(j,listSelections);
                        break;
                    }
                }
                break;

            } else if (nCurrentPos == nEndPos) {
                m_wordMarkSelections.removeAt(i);
                selection.format.setBackground(strColor);
                selection.cursor = textCursor();

                if (m_bIsInputMethod) {
                    selection.cursor.setPosition(nStartPos, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(nEndPos - m_qstrCommitString.count(), QTextCursor::KeepAnchor);
                    m_bIsInputMethod = false;
                } else {
                    selection.cursor.setPosition(nStartPos, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(from, QTextCursor::KeepAnchor);
                }

                m_wordMarkSelections.insert(i,selection);

                bool bIsFind = false;
                for (int j = 0;j < m_mapWordMarkSelections.count();j++) {
                    auto list = m_mapWordMarkSelections.value(j);
                    for (int k = 0;k < list.count();k++) {
                        if (list.value(k).cursor == wordMarkSelections.value(i).cursor
                                && list.value(k).format == wordMarkSelections.value(i).format) {
                            list.removeAt(k);
                            listSelections = list;
                            listSelections.insert(k,selection);
                            bIsFind = true;
                            break;
                        }
                    }

                    if (bIsFind) {
                        m_mapWordMarkSelections.remove(j);
                        m_mapWordMarkSelections.insert(j,listSelections);
                        break;
                    }
                }
                break;
            }
        }
    }
    renderAllSelections();
}

void TextEdit::setCursorStart(int _)
{
    m_cursorStart = _;
}
void TextEdit::completionWord(QString word)
{
    QString wordAtCursor = getWordAtCursor();
    QTextCursor cursor = textCursor();

    QString completionString = word.remove(0, wordAtCursor.size());
    if (completionString.size() > 0) {
        cursor = textCursor();
        cursor.insertText(completionString);

        setTextCursor(cursor);
    }
}

bool TextEdit::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        m_mouseClickPos = mouseEvent->pos();

        emit click();

        if (object == m_pLeftAreaWidget->m_bookMarkArea) {
            m_mouseClickPos = mouseEvent->pos();
            if (mouseEvent->button() == Qt::RightButton) {
                m_rightMenu->clear();
                int line = getLineFromPoint(mouseEvent->pos());
                if (m_listBookmark.contains(line)) {
                    m_rightMenu->addAction(m_cancelBookMarkAction);
                    if (m_listBookmark.count() > 1) {
                        m_rightMenu->addAction(m_preBookMarkAction);
                        m_rightMenu->addAction(m_nextBookMarkAction);
                    }
                } else {
                    m_rightMenu->addAction(m_addBookMarkAction);
                }

                if (!m_listBookmark.isEmpty()) {
                    m_rightMenu->addAction(m_clearBookMarkAction);
                }

                m_rightMenu->exec(mouseEvent->globalPos());
            } else {
                qDebug() << "bookMarkAreaClicked:" << mouseEvent->pos();
                addOrDeleteBookMark();
            }
        } else if (object == m_pLeftAreaWidget->m_flodArea) {
            m_foldCodeShow->hide();
            if (mouseEvent->button() == Qt::LeftButton) {
                int line = getLineFromPoint(mouseEvent->pos());
                m_markFoldHighLightSelections.clear();
                renderAllSelections();
                //判断下一行是否可见，并且下一行是以左括号开头的（类似c++文件中的函数定义的书写）则是从该行开始做折叠处理，但该行不隐藏
                if (document()->findBlockByNumber(line).isVisible() && document()->findBlockByNumber(line).text().contains("{")
                        && document()->findBlockByNumber(line).text().trimmed().startsWith("{")) {
                    getNeedControlLine(line - 1, false);
                    document()->adjustSize();
                } else if (!document()->findBlockByNumber(line).isVisible() && document()->findBlockByNumber(line).text().contains("{")
                           && document()->findBlockByNumber(line).text().trimmed().startsWith("{")) {
                    getNeedControlLine(line - 1, true);
                    document()->adjustSize();
                }

                //判断下一行是否可见，左括号存在于该行（类似c++文件中的if else 书写），但不是左括号开头的，则是从该行开始做折叠处理，但该行不隐藏
                if (document()->findBlockByNumber(line).isVisible() && document()->findBlockByNumber(line - 1).text().contains("{")
                        && !document()->findBlockByNumber(line - 1).text().trimmed().startsWith("{")) {
                    getNeedControlLine(line - 1, false);
                    document()->adjustSize();

                } else if (!document()->findBlockByNumber(line).isVisible() && document()->findBlockByNumber(line - 1).text().contains("{")
                           && !document()->findBlockByNumber(line - 1).text().trimmed().startsWith("{")) {
                    getNeedControlLine(line - 1, true);
                    document()->adjustSize();
                }

                //判断下一行是否可见，左括号存在于该行（类似文件中的json文件书写，第一行就存在左括号开头的现象），且是左括号开头的，则是从下一行开始做折叠处理
                if (document()->findBlockByNumber(line).isVisible() && document()->findBlockByNumber(line - 1).text().contains("{")
                        && document()->findBlockByNumber(line - 1).text().trimmed().startsWith("{")) {
                    getNeedControlLine(line, false, 1, false);
                    document()->adjustSize();

                } else if (!document()->findBlockByNumber(line).isVisible() && document()->findBlockByNumber(line - 1).text().contains("{")
                           && document()->findBlockByNumber(line - 1).text().trimmed().startsWith("{")) {
                    getNeedControlLine(line, true, 1, false);
                    document()->adjustSize();
                }
                viewport()->update();
                m_pLeftAreaWidget->m_flodArea->update();
                m_pLeftAreaWidget->m_linenumberarea->update();
                m_pLeftAreaWidget->m_bookMarkArea->update();
            } else {
                m_mouseClickPos = mouseEvent->pos();
                m_rightMenu->clear();
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
                m_rightMenu->exec(mouseEvent->globalPos());
            }


        }
    } else if (event->type() == QEvent::HoverMove) {
        QHoverEvent *hoverEvent = static_cast<QHoverEvent *>(event);
        int line = getLineFromPoint(hoverEvent->pos());

        if (object == m_pLeftAreaWidget->m_bookMarkArea) {
            m_nBookMarkHoverLine = line;
            m_pLeftAreaWidget->m_bookMarkArea->update();
        }

        if (object == m_pLeftAreaWidget->m_flodArea) {
            m_markFoldHighLightSelections.clear();
            renderAllSelections();
            QHoverEvent *hoverEvent = static_cast<QHoverEvent *>(event);
            int line = getLineFromPoint(hoverEvent->pos());
            if (m_listFlodIconPos.contains(line - 1)) {
                if (!document()->findBlockByNumber(line).isVisible()) {
                    m_foldCodeShow->clear();
                    getHideRowContent(line - 1);
                    m_foldCodeShow->show();
                    m_foldCodeShow->move(0, getLinePosByLineNum(line - 1) + 5);
                } else {
                    QTextCursor previousCursor = textCursor();
                    int ivalue =  this->verticalScrollBar()->value();
                    int iLine =  getHighLightRowContentLineNum(line - 1);
                    QTextEdit::ExtraSelection selection;

                    selection.format.setBackground(QColor(FOLD_HIGHLIGHT_COLOR));
                    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
                    QTextBlock startblock;
                    if (line == 1 && document()->findBlockByNumber(0).text().trimmed().startsWith("{")) {
                        startblock = document()->findBlockByNumber(line - 1);
                    } else {
                        startblock = document()->findBlockByNumber(line);
                    }
                    QTextCursor beginCursor(startblock);
                    beginCursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
                    if (line == 1 && document()->findBlockByNumber(0).text().trimmed().startsWith("{")) {
                        beginCursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, iLine - line + 2);
                    } else {
                        beginCursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, iLine - line + 1);
                    }
                    if (iLine == document()->blockCount() - 1)
                        beginCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                    setTextCursor(beginCursor);
                    selection.cursor = textCursor();
                    m_markFoldHighLightSelections.push_back(selection);

                    renderAllSelections();
                    setTextCursor(QTextCursor(document()->findBlockByNumber(line - 1)));
                    this->verticalScrollBar()->setValue(ivalue);

                }
            } else {
                m_foldCodeShow->hide();
            }
        }
    } else if (event->type() == QEvent::HoverLeave) {
        if (object == m_pLeftAreaWidget->m_bookMarkArea) {
            m_nBookMarkHoverLine = -1;
        }

        if (object == m_pLeftAreaWidget->m_flodArea) {
            m_markFoldHighLightSelections.clear();
            m_foldCodeShow->hide();
            renderAllSelections();
        }
    }
    return false;
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

bool TextEdit::containsExtraSelection(QList<QTextEdit::ExtraSelection> listSelections, QTextEdit::ExtraSelection selection)
{
    for (int i = 0;i < listSelections.count();i++) {
        if(listSelections.value(i).cursor == selection.cursor
                && listSelections.value(i).format == selection.format) {
            return true;
        }
    }
    return false;
}

void TextEdit::appendExtraSelection(QList<QTextEdit::ExtraSelection> wordMarkSelections
                                    ,QTextEdit::ExtraSelection selection,QString strColor
                                    ,QList<QTextEdit::ExtraSelection> *listSelections)
{
    if (wordMarkSelections.count() > 0) {
        bool bIsContains = false;
        int nWordMarkSelectionStart = 0,nSelectionStart = 0,nWordMarkSelectionEnd = 0,nSelectionEnd = 0;

        if (selection.cursor.selectionStart() > selection.cursor.selectionEnd()) {
            nSelectionStart = selection.cursor.selectionEnd();
            nSelectionEnd = selection.cursor.selectionStart();
        } else {
            nSelectionStart = selection.cursor.selectionStart();
            nSelectionEnd = selection.cursor.selectionEnd();
        }

        for (int i = 0;i < wordMarkSelections.count();i++) {

            if (wordMarkSelections.value(i).cursor.selectionStart() > wordMarkSelections.value(i).cursor.selectionEnd()) {
                nWordMarkSelectionStart = wordMarkSelections.value(i).cursor.selectionEnd();
                nWordMarkSelectionEnd = wordMarkSelections.value(i).cursor.selectionStart();
            } else {
                nWordMarkSelectionStart = wordMarkSelections.value(i).cursor.selectionStart();
                nWordMarkSelectionEnd = wordMarkSelections.value(i).cursor.selectionEnd();
            }

            if ((nWordMarkSelectionStart <= nSelectionStart && nWordMarkSelectionEnd > nSelectionEnd)
                    || (nWordMarkSelectionStart < nSelectionStart && nWordMarkSelectionEnd >= nSelectionEnd)) {
                bIsContains = true;
                selection.format.setBackground(QColor(strColor));
                if (wordMarkSelections.value(i).format != selection.format) {
                    int nRemPos = 0;
                    for (int j = 0;j < wordMarkSelections.count();j++) {
                        if (m_wordMarkSelections.value(j).cursor == wordMarkSelections.value(i).cursor
                                && m_wordMarkSelections.value(j).format == wordMarkSelections.value(i).format) {
                            m_wordMarkSelections.removeAt(j);
                            nRemPos = j;
                            break;
                        }
                    }

                    selection.cursor.setPosition(nWordMarkSelectionStart, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(nSelectionStart, QTextCursor::KeepAnchor);
                    selection.format.setBackground(wordMarkSelections.value(i).format.background());

                    bool bIsInsert = false;

                    if (selection.cursor.selectedText() != "") {
                        bIsInsert = true;
                        m_wordMarkSelections.insert(nRemPos,selection);
                    }

                    QTextEdit::ExtraSelection preSelection;
                    preSelection.format = selection.format;
                    preSelection.cursor = selection.cursor;

                    selection.cursor.setPosition(nSelectionEnd, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(nWordMarkSelectionEnd, QTextCursor::KeepAnchor);

                    if (selection.cursor.selectedText() != "") {
                        if (bIsInsert) {
                            m_wordMarkSelections.insert(nRemPos + 1,selection);
                        } else {
                            m_wordMarkSelections.insert(nRemPos,selection);
                        }
                    }

                    QList<QTextEdit::ExtraSelection> selecList;
                    bool bIsFind = false;
                    for (int j = 0;j < m_mapWordMarkSelections.count();j++) {
                        auto list = m_mapWordMarkSelections.value(j);
                        for (int k = 0;k < list.count();k++) {
                            if (list.value(k).cursor == wordMarkSelections.value(i).cursor
                                    && list.value(k).format == wordMarkSelections.value(i).format) {
                                list.removeAt(k);
                                selecList = list;
                                bIsInsert = false;

                                if (preSelection.cursor.selectedText() != "") {
                                    bIsInsert = true;
                                    selecList.insert(k,preSelection);
                                }

                                if (selection.cursor.selectedText() != "") {
                                    if (bIsInsert) {
                                        selecList.insert(k + 1,selection);
                                    } else {
                                        selecList.insert(k,selection);
                                    }
                                }

                                bIsFind = true;
                                break;
                            }
                        }

                        if (bIsFind) {
                            m_mapWordMarkSelections.remove(j);
                            m_mapWordMarkSelections.insert(j,selecList);
                            break;
                        }
                    }

                    selection.cursor.setPosition(nSelectionStart, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(nSelectionEnd, QTextCursor::KeepAnchor);
                    selection.format.setBackground(QColor(strColor));
                    m_wordMarkSelections.append(selection);
                    listSelections->append(selection);
                }
            } else if (nWordMarkSelectionStart >= nSelectionStart && nWordMarkSelectionEnd <= nSelectionEnd) {
                bIsContains = true;
                selection.format.setBackground(QColor(strColor));

                for (int j = 0;j < wordMarkSelections.count();j++) {
                    if (m_wordMarkSelections.value(j).cursor == wordMarkSelections.value(i).cursor
                            && m_wordMarkSelections.value(j).format == wordMarkSelections.value(i).format) {
                        m_wordMarkSelections.removeAt(j);
                        break;
                    }
                }

                m_wordMarkSelections.append(selection);

                if (wordMarkSelections.value(i).format != selection.format) {                   

                    QList<QTextEdit::ExtraSelection> selecList;
                    bool bIsFind = false;

                    for (int j = 0;j < m_mapWordMarkSelections.count();j++) {
                        auto list = m_mapWordMarkSelections.value(j);
                        for (int k = 0;k < list.count();k++) {
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
                            m_mapWordMarkSelections.insert(j,selecList);
                            break;
                        }
                    }
                }

                listSelections->append(selection);
            } else if (nWordMarkSelectionEnd < nSelectionEnd && nWordMarkSelectionStart < nSelectionStart
                       && nWordMarkSelectionEnd > nSelectionStart) {
                selection.format.setBackground(QColor(strColor));

                int nRemPos = 0;
                for (int j = 0;j < wordMarkSelections.count();j++) {
                    if (m_wordMarkSelections.value(j).cursor == wordMarkSelections.value(i).cursor
                            && m_wordMarkSelections.value(j).format == wordMarkSelections.value(i).format) {
                        m_wordMarkSelections.removeAt(j);
                        nRemPos = j;
                        break;
                    }
                }

                if (wordMarkSelections.value(i).format != selection.format) {

                    selection.cursor.setPosition(nWordMarkSelectionStart, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(nSelectionStart, QTextCursor::KeepAnchor);
                    selection.format.setBackground(wordMarkSelections.value(i).format.background());
                    m_wordMarkSelections.insert(nRemPos,selection);

                    QList<QTextEdit::ExtraSelection> selecList;
                    bool bIsFind = false;

                    for (int j = 0;j < m_mapWordMarkSelections.count();j++) {
                        auto list = m_mapWordMarkSelections.value(j);
                        for (int k = 0;k < list.count();k++) {
                            if (list.value(k).cursor == wordMarkSelections.value(i).cursor
                                    && list.value(k).format == wordMarkSelections.value(i).format) {
                                list.removeAt(k);
                                selecList = list;
                                selecList.insert(k,selection);
                                bIsFind = true;
                                break;
                            }
                        }

                        if (bIsFind) {
                            m_mapWordMarkSelections.remove(j);
                            m_mapWordMarkSelections.insert(j,selecList);
                            break;
                        }
                    }

                    selection.cursor.setPosition(nSelectionStart, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(nSelectionEnd, QTextCursor::KeepAnchor);
                    selection.format.setBackground(QColor(strColor));
                    m_wordMarkSelections.append(selection);
                } else {
                    selection.cursor.setPosition(nWordMarkSelectionStart, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(nSelectionEnd, QTextCursor::KeepAnchor);
                    m_wordMarkSelections.insert(nRemPos,selection);
                }

                if (!bIsContains) {
                    listSelections->append(selection);
                }

                bIsContains = true;
            } else if (nWordMarkSelectionEnd > nSelectionEnd && nWordMarkSelectionStart > nSelectionStart
                       && nWordMarkSelectionStart < nSelectionEnd) {
                selection.format.setBackground(QColor(strColor));

                int nRemPos = 0;
                for (int j = 0;j < wordMarkSelections.count();j++) {
                    if (m_wordMarkSelections.value(j).cursor == wordMarkSelections.value(i).cursor
                            && m_wordMarkSelections.value(j).format == wordMarkSelections.value(i).format) {
                        m_wordMarkSelections.removeAt(j);
                        nRemPos = j;
                        break;
                    }
                }

                if (wordMarkSelections.value(i).format != selection.format) {

                    selection.cursor.setPosition(nSelectionEnd, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(nWordMarkSelectionEnd, QTextCursor::KeepAnchor);
                    selection.format.setBackground(wordMarkSelections.value(i).format.background());
                    m_wordMarkSelections.insert(nRemPos,selection);

                    QList<QTextEdit::ExtraSelection> selecList;
                    bool bIsFind = false;

                    for (int j = 0;j < m_mapWordMarkSelections.count();j++) {
                        auto list = m_mapWordMarkSelections.value(j);
                        for (int k = 0;k < list.count();k++) {
                            if (list.value(k).cursor == wordMarkSelections.value(i).cursor
                                    && list.value(k).format == wordMarkSelections.value(i).format) {
                                list.removeAt(k);
                                selecList = list;
                                selecList.insert(k,selection);
                                bIsFind = true;
                                break;
                            }
                        }

                        if (bIsFind) {
                            m_mapWordMarkSelections.remove(j);
                            m_mapWordMarkSelections.insert(j,selecList);
                            break;
                        }
                    }

                    selection.cursor.setPosition(nSelectionStart, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(nSelectionEnd, QTextCursor::KeepAnchor);
                    selection.format.setBackground(QColor(strColor));
                    m_wordMarkSelections.append(selection);
                } else {
                    selection.cursor.setPosition(nSelectionStart, QTextCursor::MoveAnchor);
                    selection.cursor.setPosition(nWordMarkSelectionEnd, QTextCursor::KeepAnchor);
                    m_wordMarkSelections.insert(nRemPos,selection);
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

    } else {
        selection.format.setBackground(QColor(strColor));
        m_wordMarkSelections.append(selection);
        listSelections->append(selection);
    }
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
}

void TextEdit::dragEnterEvent(QDragEnterEvent *event)
{
    QTextEdit::dragEnterEvent(event);
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
        QTextEdit::dragMoveEvent(event);
    }
}

void TextEdit::dropEvent(QDropEvent *event)
{
    const QMimeData *data = event->mimeData();

    if (data->hasUrls() && data->urls().first().isLocalFile()) {
        qobject_cast<Window *>(this->window())->requestDropEvent(event);
    } else if (data->hasText() && !m_readOnlyMode) {
        QTextEdit::dropEvent(event);
    }
}

void TextEdit::inputMethodEvent(QInputMethodEvent *e)
{
    m_bIsInputMethod = true;
    m_qstrCommitString = e->commitString();
    if (!m_readOnlyMode && e->commitString() != "") {
        QTextEdit::inputMethodEvent(e);
    }
}

void TextEdit::mousePressEvent(QMouseEvent *e)
{
    if (e->source() == Qt::MouseEventSynthesizedByQt) {
        m_lastTouchBeginPos = e->pos();

        if (QScroller::hasScroller(this)) {
            QScroller::scroller(this)->deleteLater();
        }

        if (m_updateEnableSelectionByMouseTimer) {
            m_updateEnableSelectionByMouseTimer->stop();
        } else {
            m_updateEnableSelectionByMouseTimer = new QTimer(this);
            m_updateEnableSelectionByMouseTimer->setSingleShot(true);

            static QObject *theme_settings = reinterpret_cast<QObject*>(qvariant_cast<quintptr>(qApp->property("_d_theme_settings_object")));
            QVariant touchFlickBeginMoveDelay;

            if (theme_settings) {
                touchFlickBeginMoveDelay = theme_settings->property("touchFlickBeginMoveDelay");
            }

            m_updateEnableSelectionByMouseTimer->setInterval(touchFlickBeginMoveDelay.isValid() ? touchFlickBeginMoveDelay.toInt() : 300);

            connect(m_updateEnableSelectionByMouseTimer, &QTimer::timeout, m_updateEnableSelectionByMouseTimer, &QTimer::deleteLater);
        }

        m_updateEnableSelectionByMouseTimer->start();
    }

    QTextEdit::mousePressEvent(e);
}

void TextEdit::mouseMoveEvent(QMouseEvent *e)
{
    if (e->source() == Qt::MouseEventSynthesizedByQt) {
        if (QScroller::hasScroller(this))
            return;

        if (m_updateEnableSelectionByMouseTimer
                && m_updateEnableSelectionByMouseTimer->isActive()) {
            const QPoint difference_pos = e->pos() - m_lastTouchBeginPos;

            if (std::abs(difference_pos.x()) > m_touchTapDistance
                    || std::abs(difference_pos.y()) > m_touchTapDistance) {
                QScroller::grabGesture(this);
                QScroller *scroller = QScroller::scroller(this);

                scroller->handleInput(QScroller::InputPress, e->localPos(), e->timestamp());
                scroller->handleInput(QScroller::InputMove, e->localPos(), e->timestamp());
            }

            return;
        }
    }

    // other apps will override their own cursor when opened
    // so they need to be restored.
    QApplication::restoreOverrideCursor();

    if (viewport()->cursor().shape() != Qt::IBeamCursor) {
        viewport()->setCursor(Qt::IBeamCursor);
    }

    QTextEdit::mouseMoveEvent(e);
}

void TextEdit::keyPressEvent(QKeyEvent *e)
{
    // if (!isModifier(e)) {
    //     viewport()->setCursor(Qt::BlankCursor);
    // }

    const QString &key = Utils::getKeyshortcut(e);
//    qDebug() << "key" << key;
    if (m_readOnlyMode || m_bReadOnlyPermission) {
        if (key == "J") {
            nextLine();
        } else if (key == "K") {
            prevLine();
        } else if (key == ",") {
            moveToEnd();
        } else if (key == ".") {
            moveToStart();
        } else if (key == "H") {
            backwardChar();
        } else if (key == "L") {
            forwardChar();
        } else if (key == "Space") {
            scrollUp();
        } else if (key == "V") {
            scrollDown();
        } else if (key == "F") {
            forwardWord();
        } else if (key == "B") {
            backwardWord();
        } else if (key == "A") {
            moveToStartOfLine();
        } else if (key == "E") {
            moveToEndOfLine();
        } else if (key == "M") {
            moveToLineIndentation();
        } else if (key == "Q" && m_bReadOnlyPermission == false) {
            toggleReadOnlyMode();
        } else if (key == "Shfit+J") {
            scrollLineUp();
        } else if (key == "Shift+K") {
            scrollLineDown();
        } else if (key == "P") {
            forwardPair();
        } else if (key == "N") {
            backwardPair();
        } else if (key == "Shift+:") {
            copyLines();
        } else if ((key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "togglereadonlymode")/* || key=="Alt+Meta+L"*/)
                   && m_bReadOnlyPermission == false) {
            toggleReadOnlyMode();
        } else if (key == "Shift+/" && e->modifiers() == Qt::ControlModifier) {
            e->ignore();
        } else if (e->key() == Qt::Key_Control || e->key() == Qt::Key_Shift) {
            e->ignore();
        } else if (e->key() == Qt::Key_F11 || e->key() == Qt::Key_F5) {
            e->ignore();
        } else if (e->modifiers() == Qt::NoModifier || e->modifiers() == Qt::KeypadModifier) {
            popupNotify(tr("Read-Only mode is on"));
        } else {
            // If press another key
            // the main window does not receive
            e->ignore();
        }

    } else {
        if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "indentline")) {
            indentText();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "backindentline")) {
            unindentText();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "forwardchar")) {
            forwardChar();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "backwardchar")) {
            backwardChar();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "forwardword")) {
            forwardWord();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "backwardword")) {
            backwardWord();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "nextline")) {
            nextLine();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "prevline")) {
            prevLine();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "newline")/* || key == "Return"*/) {
            newline();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "opennewlineabove")) {
            openNewlineAbove();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "opennewlinebelow")) {
            openNewlineBelow();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "duplicateline")) {
            duplicateLine();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "killline")) {
            killLine();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "killcurrentline")) {
            killCurrentLine();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "swaplineup")) {
            moveLineDownUp(true);
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "swaplinedown")) {
            moveLineDownUp(false);
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "scrolllineup")) {
            scrollLineUp();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "scrolllinedown")) {
            scrollLineDown();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "scrollup")) {
            scrollUp();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "scrolldown")) {
            scrollDown();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "movetoendofline")) {
            moveToEndOfLine();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "movetostartofline")) {
            moveToStartOfLine();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "movetostart")) {
            moveToStart();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "movetoend")) {
            moveToEnd();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "movetolineindentation")) {
            moveToLineIndentation();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "upcaseword")) {
            upcaseWord();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "downcaseword")) {
            downcaseWord();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "capitalizeword")) {
            capitalizeWord();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "killbackwardword")) {
            killBackwardWord();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "killforwardword")) {
            killForwardWord();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "forwardpair")) {
            forwardPair();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "backwardpair")) {
            backwardPair();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "transposechar")) {
            transposeChar();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "cut")) {
            cutSelectedText();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "paste")) {
            pasteText();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "setmark")) {
            setMark();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "exchangemark")) {
            exchangeMark();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "copylines")) {
            copyLines();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "cutlines")) {
            cutlines();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "joinlines")) {
            joinLines();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "togglereadonlymode")/*|| key=="Alt+Meta+L"*/) {
            toggleReadOnlyMode();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "togglecomment")) {
            toggleComment(true);
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "removecomment")) {
            toggleComment(false);
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "undo")) {
            QTextEdit::undo();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "redo")) {
            QTextEdit::redo();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "switchbookmark")) {
            m_bIsShortCut = true;
            addOrDeleteBookMark();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "movetoprebookmark")) {
            moveToPreviousBookMark();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "movetonextbookmark")) {
            moveToNextBookMark();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "mark")) {
            markSelectWord();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "escape")) {
            escape();
        } else if (e->key() == Qt::Key_Insert && key != "Shift+Ins") {
            if (e->modifiers() == Qt::NoModifier) {
                setOverwriteMode(!overwriteMode());
                update();
                e->accept();

                m_cursorMode = overwriteMode() ? Overwrite : Insert;
                emit cursorModeChanged(m_cursorMode);
            }
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

            // Text editor handle key self.
            QTextEdit::keyPressEvent(e);
        }
    }

    if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "selectall")) {
        selectAll();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "copy")) {
        copySelectedText();
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

    QTextEdit::wheelEvent(e);
}

void TextEdit::contextMenuEvent(QContextMenuEvent *event)
{
    m_rightMenu->clear();

    QString wordAtCursor = getWordAtMouse();
    m_mouseClickPos = event->pos();
    QTextCursor selectionCursor = textCursor();
    selectionCursor.movePosition(QTextCursor::StartOfBlock);
    selectionCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    QString text = selectionCursor.selectedText();

    // init base.
    bool isBlankLine = text.trimmed().isEmpty();

    if (m_canUndo) {
        if (m_bReadOnlyPermission == false && m_readOnlyMode == false) {
            m_rightMenu->addAction(m_undoAction);
        }
    }
    if (m_canRedo) {
        if (m_bReadOnlyPermission == false && m_readOnlyMode == false) {
            m_rightMenu->addAction(m_redoAction);
        }
    }
    m_rightMenu->addSeparator();
    if (textCursor().hasSelection()) {
        if (m_bReadOnlyPermission == false && m_readOnlyMode == false) {
            m_rightMenu->addAction(m_cutAction);
        }
        m_rightMenu->addAction(m_copyAction);
    } else {
        // Just show copy/cut menu item when cursor rectangle contain moue pointer coordinate.
//        m_haveWordUnderCursor = highlightWordUnderMouse(event->pos());
//        if (m_haveWordUnderCursor) {
//            if (!wordAtCursor.isEmpty()) {
//                if (m_bReadOnlyPermission == false && m_readOnlyMode == false) {k
//                    m_rightMenu->addAction(m_cutAction);
//                }
//                m_rightMenu->addAction(m_copyAction);
//            }
//        }
    }
    if (canPaste()) {
        if (m_bReadOnlyPermission == false && m_readOnlyMode == false) {
            m_rightMenu->addAction(m_pasteAction);
        }
    }
    if (textCursor().hasSelection()) {
        if (m_bReadOnlyPermission == false && m_readOnlyMode == false) {
            m_rightMenu->addAction(m_deleteAction);
        }

    }
    if (!toPlainText().isEmpty()) {
        m_rightMenu->addAction(m_selectAllAction);
    }
    m_rightMenu->addSeparator();
    if (!toPlainText().isEmpty()) {
        m_rightMenu->addAction(m_findAction);
        if (m_bReadOnlyPermission == false && m_readOnlyMode == false) {
            m_rightMenu->addAction(m_replaceAction);
        }
        m_rightMenu->addAction(m_jumpLineAction);
        m_rightMenu->addSeparator();
    }
    if(textCursor().hasSelection()){
        if(m_bReadOnlyPermission == false &&m_readOnlyMode == false){
            m_rightMenu->addMenu(m_convertCaseMenu);
        }
    } else {
        m_convertCaseMenu->hide();
    }

    // intelligent judge whether to support comments.
    const auto def = m_repository.definitionForFileName(QFileInfo(filepath).fileName());
    if (!toPlainText().isEmpty() &&
        (textCursor().hasSelection() || !isBlankLine) &&
        !def.filePath().isEmpty()) {
//        m_rightMenu->addAction(m_toggleCommentAction);

        //yanyuhan 折叠、代码注释（有代码选中时增加注释选项显示）
//        m_rightMenu->addMenu(m_collapseExpandMenu);             //折叠展开

        m_rightMenu->addAction(m_addComment);
        m_rightMenu->addAction(m_cancelComment);

        if (m_readOnlyMode == true) {
//            m_toggleCommentAction->setEnabled(false);
            m_addComment->setEnabled(false);
            m_cancelComment->setEnabled(false);
        } else {
//            m_toggleCommentAction->setEnabled(true);
            m_addComment->setEnabled(true);
            m_cancelComment->setEnabled(true);
        }
    }

    m_rightMenu->addSeparator();
    if (m_bReadOnlyPermission == false) {
        if (m_readOnlyMode) {
            m_rightMenu->addAction(m_disableReadOnlyModeAction);
        } else {
            m_rightMenu->addAction(m_enableReadOnlyModeAction);
        }
    }

    m_rightMenu->addAction(m_openInFileManagerAction);
    m_rightMenu->addSeparator();
    if (static_cast<Window*>(this->window())->isFullScreen()) {
        m_rightMenu->addAction(m_exitFullscreenAction);
    } else {
        m_rightMenu->addAction(m_fullscreenAction);
    }

    bool b_Ret = DSysInfo::isCommunityEdition();
    if(!b_Ret){
        bool stopReadingState = false;
        QDBusMessage stopReadingMsg = QDBusMessage::createMethodCall("com.iflytek.aiassistant",
                                                          "/aiassistant/tts",
                                                          "com.iflytek.aiassistant.tts",
                                                          "isTTSInWorking");

        QDBusReply<bool> stopReadingStateRet = QDBusConnection::sessionBus().call(stopReadingMsg, QDBus::BlockWithGui);
        if (stopReadingStateRet.isValid()) {
            stopReadingState = stopReadingStateRet.value();
        }
        if(!stopReadingState){
            m_rightMenu->addAction(m_voiceReadingAction);
            m_voiceReadingAction->setEnabled(false);
        }
        else {
            m_rightMenu->removeAction(m_voiceReadingAction);
            m_rightMenu->addAction(m_stopReadingAction);
        }



        bool voiceReadingState = false;
        QDBusMessage voiceReadingMsg = QDBusMessage::createMethodCall("com.iflytek.aiassistant",
                                                          "/aiassistant/tts",
                                                          "com.iflytek.aiassistant.tts",
                                                          "getTTSEnable");

        QDBusReply<bool> voiceReadingStateRet = QDBusConnection::sessionBus().call(voiceReadingMsg, QDBus::BlockWithGui);
        if (voiceReadingStateRet.isValid()) {
            voiceReadingState = voiceReadingStateRet.value();
        }
        if (textCursor().hasSelection()&&voiceReadingState) {
            m_voiceReadingAction->setEnabled(true);
        }

        m_rightMenu->addAction(m_dictationAction);
        bool dictationState = false;
        QDBusMessage dictationMsg = QDBusMessage::createMethodCall("com.iflytek.aiassistant",
                                                          "/aiassistant/iat",
                                                          "com.iflytek.aiassistant.iat",
                                                          "getIatEnable");

        QDBusReply<bool> dictationStateRet = QDBusConnection::sessionBus().call(dictationMsg, QDBus::BlockWithGui);
        if (dictationStateRet.isValid()) {
            dictationState = dictationStateRet.value();
        }
        m_dictationAction->setEnabled(dictationState);
        if(m_readOnlyMode){
            m_dictationAction->setEnabled(false);
        }

        m_rightMenu->addAction(m_translateAction);
        m_translateAction->setEnabled(false);
        bool translateState = false;
        QDBusMessage translateReadingMsg = QDBusMessage::createMethodCall("com.iflytek.aiassistant",
                                                          "/aiassistant/trans",
                                                          "com.iflytek.aiassistant.trans",
                                                          "getTransEnable");

        QDBusReply<bool> translateStateRet = QDBusConnection::sessionBus().call(translateReadingMsg, QDBus::BlockWithGui);
        if (translateStateRet.isValid()) {
            translateState = translateStateRet.value();
        }
        if (textCursor().hasSelection()&&translateState) {
            m_translateAction->setEnabled(translateState);
        }
    }

    //yanyuhan
//    m_rightMenu->addMenu(m_colormarkMenu);          //标记功能
//    m_rightMenu->addAction(m_columnEditACtion);           //列编辑模式


//    m_rightMenu->addSeparator();
//    m_rightMenu->addMenu(m_markAllLine);
//    m_rightMenu->addAction(m_cancleMarkAllLine);

//    if (textCursor().hasSelection()) {
//        m_rightMenu->addMenu(m_markCurrentLine);
//        m_rightMenu->addAction(m_cancleMarkCurrentLine);
//    }
//    if (m_wordMarkSelections.size() > 1) {
//        m_rightMenu->addAction(m_cancleLastMark);
//    }
    if(! this->document()->isEmpty()) {
        m_rightMenu->addSeparator();
        m_rightMenu->addMenu(m_colorMarkMenu);
        m_colorMarkMenu->clear();
        m_colorMarkMenu->addMenu(m_markCurrentLine);
        if (m_wordMarkSelections.size() > 0) {
            m_colorMarkMenu->addAction(m_cancleLastMark);
        }
        m_colorMarkMenu->addAction(m_cancleMarkAllLine);
        m_colorMarkMenu->addMenu(m_markAllLine);
    }

//    if (textCursor().hasSelection()) {
//        m_colorMarkMenu->addMenu(m_markCurrentLine);
//        m_colorMarkMenu->addAction(m_cancleMarkCurrentLine);
//    }

    m_rightMenu->exec(event->globalPos());
}

void TextEdit::highlightCurrentLine()
{
    updateHighlightLineSelection();
    renderAllSelections();
    //adjustScrollbarMargins();
}
