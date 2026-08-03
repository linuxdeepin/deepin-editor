// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gtest/gtest.h"
#include "../../src/editor/changemarkcommand.h"
#include "../../src/editor/dtextedit.h"
#include "../stub.h"

// ChangeMarkCommand::redo() (exercise the redo path as well)
TEST(UT_ChangeMarkCommand, redo)
{
    TextEdit *edit = new TextEdit;
    edit->setPlainText("hello world");

    QList<TextEdit::MarkReplaceInfo> newMark;
    TextEdit::MarkReplaceInfo info;
    info.start = 0;
    info.end = 5;
    info.time = 1;
    newMark.append(info);

    ChangeMarkCommand *cmd = new ChangeMarkCommand(edit,
                                                    QList<TextEdit::MarkReplaceInfo>(),
                                                    newMark);
    cmd->redo();

    delete cmd;
    delete edit;
}

// ChangeMarkCommand::undo()
TEST(UT_ChangeMarkCommand, undo)
{
    TextEdit *edit = new TextEdit;
    edit->setPlainText("hello world");

    QList<TextEdit::MarkReplaceInfo> oldMark;
    TextEdit::MarkReplaceInfo info;
    info.start = 0;
    info.end = 5;
    info.time = 1;
    oldMark.append(info);

    ChangeMarkCommand *cmd = new ChangeMarkCommand(edit, oldMark,
                                                    QList<TextEdit::MarkReplaceInfo>());
    cmd->undo();

    delete cmd;
    delete edit;
}

// ChangeMarkCommand::~ChangeMarkCommand (both destructor variants)
TEST(UT_ChangeMarkCommand, destructor)
{
    TextEdit *edit = new TextEdit;
    edit->setPlainText("hello");

    ChangeMarkCommand *cmd = new ChangeMarkCommand(edit,
                                                    QList<TextEdit::MarkReplaceInfo>(),
                                                    QList<TextEdit::MarkReplaceInfo>());
    delete cmd;

    delete edit;
}
