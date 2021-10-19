#include "ut_deletetextundocommand.h"
#include "src/stub.h"

namespace deletetextstub {

int intvalue=1;
int retintstub()
{
    return intvalue;
}

bool rettruestub()
{
    return true;
}
bool retfalsestub()
{
    return false;
}

}

using namespace deletetextstub;


UT_Deletetextundocommond::UT_Deletetextundocommond()
{

}

TEST(UT_Deletetextundocommond_DeleteTextUndoCommand, UT_Deletetextundocommond_DeleteTextUndoCommand)
{
#if 0
    Window* window = new Window;
    EditWrapper *wrapper = window->createEditor();
    TextEdit * edit = wrapper->textEditor();
    edit->insertTextEx(edit->textCursor(),"123456");
    auto cursor1 = edit->textCursor();
    edit->selectAll();
    auto cursor2 = edit->textCursor();


    DeleteTextUndoCommand * commond1 = new DeleteTextUndoCommand(cursor1);
    DeleteTextUndoCommand * commond2 = new DeleteTextUndoCommand(cursor2);

    QTextEdit::ExtraSelection select[2];
    select[0].cursor = cursor1;
    select[1].cursor = cursor2;

    QList<QTextEdit::ExtraSelection> selections{select[0],select[1]};
    DeleteTextUndoCommand * commond3 = new DeleteTextUndoCommand(selections);

    window->deleteLater();
    wrapper->deleteLater();
    edit->deleteLater();
    delete commond1;commond1=nullptr;
    delete commond2;commond2=nullptr;
    delete commond3;commond3=nullptr;
#endif

}

TEST(UT_Deletetextundocommond_undo, UT_Deletetextundocommond_undo)
{
    QTextCursor cursor;
    DeleteTextUndoCommand * commond1 = new DeleteTextUndoCommand(cursor);
    commond1->m_sInsertText = "ddd";

    QList<QTextEdit::ExtraSelection> extraSelections;
    QTextEdit::ExtraSelection selection;
    QColor lineColor = QColor(Qt::yellow).lighter(160);
    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor.clearSelection();
    extraSelections.append(selection);
    DeleteTextUndoCommand * commond2 = new DeleteTextUndoCommand(extraSelections);

    commond1->undo();
    commond2->undo();

    int iRet = commond1->m_sInsertText.length();
    ASSERT_TRUE(iRet == QString("ddd").length());

    delete commond1;commond1=nullptr;
    delete commond2;commond2=nullptr;
}


TEST(UT_Deletetextundocommond_redo, UT_Deletetextundocommond_redo)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world."));
    QTextCursor cursor1 = pWindow->currentWrapper()->textEditor()->textCursor();
    auto cursor2 = pWindow->currentWrapper()->textEditor()->textCursor();
    DeleteTextUndoCommand * commond1 = new DeleteTextUndoCommand(cursor1);
    DeleteTextUndoCommand * commond2 = new DeleteTextUndoCommand(cursor2);
    QTextEdit::ExtraSelection select[2];
    select[0].cursor = cursor1;
    select[1].cursor = cursor2;

    QList<QTextEdit::ExtraSelection> selections{select[0],select[1]};
    DeleteTextUndoCommand * commond3 = new DeleteTextUndoCommand(selections);
    commond1->redo();
    commond2->redo();
    commond3->redo();

    int iRet = commond1->m_sInsertText.length();
    ASSERT_TRUE(iRet == 1);

    delete commond1;commond1=nullptr;
    delete commond2;commond2=nullptr;
    delete commond3;commond3=nullptr;


}

TEST(UT_Deletetextundocommond_DeleteTextUndoCommand2, UT_Deletetextundocommond_DeleteTextUndoCommand2_001)
{
//    Window* window = new Window;
//    EditWrapper* wrapper = new EditWrapper(window);
//    TextEdit * edit = new TextEdit(window);
//    edit->m_wrapper = wrapper;

//    QTextCursor cursor1,cursor2;

//    DeleteTextUndoCommand2 * commond1 = new DeleteTextUndoCommand2(cursor1,"ddd",edit,false);

//    QTextEdit::ExtraSelection select[2];
//    select[0].cursor = cursor1;
//    select[1].cursor = cursor2;

//    QList<QTextEdit::ExtraSelection> selections{select[0],select[1]};

//    Stub s1;
//    s1.set(ADDR(QTextCursor,positionInBlock),retintstub);
//    s1.set(ADDR(QTextCursor,hasSelection),retfalsestub);
//    s1.set(ADDR(QString,at),rettruestub);

//    intvalue = 1;
//    DeleteTextUndoCommand2 * commond3 = new DeleteTextUndoCommand2(selections,"test",edit,false);

//    EXPECT_NE(edit,nullptr);
//    window->deleteLater();
//    wrapper->deleteLater();
//    edit->deleteLater();
//    delete commond1;
//    commond1=nullptr;
//    delete commond3;
//    commond3=nullptr;
}


TEST(UT_Deletetextundocommond_DeleteTextUndoCommand2, UT_Deletetextundocommond_DeleteTextUndoCommand2_002)
{

    Window* window = new Window;
    EditWrapper* wrapper = new EditWrapper(window);
    TextEdit * edit = new TextEdit(window);
    edit->m_wrapper = wrapper;

    QTextCursor cursor1,cursor2;


    QTextEdit::ExtraSelection select[2];
    select[0].cursor = cursor1;
    select[1].cursor = cursor2;

    QList<QTextEdit::ExtraSelection> selections{select[0],select[1]};

    Stub s1;
    s1.set(ADDR(QTextCursor,positionInBlock),retintstub);
    s1.set(ADDR(QTextCursor,hasSelection),retfalsestub);
    s1.set(ADDR(QString,at),rettruestub);

    intvalue = -1;
    DeleteTextUndoCommand2 * commond3 = new DeleteTextUndoCommand2(selections,"test",edit,false);

    EXPECT_NE(edit,nullptr);
    window->deleteLater();
    wrapper->deleteLater();
    edit->deleteLater();
    delete commond3;
    commond3=nullptr;

}

TEST(UT_Deletetextundocommond_DeleteTextUndoCommand2, UT_Deletetextundocommond_DeleteTextUndoCommand2_003)
{

    Window* window = new Window;
    EditWrapper* wrapper = new EditWrapper(window);
    TextEdit * edit = new TextEdit(window);
    edit->m_wrapper = wrapper;

    QTextCursor cursor1,cursor2;


    QTextEdit::ExtraSelection select[2];
    select[0].cursor = cursor1;
    select[1].cursor = cursor2;

    QList<QTextEdit::ExtraSelection> selections{select[0],select[1]};

    Stub s1;
    s1.set(ADDR(QTextCursor,positionInBlock),retintstub);
    s1.set(ADDR(QTextCursor,hasSelection),rettruestub);
    s1.set(ADDR(QString,at),rettruestub);

    intvalue = -1;
    DeleteTextUndoCommand2 * commond3 = new DeleteTextUndoCommand2(selections,"test",edit,false);

    EXPECT_NE(edit,nullptr);
    window->deleteLater();
    wrapper->deleteLater();
    edit->deleteLater();
    delete commond3;
    commond3=nullptr;

}

TEST(UT_Deletetextundocommond2_undo, UT_Deletetextundocommond2_undo_001)
{

    Window* window = new Window;
    EditWrapper* wrapper = new EditWrapper(window);
    TextEdit * edit = new TextEdit(window);
    edit->m_wrapper = wrapper;

    QTextCursor cursor1,cursor2;


    QTextEdit::ExtraSelection select[2];
    select[0].cursor = cursor1;
    select[1].cursor = cursor2;

    QList<QTextEdit::ExtraSelection> selections{select[0],select[1]};

    Stub s1;
    s1.set(ADDR(QTextCursor,positionInBlock),retintstub);
    s1.set(ADDR(QTextCursor,hasSelection),retfalsestub);
    s1.set(ADDR(QString,at),rettruestub);

    intvalue = -1;
    DeleteTextUndoCommand2 * commond3 = new DeleteTextUndoCommand2(selections,"test",edit,false);
    commond3->m_ColumnEditSelections = selections;
    commond3->undo();

    EXPECT_NE(edit,nullptr);
    window->deleteLater();
    wrapper->deleteLater();
    edit->deleteLater();
    delete commond3;
    commond3=nullptr;

}

TEST(UT_Deletetextundocommond2_undo, UT_Deletetextundocommond2_undo_002)
{

    Window* window = new Window;
    EditWrapper* wrapper = new EditWrapper(window);
    TextEdit * edit = new TextEdit(window);
    edit->m_wrapper = wrapper;

    QTextCursor cursor1,cursor2;


    QTextEdit::ExtraSelection select[2];
    select[0].cursor = cursor1;
    select[1].cursor = cursor2;

    QList<QTextEdit::ExtraSelection> selections{select[0],select[1]};

    Stub s1;
    s1.set(ADDR(QTextCursor,positionInBlock),retintstub);
    s1.set(ADDR(QTextCursor,hasSelection),retfalsestub);
    s1.set(ADDR(QString,at),rettruestub);

    intvalue = -1;
    DeleteTextUndoCommand2 * commond3 = new DeleteTextUndoCommand2(selections,"test",edit,false);
    commond3->m_ColumnEditSelections.clear();
    commond3->undo();

    EXPECT_NE(edit,nullptr);
    window->deleteLater();
    wrapper->deleteLater();
    edit->deleteLater();
    delete commond3;
    commond3=nullptr;

}

TEST(UT_Deletetextundocommond2_redo, UT_Deletetextundocommond2_redo_001)
{

    Window* window = new Window;
    EditWrapper* wrapper = new EditWrapper(window);
    TextEdit * edit = new TextEdit(window);
    edit->m_wrapper = wrapper;

    QTextCursor cursor1,cursor2;


    QTextEdit::ExtraSelection select[2];
    select[0].cursor = cursor1;
    select[1].cursor = cursor2;

    QList<QTextEdit::ExtraSelection> selections{select[0],select[1]};

    Stub s1;
    s1.set(ADDR(QTextCursor,positionInBlock),retintstub);
    s1.set(ADDR(QTextCursor,hasSelection),retfalsestub);
    s1.set(ADDR(QString,at),rettruestub);

    intvalue = -1;
    DeleteTextUndoCommand2 * commond3 = new DeleteTextUndoCommand2(selections,"test",edit,false);
    commond3->m_ColumnEditSelections=selections;
    commond3->redo();

    EXPECT_NE(edit,nullptr);
    window->deleteLater();
    wrapper->deleteLater();
    edit->deleteLater();
    delete commond3;
    commond3=nullptr;

}

TEST(UT_Deletetextundocommond2_redo, UT_Deletetextundocommond2_redo_002)
{

    Window* window = new Window;
    EditWrapper* wrapper = new EditWrapper(window);
    TextEdit * edit = new TextEdit(window);
    edit->m_wrapper = wrapper;

    QTextCursor cursor1,cursor2;


    QTextEdit::ExtraSelection select[2];
    select[0].cursor = cursor1;
    select[1].cursor = cursor2;

    QList<QTextEdit::ExtraSelection> selections{select[0],select[1]};

    Stub s1;
    s1.set(ADDR(QTextCursor,positionInBlock),retintstub);
    s1.set(ADDR(QTextCursor,hasSelection),retfalsestub);
    s1.set(ADDR(QTextCursor,atBlockEnd),rettruestub);
    s1.set(ADDR(QTextCursor,atBlockStart),rettruestub);
    s1.set(ADDR(QString,at),rettruestub);

    intvalue = -1;
    DeleteTextUndoCommand2 * commond3 = new DeleteTextUndoCommand2(selections,"test",edit,false);
    commond3->m_ColumnEditSelections.clear();

    commond3->m_iscurrLine=false;
    commond3->redo();

    EXPECT_NE(edit,nullptr);
    window->deleteLater();
    wrapper->deleteLater();
    edit->deleteLater();
    delete commond3;
    commond3=nullptr;

}

TEST(UT_Deletetextundocommond2_redo, UT_Deletetextundocommond2_redo_003)
{

    Window* window = new Window;
    EditWrapper* wrapper = new EditWrapper(window);
    TextEdit * edit = new TextEdit(window);
    edit->m_wrapper = wrapper;

    QTextCursor cursor1,cursor2;


    QTextEdit::ExtraSelection select[2];
    select[0].cursor = cursor1;
    select[1].cursor = cursor2;

    QList<QTextEdit::ExtraSelection> selections{select[0],select[1]};

    Stub s1;
    s1.set(ADDR(QTextCursor,positionInBlock),retintstub);
    s1.set(ADDR(QTextCursor,hasSelection),retfalsestub);
    s1.set(ADDR(QTextCursor,atBlockEnd),rettruestub);
    s1.set(ADDR(QTextCursor,atBlockStart),rettruestub);
    s1.set(ADDR(QString,at),rettruestub);

    intvalue = -1;
    DeleteTextUndoCommand2 * commond3 = new DeleteTextUndoCommand2(selections,"test",edit,false);
    commond3->m_ColumnEditSelections.clear();

    commond3->m_iscurrLine=true;
    commond3->redo();

    EXPECT_NE(edit,nullptr);
    window->deleteLater();
    wrapper->deleteLater();
    edit->deleteLater();
    delete commond3;
    commond3=nullptr;

}





