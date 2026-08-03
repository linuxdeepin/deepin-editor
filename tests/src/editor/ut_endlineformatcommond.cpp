// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gtest/gtest.h"
#include "../../src/editor/endlineformatcommond.h"
#include "../../src/editor/dtextedit.h"
#include "../../src/widgets/bottombar.h"
#include "../stub.h"

// EndlineFormartCommand::EndlineFormartCommand + ~EndlineFormartCommand (both variants)
TEST(UT_EndlineFormartCommand, constructor_and_destructor)
{
    TextEdit *edit = new TextEdit;
    edit->setPlainText("hello");
    BottomBar *bar = new BottomBar;

    EndlineFormartCommand *cmd = new EndlineFormartCommand(edit, bar,
                                                           BottomBar::EndlineFormat::Unix,
                                                           BottomBar::EndlineFormat::Windows);
    ASSERT_TRUE(cmd != nullptr);
    delete cmd;

    delete bar;
    delete edit;
}

// EndlineFormartCommand::redo()
TEST(UT_EndlineFormartCommand, redo)
{
    TextEdit *edit = new TextEdit;
    edit->setPlainText("hello");
    BottomBar *bar = new BottomBar;

    EndlineFormartCommand *cmd = new EndlineFormartCommand(edit, bar,
                                                           BottomBar::EndlineFormat::Unix,
                                                           BottomBar::EndlineFormat::Windows);
    cmd->redo();

    delete cmd;
    delete bar;
    delete edit;
}

// EndlineFormartCommand::undo()
TEST(UT_EndlineFormartCommand, undo)
{
    TextEdit *edit = new TextEdit;
    edit->setPlainText("hello");
    BottomBar *bar = new BottomBar;

    EndlineFormartCommand *cmd = new EndlineFormartCommand(edit, bar,
                                                           BottomBar::EndlineFormat::Unix,
                                                           BottomBar::EndlineFormat::Windows);
    cmd->undo();

    delete cmd;
    delete bar;
    delete edit;
}
