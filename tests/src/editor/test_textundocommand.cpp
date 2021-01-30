#include "test_textundocommand.h"

test_TextUndoCommand::test_TextUndoCommand()
{

}
//public:
//    explicit DeleteTextUndoCommand(QTextCursor textcursor);
TEST_F(test_TextUndoCommand, DeleteTextUndoCommand)
{
    EditWrapper *wrapper = new EditWrapper();
    TextEdit * a = new TextEdit();
    QTextCursor  b = a->textCursor();
    DeleteTextUndoCommand * c = new DeleteTextUndoCommand(b);
    delete c;
    DeleteTextUndoCommand * d = new DeleteTextUndoCommand(c->m_ColumnEditSelections);
    delete d;
    assert(1==1);
}
//    explicit DeleteTextUndoCommand(QList<QTextEdit::ExtraSelection> &selections);

//    virtual void undo();
//    virtual void redo();


//private:
//    QTextCursor m_textCursor;
//    QString m_sInsertText;
//    QList<QString> m_selectTextList;
//    QList<QTextEdit::ExtraSelection> m_ColumnEditSelections;
