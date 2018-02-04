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

    void lineNumberAreaPaintEvent(QPaintEvent *event);

    void nextLine();
    void prevLine();
    void forwardChar();
    void backwardChar();
    void forwardWord();
    void backwardWord();

    void keyPressEvent(QKeyEvent *e);

    int getPosition();
    int getCurrentLine();
    int getCurrentColumn();
    int getScrollOffset();
    void jumpToLine(int line, bool keepLineAtCenter);

    void keepCurrentLineAtCenter();
    void scrollToLine(int scrollOffset, int row, int column);

    void setFontSize(int fontSize);

    void openNewlineAbove();
    void openNewlineBelow();
    void duplicateLine();
    void killLine();

    void swapLineUp();
    void swapLineDown();

    void moveToLineIndentation();
    void moveToStartOfLine();
    void moveToEndOfLine();

    void jumpToLine();

    void highlightKeyword(QString keyword, int position);

    QWidget *lineNumberArea;

    void updateHighlightLineSeleciton();
    void updateKeywordSelections(QString keyword);
    void updateCursorKeywordSelection(int position, bool findNext);

    void renderAllSelections();
    
    void replaceNext(QString replaceText, QString withText);
    void replaceRest(QString replaceText, QString withText);
    void replaceAll(QString replaceText, QString withText);
    
    void cleanKeywords();

signals:
    void jumpLine(int line, int lineCount, int scrollOffset);

public slots:
    void handleUpdateRequest(const QRect &rect, int dy);
    void updateLineNumber();
    void highlightCurrentLine();
    void handleScrollFinish();

private:
    int lineNumberPaddingX = 5;
    int lineNumberOffset = 2;
    int restoreRow;
    int restoreColumn;
    Highlighter *highlighter;

    QPropertyAnimation *scrollAnimation;

    QTextEdit::ExtraSelection currentLineSelection;
    QTextEdit::ExtraSelection cursorKeywordSelection;
    QList<QTextEdit::ExtraSelection> keywordSelections;
    
    bool setCursorKeywordSeletoin(int position, bool findNext);
};

#endif
