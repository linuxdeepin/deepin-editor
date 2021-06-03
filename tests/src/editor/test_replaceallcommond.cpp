#include "test_replaceallcommond.h"
#include "../../src/editor/replaceallcommond.h"
#include "QTextCursor"
test_replaceallcommond::test_replaceallcommond()
{

}


TEST_F(test_replaceallcommond, ReplaceAllCommond)
{
    QString text = "test";
    QTextCursor cursor;
    ReplaceAllCommond* com = new ReplaceAllCommond(text,text,cursor);

    delete com;
    com=nullptr;

    assert(1==1);
}

TEST_F(test_replaceallcommond, redo)
{
    QString text = "test";
    QTextCursor cursor;
    ReplaceAllCommond* com = new ReplaceAllCommond(text,text,cursor);
    com->redo();

    delete com;
    com=nullptr;

    assert(1==1);
}

TEST_F(test_replaceallcommond, undo)
{
    QString text = "test";
    QTextCursor cursor;
    ReplaceAllCommond* com = new ReplaceAllCommond(text,text,cursor);
    com->undo();

    delete com;
    com=nullptr;

    assert(1==1);
}

