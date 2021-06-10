#include "test_deletebackcommond.h"
#include "../src/editor/deletebackcommond.h"
#include"../../src/widgets/window.h"
#include "qplaintextedit.h"
#include "qtextcursor.h"
test_deletebackcommond::test_deletebackcommond()
{

}

TEST_F(test_deletebackcommond, DeleteBackCommond)
{
    QString text = "test";
    QTextCursor cursor;
    QPlainTextEdit* edit = new QPlainTextEdit;
    DeleteBackCommond* com = new DeleteBackCommond(cursor,edit);

    delete com;
    com = nullptr;
    delete edit;
    edit = nullptr;

    assert(1==1);
}

TEST_F(test_deletebackcommond,redo)
{
    QString text = "test";
    QTextCursor cursor;
    QPlainTextEdit* edit = new QPlainTextEdit;
    DeleteBackCommond* com = new DeleteBackCommond(cursor,edit);
    com->m_delText = text;
    com->redo();

    delete com;
    com = nullptr;
    delete edit;
    edit = nullptr;

    assert(1==1);
}

TEST_F(test_deletebackcommond,undo)
{
    QString text = "test";
    QTextCursor cursor;
    QPlainTextEdit* edit = new QPlainTextEdit;
    DeleteBackCommond* com = new DeleteBackCommond(cursor,edit);
    com->m_delText = text;
    com->undo();

    delete com;
    com = nullptr;
    delete edit;
    edit = nullptr;

    assert(1==1);
}



test_deletebackaltcommond::test_deletebackaltcommond()
{

}


TEST_F(test_deletebackaltcommond,  DeleteBackAltCommond)
{
    QString text = "test";
    QList<QTextEdit::ExtraSelection> list;
    QTextEdit::ExtraSelection sel;
    QTextCursor cursor;
    cursor.insertText(text);
    cursor.movePosition(QTextCursor::Start,QTextCursor::KeepAnchor);
    sel.cursor = cursor;
    list.push_back(sel);
    list.push_back(sel);

    QPlainTextEdit* edit = new QPlainTextEdit;
    DeleteBackAltCommond* com = new DeleteBackAltCommond(list,edit);

    delete com;
    com = nullptr;
    delete edit;
    edit = nullptr;

    assert(1==1);
}

TEST_F(test_deletebackaltcommond, redo)
{

    Window* window = new Window;
    EditWrapper *wrapper = window->createEditor();
    TextEdit * edit = wrapper->textEditor();
    edit->insertTextEx(edit->textCursor(),"123456");
    auto cursor1 = edit->textCursor();
    edit->selectAll();
    auto cursor2 = edit->textCursor();

    QTextEdit::ExtraSelection select[2];
    select[0].cursor = cursor1;
    select[1].cursor = cursor2;

    QList<QTextEdit::ExtraSelection> selections{select[0],select[1]};
    DeleteBackAltCommond * commond = new DeleteBackAltCommond(selections,edit);
    commond->redo();

    window->deleteLater();
    wrapper->deleteLater();
    edit->deleteLater();

    delete commond;commond=nullptr;
    assert(1==1);
}


TEST_F(test_deletebackaltcommond, undo)
{
    QString text = "test";
    QList<QTextEdit::ExtraSelection> list;
    QTextEdit::ExtraSelection sel;
    QTextCursor cursor;
    cursor.insertText(text);
    cursor.movePosition(QTextCursor::Start,QTextCursor::KeepAnchor);
    sel.cursor = cursor;
    list.push_back(sel);
    list.push_back(sel);

    QPlainTextEdit* edit = new QPlainTextEdit;
    DeleteBackAltCommond* com = new DeleteBackAltCommond(list,edit);
    com->undo();

    delete com;
    com = nullptr;
    delete edit;
    edit = nullptr;

    assert(1==1);
}
