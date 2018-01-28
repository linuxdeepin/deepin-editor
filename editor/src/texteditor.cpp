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
    QFont font;
    font.setFamily("Note Mono");
    font.setFixedPitch(true);
    font.setPointSize(12);
    this->setFont(font);

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

    Utils::setFontSize(painter, 11);
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
    QList<QTextEdit::ExtraSelection> extraSelections;

    QTextEdit::ExtraSelection selection;

    QColor lineColor = QColor("#333333");

    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();
    extraSelections.append(selection);

    setExtraSelections(extraSelections);
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
        QScrollBar *scrollbar = verticalScrollBar();

        jumpLine(getCurrentLine(), blockCount(), scrollbar->value());
    } else {
        QPlainTextEdit::keyPressEvent(keyEvent);
    }
}

int TextEditor::getCurrentLine()
{
    return textCursor().blockNumber() + 1;
}

void TextEditor::jumpToLine(int line)
{
    QTextCursor cursor(document()->findBlockByLineNumber(line - 1)); // line - 1 because line number starts from 0
    setTextCursor(cursor);
    
    keepCurrentLineAtCenter();
}

void TextEditor::keepCurrentLineAtCenter()
{
    QScrollBar *scrollbar = verticalScrollBar();

    int currentLine = cursorRect().top() / cursorRect().height();
    int halfEditorLines = rect().height() / 2 / cursorRect().height();
    scrollbar->setValue(scrollbar->value() + currentLine - halfEditorLines);
}

void TextEditor::scrollToLine(int scrollOffset, int line)
{
    scrollLineNumber = line;
    
    QScrollBar *scrollbar = verticalScrollBar();

    scrollAnimation->setStartValue(scrollbar->value());
    scrollAnimation->setEndValue(scrollOffset);
    scrollAnimation->start();
}

void TextEditor::handleScrollFinish()
{
    jumpToLine(scrollLineNumber);
}
