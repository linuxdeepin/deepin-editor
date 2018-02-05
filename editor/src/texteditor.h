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
    
    int getCurrentLine();
    int getCurrentColumn();
    int getPosition();
    int getScrollOffset();
    
    void forwardChar();
    void backwardChar();
    void forwardWord();
    void backwardWord();
    
    void moveToStartOfLine();
    void moveToEndOfLine();
    void moveToLineIndentation();
    void nextLine();
    void prevLine();
    void jumpToLine(int line, bool keepLineAtCenter);
    
    void openNewlineAbove();
    void openNewlineBelow();
    void swapLineDown();
    void swapLineUp();
    void duplicateLine();
    void killLine();
    
    void keepCurrentLineAtCenter();
    void scrollToLine(int scrollOffset, int row, int column);
    
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

public slots:
    void highlightCurrentLine();
    void updateLineNumber();
    void handleScrollFinish();
    void handleUpdateRequest(const QRect &rect, int dy);

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
