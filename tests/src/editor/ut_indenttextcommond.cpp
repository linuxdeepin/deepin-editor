// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gtest/gtest.h"
#include "../../src/editor/indenttextcommond.h"
#include "../../src/editor/dtextedit.h"
#include "../stub.h"

// IndentTextCommand::~IndentTextCommand (both destructor variants)
TEST(UT_IndentTextCommand, destructor)
{
    TextEdit *edit = new TextEdit;
    edit->setPlainText("aaa\nbbb\nccc");
    QTextCursor c = edit->textCursor();
    c.setPosition(0);
    edit->setTextCursor(c);

    IndentTextCommand *cmd = new IndentTextCommand(edit, 0, 0, 0, 0);
    delete cmd;

    delete edit;
}

// IndentTextCommand::undo()
TEST(UT_IndentTextCommand, undo)
{
    TextEdit *edit = new TextEdit;
    edit->setPlainText("aaa\nbbb\nccc");
    QTextCursor c = edit->textCursor();
    c.setPosition(0);
    c.setPosition(3, QTextCursor::KeepAnchor);
    edit->setTextCursor(c);

    IndentTextCommand *cmd = new IndentTextCommand(edit, 0, 3, 0, 0);
    cmd->redo();   // add a tab to the first line
    cmd->undo();   // remove the tab from the first line
    EXPECT_EQ(edit->toPlainText(), QString("aaa\nbbb\nccc"));

    delete cmd;
    delete edit;
}

// UnindentTextCommand::UnindentTextCommand(...) + ~UnindentTextCommand (both variants)
TEST(UT_UnindentTextCommand, constructor_and_destructor)
{
    TextEdit *edit = new TextEdit;
    edit->setPlainText("aaa");
    QTextCursor c = edit->textCursor();
    c.setPosition(0);
    edit->setTextCursor(c);

    UnindentTextCommand *cmd = new UnindentTextCommand(edit, 0, 0, 0, 0, 4);
    ASSERT_TRUE(cmd != nullptr);
    delete cmd;

    delete edit;
}

// UnindentTextCommand::redo()
TEST(UT_UnindentTextCommand, redo)
{
    TextEdit *edit = new TextEdit;
    edit->setPlainText("\taaa\n  bbb\nccc");
    QTextCursor c = edit->textCursor();
    c.setPosition(0);
    edit->setTextCursor(c);

    UnindentTextCommand *cmd = new UnindentTextCommand(edit, 0, 0, 0, 2, 4);
    cmd->redo();
    // first line tab removed, second line two spaces removed
    EXPECT_EQ(edit->toPlainText(), QString("aaa\nbbb\nccc"));

    delete cmd;
    delete edit;
}

// UnindentTextCommand::undo()
TEST(UT_UnindentTextCommand, undo)
{
    TextEdit *edit = new TextEdit;
    edit->setPlainText("\taaa\n  bbb\nccc");
    QTextCursor c = edit->textCursor();
    c.setPosition(0);
    edit->setTextCursor(c);

    UnindentTextCommand *cmd = new UnindentTextCommand(edit, 0, 0, 0, 2, 4);
    cmd->redo();
    cmd->undo();
    // restored after a redo/undo round-trip
    EXPECT_EQ(edit->toPlainText(), QString("\taaa\n  bbb\nccc"));

    delete cmd;
    delete edit;
}
