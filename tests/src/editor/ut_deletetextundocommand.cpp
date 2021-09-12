#include "ut_deletetextundocommand.h"

test_deletetextundocommond::test_deletetextundocommond()
{

}

TEST_F(test_deletetextundocommond, DeleteTextUndoCommand)
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

TEST_F(test_deletetextundocommond, undo)
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


TEST_F(test_deletetextundocommond, redo)
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

TEST_F(test_deletetextundocommond, DeleteTextUndoCommand2)
{
    #if 0
    Window* window = new Window;
    EditWrapper *wrapper = window->createEditor();
    TextEdit * edit = wrapper->textEditor();
    edit->insertTextEx(edit->textCursor(),"123456");
    auto cursor1 = edit->textCursor();
    edit->selectAll();
    auto cursor2 = edit->textCursor();


    DeleteTextUndoCommand2 * commond1 = new DeleteTextUndoCommand2(cursor1,cursor1.selectedText(),edit,false);
    DeleteTextUndoCommand2 * commond2 = new DeleteTextUndoCommand2(cursor2,cursor2.selectedText(),edit,true);
    commond1->redo();
    commond1->undo();
    commond2->redo();

    QTextEdit::ExtraSelection select[2];
    select[0].cursor = cursor1;
    select[1].cursor = cursor2;

    QList<QTextEdit::ExtraSelection> selections{select[0],select[1]};
    DeleteTextUndoCommand2 * commond3 = new DeleteTextUndoCommand2(selections,"test",edit,false);
    commond3->redo();
    commond3->undo();
    window->deleteLater();
    wrapper->deleteLater();
    edit->deleteLater();
    delete commond1;commond1=nullptr;
    delete commond2;commond2=nullptr;
    delete commond3;commond3=nullptr;
    #endif
}


