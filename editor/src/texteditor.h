#ifndef TEXTEEDITOR_H
#define TEXTEEDITOR_H

#include "highlighter.h"
#include <QPlainTextEdit>
#include <QPaintEvent>
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
    
    QWidget *lineNumberArea;
    
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
};

#endif
