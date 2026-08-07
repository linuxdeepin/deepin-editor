// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../../src/editor/deletetextundocommand.h"
#include "../../src/editor/inserttextundocommand.h"

#include <gtest/gtest.h>
#include <QPlainTextEdit>
#include <QTextCursor>

// DeleteTextUndoCommand2 (Ctrl+K / Ctrl+Shift+K) deletes to line end / whole
// line on redo and restores on undo. We verify the round-trip on a plain edit.
TEST(DeleteTextUndoCommand2Test, DeleteToEndOfLine_RedoThenUndo_RestoresText)
{
    QPlainTextEdit edit;
    const QString original = QStringLiteral("hello world\nsecond line");
    edit.setPlainText(original);

    QTextCursor c = edit.textCursor();
    c.setPosition(2);  // inside "hello world"
    edit.setTextCursor(c);

    DeleteTextUndoCommand2 cmd(edit.textCursor(), QString(), &edit, /*currLine=*/false);
    cmd.redo();
    // redo should have removed characters from the first line.
    EXPECT_NE(edit.toPlainText(), original);
    cmd.undo();
    EXPECT_EQ(edit.toPlainText(), original);
}

TEST(DeleteTextUndoCommand2Test, DeleteCurrentLine_RedoThenUndo_RestoresText)
{
    QPlainTextEdit edit;
    const QString original = QStringLiteral("line one\nline two\nline three");
    edit.setPlainText(original);

    QTextCursor c = edit.textCursor();
    c.setPosition(8);  // inside "line two"
    edit.setTextCursor(c);

    DeleteTextUndoCommand2 cmd(edit.textCursor(), QString(), &edit, /*currLine=*/true);
    cmd.redo();
    EXPECT_NE(edit.toPlainText(), original);
    cmd.undo();
    EXPECT_EQ(edit.toPlainText(), original);
}

// MidButtonInsertTextUndoCommand inserts text on redo and removes it on undo.
TEST(MidButtonInsertTextUndoCommandTest, InsertText_RedoThenUndo_RestoresText)
{
    QPlainTextEdit edit;
    const QString original = QStringLiteral("deepin-editor");
    edit.setPlainText(original);

    QTextCursor c = edit.textCursor();
    c.setPosition(6);  // between "deepin" and "-editor"
    edit.setTextCursor(c);

    MidButtonInsertTextUndoCommand cmd(edit.textCursor(), QStringLiteral("INSERT"), &edit);
    cmd.redo();
    EXPECT_EQ(edit.toPlainText(), QStringLiteral("deepinINSERT-editor"));
    cmd.undo();
    EXPECT_EQ(edit.toPlainText(), original);
}

// DragInsertTextUndoCommand handles drag-drop text insertion.
TEST(DragInsertTextUndoCommandTest, InsertText_RedoThenUndo_RestoresText)
{
    QPlainTextEdit edit;
    const QString original = QStringLiteral("drag drop target");
    edit.setPlainText(original);

    QTextCursor c = edit.textCursor();
    c.setPosition(5);  // inside "drag drop target"
    edit.setTextCursor(c);

    DragInsertTextUndoCommand cmd(edit.textCursor(), QStringLiteral("MOVED"), &edit);
    cmd.redo();
    EXPECT_NE(edit.toPlainText(), original);
    cmd.undo();
    EXPECT_EQ(edit.toPlainText(), original);
}
