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

#include "highlighter.h"

#include <QPaintEvent>
#include <QPlainTextEdit>
#include <QPropertyAnimation>

class TextEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    TextEditor(QPlainTextEdit *parent = 0);

    QWidget *lineNumberArea;
    QString filepath;
    
    int getCurrentColumn();
    int getCurrentLine();
    int getPosition();
    int getScrollOffset();
    void backwardChar();
    void backwardWord();
    void cleanKeywords();
    void duplicateLine();
    void forwardChar();
    void forwardWord();
    void highlightKeyword(QString keyword, int position);
    void jumpToLine(int line, bool keepLineAtCenter);
    void keepCurrentLineAtCenter();
    void keyPressEvent(QKeyEvent *e);
    void killLine();
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    void moveToEndOfLine();
    void moveToLineIndentation();
    void moveToStartOfLine();
    void nextLine();
    void openNewlineAbove();
    void openNewlineBelow();
    void prevLine();
    void renderAllSelections();
    void replaceAll(QString replaceText, QString withText);
    void replaceNext(QString replaceText, QString withText);
    void replaceRest(QString replaceText, QString withText);
    void scrollToLine(int scrollOffset, int row, int column);
    void setFontSize(int fontSize);
    void swapLineDown();
    void swapLineUp();
    void updateCursorKeywordSelection(int position, bool findNext);
    void updateHighlightLineSeleciton();
    void updateKeywordSelections(QString keyword);

signals:
    void popupJumpLineBar(QString filepath, int line, int lineCount, int scrollOffset);

public slots:
    void handleScrollFinish();
    void handleUpdateRequest(const QRect &rect, int dy);
    void highlightCurrentLine();
    void updateLineNumber();

private:
    Highlighter *highlighter;
    QList<QTextEdit::ExtraSelection> keywordSelections;
    QPropertyAnimation *scrollAnimation;
    QTextEdit::ExtraSelection currentLineSelection;
    QTextEdit::ExtraSelection cursorKeywordSelection;
    int lineNumberOffset = 2;
    int lineNumberPaddingX = 5;
    int restoreColumn;
    int restoreRow;
    
    bool setCursorKeywordSeletoin(int position, bool findNext);
};

#endif
