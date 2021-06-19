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
    Window* window = new Window;
    EditWrapper *wrapper = window->createEditor();
    TextEdit * edit = wrapper->textEditor();
    DeleteBackCommond* com = new DeleteBackCommond(cursor,edit);
    com->m_delText = text;
    com->redo();

    window->deleteLater();
    wrapper->deleteLater();
    edit->deleteLater();
    delete com;
    com = nullptr;
    assert(1==1);
}

TEST_F(test_deletebackcommond,undo)
{
    QString text = "test";
    QTextCursor cursor;
    Window* window = new Window;
    EditWrapper *wrapper = window->createEditor();
    TextEdit * edit = wrapper->textEditor();
    DeleteBackCommond* com = new DeleteBackCommond(cursor,edit);
    com->m_delText = text;
    com->undo();

    window->deleteLater();
    wrapper->deleteLater();
    edit->deleteLater();
    delete com;
    com = nullptr;
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
    QString text = "test";
    QList<QTextEdit::ExtraSelection> list;
    QTextEdit::ExtraSelection sel;
    QTextCursor cursor;
    cursor.insertText(text);
    cursor.movePosition(QTextCursor::Start,QTextCursor::KeepAnchor);
    sel.cursor = cursor;
    list.push_back(sel);
    list.push_back(sel);
    DeleteBackAltCommond * commond = new DeleteBackAltCommond(list,edit);
    commond->redo();

    window->deleteLater();
    wrapper->deleteLater();
    edit->deleteLater();

    delete commond;commond=nullptr;
    assert(1==1);
}


TEST_F(test_deletebackaltcommond, undo)
{
    Window* window = new Window;
    EditWrapper *wrapper = window->createEditor();
    TextEdit * edit = wrapper->textEditor();
    QString text = "test";
    QList<QTextEdit::ExtraSelection> list;
    QTextEdit::ExtraSelection sel;
    QTextCursor cursor;
    cursor.insertText(text);
    cursor.movePosition(QTextCursor::Start,QTextCursor::KeepAnchor);
    sel.cursor = cursor;
    list.push_back(sel);
    list.push_back(sel);

    DeleteBackAltCommond* com = new DeleteBackAltCommond(list,edit);
    com->undo();

    window->deleteLater();
    wrapper->deleteLater();
    edit->deleteLater();
    delete com;
    com = nullptr;
    assert(1==1);
}
