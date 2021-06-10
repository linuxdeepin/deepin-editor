#include "../../src/editor/bookmarkwidget.h"
#include "gtest/gtest.h"
#include "../../src/editor/editwrapper.h"
#include "../../src/widgets/window.h"
#include "../../src/startmanager.h"
#include "../../src/editor/dtextedit.h"
#include "../../src/editor/FlashTween.h"
#include "../../src/editor/deletetextundocommand.h"
#include "../stub.h"
#include <QObject>
#include <QWindow>
#include <QEvent>

class test_TextUndoCommand : public ::testing::Test
{
public:
    test_TextUndoCommand()
    {
    }
};

//public:
//    explicit DeleteTextUndoCommand(QTextCursor textcursor);
TEST_F(test_TextUndoCommand, DeleteTextUndoCommand)
{
    //    EditWrapper *wrapper = new EditWrapper();
    //    TextEdit *a = new TextEdit();
    //    QTextCursor b = a->textCursor();
    //    DeleteTextUndoCommand *c = new DeleteTextUndoCommand(b);
    //    DeleteTextUndoCommand *d = new DeleteTextUndoCommand(c->m_ColumnEditSelections);
    //    delete c;
    //    delete d;
    //    delete a;
    //    assert(1 == 1);
}
//    explicit DeleteTextUndoCommand(QList<QTextEdit::ExtraSelection> &selections);

//    virtual void undo();
//    virtual void redo();

//private:
//    QTextCursor m_textCursor;
//    QString m_sInsertText;
//    QList<QString> m_selectTextList;
//    QList<QTextEdit::ExtraSelection> m_ColumnEditSelections;
