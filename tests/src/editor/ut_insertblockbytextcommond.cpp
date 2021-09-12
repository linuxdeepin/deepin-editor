#include "ut_insertblockbytextcommond.h"
#include "../src/editor/insertblockbytextcommond.h"
#include "../src/editor/dtextedit.h"
#include "../src/editor/editwrapper.h"
#include "../src/widgets/window.h"
#include "qtextcursor.h"

test_insertblockbytextcommond::test_insertblockbytextcommond()
{

}

TEST_F(test_insertblockbytextcommond, InsertBlockByTextCommond)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world."));

    QTextCursor textCursor = pWindow->currentWrapper()->textEditor()->textCursor();
    textCursor.movePosition(QTextCursor::Start, QTextCursor::KeepAnchor);
    pWindow->currentWrapper()->textEditor()->setTextCursor(textCursor);
    InsertBlockByTextCommond *pInsertBlockByTextCommond = new InsertBlockByTextCommond(QString("Hei man"),
                                                                                       pWindow->currentWrapper()->textEditor(),
                                                                                       pWindow->currentWrapper());
    QString strRet(pInsertBlockByTextCommond->m_selected);
    ASSERT_TRUE(!strRet.compare(QString("Holle world.")));

    pWindow->deleteLater();
}

TEST_F(test_insertblockbytextcommond, redo)
{
    QString text = "tt";
    for(int i=0;i<1024*1024;i++)
        text += "tt";

    QTextCursor cursor;
    cursor.insertText(text);
    cursor.movePosition(QTextCursor::Start,QTextCursor::KeepAnchor);
    TextEdit* edit = new TextEdit;
    edit->setTextCursor(cursor);
    InsertBlockByTextCommond* com = new InsertBlockByTextCommond(text,edit,nullptr);
    com->redo();

    ASSERT_TRUE(!edit->textCursor().hasSelection());

    delete com;
    com=nullptr;
    edit->deleteLater();
}


TEST_F(test_insertblockbytextcommond, undo)
{
    QString text = "tt";
    for(int i=0;i<1024*1024;i++)
        text += "tt";

    QTextCursor cursor;
    cursor.insertText(text);
    cursor.movePosition(QTextCursor::Start,QTextCursor::KeepAnchor);
    TextEdit* edit = new TextEdit;
    edit->setTextCursor(cursor);
    InsertBlockByTextCommond* com = new InsertBlockByTextCommond(text,edit,nullptr);
    com->undo();

    ASSERT_TRUE(!edit->textCursor().hasSelection());

    delete com;
    com=nullptr;
    edit->deleteLater();
}


TEST_F(test_insertblockbytextcommond, treat)
{
    QString text = "tt";
    for(int i=0;i<1024*1024;i++)
        text += "tt";

    QTextCursor cursor;
    cursor.insertText(text);
    cursor.movePosition(QTextCursor::Start, QTextCursor::KeepAnchor);
    TextEdit* edit = new TextEdit;
    edit->setTextCursor(cursor);
    InsertBlockByTextCommond* com = new InsertBlockByTextCommond(text,edit,nullptr);
    com->treat();

    ASSERT_TRUE(!edit->textCursor().hasSelection());

    delete com;
    com=nullptr;
    edit->deleteLater();
}


TEST_F(test_insertblockbytextcommond, insertByBlock)
{
    QString text = "tt";
    for(int i=0;i<1024*1024;i++)
        text += "tt";

    QTextCursor cursor;
    cursor.insertText(text);
    cursor.movePosition(QTextCursor::Start,QTextCursor::KeepAnchor);
    TextEdit* edit = new TextEdit;
    edit->setTextCursor(cursor);
    InsertBlockByTextCommond* com = new InsertBlockByTextCommond(text,edit,nullptr);
    com->insertByBlock();

    ASSERT_TRUE(!edit->textCursor().hasSelection());

    delete com;
    com=nullptr;
    edit->deleteLater(); 
}

