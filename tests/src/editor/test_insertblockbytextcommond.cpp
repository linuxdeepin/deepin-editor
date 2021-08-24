#include "test_insertblockbytextcommond.h"
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
    QString text = "tt";
    for(int i=0;i<1024*1024;i++)
        text += "tt";

    QTextCursor cursor;
    cursor.insertText(text);
    cursor.movePosition(QTextCursor::Start,QTextCursor::KeepAnchor);
    TextEdit* edit = new TextEdit;
    edit->setTextCursor(cursor);
    EditWrapper* wrapper = new EditWrapper;
    InsertBlockByTextCommond* com = new InsertBlockByTextCommond(text,edit,wrapper);


    delete com;
    com=nullptr;
    wrapper->deleteLater();
    edit->deleteLater();

    
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
    cursor.movePosition(QTextCursor::Start,QTextCursor::KeepAnchor);
    TextEdit* edit = new TextEdit;
    edit->setTextCursor(cursor);
    InsertBlockByTextCommond* com = new InsertBlockByTextCommond(text,edit,nullptr);
    com->treat();

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

    delete com;
    com=nullptr;
    edit->deleteLater();

    
}

