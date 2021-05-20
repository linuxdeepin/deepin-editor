#include "test_inserttextundocommand.h"
#include "../../src/editor/inserttextundocommand.h"

test_InsertTextUndoCommand::test_InsertTextUndoCommand()
{
}

void test_InsertTextUndoCommand::SetUp()
{
    QTextCursor textcursor;
    QString test;
    ituc = new InsertTextUndoCommand(textcursor, test);
}

void test_InsertTextUndoCommand::TearDown()
{
    delete ituc;
}

TEST_F(test_InsertTextUndoCommand, undo)
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    QTextEdit::ExtraSelection selection;
    QColor lineColor = QColor(Qt::yellow).lighter(160);
    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor.clearSelection();
    extraSelections.append(selection);

    QString text = "aaa";
    InsertTextUndoCommand *command = new InsertTextUndoCommand(extraSelections, text);
    command->undo();
}

TEST_F(test_InsertTextUndoCommand, undo2)
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    QString text = "aaa";
    InsertTextUndoCommand *command = new InsertTextUndoCommand(extraSelections, text);
    command->undo();
}

TEST_F(test_InsertTextUndoCommand, redo)
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    QString text = "aaa";
    InsertTextUndoCommand *command = new InsertTextUndoCommand(extraSelections, text);
    ituc->redo();
}

TEST_F(test_InsertTextUndoCommand, redo2)
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    QTextEdit::ExtraSelection selection;
    QColor lineColor = QColor(Qt::yellow).lighter(160);
    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor.clearSelection();
    extraSelections.append(selection);
    QString text = "aaa";
    InsertTextUndoCommand *command = new InsertTextUndoCommand(extraSelections, text);
    command->redo();
}
