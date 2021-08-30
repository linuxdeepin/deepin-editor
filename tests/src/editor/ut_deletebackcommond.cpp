#include "ut_deletebackcommond.h"
#include "../src/editor/deletebackcommond.h"
#include"../../src/widgets/window.h"
#include "qplaintextedit.h"
#include "qtextcursor.h"
test_deletebackcommond::test_deletebackcommond()
{

}

TEST_F(test_deletebackcommond, DeleteBackCommond)
{
    QTextCursor cursor;
    QPlainTextEdit *pEdit = new QPlainTextEdit;
    DeleteBackCommond *pCom = new DeleteBackCommond(cursor, pEdit);
    ASSERT_TRUE(pCom->m_insertPos != 0);

    delete pCom;
    pCom = nullptr;
    delete pEdit;
    pEdit = nullptr;
}

TEST_F(test_deletebackcommond, redo)
{
    QString text = "test";
    Window *pWindow = new Window;
    pWindow->addBlankTab(QString());
    QTextCursor cursor = pWindow->currentWrapper()->textEditor()->textCursor();
    pWindow->currentWrapper()->textEditor()->insertPlainText(QString("12345"));
    cursor.setPosition(10, QTextCursor::MoveMode::KeepAnchor);
    DeleteBackCommond *pCom = new DeleteBackCommond(cursor, pWindow->currentWrapper()->textEditor());
    pCom->m_delText = text;
    pCom->redo();

    ASSERT_EQ(cursor.position(), pWindow->currentWrapper()->textEditor()->textCursor().position());

    delete pCom;
    pCom = nullptr;
    delete pWindow;
    pWindow = nullptr;
}

TEST_F(test_deletebackcommond, undo)
{
    QString text = "test";
    Window *pWindow = new Window;
    pWindow->addBlankTab(QString());
    QTextCursor cursor = pWindow->currentWrapper()->textEditor()->textCursor();
    pWindow->currentWrapper()->textEditor()->insertPlainText(QString("12345"));
    cursor.setPosition(10, QTextCursor::MoveMode::KeepAnchor);
    DeleteBackCommond *pCom = new DeleteBackCommond(cursor, pWindow->currentWrapper()->textEditor());
    pCom->m_delText = text;
    pCom->undo();

    ASSERT_EQ(cursor.position(), pWindow->currentWrapper()->textEditor()->textCursor().position());

    delete pCom;
    pCom = nullptr;
    delete pWindow;
    pWindow = nullptr;
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
    cursor.movePosition(QTextCursor::Start, QTextCursor::KeepAnchor);
    sel.cursor = cursor;
    list.push_back(sel);
    list.push_back(sel);

    QPlainTextEdit* edit = new QPlainTextEdit;
    DeleteBackAltCommond* com = new DeleteBackAltCommond(list, edit);

    delete com;
    com = nullptr;
    delete edit;
    edit = nullptr;
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

}
