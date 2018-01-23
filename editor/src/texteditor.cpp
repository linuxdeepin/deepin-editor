#include "texteditor.h"
#include "utils.h"
#include <QTextBlock>
#include <QTimer>
#include <QPainter>
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
