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

#include <KF5/KSyntaxHighlighting/definition.h>
#include <KF5/KSyntaxHighlighting/syntaxhighlighter.h>
#include <KF5/KSyntaxHighlighting/theme.h>

#include <QAbstractTextDocumentLayout>
#include <QTextDocumentFragment>
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

#include <private/qguiapplication_p.h>
#include <qpa/qplatformtheme.h>

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

DTextEdit::DTextEdit(QWidget *parent)
    : QTextEdit(parent),
      m_wrapper(nullptr),
      m_highlighter(new KSyntaxHighlighting::SyntaxHighlighter(document()))
{
    lineNumberArea = new LineNumberArea(this);

#if QT_VERSION < QT_VERSION_CHECK(5,9,0)
    m_touchTapDistance = 15;
#else
    m_touchTapDistance = QGuiApplicationPrivate::platformTheme()->themeHint(QPlatformTheme::TouchDoubleTapDistance).toInt();
#endif

    viewport()->installEventFilter(this);
    viewport()->setCursor(Qt::IBeamCursor);

    // Don't draw frame around editor widget.
    setFrameShape(QFrame::NoFrame);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAcceptRichText(false);

    // Init widgets.
    connect(this->verticalScrollBar(), &QScrollBar::valueChanged, this, &DTextEdit::updateLineNumber);
    connect(this, &QTextEdit::textChanged, this, [this]() {
        updateLineNumber();
        updateWordCount();
    });
    connect(this, &QTextEdit::cursorPositionChanged, this, &DTextEdit::cursorPositionChanged);
    connect(document(), &QTextDocument::modificationChanged, this, &DTextEdit::setModified);

    // Init menu.
    m_rightMenu = new QMenu();
    m_rightMenu->setStyle(QStyleFactory::create("dlight"));
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

    connect(m_rightMenu, &QMenu::aboutToHide, this, &DTextEdit::removeHighlightWordUnderCursor);
    connect(m_undoAction, &QAction::triggered, this, &DTextEdit::undo);
    connect(m_redoAction, &QAction::triggered, this, &DTextEdit::redo);
    connect(m_cutAction, &QAction::triggered, this, &DTextEdit::clickCutAction);
    connect(m_copyAction, &QAction::triggered, this, &DTextEdit::clickCopyAction);
    connect(m_pasteAction, &QAction::triggered, this, &DTextEdit::clickPasteAction);
    connect(m_deleteAction, &QAction::triggered, this, &DTextEdit::clickDeleteAction);
    connect(m_selectAllAction, &QAction::triggered, this, &DTextEdit::selectAll);
    connect(m_findAction, &QAction::triggered, this, &DTextEdit::clickFindAction);
    connect(m_replaceAction, &QAction::triggered, this, &DTextEdit::clickReplaceAction);
    connect(m_jumpLineAction, &QAction::triggered, this, &DTextEdit::clickJumpLineAction);
    connect(m_fullscreenAction, &QAction::triggered, this, &DTextEdit::clickFullscreenAction);
    connect(m_exitFullscreenAction, &QAction::triggered, this, &DTextEdit::clickFullscreenAction);
    connect(m_enableReadOnlyModeAction, &QAction::triggered, this, &DTextEdit::toggleReadOnlyMode);
    connect(m_disableReadOnlyModeAction, &QAction::triggered, this, &DTextEdit::toggleReadOnlyMode);
    connect(m_openInFileManagerAction, &QAction::triggered, this, &DTextEdit::clickOpenInFileManagerAction);
    connect(m_toggleCommentAction, &QAction::triggered, this, &DTextEdit::toggleComment);

    // Init convert case sub menu.
    m_haveWordUnderCursor = false;
    m_convertCaseMenu = new QMenu(tr("Change Case"));
    m_upcaseAction = new QAction(tr("Upper Case"), this);
    m_downcaseAction = new QAction(tr("Lower Case"), this);
    m_capitalizeAction = new QAction(tr("Capitalize"), this);

    m_convertCaseMenu->addAction(m_upcaseAction);
    m_convertCaseMenu->addAction(m_downcaseAction);
    m_convertCaseMenu->addAction(m_capitalizeAction);

    connect(m_upcaseAction, &QAction::triggered, this, &DTextEdit::upcaseWord);
    connect(m_downcaseAction, &QAction::triggered, this, &DTextEdit::downcaseWord);
    connect(m_capitalizeAction, &QAction::triggered, this, &DTextEdit::capitalizeWord);

    m_canUndo = false;
    m_canRedo = false;

    connect(this, &DTextEdit::undoAvailable, this, [=] (bool undoIsAvailable) {
        m_canUndo = undoIsAvailable;
    });
    connect(this, &DTextEdit::redoAvailable, this, [=] (bool redoIsAvailable) {
        m_canRedo = redoIsAvailable;
    });

    // Init scroll animation.
    m_scrollAnimation = new QPropertyAnimation(verticalScrollBar(), "value");
    m_scrollAnimation->setEasingCurve(QEasingCurve::InOutExpo);
    m_scrollAnimation->setDuration(300);

    m_cursorMode = Insert;

    connect(m_scrollAnimation, &QPropertyAnimation::finished, this, &DTextEdit::handleScrollFinish, Qt::QueuedConnection);

    // Monitor cursor mark status to update in line number area.
    connect(this, &DTextEdit::cursorMarkChanged, this, &DTextEdit::handleCursorMarkChanged);

    // configure content area
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, &DTextEdit::adjustScrollbarMargins, Qt::QueuedConnection);

    // Don't blink the cursor when selecting text
    // Recover blink when not selecting text.
    connect(this, &DTextEdit::selectionChanged, this, [=] {
        if (textCursor().hasSelection()) {
            hideCursorBlink();
        } else {
            showCursorBlink();
        }
    });

    // syntax selection

    m_hlGroupMenu = new QMenu;
    QAction *noHlAction = m_hlGroupMenu->addAction(tr("None"));

    m_hlActionGroup = new QActionGroup(m_hlGroupMenu);
    m_hlActionGroup->setExclusive(true);

    noHlAction->setCheckable(true);
    m_hlActionGroup->addAction(noHlAction);
    noHlAction->setChecked(!m_highlighter->definition().isValid());

    QMenu *hlSubMenu = nullptr;
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
        m_highlighter->setDefinition(def);
        emit hightlightChanged(action->text());
    });
}

void DTextEdit::setWrapper(EditWrapper *w)
{
    m_wrapper = w;
}

int DTextEdit::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, this->document()->blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 13 +  fontMetrics().width(QLatin1Char('9')) * (digits);

    return space;
}

int DTextEdit::getCurrentLine()
{
    return textCursor().blockNumber() + 1;
}

int DTextEdit::getCurrentColumn()
{
    return textCursor().columnNumber();
}

int DTextEdit::getPosition()
{
    return textCursor().position();
}

int DTextEdit::getScrollOffset()
{
    QScrollBar *scrollbar = verticalScrollBar();

    return scrollbar->value();
}

void DTextEdit::forwardChar()
{
    if (m_cursorMark) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        moveCursorNoBlink(QTextCursor::NextCharacter);
    }
}

void DTextEdit::backwardChar()
{
    if (m_cursorMark) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        moveCursorNoBlink(QTextCursor::PreviousCharacter);
    }
}

void DTextEdit::forwardWord()
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

void DTextEdit::backwardWord()
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

void DTextEdit::forwardPair()
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

void DTextEdit::backwardPair()
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

int DTextEdit::blockCount() const
{
    return document()->blockCount();
}

int DTextEdit::characterCount() const
{
    return document()->characterCount();
}

QTextBlock DTextEdit::firstVisibleBlock()
{
    return document()->findBlockByLineNumber(getFirstVisibleBlockId());
}

void DTextEdit::moveToStart()
{
    if (m_cursorMark) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Start, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        moveCursorNoBlink(QTextCursor::Start);
    }
}

void DTextEdit::moveToEnd()
{
    if (m_cursorMark) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        moveCursorNoBlink(QTextCursor::End);
    }
}

void DTextEdit::moveToStartOfLine()
{
    if (m_cursorMark) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        moveCursorNoBlink(QTextCursor::StartOfBlock);
    }
}

void DTextEdit::moveToEndOfLine()
{
    if (m_cursorMark) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        moveCursorNoBlink(QTextCursor::EndOfBlock);
    }
}

void DTextEdit::moveToLineIndentation()
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

void DTextEdit::nextLine()
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

void DTextEdit::prevLine()
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

void DTextEdit::moveCursorNoBlink(QTextCursor::MoveOperation operation, QTextCursor::MoveMode mode)
{
    // Function moveCursorNoBlink will blink cursor when move cursor.
    // But function movePosition won't, so we use movePosition to avoid that cursor link when moving cursor.
    QTextCursor cursor = textCursor();
    cursor.movePosition(operation, mode);
    setTextCursor(cursor);
}

void DTextEdit::jumpToLine(int line, bool keepLineAtCenter)
{
    QTextCursor cursor(document()->findBlockByNumber(line - 1)); // line - 1 because line number starts from 0

    // Update cursor.
    setTextCursor(cursor);

    if (keepLineAtCenter) {
        keepCurrentLineAtCenter();
    }
}

void DTextEdit::autoIndent(QTextCursor& cursor)
{
    auto curBlock = cursor.block().text();
    auto *indentEndIter = std::find_if_not(curBlock.begin(), curBlock.end(), [](QChar &c) {
        return c == '\t' || c == ' ';
    });
    auto indentEndPos = indentEndIter - curBlock.begin();
    auto curIndent = curBlock.midRef(0, static_cast<int>(indentEndPos));
    int curIndentSize = 0;
    for (const auto &c : curIndent) {
        if (c == '\t')
            curIndentSize += m_tabSpaceNumber;
        else
            curIndentSize++;
    }

    QString newIndent;
    if (m_useTab)
        newIndent = QString(curIndentSize / m_tabSpaceNumber, '\t');
    else
        newIndent = QString(curIndentSize, ' ');
    cursor.insertText("\n" + newIndent);
}

void DTextEdit::newline()
{
    // Stop mark if mark is set.
    tryUnsetMark();

    QTextCursor cursor = textCursor();

    if (!m_autoIndent)
        cursor.insertText("\n");
    else
        autoIndent(cursor);

    setTextCursor(cursor);
}

void DTextEdit::openNewlineAbove()
{
    // Stop mark if mark is set.
    tryUnsetMark();

    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
    cursor.insertText("\n");
    cursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor);

    setTextCursor(cursor);
}

void DTextEdit::openNewlineBelow()
{
    // Stop mark if mark is set.
    tryUnsetMark();

    moveCursorNoBlink(QTextCursor::EndOfBlock);
    textCursor().insertText("\n");
}

void DTextEdit::moveLineDownUp(bool up)
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

void DTextEdit::scrollLineUp()
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

void DTextEdit::scrollLineDown()
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

void DTextEdit::scrollUp()
{
    QScrollBar *scrollbar = verticalScrollBar();

    int lines = rect().height() / fontMetrics().height();

    scrollbar->setValue(scrollbar->value() + lines);

    if (scrollbar->value() >= getCurrentLine()) {
        auto moveMode = m_cursorMark ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;

        int line = scrollbar->value();
        QTextCursor lineCursor(document()->findBlockByLineNumber(line - 1)); // line - 1 because line number starts from 0

        QTextCursor cursor = textCursor();
        cursor.setPosition(lineCursor.position(), moveMode);
        setTextCursor(cursor);
    }
}

void DTextEdit::scrollDown()
{
    QScrollBar *scrollbar = verticalScrollBar();

    int lines = rect().height() / fontMetrics().height();

    scrollbar->setValue(scrollbar->value() - lines);

    auto moveMode = m_cursorMark ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;

    int line = scrollbar->value() + lines;
    QTextCursor lineCursor(document()->findBlockByLineNumber(line - 1)); // line - 1 because line number starts from 0

    QTextCursor cursor = textCursor();
    cursor.setPosition(lineCursor.position(), moveMode);
    setTextCursor(cursor);
}

void DTextEdit::duplicateLine()
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

void DTextEdit::copyLines()
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

        // popupNotify(tr("已经拷贝选中行到剪切板"));
    } else {
        // Selection current line.
        copyCursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        copyCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

        // popupNotify(tr("已经拷贝当前行到剪切板"));
    }

    // Copy lines to system clipboard.
    setTextCursor(copyCursor);
    copySelectedText();

    // Reset cursor before copy lines.
    copyCursor.setPosition(currentCursor.position(), QTextCursor::MoveAnchor);
    setTextCursor(copyCursor);
}

void DTextEdit::cutlines()
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

        // popupNotify(tr("已经剪切选中行到剪切板"));
    } else {
        // Selection current line.
        copyCursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        copyCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

        // popupNotify(tr("已经剪切当前行到剪切板"));
    }

    // Copy lines to system clipboard.
    setTextCursor(copyCursor);
    cutSelectedText();

    // Reset cursor before copy lines.
    copyCursor.setPosition(currentCursor.position(), QTextCursor::MoveAnchor);
    setTextCursor(copyCursor);
}

void DTextEdit::joinLines()
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

void DTextEdit::killLine()
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

void DTextEdit::killCurrentLine()
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

void DTextEdit::killBackwardWord()
{
    tryUnsetMark();

    if (textCursor().hasSelection()) {
        textCursor().removeSelectedText();
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

void DTextEdit::killForwardWord()
{
    tryUnsetMark();

    if (textCursor().hasSelection()) {
        textCursor().removeSelectedText();
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

void DTextEdit::escape()
{
    emit pressEsc();

    tryUnsetMark();
}

void DTextEdit::indentText()
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
            QString indentStr = "\t";
            if (!m_useTab)
                indentStr = QString(m_tabSpaceNumber, ' ');
            cursor.setPosition(block.position());
            cursor.insertText(indentStr);
            block = block.next();
        }

        cursor.endEditBlock();
    } else {
        cursor.beginEditBlock();

        QString indentStr = "\t";
        if (!m_useTab) {
            int indent = m_tabSpaceNumber - (cursor.positionInBlock() % m_tabSpaceNumber);
            indentStr = QString(indent, ' ');
        }
        cursor.insertText(indentStr);

        cursor.endEditBlock();
    }

    showCursorBlink();
}

void DTextEdit::unindentText()
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

void DTextEdit::setTabSpaceNumber(int number)
{
    m_tabSpaceNumber = number;
    updateFont();
    updateLineNumber();
}

void DTextEdit::setUseTab(bool useTab)
{
    m_useTab = useTab;
}

void DTextEdit::setAutoIndent(bool autoIndent)
{
    m_autoIndent = autoIndent;
}

void DTextEdit::upcaseWord()
{
    tryUnsetMark();

    convertWordCase(UPPER);
}

void DTextEdit::downcaseWord()
{
    tryUnsetMark();

    convertWordCase(LOWER);
}

void DTextEdit::capitalizeWord()
{
    tryUnsetMark();

    convertWordCase(CAPITALIZE);
}

void DTextEdit::transposeChar()
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

void DTextEdit::handleCursorMarkChanged(bool mark, QTextCursor cursor)
{
    if (mark) {
        m_markStartLine = cursor.blockNumber() + 1;
    } else {
        m_markStartLine = -1;
    }

    lineNumberArea->update();
}

void DTextEdit::convertWordCase(ConvertCase convertCase)
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

QString DTextEdit::capitalizeText(QString text)
{
    QString newText = text.toLower();
    QChar currentChar;
    for (int i = 0; i < newText.size(); i++) {
        currentChar = newText.at(i);
        if (!currentChar.isSpace() && !m_wordSepartors.contains(QString(currentChar))) {
            newText.replace(i, 1, currentChar.toUpper());
            break;
        }
    }

    return newText;
}

void DTextEdit::keepCurrentLineAtCenter()
{
    QScrollBar *scrollbar = verticalScrollBar();

    int currentLine = cursorRect().top() / cursorRect().height();
    int halfEditorLines = rect().height() / 2 / cursorRect().height();
    scrollbar->setValue(scrollbar->value() + currentLine - halfEditorLines);
}

void DTextEdit::scrollToLine(int scrollOffset, int row, int column)
{
    // Save cursor postion.
    m_restoreRow = row;
    m_restoreColumn = column;

    // Start scroll animation.
    m_scrollAnimation->setStartValue(verticalScrollBar()->value());
    m_scrollAnimation->setEndValue(scrollOffset);
    m_scrollAnimation->start();
}

void DTextEdit::setLineWrapMode(bool enable)
{
    QTextEdit::setLineWrapMode(enable ? QTextEdit::WidgetWidth : QTextEdit::NoWrap);
}

void DTextEdit::setFontFamily(QString name)
{
    // Update font.
    m_fontName = name;
    updateFont();
    updateLineNumber();
}

void DTextEdit::setFontSize(int size)
{
    // Update font.
    m_fontSize = size;
    updateFont();

    // Update line number after adjust font size.
    updateLineNumber();
}

void DTextEdit::updateFont()
{
    QFont font = document()->defaultFont();
    font.setFixedPitch(true);
    font.setPointSize(m_fontSize);
    font.setFamily(m_fontName);
    setFont(font);
    setTabStopWidth(m_tabSpaceNumber * QFontMetrics(font).width(' '));
}

void DTextEdit::replaceAll(const QString &replaceText, const QString &withText)
{
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

void DTextEdit::replaceNext(const QString &replaceText, const QString &withText)
{
    if (replaceText.isEmpty() ||
        !m_findHighlightSelection.cursor.hasSelection()) {
        // 无限替换的根源
        highlightKeyword(replaceText, getPosition());
        return;
    }

    QTextCursor cursor = textCursor();
    cursor.setPosition(m_findHighlightSelection.cursor.position() - replaceText.size());
    cursor.movePosition(QTextCursor::NoMove, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, replaceText.size());
    cursor.insertText(withText);

    // Update cursor.
    setTextCursor(cursor);
    highlightKeyword(replaceText, getPosition());
}

void DTextEdit::replaceRest(const QString &replaceText, const QString &withText)
{
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

bool DTextEdit::findKeywordForward(const QString &keyword)
{
    if (textCursor().hasSelection()) {
        // Get selection bound.
        int startPos = textCursor().anchor();
        int endPos = textCursor().position();

        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
        setTextCursor(cursor);

        QTextDocument::FindFlags options;
        options &= QTextDocument::FindCaseSensitively;

        bool foundOne = find(keyword, options);

        cursor.setPosition(endPos, QTextCursor::MoveAnchor);
        cursor.setPosition(startPos, QTextCursor::KeepAnchor);
        setTextCursor(cursor);

        return foundOne;
    } else {
        QTextCursor recordCursor = textCursor();

        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
        setTextCursor(cursor);

        QTextDocument::FindFlags options;
        options &= QTextDocument::FindCaseSensitively;

        bool foundOne = find(keyword, options);

        setTextCursor(recordCursor);

        return foundOne;
    }
}

void DTextEdit::removeKeywords()
{
    m_findHighlightSelection.cursor = textCursor();
    m_findHighlightSelection.cursor.clearSelection();

    m_findMatchSelections.clear();

    updateHighlightLineSelection();

    renderAllSelections();

    setFocus();
}

void DTextEdit::highlightKeyword(QString keyword, int position)
{
    updateKeywordSelections(keyword);
    updateCursorKeywordSelection(position, true);
    updateHighlightLineSelection();
    renderAllSelections();
}

void DTextEdit::updateCursorKeywordSelection(int position, bool findNext)
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

void DTextEdit::updateHighlightLineSelection()
{
    QTextEdit::ExtraSelection selection;

    selection.format.setBackground(m_currentLineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();

    m_currentLineSelection = selection;
}

void DTextEdit::updateKeywordSelections(QString keyword)
{
    // Clear keyword selections first.
    m_findMatchSelections.clear();

    // Update selections with keyword.
    if (!keyword.isEmpty()) {
        QTextCursor cursor(document());

        QTextDocument::FindFlags flags;
        flags &= QTextDocument::FindCaseSensitively;
        cursor = document()->find(keyword, cursor, flags);

        while (!cursor.isNull()) {
            QTextEdit::ExtraSelection extra;
            extra.format = m_findMatchFormat;
            extra.cursor = cursor;

            cursor = document()->find(keyword, cursor, flags);
            m_findMatchSelections.append(extra);
        }

        setExtraSelections(m_findMatchSelections);
    }
}

void DTextEdit::renderAllSelections()
{
    QList<QTextEdit::ExtraSelection> selections;

//    for (auto findMatch : m_findMatchSelections) {
//        findMatch.format = m_findMatchFormat;
//        selections.append(findMatch);
//    }

    selections.append(m_currentLineSelection);
    selections.append(m_findMatchSelections);
    selections.append(m_findHighlightSelection);
    selections.append(m_wordUnderCursorSelection);
    selections.append(m_beginBracketSelection);
    selections.append(m_endBracketSelection);

    setExtraSelections(selections);
}

QMenu *DTextEdit::getHighlightMenu()
{
    return m_hlGroupMenu;
}

void DTextEdit::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), m_backgroundColor);

    QColor splitLineColor;
    if (QColor(m_backgroundColor).lightness() < 128) {
        splitLineColor = "#ffffff";
    } else {
        splitLineColor = "#000000";
    }
    splitLineColor.setAlphaF(0.05);
    painter.fillRect(QRect(event->rect().x() + event->rect().width() - 1,
                           event->rect().y(), 1, event->rect().height()), splitLineColor);

    int blockNumber = getFirstVisibleBlockId();
    QTextBlock block = document()->findBlockByNumber(blockNumber);
    QTextBlock prev_block = (blockNumber > 0) ? document()->findBlockByNumber(blockNumber-1) : block;
    int translate_y = (blockNumber > 0) ? -verticalScrollBar()->sliderPosition() : 0;

    int top = this->viewport()->geometry().top();

    // Adjust text position according to the previous "non entirely visible" block
    // if applicable. Also takes in consideration the document's margin offset.
    int additional_margin;
    if (blockNumber == 0)
        // Simply adjust to document's margin
        additional_margin = document()->documentMargin() -1 - this->verticalScrollBar()->sliderPosition();
    else
        // Getting the height of the visible part of the previous "non entirely visible" block
        additional_margin = document()->documentLayout()->blockBoundingRect(prev_block)
                .translated(0, translate_y).intersected(this->viewport()->geometry()).height();

    // Shift the starting point
    top += additional_margin;

    int bottom = top + document()->documentLayout()->blockBoundingRect(block).height();

    Utils::setFontSize(painter, document()->defaultFont().pointSize() - 2);
    // Draw the numbers (displaying the current line number in green)
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            if (blockNumber + 1 == m_markStartLine) {
                painter.setPen(m_regionMarkerColor);
            } else {
                painter.setPen(m_lineNumbersColor);
            }

            painter.drawText(0, top,
                             lineNumberArea->width(), fontMetrics().height(),
                             Qt::AlignTop | Qt::AlignHCenter, QString::number(blockNumber + 1));
        }

        block = block.next();
        top = bottom;
        bottom = top + document()->documentLayout()->blockBoundingRect(block).height();
        ++blockNumber;
    }
}

void DTextEdit::updateLineNumber()
{
    // Update line number painter.

    int blockSize = QString::number(blockCount()).size();

    lineNumberArea->setFixedWidth(blockSize * fontMetrics().width('9') + m_lineNumberPaddingX * 4);
    lineNumberArea->update();
}

void DTextEdit::updateWordCount()
{
    m_wrapper->bottomBar()->updateWordCount(characterCount());
}

void DTextEdit::handleScrollFinish()
{
    // Restore cursor postion.
    jumpToLine(m_restoreRow, false);

    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, m_restoreColumn);

    // Update cursor.
    setTextCursor(cursor);
}

bool DTextEdit::setCursorKeywordSeletoin(int position, bool findNext)
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

void DTextEdit::cursorPositionChanged()
{
    m_beginBracketSelection = QTextEdit::ExtraSelection();
    m_endBracketSelection = QTextEdit::ExtraSelection();

    updateHighlightLineSelection();
    updateHighlightBrackets('(', ')');
    updateHighlightBrackets('{', '}');
    updateHighlightBrackets('[', ']');
    renderAllSelections();

    if (m_wrapper) {
        QTextCursor cursor = textCursor();
        m_wrapper->bottomBar()->updatePosition(cursor.blockNumber() + 1,
                                               cursor.columnNumber() + 1);
    }
}

void DTextEdit::updateHighlightBrackets(const QChar &openChar, const QChar &closeChar)
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

int DTextEdit::getFirstVisibleBlockId() const
{
    // Detect the first block for which bounding rect - once translated
    // in absolute coordinated - is contained by the editor's text area

    // Costly way of doing but since "blockBoundingGeometry(...)" doesn't
    // exists for "QTextEdit"...

    QTextCursor curs = QTextCursor(this->document());
    curs.movePosition(QTextCursor::Start);
    for (int i=0; i < this->document()->blockCount(); ++i) {
        QTextBlock block = curs.block();

        QRect r1 = this->viewport()->geometry();
        QRect r2 = this->document()->documentLayout()->blockBoundingRect(block).translated(
                    this->viewport()->geometry().x(), this->viewport()->geometry().y() - (
                        this->verticalScrollBar()->sliderPosition())).toRect();

        if (r1.contains(r2, true)) {
            return i;
        }

        curs.movePosition(QTextCursor::NextBlock);
    }

    return 0;
}

void DTextEdit::setThemeWithPath(const QString &path)
{
    const KSyntaxHighlighting::Theme theme = m_repository.theme("");
    setTheme(theme, path);
}

void DTextEdit::setTheme(const KSyntaxHighlighting::Theme &theme, const QString &path)
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

    if (m_backgroundColor.lightness() < 128) {
        m_highlighter->setTheme(m_repository.defaultTheme(KSyntaxHighlighting::Repository::DarkTheme));
    } else {
        m_highlighter->setTheme(m_repository.defaultTheme(KSyntaxHighlighting::Repository::LightTheme));
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
    if (m_highlighted) {
        m_highlighter->rehighlight();
    }

    lineNumberArea->update();
    highlightCurrentLine();
}

void DTextEdit::loadHighlighter()
{
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
        m_highlighted = false;
    }
}

bool DTextEdit::highlightWordUnderMouse(QPoint pos)
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

        m_wordUnderCursorSelection = selection;

        renderAllSelections();

        return true;
    } else {
        return false;
    }
}

void DTextEdit::removeHighlightWordUnderCursor()
{
    m_highlightWordCacheCursor = m_wordUnderCursorSelection.cursor;

    QTextEdit::ExtraSelection selection;
    m_wordUnderCursorSelection = selection;

    renderAllSelections();
}

void DTextEdit::setSettings(Settings *keySettings)
{
    m_settings = keySettings;
}

void DTextEdit::setModified(bool modified)
{
    document()->setModified(modified);

    emit modificationChanged(filepath, modified);
}

void DTextEdit::copySelectedText()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(textCursor().selection().toPlainText());
    tryUnsetMark();
}

void DTextEdit::cutSelectedText()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(textCursor().selection().toPlainText());

    QTextCursor cursor = textCursor();
    cursor.removeSelectedText();
    setTextCursor(cursor);

    unsetMark();
}

void DTextEdit::pasteText()
{
    QTextEdit::paste();

    unsetMark();
}

void DTextEdit::setMark()
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

void DTextEdit::unsetMark()
{
    bool currentMark = m_cursorMark;

    m_cursorMark = false;

    if (m_cursorMark != currentMark) {
        cursorMarkChanged(m_cursorMark, textCursor());
    }
}

bool DTextEdit::tryUnsetMark()
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

void DTextEdit::exchangeMark()
{
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

void DTextEdit::saveMarkStatus()
{
    m_cursorMarkStatus = m_cursorMark;
    m_cursorMarkPosition = textCursor().anchor();
}

void DTextEdit::restoreMarkStatus()
{
    if (m_cursorMarkStatus) {
        QTextCursor currentCursor = textCursor();

        QTextCursor cursor = textCursor();
        cursor.setPosition(m_cursorMarkPosition, QTextCursor::MoveAnchor);
        cursor.setPosition(currentCursor.position(), QTextCursor::KeepAnchor);

        setTextCursor(cursor);
    }
}

void DTextEdit::clickCutAction()
{
    if (textCursor().hasSelection()) {
        cutSelectedText();
    } else {
        cutWordUnderCursor();
    }
}

void DTextEdit::clickCopyAction()
{
    if (textCursor().hasSelection()) {
        copySelectedText();
    } else {
        copyWordUnderCursor();
    }
}

void DTextEdit::clickPasteAction()
{
    if (textCursor().hasSelection()) {
        pasteText();
    } else {
        QTextCursor cursor;

        // Move to word cursor if have word around mouse.
        // Otherwise find nearest cursor with mouse click.
        if (m_highlightWordCacheCursor.position() != -1) {
            cursor = textCursor();
            cursor.setPosition(m_highlightWordCacheCursor.position(), QTextCursor::MoveAnchor);
        } else {
            auto pos = mapFromGlobal(m_mouseClickPos);
            cursor = cursorForPosition(pos);
        }

        setTextCursor(cursor);

        pasteText();
    }
}

void DTextEdit::clickDeleteAction()
{
    if (textCursor().hasSelection()) {
        textCursor().removeSelectedText();
    } else {
        setTextCursor(m_highlightWordCacheCursor);
        textCursor().removeSelectedText();
    }
}

void DTextEdit::clickOpenInFileManagerAction()
{
    DDesktopServices::showFileItem(filepath);
}

void DTextEdit::copyWordUnderCursor()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(m_highlightWordCacheCursor.selectedText());
}

void DTextEdit::cutWordUnderCursor()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(m_highlightWordCacheCursor.selectedText());

    setTextCursor(m_highlightWordCacheCursor);
    textCursor().removeSelectedText();
}

QString DTextEdit::getWordAtCursor()
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

QString DTextEdit::getWordAtMouse()
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

void DTextEdit::toggleReadOnlyMode()
{
    if (m_readOnlyMode) {
        if (m_cursorMode == Overwrite) {
            emit cursorModeChanged(Overwrite);
        } else {
            emit cursorModeChanged(Insert);
        }

        m_readOnlyMode = false;
        popupNotify(tr("Read-Only mode is off"));
    } else {
        m_readOnlyMode = true;

        popupNotify(tr("Read-Only mode is on"));
        emit cursorModeChanged(Readonly);
    }
}

void DTextEdit::toggleComment()
{
    const auto def = m_repository.definitionForFileName(QFileInfo(filepath).fileName());

    if (!def.filePath().isEmpty()) {
        Comment::unCommentSelection(this, m_commentDefinition);
    } else {
        // do not need to prompt the user.
        // popupNotify(tr("File does not support syntax comments"));
    }
}

int DTextEdit::getNextWordPosition(QTextCursor cursor, QTextCursor::MoveMode moveMode)
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
    }
    // Just to next word-separator char.
    else {
        while (cursor.position() < toPlainText().length() && !atWordSeparator(cursor.position())) {
            cursor.movePosition(QTextCursor::NextCharacter, moveMode);
        }
    }

    return cursor.position();
}

int DTextEdit::getPrevWordPosition(QTextCursor cursor, QTextCursor::MoveMode moveMode)
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

bool DTextEdit::atWordSeparator(int position)
{
    return m_wordSepartors.contains(QString(toPlainText().at(position)));
}

void DTextEdit::showCursorBlink()
{
    // -1 表示恢复Qt的默认值
    QApplication::setCursorFlashTime(-1);
}

void DTextEdit::hideCursorBlink()
{
    QApplication::setCursorFlashTime(0);
}

void DTextEdit::completionWord(QString word)
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

bool DTextEdit::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        m_mouseClickPos = QCursor::pos();

        emit click();
    }

    return false;
}

void DTextEdit::adjustScrollbarMargins()
{
    QEvent event(QEvent::LayoutRequest);
    QApplication::sendEvent(this, &event);

    if (!verticalScrollBar()->visibleRegion().isEmpty()) {
        setViewportMargins(0, 0, -verticalScrollBar()->sizeHint().width(), 0);
    } else {
        setViewportMargins(0, 0, 0, 0);
    }
}

void DTextEdit::dragEnterEvent(QDragEnterEvent *event)
{
    QTextEdit::dragEnterEvent(event);
    qobject_cast<Window *>(this->window())->requestDragEnterEvent(event);
}

void DTextEdit::dragMoveEvent(QDragMoveEvent *event)
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

void DTextEdit::dropEvent(QDropEvent *event)
{
    const QMimeData *data = event->mimeData();

    if (data->hasUrls() && data->urls().first().isLocalFile()) {
        qobject_cast<Window *>(this->window())->requestDropEvent(event);
    } else if (data->hasText() && !m_readOnlyMode) {
        QTextEdit::dropEvent(event);
    }
}

void DTextEdit::inputMethodEvent(QInputMethodEvent *e)
{
    if (!m_readOnlyMode) {
        QTextEdit::inputMethodEvent(e);
    }
}

void DTextEdit::mousePressEvent(QMouseEvent *e)
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

void DTextEdit::mouseMoveEvent(QMouseEvent *e)
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

void DTextEdit::keyPressEvent(QKeyEvent *e)
{
    // if (!isModifier(e)) {
    //     viewport()->setCursor(Qt::BlankCursor);
    // }

    const QString &key = Utils::getKeyshortcut(e);

    if (m_readOnlyMode) {
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
        } else if (key == "Q") {
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
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "togglereadonlymode")) {
            toggleReadOnlyMode();
        } else if (key == "Shift+/" && e->modifiers() == Qt::ControlModifier) {
            e->ignore();
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
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "newline") || key == "Return") {
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
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "selectall")) {
            selectAll();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "copy")) {
            copySelectedText();
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
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "togglereadonlymode")) {
            toggleReadOnlyMode();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "togglecomment")) {
            toggleComment();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "undo")) {
            QTextEdit::undo();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "editor", "redo")) {
            QTextEdit::redo();
        } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "escape")) {
            escape();
        } else if (e->key() == Qt::Key_Insert) {
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
}

void DTextEdit::wheelEvent(QWheelEvent *e)
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

void DTextEdit::contextMenuEvent(QContextMenuEvent *event)
{
    m_rightMenu->clear();

    QString wordAtCursor = getWordAtMouse();

    QTextCursor selectionCursor = textCursor();
    selectionCursor.movePosition(QTextCursor::StartOfBlock);
    selectionCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    QString text = selectionCursor.selectedText();

    // init base.
    bool isBlankLine = text.trimmed().isEmpty();

    if (m_canUndo) {
        m_rightMenu->addAction(m_undoAction);
    }
    if (m_canRedo) {
        m_rightMenu->addAction(m_redoAction);
    }
    m_rightMenu->addSeparator();
    if (textCursor().hasSelection()) {
        m_rightMenu->addAction(m_cutAction);
        m_rightMenu->addAction(m_copyAction);
    } else {
        // Just show copy/cut menu item when cursor rectangle contain moue pointer coordinate.
        m_haveWordUnderCursor = highlightWordUnderMouse(event->pos());
        if (m_haveWordUnderCursor) {
            if (!wordAtCursor.isEmpty()) {
                m_rightMenu->addAction(m_cutAction);
                m_rightMenu->addAction(m_copyAction);
            }
        }
    }
    if (canPaste()) {
        m_rightMenu->addAction(m_pasteAction);
    }

    if (!wordAtCursor.isEmpty()) {
        m_rightMenu->addAction(m_deleteAction);
    }
    if (!toPlainText().isEmpty()) {
        m_rightMenu->addAction(m_selectAllAction);
    }
    m_rightMenu->addSeparator();
    if (!toPlainText().isEmpty()) {
        m_rightMenu->addAction(m_findAction);
        m_rightMenu->addAction(m_replaceAction);
        m_rightMenu->addAction(m_jumpLineAction);
        m_rightMenu->addSeparator();
    }
    if (!wordAtCursor.isEmpty()) {
        m_rightMenu->addMenu(m_convertCaseMenu);
    }

    // intelligent judge whether to support comments.
    const auto def = m_repository.definitionForFileName(QFileInfo(filepath).fileName());
    if (!toPlainText().isEmpty() &&
        (textCursor().hasSelection() || !isBlankLine) &&
        !def.filePath().isEmpty()) {
        m_rightMenu->addAction(m_toggleCommentAction);
    }

    m_rightMenu->addSeparator();
    if (m_readOnlyMode) {
        m_rightMenu->addAction(m_disableReadOnlyModeAction);
    } else {
        m_rightMenu->addAction(m_enableReadOnlyModeAction);
    }
    m_rightMenu->addAction(m_openInFileManagerAction);
    m_rightMenu->addSeparator();
    if (static_cast<Window*>(this->window())->isFullScreen()) {
        m_rightMenu->addAction(m_exitFullscreenAction);
    } else {
        m_rightMenu->addAction(m_fullscreenAction);
    }

    m_rightMenu->exec(event->globalPos());
}

void DTextEdit::highlightCurrentLine()
{
    updateHighlightLineSelection();
    renderAllSelections();

    // Adjust scrollbar margins if reach last line.
    // FIXME(rekols): Temporarily do not need this function.
    // if (getCurrentLine() == blockCount()) {
    //     // Adjust y coordinate up one line height.
    //     m_scrollbarMargin = fontMetrics().height();

    //     adjustScrollbarMargins();

    //     // NOTE: do nextLine make scrollbar adjust y coordinate.
    //     nextLine();
    // } else {
    //     m_scrollbarMargin = 0;

    //     adjustScrollbarMargins();
    // }

    adjustScrollbarMargins();

    // Keep current line at visible area.
    // if (cursorRect().top() + fontMetrics().height() >= rect().height()) {
    //     scrollLineUp();
    // }
}
