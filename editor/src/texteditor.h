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

#ifndef TEXTEEDITOR_H
#define TEXTEEDITOR_H

#include "Repository"

#include "settings.h"
#include <QAction>
#include <QMenu>
#include <QPaintEvent>
#include <QPlainTextEdit>
#include <QPropertyAnimation>
#include <QSqlDatabase>

namespace KSyntaxHighlighting {
    class SyntaxHighlighter;
}

enum ConvertCase {UPPER, LOWER, CAPITALIZE};

class TextEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    TextEditor(QPlainTextEdit *parent = 0);

    QWidget *lineNumberArea;
    QString filepath;
    
    int getCurrentLine();
    int getCurrentColumn();
    int getPosition();
    int getScrollOffset();
    
    void forwardChar();
    void backwardChar();
    void forwardWord();
    void backwardWord();
    void forwardPair();
    void backwardPair();
    
    void moveToStartOfLine();
    void moveToEndOfLine();
    void moveToLineIndentation();
    void nextLine();
    void prevLine();
    void jumpToLine(int line, bool keepLineAtCenter);
    
    void newline();
    void openNewlineAbove();
    void openNewlineBelow();
    void swapLineDown();
    void swapLineUp();
    void scrollLineUp();
    void scrollLineDown();
    void duplicateLine();
    void copyLines();
    
    void killLine();
    void killCurrentLine();
    void killBackwardWord();
    void killForwardWord();
    
    void indentLine();
    void backIndentLine();
    void setTabSpaceNumber(int number);
    void convertWordCase(ConvertCase convertCase);
    
    void keepCurrentLineAtCenter();
    void scrollToLine(int scrollOffset, int row, int column);
    
    void setFontFamily(QString fontName);
    void setFontSize(int fontSize);
    
    void replaceAll(QString replaceText, QString withText);
    void replaceNext(QString replaceText, QString withText);
    void replaceRest(QString replaceText, QString withText);
    
    void removeKeywords();
    void highlightKeyword(QString keyword, int position);
    void updateCursorKeywordSelection(int position, bool findNext);
    void updateHighlightLineSeleciton();
    void updateKeywordSelections(QString keyword);
    void renderAllSelections();
    
    void keyPressEvent(QKeyEvent *e);
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);
                                                     
    void setTheme(const KSyntaxHighlighting::Theme &theme);
    void loadHighlighter();
    
    bool highlightWordUnderMouse(QPoint pos);
    void removeHighlightWordUnderCursor();
    
    void setSettings(Settings *settings);
    
    void copySelectedText();
    void cutSelectedText();
    void pasteText();
    
    void setMark();
    void unsetMark();
    bool tryUnsetMark();
    void exchangeMark();
    
    void setEnglishWordsDB(QSqlDatabase wordsDB);
    void completionWord(QString word);
    QString getWordAtCursor();
    
signals:
    void clickFindAction();
    void clickReplaceAction();
    void clickJumpLineAction();
    void clickFullscreenAction();
    void cursorMarkChanged(bool mark, QTextCursor cursor);
    void popupCompletionWindow(QPoint pos, QStringList words);
                                                             
    void selectNextCompletion();
    void selectPrevCompletion();
    void selectFirstCompletion();
    void selectLastCompletion();
    void confirmCompletion();
    
public slots:
    void highlightCurrentLine();
    void updateLineNumber();
    void handleScrollFinish();
    void handleUpdateRequest(const QRect &rect, int dy);
    
    void clickCutAction();
    void clickCopyAction();
    void clickPasteAction();
    void clickDeleteAction();
    void clickOpenInFileManagerAction();
    
    void copyWordUnderCursor();
    void cutWordUnderCursor();
    
    void upcaseWord();
    void downcaseWord();
    void capitalizeWord();
    void transposeChar();
    
    void changeToEditCursor();
    void changeToWaitCursor();
    void handleCursorMarkChanged(bool mark, QTextCursor cursor);
    
    void tryCompleteWord();
    
protected:
    void focusOutEvent(QFocusEvent* event);    
    
private:
    QPropertyAnimation *scrollAnimation;
    
    QList<QTextEdit::ExtraSelection> keywordSelections;
    QTextEdit::ExtraSelection currentLineSelection;
    QTextEdit::ExtraSelection cursorKeywordSelection;
    QTextEdit::ExtraSelection wordUnderCursorSelection;
    
    QTextCursor highlightWordCacheCursor;
    QTextCursor wordUnderPointerCursor;
    
    int lineNumberOffset = 2;
    int lineNumberPaddingX = 5;
    
    int restoreColumn;
    int restoreRow;
    
    int tabSpaceNumber = 4;
    
    KSyntaxHighlighting::Repository m_repository;
    KSyntaxHighlighting::SyntaxHighlighter *m_highlighter;
    
    QMenu *rightMenu;
    QAction *undoAction;
    QAction *redoAction;
    QAction *cutAction;
    QAction *copyAction;
    QAction *pasteAction;
    QAction *deleteAction;
    QAction *selectAllAction;
    QAction *findAction;
    QAction *replaceAction;
    QAction *jumpLineAction;
    QAction *fullscreenAction;
    QAction *exitFullscreenAction;
    QAction *openInFileManagerAction;
    
    QMenu *convertCaseMenu;
    QAction *upcaseAction;
    QAction *downcaseAction;
    QAction *capitalizeAction;
    
    bool canUndo;
    bool canRedo;
    
    bool haveWordUnderCursor;
    
    bool cursorMark = false;
    int markStartLine = -1;
    
    Settings *settings;
    
    QTimer* changeCursorWidthTimer;
    QTimer* englishHelperTimer;
    
    QSqlDatabase wordsDB;
    
    bool setCursorKeywordSeletoin(int position, bool findNext);
    
    bool hasCompletionWords = false;
};

#endif
