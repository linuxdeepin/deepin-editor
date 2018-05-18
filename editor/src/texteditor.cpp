/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 *               2011 ~ 2018 Wang Yong
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Wang Yong <wangyong@deepin.com>
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

#include "texteditor.h"
#include "utils.h"
#include "window.h"

#include "Definition"
#include "SyntaxHighlighter"
#include "Theme"

#include <DDesktopServices>
#include <QApplication>
#include <DSettingsGroup>
#include <DSettingsOption>
#include <DSettings>
#include <QClipboard>
#include <QFileInfo>
#include <QDebug>
#include <QPainter>
#include <QScrollBar>
#include <QStyleFactory>
#include <QTextBlock>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QTimer>

class LineNumberArea : public QWidget
{
public:
    LineNumberArea(TextEditor *editor)
        : QWidget(editor),
          editor(editor) {
    }

    void paintEvent(QPaintEvent *event) {
        editor->lineNumberAreaPaintEvent(event);
    }

    TextEditor *editor;
};

TextEditor::TextEditor(QPlainTextEdit *parent) :
    QPlainTextEdit(parent),
    m_highlighter(new KSyntaxHighlighting::SyntaxHighlighter(document()))
{
    viewport()->installEventFilter(this);

    // Don't draw frame around editor widget.
    setFrameShape(QFrame::NoFrame);

    // Init widgets.
    lineNumberArea = new LineNumberArea(this);

    connect(this, &QPlainTextEdit::updateRequest, this, &TextEditor::handleUpdateRequest);
    connect(this, &QPlainTextEdit::textChanged, this, &TextEditor::updateLineNumber, Qt::QueuedConnection);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &TextEditor::highlightCurrentLine, Qt::QueuedConnection);

    // Init menu.
    rightMenu = new QMenu();
    rightMenu->setStyle(QStyleFactory::create("dlight"));
    undoAction = new QAction("Undo", this);
    redoAction = new QAction("Redo", this);
    cutAction = new QAction("Cut", this);
    copyAction = new QAction("Copy", this);
    pasteAction = new QAction("Paste", this);
    deleteAction = new QAction("Delete", this);
    selectAllAction = new QAction("Select All", this);
    findAction = new QAction("Find", this);
    replaceAction = new QAction("Replace", this);
    jumpLineAction = new QAction("Jump line", this);
    enableEnglishCompleterAction = new QAction("Enable english completer", this);
    disableEnglishCompleterAction = new QAction("Disable english completer", this);
    enableReadOnlyModeAction = new QAction("Turn on read only mode", this);
    disableReadOnlyModeAction = new QAction("Turn off read only mode", this);
    fullscreenAction = new QAction("Fullscreen", this);
    exitFullscreenAction = new QAction("Exit fullscreen", this);
    openInFileManagerAction = new QAction("Open in file manager", this);
    toggleCommentAction = new QAction("Toggle comment", this);
    toggleBulletAction = new QAction("Toggle bullet", this);

    connect(rightMenu, &QMenu::aboutToHide, this, &TextEditor::removeHighlightWordUnderCursor);
    connect(undoAction, &QAction::triggered, this, &TextEditor::undo);
    connect(redoAction, &QAction::triggered, this, &TextEditor::redo);
    connect(cutAction, &QAction::triggered, this, &TextEditor::clickCutAction);
    connect(copyAction, &QAction::triggered, this, &TextEditor::clickCopyAction);
    connect(pasteAction, &QAction::triggered, this, &TextEditor::clickPasteAction);
    connect(deleteAction, &QAction::triggered, this, &TextEditor::clickDeleteAction);
    connect(selectAllAction, &QAction::triggered, this, &TextEditor::selectAll);
    connect(findAction, &QAction::triggered, this, &TextEditor::clickFindAction);
    connect(replaceAction, &QAction::triggered, this, &TextEditor::clickReplaceAction);
    connect(jumpLineAction, &QAction::triggered, this, &TextEditor::clickJumpLineAction);
    connect(fullscreenAction, &QAction::triggered, this, &TextEditor::clickFullscreenAction);
    connect(exitFullscreenAction, &QAction::triggered, this, &TextEditor::clickFullscreenAction);
    connect(enableReadOnlyModeAction, &QAction::triggered, this, &TextEditor::toggleReadOnlyMode);
    connect(disableReadOnlyModeAction, &QAction::triggered, this, &TextEditor::toggleReadOnlyMode);
    connect(enableEnglishCompleterAction, &QAction::triggered, this, &TextEditor::toggleEnglishCompleter);
    connect(disableEnglishCompleterAction, &QAction::triggered, this, &TextEditor::toggleEnglishCompleter);
    connect(openInFileManagerAction, &QAction::triggered, this, &TextEditor::clickOpenInFileManagerAction);
    connect(toggleCommentAction, &QAction::triggered, this, &TextEditor::toggleComment);
    connect(toggleBulletAction, &QAction::triggered, this, &TextEditor::toggleBullet);

    // Init convert case sub menu.
    haveWordUnderCursor = false;
    convertCaseMenu = new QMenu("Convert Case");
    upcaseAction = new QAction("Upcase", this);
    downcaseAction = new QAction("Downcase", this);
    capitalizeAction = new QAction("Capitalize", this);

    convertCaseMenu->addAction(upcaseAction);
    convertCaseMenu->addAction(downcaseAction);
    convertCaseMenu->addAction(capitalizeAction);

    connect(upcaseAction, &QAction::triggered, this, &TextEditor::upcaseWord);
    connect(downcaseAction, &QAction::triggered, this, &TextEditor::downcaseWord);
    connect(capitalizeAction, &QAction::triggered, this, &TextEditor::capitalizeWord);

    canUndo = false;
    canRedo = false;

    connect(this, &TextEditor::undoAvailable, this,
            [=] (bool undoIsAvailable) {
                canUndo = undoIsAvailable;
            });
    connect(this, &TextEditor::redoAvailable, this,
            [=] (bool redoIsAvailable) {
                canRedo = redoIsAvailable;
            });

    // Init scroll animation.
    scrollAnimation = new QPropertyAnimation(verticalScrollBar(), "value");
    scrollAnimation->setEasingCurve(QEasingCurve::InOutExpo);
    scrollAnimation->setDuration(300);

    connect(scrollAnimation, &QPropertyAnimation::finished, this, &TextEditor::handleScrollFinish, Qt::QueuedConnection);

    // Highlight line and focus.
    highlightCurrentLine();
    QTimer::singleShot(0, this, SLOT(setFocus()));

    // Init change cursor width timer.
    changeCursorWidthTimer = new QTimer(this);
    changeCursorWidthTimer->setSingleShot(true);
    connect(changeCursorWidthTimer, &QTimer::timeout, this, &TextEditor::changeToWaitCursor);

    changeToWaitCursor();

    // Monitor cursor mark status to update in line number area.
    connect(this, &TextEditor::cursorMarkChanged, this, &TextEditor::handleCursorMarkChanged);

    // Init english helper timer.
    englishHelperTimer = new QTimer(this);
    englishHelperTimer->setSingleShot(true);
    connect(englishHelperTimer, &QTimer::timeout, this, &TextEditor::tryCompleteWord);

    // configure content area
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    
    connect(verticalScrollBar(), &QScrollBar::rangeChanged, 
            this, 
            [=]() {
                if (isVisible() && !scrollbarLock) {
                    scrollbarLock = true;
                    auto documentHeight = (verticalScrollBar()->maximum() - verticalScrollBar()->minimum() + verticalScrollBar()->pageStep()) * fontMetrics().height();
                    if (documentHeight > rect().height()) {
                        setViewportMargins(0, 0, -verticalScrollBar()->sizeHint().width(), -horizontalScrollBar()->sizeHint().height());
                    } else {
                        setViewportMargins(0, 0, 0, 0);
                    }
                    scrollbarLock = false;
                }
            });
}

int TextEditor::getCurrentLine()
{
    return textCursor().blockNumber() + 1;
}

int TextEditor::getCurrentColumn()
{
    return textCursor().columnNumber();
}

int TextEditor::getPosition()
{
    return textCursor().position();
}

int TextEditor::getScrollOffset()
{
    QScrollBar *scrollbar = verticalScrollBar();

    return scrollbar->value();
}

void TextEditor::forwardChar()
{
    if (cursorMark) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        moveCursor(QTextCursor::NextCharacter);
    }
}

void TextEditor::backwardChar()
{
    if (cursorMark) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        moveCursor(QTextCursor::PreviousCharacter);
    }
}

void TextEditor::forwardWord()
{
    QTextCursor cursor = textCursor();

    if (cursorMark) {
        cursor.setPosition(getNextWordPosition(cursor, QTextCursor::KeepAnchor), QTextCursor::KeepAnchor);
    } else {
        cursor.setPosition(getNextWordPosition(cursor, QTextCursor::MoveAnchor), QTextCursor::MoveAnchor);
    }

    setTextCursor(cursor);
}

void TextEditor::backwardWord()
{
    QTextCursor cursor = textCursor();

    if (cursorMark) {
        cursor.setPosition(getPrevWordPosition(cursor, QTextCursor::KeepAnchor), QTextCursor::KeepAnchor);
    } else {
        cursor.setPosition(getPrevWordPosition(cursor, QTextCursor::MoveAnchor), QTextCursor::MoveAnchor);
    }

    setTextCursor(cursor);
}

void TextEditor::forwardPair()
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
        auto moveMode = cursorMark ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;

        if (actionStartPos == selectionStartPos) {
            cursor.setPosition(selectionEndPos, QTextCursor::MoveAnchor);
            cursor.setPosition(findPos, moveMode);
        } else {
            cursor.setPosition(selectionStartPos, QTextCursor::MoveAnchor);
            cursor.setPosition(findPos, moveMode);
        }
        
        setTextCursor(cursor);
        
        // Keep current line at visible area when find next pair.
        if (cursorRect().top() + fontMetrics().height() >= rect().height()) {
            scrollLineUp();
        }
    }
}

void TextEditor::backwardPair()
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
        auto moveMode = cursorMark ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;

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

void TextEditor::moveToStart()
{
    if (cursorMark) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Start, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        moveCursor(QTextCursor::Start);
    }
}

void TextEditor::moveToEnd()
{
    if (cursorMark) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        moveCursor(QTextCursor::End);
    }
}

void TextEditor::moveToStartOfLine()
{
    if (cursorMark) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        moveCursor(QTextCursor::StartOfBlock);
    }
}

void TextEditor::moveToEndOfLine()
{
    if (cursorMark) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        moveCursor(QTextCursor::EndOfBlock);
    }
}

void TextEditor::moveToLineIndentation()
{
    // Init cursor and move type.
    QTextCursor cursor = textCursor();
    auto moveMode = cursorMark ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;

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

void TextEditor::nextLine()
{
    if (cursorMark) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        moveCursor(QTextCursor::Down);
    }
}

void TextEditor::prevLine()
{
    if (cursorMark) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Up, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    } else {
        moveCursor(QTextCursor::Up);
    }
}

void TextEditor::jumpToLine(int line, bool keepLineAtCenter)
{
    QTextCursor cursor(document()->findBlockByNumber(line - 1)); // line - 1 because line number starts from 0

    // Update cursor.
    setTextCursor(cursor);

    if (keepLineAtCenter) {
        keepCurrentLineAtCenter();
    }
}

void TextEditor::newline()
{
    // Stop mark if mark is set.
    tryUnsetMark();

    QTextCursor cursor = textCursor();
    cursor.insertText("\n");
    setTextCursor(cursor);
}

void TextEditor::openNewlineAbove()
{
    // Stop mark if mark is set.
    tryUnsetMark();

    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
    cursor.insertText("\n");
    cursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor);

    setTextCursor(cursor);
}

void TextEditor::openNewlineBelow()
{
    // Stop mark if mark is set.
    tryUnsetMark();

    moveCursor(QTextCursor::EndOfBlock);
    textCursor().insertText("\n");
}

void TextEditor::swapLineUp()
{
    if (textCursor().hasSelection()) {
        bool cursorAtSelectionStart = (textCursor().position() == textCursor().selectionStart());

        // Get selection bound.
        int startPos = textCursor().anchor();
        int endPos = textCursor().position();

        if (startPos > endPos) {
            std::swap(startPos, endPos);
        }

        // Expand selection to multi-lines bound.
        QTextCursor startCursor = textCursor();
        startCursor.setPosition(startPos, QTextCursor::MoveAnchor);
        int startCursorColumn = startCursor.columnNumber();
        startCursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);

        QTextCursor endCursor = textCursor();
        endCursor.setPosition(endPos, QTextCursor::MoveAnchor);
        int endCursorColumn = endCursor.columnNumber();
        endCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::MoveAnchor);

        QTextCursor cursor = textCursor();
        cursor.setPosition(startCursor.position(), QTextCursor::MoveAnchor);
        cursor.setPosition(endCursor.position(), QTextCursor::KeepAnchor);

        setTextCursor(cursor);

        // Get multi-line selection text.
        QString newTop = cursor.selectedText();
        cursor.removeSelectedText();

        // Get one-line content above multi-lines selection.
        cursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor);
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        QString newBottom = cursor.selectedText();
        cursor.removeSelectedText();

        // Record new selection bound of multi-lines.
        int newSelectionStartPos = cursor.position();
        cursor.insertText(newTop);
        int newSelectionEndPos = cursor.position();
        cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor);
        cursor.insertText(newBottom);

        // Reset multi-line selection status.
        if (cursorAtSelectionStart) {
            cursor.setPosition(newSelectionEndPos, QTextCursor::MoveAnchor);
            cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
            cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, endCursorColumn);
            cursor.setPosition(newSelectionStartPos, QTextCursor::KeepAnchor);
            cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, startCursorColumn);
        } else {
            cursor.setPosition(newSelectionStartPos, QTextCursor::MoveAnchor);
            cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, startCursorColumn);
            cursor.setPosition(newSelectionEndPos, QTextCursor::KeepAnchor);
            cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
            cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, endCursorColumn);
        }

        // Update cursor.
        setTextCursor(cursor);
    } else {
        QTextCursor cursor = textCursor();

        // Rember current line's column number.
        int column = cursor.columnNumber();

        // Get current line content.
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        QString newTop = cursor.selectedText();
        cursor.removeSelectedText();

        // Get line content above current line.
        // Note: we need move cursor UP and then use *StartOfBlock*, keep this order.
        // don't use *StartOfBlock* before *UP*, it's won't work if above line is *wrap line*.
        cursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor);
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        QString newBottom = cursor.selectedText();
        cursor.removeSelectedText();

        // Swap line content.
        cursor.insertText(newTop);
        cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor);
        cursor.insertText(newBottom);

        // Move cursor to new start of line.
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        cursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor);

        // Restore cursor's column.
        cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, column);

        // Update cursor.
        setTextCursor(cursor);
    }
}

void TextEditor::swapLineDown()
{
    if (textCursor().hasSelection()) {
        bool cursorAtSelectionStart = (textCursor().position() == textCursor().selectionStart());

        // Get selection bound.
        int startPos = textCursor().anchor();
        int endPos = textCursor().position();

        if (startPos > endPos) {
            std::swap(startPos, endPos);
        }

        // Expand selection to lines bound.
        QTextCursor startCursor = textCursor();
        startCursor.setPosition(startPos, QTextCursor::MoveAnchor);
        int startCursorColumn = startCursor.columnNumber();
        startCursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);

        QTextCursor endCursor = textCursor();
        endCursor.setPosition(endPos, QTextCursor::MoveAnchor);
        int endCursorColumn = endCursor.columnNumber();
        endCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::MoveAnchor);

        QTextCursor cursor = textCursor();
        cursor.setPosition(startCursor.position(), QTextCursor::MoveAnchor);
        cursor.setPosition(endCursor.position(), QTextCursor::KeepAnchor);

        setTextCursor(cursor);

        // Get multi-line selection content.
        QString newBottom = cursor.selectedText();
        cursor.removeSelectedText();

        // Get line content below multi-line selection.
        cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor);
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        QString newTop = cursor.selectedText();
        cursor.removeSelectedText();

        // Record new selection bound after swap content.
        cursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor);
        cursor.insertText(newTop);

        cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor);
        int newSelectionStartPos = cursor.position();
        cursor.insertText(newBottom);
        int newSelectionEndPos = cursor.position();

        // Reset selection bound for multi-line content.
        if (cursorAtSelectionStart) {
            cursor.setPosition(newSelectionEndPos, QTextCursor::MoveAnchor);
            cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
            cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, endCursorColumn);
            cursor.setPosition(newSelectionStartPos, QTextCursor::KeepAnchor);
            cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, startCursorColumn);
        } else {
            cursor.setPosition(newSelectionStartPos, QTextCursor::MoveAnchor);
            cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, startCursorColumn);
            cursor.setPosition(newSelectionEndPos, QTextCursor::KeepAnchor);
            cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
            cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, endCursorColumn);
        }

        // Update cursor.
        setTextCursor(cursor);
    } else {
        QTextCursor cursor = textCursor();

        // Rember current line's column number.
        int column = cursor.columnNumber();

        // Get current line content.
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        QString newBottom = cursor.selectedText();
        cursor.removeSelectedText();

        // Get line content below current line.
        cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor);
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        QString newTop = cursor.selectedText();
        cursor.removeSelectedText();

        // Swap content.
        cursor.insertText(newBottom);
        cursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor);
        cursor.insertText(newTop);

        // Move new start of line.
        cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::MoveAnchor);

        // Restore cursor's column.
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, column);

        // Update cursor.
        setTextCursor(cursor);
    }
}

void TextEditor::scrollLineUp()
{
    QScrollBar *scrollbar = verticalScrollBar();

    scrollbar->setValue(scrollbar->value() + 1);

    if (cursorRect().y() < 0) {
        auto moveMode = cursorMark ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;

        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Down, moveMode);
        setTextCursor(cursor);
    }
}

void TextEditor::scrollLineDown()
{
    QScrollBar *scrollbar = verticalScrollBar();

    scrollbar->setValue(scrollbar->value() - 1);

    if (cursorRect().y() > rect().height() - fontMetrics().height()) {
        auto moveMode = cursorMark ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;

        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Up, moveMode);
        setTextCursor(cursor);
    }
}

void TextEditor::scrollUp()
{
    QScrollBar *scrollbar = verticalScrollBar();

    int lines = rect().height() / fontMetrics().height();

    scrollbar->setValue(scrollbar->value() + lines);

    if (scrollbar->value() >= getCurrentLine()) {
        auto moveMode = cursorMark ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;

        int line = scrollbar->value();
        QTextCursor lineCursor(document()->findBlockByLineNumber(line - 1)); // line - 1 because line number starts from 0

        QTextCursor cursor = textCursor();
        cursor.setPosition(lineCursor.position(), moveMode);
        setTextCursor(cursor);
    }
}

void TextEditor::scrollDown()
{
    QScrollBar *scrollbar = verticalScrollBar();

    int lines = rect().height() / fontMetrics().height();

    scrollbar->setValue(scrollbar->value() - lines);

    auto moveMode = cursorMark ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;

    int line = scrollbar->value() + lines;
    QTextCursor lineCursor(document()->findBlockByLineNumber(line - 1)); // line - 1 because line number starts from 0

    QTextCursor cursor = textCursor();
    cursor.setPosition(lineCursor.position(), moveMode);
    setTextCursor(cursor);
}

void TextEditor::duplicateLine()
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

void TextEditor::copyLines()
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

        popupNotify("已经拷贝选中行到剪切板");
    } else {
        // Selection current line.
        copyCursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        copyCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

        popupNotify("已经拷贝当前行到剪切板");
    }

    // Copy lines to system clipboard.
    setTextCursor(copyCursor);
    copySelectedText();

    // Reset cursor before copy lines.
    copyCursor.setPosition(currentCursor.position(), QTextCursor::MoveAnchor);
    setTextCursor(copyCursor);
}

void TextEditor::cutlines()
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

        popupNotify("已经剪切选中行到剪切板");
    } else {
        // Selection current line.
        copyCursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        copyCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

        popupNotify("已经剪切当前行到剪切板");
    }

    // Copy lines to system clipboard.
    setTextCursor(copyCursor);
    cutSelectedText();

    // Reset cursor before copy lines.
    copyCursor.setPosition(currentCursor.position(), QTextCursor::MoveAnchor);
    setTextCursor(copyCursor);
}

void TextEditor::joinLines()
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

void TextEditor::killLine()
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

void TextEditor::killCurrentLine()
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

void TextEditor::killBackwardWord()
{
    tryUnsetMark();

    if (textCursor().hasSelection()) {
        textCursor().removeSelectedText();
    } else {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::NoMove, QTextCursor::MoveAnchor);
        cursor.setPosition(getPrevWordPosition(cursor, QTextCursor::KeepAnchor), QTextCursor::KeepAnchor);
        cursor.removeSelectedText();

        setTextCursor(cursor);
    }
}

void TextEditor::killForwardWord()
{
    tryUnsetMark();

    if (textCursor().hasSelection()) {
        textCursor().removeSelectedText();
    } else {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::NoMove, QTextCursor::MoveAnchor);
        cursor.setPosition(getNextWordPosition(cursor, QTextCursor::KeepAnchor), QTextCursor::KeepAnchor);
        cursor.removeSelectedText();

        setTextCursor(cursor);
    }
}

void TextEditor::indentLine()
{
    // Stop mark if mark is set.
    tryUnsetMark();

    // Save cursor column.
    int column = getCurrentColumn();

    // If current column is not Multiples of 4, jump to 4x position before next indent.
    moveToLineIndentation();
    int indentSpace = tabSpaceNumber - (getCurrentColumn() % tabSpaceNumber);

    // Insert spaces.
    moveToStartOfLine();
    QString spaces(indentSpace, ' ');
    textCursor().insertText(spaces);

    // Restore cursor column postion.
    moveToStartOfLine();
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, column + indentSpace);
    setTextCursor(cursor);
}

void TextEditor::backIndentLine()
{
    // Stop mark if mark is set.
    tryUnsetMark();

    // Save cursor column.
    int column = getCurrentColumn();

    // If current column is not Multiples of 4, jump to 4x position before back indent.
    moveToLineIndentation();

    if (getCurrentColumn() > 0) {
        int indentSpace = getCurrentColumn() % tabSpaceNumber;
        if (indentSpace == 0 && getCurrentColumn() >= tabSpaceNumber) {
            indentSpace = tabSpaceNumber;
        }

        // Remove spaces.
        QTextCursor deleteCursor = textCursor();
        for (int i = 0; i < indentSpace; i++) {
            deleteCursor.deletePreviousChar();
        }
        setTextCursor(deleteCursor);

        // Restore cursor column postion.
        moveToStartOfLine();
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, column - indentSpace);
        setTextCursor(cursor);
    }
}

void TextEditor::setTabSpaceNumber(int number)
{
    tabSpaceNumber = number;
}

void TextEditor::upcaseWord()
{
    tryUnsetMark();

    convertWordCase(UPPER);
}

void TextEditor::downcaseWord()
{
    tryUnsetMark();

    convertWordCase(LOWER);
}

void TextEditor::capitalizeWord()
{
    tryUnsetMark();

    convertWordCase(CAPITALIZE);
}

void TextEditor::transposeChar()
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

void TextEditor::changeToEditCursor()
{
    setCursorWidth(cursorNormalWidth);

    // Need repaint after change to edit stauts,
    // avoid cursor width not flash after press key.
    repaint();
}

void TextEditor::changeToWaitCursor()
{
    QTextCursor cursor = textCursor();
    cursor.setPosition(cursor.position(), QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
    QString currentChar = cursor.selectedText();
    
    // qDebug() << QString("'%1'").arg(currentChar);
    
    // Convert TAB char to one single space.
    if (currentChar == "\t") {
        currentChar = " ";
    }
    
    setCursorWidth(std::max(cursorNormalWidth, (fontMetrics().width(currentChar))));
}

void TextEditor::handleCursorMarkChanged(bool mark, QTextCursor cursor)
{
    if (mark) {
        markStartLine = cursor.blockNumber() + 1;
    } else {
        markStartLine = -1;
    }

    lineNumberArea->update();
}

void TextEditor::convertWordCase(ConvertCase convertCase)
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
        if (haveWordUnderCursor) {
            setTextCursor(wordUnderPointerCursor);
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

        haveWordUnderCursor = false;
    }
}

QString TextEditor::capitalizeText(QString text)
{
    QString newText = text.toLower();
    QChar currentChar;
    for (int i = 0; i < newText.size(); i++) {
        currentChar = newText.at(i);
        if (!currentChar.isSpace() && !wordSepartors.contains(QString(currentChar))) {
            newText.replace(i, 1, currentChar.toUpper());
            break;
        }
    }

    return newText;
}

void TextEditor::keepCurrentLineAtCenter()
{
    QScrollBar *scrollbar = verticalScrollBar();

    int currentLine = cursorRect().top() / cursorRect().height();
    int halfEditorLines = rect().height() / 2 / cursorRect().height();
    scrollbar->setValue(scrollbar->value() + currentLine - halfEditorLines);
}

void TextEditor::scrollToLine(int scrollOffset, int row, int column)
{
    // Save cursor postion.
    restoreRow = row;
    restoreColumn = column;

    // Start scroll animation.
    scrollAnimation->setStartValue(verticalScrollBar()->value());
    scrollAnimation->setEndValue(scrollOffset);
    scrollAnimation->start();
}

void TextEditor::setFontFamily(QString name)
{
    // Update font.
    fontName = name;
    updateFont();
}

void TextEditor::setFontSize(int size)
{
    // Update font.
    fontSize = size;
    updateFont();

    // Update line number after adjust font size.
    updateLineNumber();
}

void TextEditor::updateFont()
{
    QFont font = document()->defaultFont();
    font.setFixedPitch(true);
    font.setPointSize(fontSize);
    font.setFamily(fontName);
    setFont(font);
}

void TextEditor::replaceAll(QString replaceText, QString withText)
{
    QTextCursor cursor = textCursor();

    cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    QString text = cursor.selectedText();

    cursor.insertText(text.replace(replaceText, withText, Qt::CaseInsensitive));
    cursor.clearSelection();

    // Update cursor.
    setTextCursor(cursor);

    highlightKeyword(replaceText, getPosition());
}

void TextEditor::replaceNext(QString replaceText, QString withText)
{
    QTextCursor cursor = textCursor();

    cursor.setPosition(cursorKeywordSelection.cursor.position() - replaceText.size());
    cursor.movePosition(QTextCursor::NoMove, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, replaceText.size());
    cursor.insertText(withText);

    // Update cursor.
    setTextCursor(cursor);

    highlightKeyword(replaceText, getPosition());
}

void TextEditor::replaceRest(QString replaceText, QString withText)
{
    // If replace text is nothing, don't do replace action.
    if (replaceText.size() == 0) {
        qDebug() << "Replace text is empty.";
        return;
    }
    
    // Try get replace text in rest content.
    QTextCursor cursor = textCursor();
    cursor.setPosition(cursorKeywordSelection.cursor.position() - replaceText.size());
    cursor.movePosition(QTextCursor::NoMove, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    QString text = cursor.selectedText();
    QString textAfterReplace = cursor.selectedText().replace(replaceText, withText, Qt::CaseInsensitive);
    
    // Don't move cursor if nothing need to replace in rest content.
    if (text == textAfterReplace) {
        qDebug() << "Nothing need replace in rest content.";
        return;
    }
    
    // If rest content can replace, variable keywordSelections must have items.
    if (keywordSelections.size() == 0) {
        qDebug() << "The code of TextEditor::replaceRest is wrong, need review.";
        return;
    }
    
    // Get last keyword position.
    auto lastKeywordPosition = keywordSelections.last().cursor.position();

    // Replace file content.
    cursor.insertText(textAfterReplace);
    cursor.clearSelection();
    setTextCursor(cursor);

    // Re-highlight keywords.
    highlightKeyword(replaceText, getPosition());

    // Restore last keyword position.
    QTextCursor lastKeywordCursor = textCursor();
    lastKeywordCursor.setPosition(lastKeywordPosition);
    setTextCursor(lastKeywordCursor);
}

bool TextEditor::findKeywordForward(QString keyword)
{
    if (textCursor().hasSelection()) {
        // Get selection bound.
        int startPos = textCursor().anchor();
        int endPos = textCursor().position();

        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
        setTextCursor(cursor);

        QTextDocument::FindFlags options;
        options |= QTextDocument::FindCaseSensitively;
        
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
        options |= QTextDocument::FindCaseSensitively;
        
        bool foundOne = find(keyword, options);

        setTextCursor(recordCursor);

        return foundOne;
    }
}

void TextEditor::removeKeywords()
{
    cursorKeywordSelection.cursor = textCursor();
    cursorKeywordSelection.cursor.clearSelection();

    keywordSelections.clear();

    updateHighlightLineSeleciton();

    renderAllSelections();

    setFocus();
}

void TextEditor::highlightKeyword(QString keyword, int position)
{
    updateKeywordSelections(keyword);

    updateCursorKeywordSelection(position, true);

    updateHighlightLineSeleciton();

    renderAllSelections();
}

void TextEditor::updateCursorKeywordSelection(int position, bool findNext)
{
    bool findOne = setCursorKeywordSeletoin(position, findNext);

    if (!findOne) {
        if (findNext) {
            // Clear keyword if keyword not match anything.
            if (!setCursorKeywordSeletoin(0, findNext)) {
                cursorKeywordSelection.cursor = textCursor();

                keywordSelections.clear();
                renderAllSelections();
            }
        } else {
            QTextCursor cursor = textCursor();
            cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);

            setCursorKeywordSeletoin(cursor.position(), findNext);
        }
    }
}

void TextEditor::updateHighlightLineSeleciton()
{
    QTextEdit::ExtraSelection selection;

    selection.format.setBackground(currentLineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();

    currentLineSelection = selection;
}

void TextEditor::updateKeywordSelections(QString keyword)
{
    // Clear keyword selections first.
    keywordSelections.clear();

    // Update selections with keyword.
    if (keyword != "") {
        moveCursor(QTextCursor::Start);

        QTextDocument::FindFlags options;
        options |= QTextDocument::FindCaseSensitively;
        
        while(find(keyword, options)) {
            QTextEdit::ExtraSelection extra;

            QPen outline(selectionColor.lighter(120), 1, Qt::SolidLine);
            extra.format.setProperty(QTextFormat::OutlinePen, outline);

            QBrush brush(selectionColor);
            extra.format.setProperty(QTextFormat::BackgroundBrush, brush);

            extra.cursor = textCursor();
            keywordSelections.append(extra);
        }

        setExtraSelections(keywordSelections);
    }
}

void TextEditor::renderAllSelections()
{
    QList<QTextEdit::ExtraSelection> selections;

    selections.append(currentLineSelection);
    selections.append(keywordSelections);
    selections.append(cursorKeywordSelection);
    selections.append(wordUnderCursorSelection);

    setExtraSelections(selections);
}

void TextEditor::keyPressEvent(QKeyEvent *keyEvent)
{
    // Change cursor to edit status and start timer to restore to wait status.
    changeToEditCursor();
    if (changeCursorWidthTimer->isActive()) {
        changeCursorWidthTimer->stop();
    }
    changeCursorWidthTimer->start(cursorWidthChangeDelay);

    QString key = Utils::getKeyshortcut(keyEvent);
    
    // Debug usage.
    // qDebug() << key;

    if (readOnlyMode) {
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
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "togglereadonlymode")) {
            toggleReadOnlyMode();
        }
    } else {
        if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "indentline")) {
            indentLine();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "backindentline")) {
            backIndentLine();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "forwardchar")) {
            forwardChar();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "backwardchar")) {
            backwardChar();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "forwardword")) {
            forwardWord();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "backwardword")) {
            backwardWord();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "nextline")) {
            nextLine();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "prevline")) {
            prevLine();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "newline") || key == "Return") {
            if (static_cast<Window*>(this->window())->wordCompletionWindowIsVisible()) {
                confirmCompletionFlag = true;
                confirmCompletion();
            } else {
                newline();
            }
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "opennewlineabove")) {
            openNewlineAbove();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "opennewlinebelow")) {
            openNewlineBelow();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "duplicateline")) {
            duplicateLine();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "killline")) {
            killLine();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "killcurrentline")) {
            killCurrentLine();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "swaplineup")) {
            swapLineUp();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "swaplinedown")) {
            swapLineDown();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "scrolllineup")) {
            scrollLineUp();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "scrolllinedown")) {
            scrollLineDown();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "scrollup")) {
            scrollUp();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "scrolldown")) {
            scrollDown();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "movetoendofline")) {
            moveToEndOfLine();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "movetostartofline")) {
            moveToStartOfLine();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "movetostart")) {
            moveToStart();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "movetoend")) {
            moveToEnd();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "movetolineindentation")) {
            moveToLineIndentation();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "upcaseword")) {
            upcaseWord();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "downcaseword")) {
            downcaseWord();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "capitalizeword")) {
            capitalizeWord();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "killbackwardword")) {
            killBackwardWord();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "killforwardword")) {
            killForwardWord();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "forwardpair")) {
            forwardPair();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "backwardpair")) {
            backwardPair();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "transposechar")) {
            transposeChar();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "selectall")) {
            selectAll();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "copy")) {
            copySelectedText();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "cut")) {
            cutSelectedText();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "paste")) {
            pasteText();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "setmark")) {
            setMark();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "exchangemark")) {
            exchangeMark();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "copylines")) {
            copyLines();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "cutlines")) {
            cutlines();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "joinlines")) {
            joinLines();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "selectnextcompletion")) {
            selectNextCompletion();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "selectprevcompletion")) {
            selectPrevCompletion();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "selectfirstcompletion")) {
            selectFirstCompletion();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "selectlastcompletion")) {
            selectLastCompletion();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "toggleenglishcompleter")) {
            toggleEnglishCompleter();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "togglereadonlymode")) {
            toggleReadOnlyMode();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "togglecomment")) {
            toggleComment();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "togglebullet")) {
            toggleBullet();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "undo")) {
            undo();
        } else if (key == Utils::getKeyshortcutFromKeymap(settings, "editor", "redo")) {
            redo();
        } else if (key == "Esc") {
            emit pressEsc();
        } else {
            // Post event to window widget if key match window key list.
            for (auto option : settings->settings->group("shortcuts.window")->options()) {
                if (key == settings->settings->option(option->key())->value().toString()) {
                    keyEvent->ignore();
                    return;
                }
            }

            // Post event to window widget if match Alt+0 ~ Alt+9
            QRegularExpression re("^Alt\\+\\d");
            QRegularExpressionMatch match = re.match(key);
            if (match.hasMatch()) {
                keyEvent->ignore();
                return;
            }

            // Text editor handle key self.
            QPlainTextEdit::keyPressEvent(keyEvent);
        }
    }
}

void TextEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    // Init.
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), backgroundColor);

    QColor splitLineColor;
    if (QColor(backgroundColor).lightness() < 128) {
        splitLineColor = "#ffffff";
    } else {
        splitLineColor = "#000000";
    }
    splitLineColor.setAlphaF(0.05);
    painter.fillRect(QRect(event->rect().x() + event->rect().width() - 1, event->rect().y(), 1, event->rect().height()), splitLineColor);

    // Update line number.
    QTextBlock block = firstVisibleBlock();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();
    int linenumber = block.blockNumber();

    Utils::setFontSize(painter, document()->defaultFont().pointSize() - 2);
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            if (linenumber + 1 == markStartLine) {
                painter.setPen(regionMarkerColor);
            } else {
                painter.setPen(lineNumbersColor);
            }
            painter.drawText(0,
                             top,
                             lineNumberArea->width(),
                             blockBoundingRect(block).height(),
                             Qt::AlignCenter,
                             QString::number(linenumber + 1));
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();

        ++linenumber;
    }
}

void TextEditor::contextMenuEvent(QContextMenuEvent *event)
{
    rightMenu->clear();

    QString wordAtCursor = getWordAtMouse();

    QTextCursor selectionCursor = textCursor();
    selectionCursor.movePosition(QTextCursor::StartOfBlock);
    selectionCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    QString text = selectionCursor.selectedText();

    bool isBlankLine = text.trimmed().size() == 0;

    if (canUndo) {
        rightMenu->addAction(undoAction);
    }
    if (canRedo) {
        rightMenu->addAction(redoAction);
    }
    rightMenu->addSeparator();
    if (textCursor().hasSelection()) {
        rightMenu->addAction(cutAction);
        rightMenu->addAction(copyAction);
    } else {
        // Just show copy/cut menu item when cursor rectangle contain moue pointer coordinate.
        haveWordUnderCursor = highlightWordUnderMouse(event->pos());
        if (haveWordUnderCursor) {
            if (wordAtCursor != "") {
                rightMenu->addAction(cutAction);
                rightMenu->addAction(copyAction);
            }
        }
    }
    if (canPaste()) {
        rightMenu->addAction(pasteAction);
    }

    if (wordAtCursor != "") {
        rightMenu->addAction(deleteAction);
    }
    if (toPlainText() != "") {
        rightMenu->addAction(selectAllAction);
    }
    rightMenu->addSeparator();
    if (toPlainText() != "") {
        rightMenu->addAction(findAction);
        rightMenu->addAction(replaceAction);
        rightMenu->addAction(jumpLineAction);
        rightMenu->addSeparator();
    }
    if (wordAtCursor != "") {
        rightMenu->addMenu(convertCaseMenu);
    }
    if (toPlainText() != "" && (textCursor().hasSelection() || !isBlankLine)) {
        rightMenu->addAction(toggleCommentAction);
        rightMenu->addAction(toggleBulletAction);
    }
    rightMenu->addSeparator();
    if (enableEnglishCompleter) {
        rightMenu->addAction(disableEnglishCompleterAction);
    } else {
        rightMenu->addAction(enableEnglishCompleterAction);
    }
    if (readOnlyMode) {
        rightMenu->addAction(disableReadOnlyModeAction);
    } else {
        rightMenu->addAction(enableReadOnlyModeAction);
    }
    rightMenu->addAction(openInFileManagerAction);
    rightMenu->addSeparator();
    if (static_cast<Window*>(this->window())->isFullScreen()) {
        rightMenu->addAction(exitFullscreenAction);
    } else {
        rightMenu->addAction(fullscreenAction);
    }

    rightMenu->exec(event->globalPos());
}

void TextEditor::highlightCurrentLine()
{
    updateHighlightLineSeleciton();
    renderAllSelections();
}

void TextEditor::updateLineNumber()
{
    // Update line number painter.
    lineNumberArea->setFixedWidth(QString("%1").arg(blockCount()).size() * fontMetrics().width('9') + lineNumberPaddingX * 2);

    // Try complete words.
    if (englishHelperTimer->isActive()) {
        englishHelperTimer->stop();
    }

    if (enableEnglishCompleter && !confirmCompletionFlag) {
        englishHelperTimer->start(500);
    } else {
        confirmCompletionFlag = false;
    }
}

void TextEditor::handleScrollFinish()
{
    // Restore cursor postion.
    jumpToLine(restoreRow, false);

    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, restoreColumn);

    // Update cursor.
    setTextCursor(cursor);
}

void TextEditor::handleUpdateRequest(const QRect &rect, int dy)
{
    if (dy) {
        lineNumberArea->scroll(0, dy);
    } else {
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    }
}

bool TextEditor::setCursorKeywordSeletoin(int position, bool findNext)
{
    int offsetLines = 3;
    
    if (findNext) {
        for (int i = 0; i < keywordSelections.size(); i++) {
            if (keywordSelections[i].cursor.position() > position) {
                cursorKeywordSelection.cursor = keywordSelections[i].cursor;

                QBrush brush(searchHighlightColor);
                cursorKeywordSelection.format.setProperty(QTextFormat::BackgroundBrush, brush);

                jumpToLine(keywordSelections[i].cursor.blockNumber() + offsetLines, false);

                QTextCursor cursor = textCursor();
                cursor.setPosition(keywordSelections[i].cursor.position());

                // Update cursor.
                setTextCursor(cursor);

                return true;
            }
        }
    } else {
        for (int i = keywordSelections.size() - 1; i >= 0; i--) {
            if (keywordSelections[i].cursor.position() < position) {
                cursorKeywordSelection.cursor = keywordSelections[i].cursor;

                QBrush brush(searchHighlightColor);
                cursorKeywordSelection.format.setProperty(QTextFormat::BackgroundBrush, brush);

                jumpToLine(keywordSelections[i].cursor.blockNumber() + offsetLines, false);

                QTextCursor cursor = textCursor();
                cursor.setPosition(keywordSelections[i].cursor.position());

                // Update cursor.
                setTextCursor(cursor);

                return true;
            }
        }
    }

    return false;
}

void TextEditor::setThemeWithName(QString themeName)
{
    const auto theme = m_repository.theme(themeName);
    setTheme(theme, themeName);
}

void TextEditor::setTheme(const KSyntaxHighlighting::Theme &theme, QString themeName)
{
    QVariantMap jsonMap = Utils::getThemeNodeMap(themeName);
    auto themeBackgroundColor = jsonMap["editor-colors"].toMap()["background-color"].toString();
    auto themeCurrentLineColor = jsonMap["editor-colors"].toMap()["current-line"].toString();

    auto pal = qApp->palette();
    if (theme.isValid()) {
        pal.setColor(QPalette::Base, QColor(themeBackgroundColor));
        pal.setColor(QPalette::Text, QColor(theme.textColor(KSyntaxHighlighting::Theme::Normal)));
        pal.setColor(QPalette::Highlight, QColor(theme.backgroundColor(KSyntaxHighlighting::Theme::RegionMarker)));
        pal.setColor(QPalette::HighlightedText, QColor(theme.selectedTextColor(KSyntaxHighlighting::Theme::RegionMarker)));
    }
    viewport()->setPalette(pal);
    viewport()->setAutoFillBackground(true);

    currentLineColor = QColor(themeCurrentLineColor);
    backgroundColor = QColor(themeBackgroundColor);

    lineNumbersColor = QColor(theme.editorColor(KSyntaxHighlighting::Theme::LineNumbers));
    currentLineNumberColor = QColor(theme.editorColor(KSyntaxHighlighting::Theme::CurrentLineNumber));
    regionMarkerColor = QColor(theme.textColor(KSyntaxHighlighting::Theme::RegionMarker));
    searchHighlightColor = QColor(theme.editorColor(KSyntaxHighlighting::Theme::SearchHighlight));
    selectionColor = QColor(theme.editorColor(KSyntaxHighlighting::Theme::TextSelection));

    m_highlighter->setTheme(theme);
    m_highlighter->rehighlight();

    highlightCurrentLine();
}

void TextEditor::loadHighlighter()
{
    const auto def = m_repository.definitionForFileName(QFileInfo(filepath).fileName());

    if (def.filePath() != "") {
        QString syntaxFile = QFileInfo(QString(":/syntax/%1").arg(QFileInfo(def.filePath()).fileName())).absoluteFilePath();

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

        commentDefinition.setComments(QString("%1 ").arg(singleLineComment), multiLineCommentStart, multiLineCommentEnd);

        m_highlighter->setDefinition(def);

        file.close();
    }
}

bool TextEditor::highlightWordUnderMouse(QPoint pos)
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
        wordUnderPointerCursor = cursor;
        wordUnderPointerCursor.select(QTextCursor::WordUnderCursor);
        wordUnderPointerCursor.setPosition(wordUnderPointerCursor.anchor(), QTextCursor::MoveAnchor);

        // Update highlight cursor.
        QTextEdit::ExtraSelection selection;

        QColor lineColor = searchHighlightColor;

        selection.format.setBackground(lineColor);
        selection.cursor = cursor;
        selection.cursor.select(QTextCursor::WordUnderCursor);

        wordUnderCursorSelection = selection;

        renderAllSelections();

        return true;
    } else {
        return false;
    }
}

void TextEditor::removeHighlightWordUnderCursor()
{
    highlightWordCacheCursor = wordUnderCursorSelection.cursor;

    QTextEdit::ExtraSelection selection;
    wordUnderCursorSelection = selection;

    renderAllSelections();
}

void TextEditor::setSettings(Settings *keySettings)
{
    settings = keySettings;
}

void TextEditor::copySelectedText()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(textCursor().selectedText());
}

void TextEditor::cutSelectedText()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(textCursor().selectedText());

    QTextCursor cursor = textCursor();
    cursor.removeSelectedText();
    setTextCursor(cursor);

    unsetMark();
}

void TextEditor::pasteText()
{
    paste();

    unsetMark();
}

void TextEditor::setMark()
{
    bool currentMark = cursorMark;
    bool markCursorChanged = false;

    if (cursorMark) {
        if (textCursor().hasSelection()) {
            markCursorChanged = true;

            QTextCursor cursor = textCursor();
            cursor.clearSelection();
            setTextCursor(cursor);
        } else {
            cursorMark = false;
        }
    } else {
        cursorMark = true;
    }

    if (cursorMark != currentMark || markCursorChanged) {
        cursorMarkChanged(cursorMark, textCursor());
    }
}

void TextEditor::unsetMark()
{
    bool currentMark = cursorMark;

    cursorMark = false;

    if (cursorMark != currentMark) {
        cursorMarkChanged(cursorMark, textCursor());
    }
}

bool TextEditor::tryUnsetMark()
{
    if (cursorMark) {
        QTextCursor cursor = textCursor();
        cursor.clearSelection();
        setTextCursor(cursor);

        unsetMark();

        return true;
    } else {
        return false;
    }
}

void TextEditor::exchangeMark()
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

void TextEditor::saveMarkStatus()
{
    cursorMarkStatus = cursorMark;
    cursorMarkPosition = textCursor().anchor();
}

void TextEditor::restoreMarkStatus()
{
    if (cursorMarkStatus) {
        QTextCursor currentCursor = textCursor();

        QTextCursor cursor = textCursor();
        cursor.setPosition(cursorMarkPosition, QTextCursor::MoveAnchor);
        cursor.setPosition(currentCursor.position(), QTextCursor::KeepAnchor);

        setTextCursor(cursor);
    }
}

void TextEditor::clickCutAction()
{
    if (textCursor().hasSelection()) {
        cutSelectedText();
    } else {
        cutWordUnderCursor();
    }
}

void TextEditor::clickCopyAction()
{
    if (textCursor().hasSelection()) {
        copySelectedText();
    } else {
        copyWordUnderCursor();
    }
}

void TextEditor::clickPasteAction()
{
    if (textCursor().hasSelection()) {
        pasteText();
    } else {
        QTextCursor cursor;

        // Move to word cursor if have word around mouse.
        // Otherwise find nearest cursor with mouse click.
        if (highlightWordCacheCursor.position() != -1) {
            cursor = textCursor();
            cursor.setPosition(highlightWordCacheCursor.position(), QTextCursor::MoveAnchor);
        } else {
            auto pos = mapFromGlobal(mouseClickPos);
            cursor = cursorForPosition(pos);
        }

        setTextCursor(cursor);

        pasteText();
    }
}

void TextEditor::clickDeleteAction()
{
    if (textCursor().hasSelection()) {
        textCursor().removeSelectedText();
    } else {
        setTextCursor(highlightWordCacheCursor);
        textCursor().removeSelectedText();
    }
}

void TextEditor::clickOpenInFileManagerAction()
{
    DDesktopServices::showFileItem(filepath);
}

void TextEditor::copyWordUnderCursor()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(highlightWordCacheCursor.selectedText());
}

void TextEditor::cutWordUnderCursor()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(highlightWordCacheCursor.selectedText());

    setTextCursor(highlightWordCacheCursor);
    textCursor().removeSelectedText();
}

QString TextEditor::getWordAtCursor()
{
    if (toPlainText() == "") {
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

QString TextEditor::getWordAtMouse()
{
    if (toPlainText() == "") {
        return "";
    } else {
        auto pos = mapFromGlobal(QCursor::pos());
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
            cursor.select(QTextCursor::WordUnderCursor);

            return cursor.selectedText();
        } else {
            return "";
        }
    }
}

void TextEditor::toggleEnglishCompleter()
{
    if (enableEnglishCompleter) {
        enableEnglishCompleter = false;

        popupCompletionWindow("", QPoint(), QStringList());

        popupNotify("英文助手已关闭");
    } else {
        enableEnglishCompleter = true;

        popupNotify("英文助手已开启");
    }
}

void TextEditor::setEnglishCompleter(bool enable)
{
    enableEnglishCompleter = enable;
}

void TextEditor::toggleReadOnlyMode()
{
    if (readOnlyMode) {
        readOnlyMode = false;

        popupNotify("只读模式关闭");
    } else {
        readOnlyMode = true;

        popupNotify("只读模式开启");
    }
}

void TextEditor::toggleComment()
{
    Comment::unCommentSelection(this, commentDefinition);
}

void TextEditor::toggleBullet()
{
    // Save current cursor.
    QTextCursor currentCursor = textCursor();
    QString bulletString = "* ";

    if (textCursor().hasSelection()) {
        // Get selection bound.
        int startPos = textCursor().anchor();
        int endPos = textCursor().position();

        if (startPos > endPos) {
            std::swap(startPos, endPos);
        }

        QTextCursor startCursor = textCursor();
        startCursor.setPosition(startPos, QTextCursor::MoveAnchor);
        int startLine = startCursor.block().blockNumber();

        QTextCursor endCursor = textCursor();
        endCursor.setPosition(endPos, QTextCursor::MoveAnchor);
        int endLine = endCursor.block().blockNumber();

        // Whether remove bullet?
        bool removeBullet = true;
        for (int i = startLine; i <= endLine; i++) {
            QTextCursor cursor(document()->findBlockByLineNumber(i));

            cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 2);

            if (cursor.selectedText() != bulletString) {
                removeBullet = false;
                break;
            }
        }

        // Add/remove bullet with selection area.
        for (int i = startLine; i <= endLine; i++) {
            toggleBulletWithLine(i, !removeBullet);
        }
    } else {
        QTextCursor lineBeginningCursor = textCursor();

        lineBeginningCursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        lineBeginningCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 2);

        QString lineBeginningString = lineBeginningCursor.selectedText();

        if (lineBeginningString != bulletString) {
            toggleBulletWithLine(textCursor().block().blockNumber(), true);
        } else {
            toggleBulletWithLine(textCursor().block().blockNumber(), false);
        }
    }

    // Restore cursor.
    setTextCursor(currentCursor);

    // Remove selection area anyway.
    QTextCursor removeSelectionCursor = textCursor();
    removeSelectionCursor.clearSelection();
    setTextCursor(removeSelectionCursor);

    tryUnsetMark();
}

void TextEditor::toggleBulletWithLine(int line, bool addBullet)
{
    QString bulletString = "* ";

    QTextCursor lineBeginningCursor(document()->findBlockByLineNumber(line));

    lineBeginningCursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
    lineBeginningCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 2);

    QString lineBeginningString = lineBeginningCursor.selectedText();

    if (addBullet) {
        // Get line start position.
        QTextCursor cursor(document()->findBlockByLineNumber(line));
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        int startColumn = cursor.columnNumber();

        // Get line end position.
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::MoveAnchor);
        int endColumn = cursor.columnNumber();

        // Move to line start first.
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);

        // Move to first non-blank char of line.
        int column = startColumn;
        while (column < endColumn) {
            QChar currentChar = toPlainText().at(std::max(cursor.position() - 1, 0));

            if (!currentChar.isSpace()) {
                cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
                break;
            } else {
                cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
            }

            column++;
        }

        // Remove blank string.
        cursor.removeSelectedText();

        // Insert bullet.
        cursor.insertText(bulletString);
        setTextCursor(cursor);
    } else {
        lineBeginningCursor.removeSelectedText();
    }
}

int TextEditor::getNextWordPosition(QTextCursor cursor, QTextCursor::MoveMode moveMode)
{
    // Move next char first.
    cursor.movePosition(QTextCursor::NextCharacter, moveMode);

    QChar currentChar = toPlainText().at(cursor.position());

    // Just to next non-space char if current char is space.
    if (currentChar.isSpace()) {
        while (cursor.position() < toPlainText().length() && currentChar.isSpace()) {
            cursor.movePosition(QTextCursor::NextCharacter, moveMode);
            currentChar = toPlainText().at(cursor.position());
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

int TextEditor::getPrevWordPosition(QTextCursor cursor, QTextCursor::MoveMode moveMode)
{
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

bool TextEditor::atWordSeparator(int position)
{
    return wordSepartors.contains(QString(toPlainText().at(position)));
}

void TextEditor::tryCompleteWord()
{
    QString wordAtCursor = getWordAtCursor();
    QTextCursor cursor = textCursor();

    auto rect = cursorRect(textCursor());

    auto cursorPos = viewport()->mapTo(static_cast<Window*>(this->window()),
                                       QPoint(rect.x() + rect.width(),
                                              rect.y() + rect.height()));

    auto windowPos = static_cast<Window*>(this->window())->mapToGlobal(QPoint(0, 0));

    QStringList completionList;

    if (wordAtCursor != "") {
        QSqlQuery query(wordsDB);
        query.prepare("SELECT word FROM words WHERE word like :word");
        query.bindValue(":word", QString("%1%%").arg(wordAtCursor));

        if (query.exec()) {
            int wordIndex = query.record().indexOf("word");

            while (query.next()) {
                auto completionWord = query.value(wordIndex).toString();

                if (completionWord != wordAtCursor) {
                    completionList << completionWord;
                }
            }
        } else {
            qDebug() << "Error: " << query.lastError();
        }
    }

    popupCompletionWindow(wordAtCursor, QPoint(windowPos.x() + cursorPos.x(), windowPos.y() + cursorPos.y()), completionList);

    hasCompletionWords = completionList.size() > 1;
}

void TextEditor::focusOutEvent(QFocusEvent*)
{
    popupCompletionWindow("", QPoint(), QStringList());
}

void TextEditor::setEnglishWordsDB(QSqlDatabase db)
{
    wordsDB = db;
}

void TextEditor::completionWord(QString word)
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

bool TextEditor::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        mouseClickPos = QCursor::pos();

        emit click();
    }

    return false;
}
