#include "texteditor.h"
#include "utils.h"
#include <QTextBlock>
#include <QTimer>
#include <QPainter>
#include <QScrollBar>
#include <QDebug>

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

TextEditor::TextEditor(QPlainTextEdit *parent) : QPlainTextEdit(parent)
{
    highlighter = new Highlighter(document());

    lineNumberArea = new LineNumberArea(this);

    connect(this, &QPlainTextEdit::updateRequest, this, &TextEditor::handleUpdateRequest);
    connect(this, &QPlainTextEdit::textChanged, this, &TextEditor::updateLineNumber, Qt::QueuedConnection);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &TextEditor::highlightCurrentLine, Qt::QueuedConnection);

    highlightCurrentLine();

    scrollAnimation = new QPropertyAnimation(verticalScrollBar(), "value");
    scrollAnimation->setEasingCurve(QEasingCurve::InOutExpo);
    scrollAnimation->setDuration(300);

    connect(scrollAnimation, &QPropertyAnimation::finished, this, &TextEditor::handleScrollFinish, Qt::QueuedConnection);

    QTimer::singleShot(0, this, SLOT(setFocus()));
}

void TextEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor(36, 36, 36));

    QTextBlock block = firstVisibleBlock();

    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();

    int linenumber = block.blockNumber();

    Utils::setFontSize(painter, document()->defaultFont().pointSize());
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            painter.setPen(QColor("#666666"));
            painter.drawText(0,
                             top + lineNumberOffset,
                             lineNumberArea->width() - lineNumberPaddingX,
                             fontMetrics().height(),
                             Qt::AlignRight | Qt::AlignBottom,
                             QString::number(linenumber + 1));
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();

        ++linenumber;
    }
}

void TextEditor::handleUpdateRequest(const QRect &rect, int dy)
{
    if (dy) {
        lineNumberArea->scroll(0, dy);
    } else {
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    }
}

void TextEditor::updateLineNumber()
{
    lineNumberArea->setFixedWidth(QString("%1").arg(blockCount()).size() * fontMetrics().width('9') + lineNumberPaddingX * 2);
}

void TextEditor::highlightCurrentLine()
{
    updateHighlightLineSeleciton();

    renderAllSelections();
}

void TextEditor::updateKeywordSelections(QString keyword)
{
    keywordSelections.clear();

    if (keyword != "") {
        moveCursor(QTextCursor::Start);

        while(find(keyword)) {
            QTextEdit::ExtraSelection extra;

            QPen outline(QColor("#D33D6D").lighter(120), 1, Qt::SolidLine);
            extra.format.setProperty(QTextFormat::OutlinePen, outline);

            QBrush brush(QColor("#303030"));
            extra.format.setProperty(QTextFormat::BackgroundBrush, brush);

            extra.cursor = textCursor();
            keywordSelections.append(extra);
        }

        setExtraSelections(keywordSelections);
    }
}

void TextEditor::updateHighlightLineSeleciton()
{
    QTextEdit::ExtraSelection selection;

    QColor lineColor = QColor("#333333");

    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();

    currentLineSelection = selection;
}

bool TextEditor::setCursorKeywordSeletoin(int line, bool findNext)
{
    if (findNext) {
        for (int i = 0; i < keywordSelections.size(); i++) {
            if (keywordSelections[i].cursor.blockNumber() >= line) {
                cursorKeywordSelection.cursor = keywordSelections[i].cursor;

                QBrush brush(QColor("#FF6347"));
                cursorKeywordSelection.format.setProperty(QTextFormat::BackgroundBrush, brush);

                jumpToLine(keywordSelections[i].cursor.blockNumber() + 1, false);

                return true;
            }
        }
    } else {
        for (int i = keywordSelections.size() - 1; i >= 0; i--) {
            if (keywordSelections[i].cursor.blockNumber() + 1 <= line - 2) {
                cursorKeywordSelection.cursor = keywordSelections[i].cursor;

                QBrush brush(QColor("#FF6347"));
                cursorKeywordSelection.format.setProperty(QTextFormat::BackgroundBrush, brush);

                jumpToLine(keywordSelections[i].cursor.blockNumber() + 1, false);

                return true;
            }
        }
    }

    return false;
}

void TextEditor::updateCursorKeywordSelection(int line, bool findNext)
{
    bool findOne = setCursorKeywordSeletoin(line, findNext);

    if (!findOne) {
        if (findNext) {
            setCursorKeywordSeletoin(0, findNext);
        } else {
            setCursorKeywordSeletoin(blockCount(), findNext);
        }
    }
}

void TextEditor::nextLine()
{
    moveCursor(QTextCursor::Down);
}

void TextEditor::prevLine()
{
    moveCursor(QTextCursor::Up);
}

void TextEditor::forwardChar()
{
    moveCursor(QTextCursor::NextCharacter);
}

void TextEditor::backwardChar()
{
    moveCursor(QTextCursor::PreviousCharacter);
}

void TextEditor::forwardWord()
{
    moveCursor(QTextCursor::NextWord);
}

void TextEditor::backwardWord()
{
    moveCursor(QTextCursor::PreviousWord);
}

void TextEditor::keyPressEvent(QKeyEvent *keyEvent)
{
    QString key = Utils::getKeymap(keyEvent);

    // qDebug() << key;

    if (key == "Ctrl + F") {
        forwardChar();
    } else if (key == "Ctrl + B") {
        backwardChar();
    } else if (key == "Alt + F") {
        forwardWord();
    } else if (key == "Alt + B") {
        backwardWord();
    } else if (key == "Ctrl + N") {
        nextLine();
    } else if (key == "Ctrl + P") {
        prevLine();
    } else if (key == "Ctrl + G") {
        jumpToLine();
    } else if (key == "Ctrl + L") {
        openNewlineAbove();
    } else if (key == "Ctrl + H") {
        openNewlineBelow();
    } else if (key == "Ctrl + Shift + L") {
        duplicateLine();
    } else if (key == "Ctrl + K") {
        killLine();
    } else if (key == "Meta + Shift + P") {
        swapLineUp();
    } else if (key == "Meta + Shift + N") {
        swapLineDown();
    } else if (key == "Ctrl + Shift + E") {
        moveToEndOfLine();
    } else if (key == "Ctrl + Shift + A") {
        moveToStartOfLine();
    } else if (key == "Alt + M") {
        moveToLineIndentation();
    } else {
        QPlainTextEdit::keyPressEvent(keyEvent);
    }
}

int TextEditor::getCurrentLine()
{
    return textCursor().blockNumber() + 1;
}

int TextEditor::getCurrentColumn()
{
    return textCursor().columnNumber();
}

int TextEditor::getScrollOffset()
{
    QScrollBar *scrollbar = verticalScrollBar();

    return scrollbar->value();
}

void TextEditor::jumpToLine(int line, bool keepLineAtCenter)
{
    QTextCursor cursor(document()->findBlockByLineNumber(line - 1)); // line - 1 because line number starts from 0
    setTextCursor(cursor);

    if (keepLineAtCenter) {
        keepCurrentLineAtCenter();
    }
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
    restoreRow = row;
    restoreColumn = column;

    QScrollBar *scrollbar = verticalScrollBar();

    scrollAnimation->setStartValue(scrollbar->value());
    scrollAnimation->setEndValue(scrollOffset);
    scrollAnimation->start();
}

void TextEditor::handleScrollFinish()
{
    jumpToLine(restoreRow, false);

    moveCursor(QTextCursor::StartOfLine);
    for (int i = 0; i < restoreColumn; i++) {
        moveCursor(QTextCursor::Right);
    }
}

void TextEditor::setFontSize(int size)
{
    QFont font;
    font.setFamily("Note Mono");
    font.setFixedPitch(true);
    font.setPointSize(size);
    setFont(font);

    updateLineNumber();
}

void TextEditor::openNewlineAbove()
{
    textCursor().insertText("\n");
    prevLine();
}

void TextEditor::openNewlineBelow()
{
    moveCursor(QTextCursor::EndOfLine);
    textCursor().insertText("\n");
}

void TextEditor::duplicateLine()
{
    // Rember current line's column number.
    int column = textCursor().columnNumber();

    // Get current line's content.
    QTextCursor cursor(textCursor().block());
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    QString text = cursor.selectedText();

    // Copy current line.
    moveCursor(QTextCursor::EndOfLine);
    textCursor().insertText("\n");
    textCursor().insertText(text);

    // Restore cursor's column.
    moveCursor(QTextCursor::StartOfLine);
    for (int i = 0; i < column; i++) {
        moveCursor(QTextCursor::Right);
    }
}

void TextEditor::killLine()
{
    // Remove selection content if has selection.
    if (textCursor().hasSelection()) {
        textCursor().removeSelectedText();
    } else {
        // Get current line content.
        QTextCursor cursor(textCursor().block());
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        QString text = cursor.selectedText();

        // Cursor is at end of line.
        bool isEmptyLine = text.size() == 0;

        // Join next line if current line is empty or cursor at end of line.
        if (isEmptyLine || textCursor().atBlockEnd()) {
            QTextCursor cursor = textCursor();

            cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::MoveAnchor);
            cursor.deleteChar();

            setTextCursor(cursor);
        }
        // Otherwise kill rest content of line.
        else {
            QTextCursor cursor = textCursor();

            cursor.movePosition(QTextCursor::NoMove, QTextCursor::KeepAnchor);
            cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();

            setTextCursor(cursor);
        }
    }
}

// Swaps Line Where Cursor Is Currently Positioned With The Line Above It
void TextEditor::swapLineUp(){
    QTextCursor cursor = textCursor();

    // Rember current line's column number.
    int column = cursor.columnNumber();

    //  Select Current Line And Store Value
    cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
    QString newTop = cursor.selectedText();
    cursor.removeSelectedText();

    // Select Line Above And Store Value
    cursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
    QString newBottom = cursor.selectedText();
    cursor.removeSelectedText();

    // Insert New Values
    cursor.insertText(newTop);
    cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor);
    cursor.insertText(newBottom);

    // Position Cursor
    cursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::MoveAnchor);

    // Update cursor.
    setTextCursor(cursor);

    // Restore cursor's column.
    moveCursor(QTextCursor::StartOfLine);
    for (int i = 0; i < column; i++) {
        moveCursor(QTextCursor::Right);
    }
}

// Swaps Line Where Cursor Is Currently Positioned With The Line Below It
void TextEditor::swapLineDown(){
    QTextCursor cursor = textCursor();

    // Rember current line's column number.
    int column = cursor.columnNumber();

    //  Select Current Line And Store Value
    cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
    QString newBottom = cursor.selectedText();
    cursor.removeSelectedText();

    // Select Line Below And Store Value
    cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
    QString newTop = cursor.selectedText();
    cursor.removeSelectedText();

    // Insert New Values
    cursor.insertText(newBottom);
    cursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor);
    cursor.insertText(newTop);

    // Position Cursor
    cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::MoveAnchor);

    // Update cursor.
    setTextCursor(cursor);

    // Restore cursor's column.
    moveCursor(QTextCursor::StartOfLine);
    for (int i = 0; i < column; i++) {
        moveCursor(QTextCursor::Right);
    }
}

void TextEditor::moveToLineIndentation()
{
    // Get line start position.
    moveCursor(QTextCursor::StartOfLine);
    int startColumn = textCursor().columnNumber();

    // Get line end position.
    moveCursor(QTextCursor::EndOfLine);
    int endColumn = textCursor().columnNumber();

    // Move to line start first.
    moveCursor(QTextCursor::StartOfLine);

    // Move to first non-blank char of line.
    int column = startColumn;
    while (column <= endColumn) {
        QChar currentChar = toPlainText().at(std::max(textCursor().position() - 1, 0));

        if (!currentChar.isSpace()) {
            moveCursor(QTextCursor::PreviousCharacter);
            break;
        } else {
            moveCursor(QTextCursor::NextCharacter);
        }

        column++;
    }
}

void TextEditor::moveToStartOfLine()
{
    moveCursor(QTextCursor::StartOfLine);
}

void TextEditor::moveToEndOfLine()
{
    moveCursor(QTextCursor::EndOfLine);
}

void TextEditor::jumpToLine()
{
    QScrollBar *scrollbar = verticalScrollBar();

    jumpLine(getCurrentLine(), blockCount(), scrollbar->value());
}

void TextEditor::highlightKeyword(QString keyword, int line)
{
    updateKeywordSelections(keyword);

    updateCursorKeywordSelection(line, true);

    updateHighlightLineSeleciton();

    renderAllSelections();
}

void TextEditor::renderAllSelections()
{
    QList<QTextEdit::ExtraSelection> selections;

    selections.append(currentLineSelection);
    selections.append(keywordSelections);
    selections.append(cursorKeywordSelection);

    setExtraSelections(selections);
}
