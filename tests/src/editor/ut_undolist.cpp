// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gtest/gtest.h"
#include "../../src/editor/undolist.h"
#include "../../src/editor/indenttextcommond.h"
#include "../../src/editor/dtextedit.h"
#include "../stub.h"

// UndoList::UndoList() + UndoList::~UndoList() (both destructor variants)
TEST(UT_UndoList, constructor_and_destructor)
{
    UndoList *list = new UndoList;
    ASSERT_TRUE(list != nullptr);
    delete list;
}

// destructor with appended child commands to exercise the cleanup loop
TEST(UT_UndoList, destructor_with_children)
{
    UndoList *list = new UndoList;
    TextEdit *edit = new TextEdit;
    edit->setPlainText("aaa\nbbb");

    QTextCursor c = edit->textCursor();
    c.setPosition(0);
    edit->setTextCursor(c);

    // append some child commands; UndoList destructor takes ownership and deletes them
    list->appendCom(new IndentTextCommand(edit, 0, 0, 0, 0));
    list->appendCom(new IndentTextCommand(edit, 0, 0, 0, 0));
    list->appendCom(nullptr);  // nullptr should be safely ignored

    delete list;
    delete edit;
}

// also exercise undo/redo for fuller coverage of the class
TEST(UT_UndoList, undo_redo)
{
    UndoList *list = new UndoList;
    TextEdit *edit = new TextEdit;
    edit->setPlainText("aaa\nbbb");

    QTextCursor c = edit->textCursor();
    c.setPosition(0);
    edit->setTextCursor(c);

    list->appendCom(new IndentTextCommand(edit, 0, 0, 0, 0));
    list->redo();
    list->undo();

    delete list;
    delete edit;
}
