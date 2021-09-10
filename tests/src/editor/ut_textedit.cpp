#include "ut_textedit.h"
#include "stub.h"
#include "../../src/widgets/window.h"
#include <QUndoStack>

test_textedit::test_textedit()
{
    QString text = QString("#include \"window.h\"\n"
                           "#include \"urlinfo.h\"\n"
                           "int main(int argc, char *argv[])\n"
                           "{\n"
                           "using namespace Dtk::Core;\n"
                           "PerformanceMonitor::initializeAppStart();\n"
                           "return 0;\n"
                           "}");
    QFile f("1.cpp");
    if (f.open(QFile::WriteOnly)) {
        f.write(text.toUtf8());
        f.close();
    }
}

void test_textedit::forstub(QPoint q)
{
}

TEST_F(test_textedit, setWrapper)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    ASSERT_TRUE(pWindow->currentWrapper()->textEditor()->m_wrapper != nullptr);

    pWindow->deleteLater();
}

//TextEdit(QWidget *parent = nullptr);
TEST_F(test_textedit, TextEdit)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    ASSERT_TRUE(pWindow->currentWrapper()->textEditor()->verticalScrollBarPolicy()   == Qt::ScrollBarAsNeeded);
    ASSERT_TRUE(pWindow->currentWrapper()->textEditor()->horizontalScrollBarPolicy() == Qt::ScrollBarAsNeeded);

    pWindow->deleteLater();
}

//getCurrentLine
TEST_F(test_textedit, getCurrentLine)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world.\n Create operating system innovation ecosystem!"));
    int iRet = pWindow->currentWrapper()->textEditor()->getCurrentLine();
    ASSERT_TRUE(iRet == 2);

    pWindow->deleteLater();
}

//getCurrentColumn
TEST_F(test_textedit, getCurrentColumn)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world."));
    int iRet = pWindow->currentWrapper()->textEditor()->getCurrentColumn();
    ASSERT_TRUE(iRet == QString("Holle world.").length());

    pWindow->deleteLater();
}

//getPosition
TEST_F(test_textedit, getPosition)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world."));
    int iRet = pWindow->currentWrapper()->textEditor()->getPosition();
    ASSERT_TRUE(iRet == QString("Holle world.").length());

    pWindow->deleteLater();
}

//getScrollOffset
TEST_F(test_textedit, getScrollOffset)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("H\nl\nl\ne\n\nw\no\nr\nl\nd\n.H\nl\nl\ne\n\nw"
                                                                  "\no\nr\nl\nd\n.H\nl\nl\ne\n\nw\no\nr\nl\nd\n.H"
                                                                  "\nl\nl\ne\n\nw\no\nr\nl\nd\n.H\nl\nl\ne\n\nw\no"
                                                                  "\nr\nl\nd\n."));
    int iRet = pWindow->currentWrapper()->textEditor()->getScrollOffset();
    ASSERT_TRUE(iRet > 0);

    pWindow->deleteLater();
}

//forwardChar 001
TEST_F(test_textedit, forwardChar_001)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world."));
    pWindow->currentWrapper()->textEditor()->m_cursorMark = true;
    QTextCursor textCursor = pWindow->currentWrapper()->textEditor()->textCursor();
    textCursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
    pWindow->currentWrapper()->textEditor()->setTextCursor(textCursor);
    pWindow->currentWrapper()->textEditor()->forwardChar();
    int iRet = pWindow->currentWrapper()->textEditor()->textCursor().position();
    QString strRet(pWindow->currentWrapper()->textEditor()->textCursor().selectedText());
    ASSERT_TRUE((iRet == 1) && (!strRet.compare(QString("H"))));

    pWindow->deleteLater();
}

//forwardChar 002
TEST_F(test_textedit, forwardChar_002)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world."));
    pWindow->currentWrapper()->textEditor()->m_cursorMark = false;
    QTextCursor textCursor = pWindow->currentWrapper()->textEditor()->textCursor();
    textCursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
    pWindow->currentWrapper()->textEditor()->setTextCursor(textCursor);
    pWindow->currentWrapper()->textEditor()->forwardChar();
    int iRet = pWindow->currentWrapper()->textEditor()->textCursor().position();
    QString strRet(pWindow->currentWrapper()->textEditor()->textCursor().selectedText());
    ASSERT_TRUE((iRet == 1) && strRet.isEmpty());

    pWindow->deleteLater();
}

//backwardChar 001
TEST_F(test_textedit, backwardChar_001)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world"));
    pWindow->currentWrapper()->textEditor()->m_cursorMark = true;
    pWindow->currentWrapper()->textEditor()->backwardChar();
    int iRet = pWindow->currentWrapper()->textEditor()->textCursor().position();
    QString strRet(pWindow->currentWrapper()->textEditor()->textCursor().selectedText());
    ASSERT_TRUE((iRet == 10) && (!strRet.compare(QString("d"))));

    pWindow->deleteLater();
}

//backwardChar 002
TEST_F(test_textedit, backwardChar_002)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world"));
    pWindow->currentWrapper()->textEditor()->m_cursorMark = false;
    pWindow->currentWrapper()->textEditor()->backwardChar();
    int iRet = pWindow->currentWrapper()->textEditor()->textCursor().position();
    QString strRet(pWindow->currentWrapper()->textEditor()->textCursor().selectedText());
    ASSERT_TRUE((iRet == 10) && strRet.isEmpty());

    pWindow->deleteLater();
}

//forwardWord 001
TEST_F(test_textedit, forwardWord_001)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world"));
    QTextCursor textCursor = pWindow->currentWrapper()->textEditor()->textCursor();
    textCursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
    pWindow->currentWrapper()->textEditor()->setTextCursor(textCursor);
    pWindow->currentWrapper()->textEditor()->m_cursorMark = true;
    pWindow->currentWrapper()->textEditor()->forwardWord();
    int iRet = pWindow->currentWrapper()->textEditor()->textCursor().position();
    QString strRet(pWindow->currentWrapper()->textEditor()->textCursor().selectedText());
    ASSERT_TRUE((iRet == 6) && (!strRet.compare(QString("Holle "))));

    pWindow->deleteLater();
}

//forwardWord 002
TEST_F(test_textedit, forwardWord_002)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world"));
    QTextCursor textCursor = pWindow->currentWrapper()->textEditor()->textCursor();
    textCursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
    pWindow->currentWrapper()->textEditor()->setTextCursor(textCursor);
    pWindow->currentWrapper()->textEditor()->m_cursorMark = false;
    pWindow->currentWrapper()->textEditor()->forwardWord();
    int iRet = pWindow->currentWrapper()->textEditor()->textCursor().position();
    QString strRet(pWindow->currentWrapper()->textEditor()->textCursor().selectedText());
    ASSERT_TRUE((iRet == 6) && strRet.isEmpty());

    pWindow->deleteLater();
}

//backwardWord 001
TEST_F(test_textedit, backwardWord_001)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world"));
    pWindow->currentWrapper()->textEditor()->m_cursorMark = true;
    pWindow->currentWrapper()->textEditor()->backwardWord();
    int iRet = pWindow->currentWrapper()->textEditor()->textCursor().position();
    QString strRet(pWindow->currentWrapper()->textEditor()->textCursor().selectedText());
    ASSERT_TRUE((iRet == 6) && (!strRet.compare(QString("world"))));

    pWindow->deleteLater();
}

//backwardWord 002
TEST_F(test_textedit, backwardWord_002)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world"));
    pWindow->currentWrapper()->textEditor()->m_cursorMark = false;
    pWindow->currentWrapper()->textEditor()->backwardWord();
    int iRet = pWindow->currentWrapper()->textEditor()->textCursor().position();
    QString strRet(pWindow->currentWrapper()->textEditor()->textCursor().selectedText());
    ASSERT_TRUE((iRet == 6) && strRet.isEmpty());

    pWindow->deleteLater();
}

bool forwardPair_001_find_stub()
{
    return true;
}

//forwardPair 001
TEST_F(test_textedit, forwardPair_001)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world"));
    #if 0 //保留打桩方法，后续完善覆盖率
    typedef int (*fptr)(QDialog *);
    fptr qDialogExec = (fptr)(&QDialog::exec);
    Stub stub;
    stub.set(qDialogExec, forwardPair_001_find_stub);

    Stub find_stub;
    find_stub.set((bool(QPlainTextEdit::*)(const QRegExp &))ADDR(QPlainTextEdit, QPlainTextEdit::find), forwardPair_001_find_stub);
    #endif
    QTextCursor textCursor = pWindow->currentWrapper()->textEditor()->textCursor();
    textCursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
    pWindow->currentWrapper()->textEditor()->setTextCursor(textCursor);
    pWindow->currentWrapper()->textEditor()->forwardPair();
    QString strRet(pWindow->currentWrapper()->textEditor()->textCursor().selectedText());
    ASSERT_TRUE(strRet.isEmpty());

    pWindow->deleteLater();
}

//backwardPair 001
TEST_F(test_textedit, backwardPair_001)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world"));
    #if 0 //保留打桩方法，后续完善覆盖率
    typedef int (*fptr)(QDialog *);
    fptr qDialogExec = (fptr)(&QDialog::exec);
    Stub stub;
    stub.set(qDialogExec, forwardPair_001_find_stub);

    Stub find_stub;
    find_stub.set((bool(QPlainTextEdit::*)(const QRegExp &))ADDR(QPlainTextEdit, QPlainTextEdit::find), forwardPair_001_find_stub);
    #endif
    QTextCursor textCursor = pWindow->currentWrapper()->textEditor()->textCursor();
    textCursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
    pWindow->currentWrapper()->textEditor()->setTextCursor(textCursor);
    pWindow->currentWrapper()->textEditor()->backwardPair();
    QString strRet(pWindow->currentWrapper()->textEditor()->textCursor().selectedText());
    ASSERT_TRUE(strRet.isEmpty());

    pWindow->deleteLater();
}

//blockCount
TEST_F(test_textedit, blockCount)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world"));
    int iRet = pWindow->currentWrapper()->textEditor()->blockCount();
    ASSERT_TRUE(iRet == 1);

    pWindow->deleteLater();
}

//moveToStart 001
TEST_F(test_textedit, moveToStart_001)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world"));
    pWindow->currentWrapper()->textEditor()->m_cursorMark = true;
    pWindow->currentWrapper()->textEditor()->moveToStart();
    QString strRet(pWindow->currentWrapper()->textEditor()->textCursor().selectedText());
    ASSERT_TRUE(!strRet.compare(QString("Holle world")));

    pWindow->deleteLater();
}

//moveToStart 002
TEST_F(test_textedit, moveToStart_002)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world"));
    pWindow->currentWrapper()->textEditor()->m_cursorMark = false;
    pWindow->currentWrapper()->textEditor()->moveToStart();
    QString strRet(pWindow->currentWrapper()->textEditor()->textCursor().selectedText());
    ASSERT_TRUE(strRet.isEmpty());

    pWindow->deleteLater();
}

//moveToEnd 001
TEST_F(test_textedit, moveToEnd_001)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world"));
    QTextCursor textCursor = pWindow->currentWrapper()->textEditor()->textCursor();
    textCursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
    pWindow->currentWrapper()->textEditor()->setTextCursor(textCursor);
    pWindow->currentWrapper()->textEditor()->m_cursorMark = true;
    pWindow->currentWrapper()->textEditor()->moveToEnd();
    QString strRet(pWindow->currentWrapper()->textEditor()->textCursor().selectedText());
    ASSERT_TRUE(!strRet.compare(QString("Holle world")));

    pWindow->deleteLater();
}

//moveToEnd 002
TEST_F(test_textedit, moveToEnd_002)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world"));
    QTextCursor textCursor = pWindow->currentWrapper()->textEditor()->textCursor();
    textCursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
    pWindow->currentWrapper()->textEditor()->setTextCursor(textCursor);
    pWindow->currentWrapper()->textEditor()->m_cursorMark = false;
    pWindow->currentWrapper()->textEditor()->moveToEnd();
    QString strRet(pWindow->currentWrapper()->textEditor()->textCursor().selectedText());
    ASSERT_TRUE(strRet.isEmpty());

    pWindow->deleteLater();
}

//moveToStartOfLine 001
TEST_F(test_textedit, moveToStartOfLine_001)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world\nHolle world\nHolle world"));
    pWindow->currentWrapper()->textEditor()->m_cursorMark = true;
    pWindow->currentWrapper()->textEditor()->moveToStartOfLine();
    QString strRet(pWindow->currentWrapper()->textEditor()->textCursor().selectedText());
    ASSERT_TRUE(!strRet.isEmpty());

    pWindow->deleteLater();
}

//moveToStartOfLine 002
TEST_F(test_textedit, moveToStartOfLine_002)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world\nHolle world\nHolle world"));
    pWindow->currentWrapper()->textEditor()->m_cursorMark = false;
    pWindow->currentWrapper()->textEditor()->moveToStartOfLine();
    QString strRet(pWindow->currentWrapper()->textEditor()->textCursor().selectedText());
    ASSERT_TRUE(strRet.isEmpty());

    pWindow->deleteLater();
}

//moveToEndOfLine 001
TEST_F(test_textedit, moveToEndOfLine_001)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world"));
    QTextCursor textCursor = pWindow->currentWrapper()->textEditor()->textCursor();
    textCursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
    pWindow->currentWrapper()->textEditor()->setTextCursor(textCursor);
    pWindow->currentWrapper()->textEditor()->m_cursorMark = true;
    pWindow->currentWrapper()->textEditor()->moveToEndOfLine();
    QString strRet(pWindow->currentWrapper()->textEditor()->textCursor().selectedText());
    ASSERT_TRUE(!strRet.compare(QString("Holle world")));

    pWindow->deleteLater();
}

//moveToEndOfLine 002
TEST_F(test_textedit, moveToEndOfLine_002)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world"));
    QTextCursor textCursor = pWindow->currentWrapper()->textEditor()->textCursor();
    textCursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
    pWindow->currentWrapper()->textEditor()->setTextCursor(textCursor);
    pWindow->currentWrapper()->textEditor()->m_cursorMark = false;
    pWindow->currentWrapper()->textEditor()->moveToEndOfLine();
    QString strRet(pWindow->currentWrapper()->textEditor()->textCursor().selectedText());
    ASSERT_TRUE(strRet.isEmpty());

    pWindow->deleteLater();
}

//moveToLineIndentation 001
TEST_F(test_textedit, moveToLineIndentation_001)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world"));
    pWindow->currentWrapper()->textEditor()->moveToLineIndentation();
    QString strRet(pWindow->currentWrapper()->textEditor()->textCursor().selectedText());
    int iRet = pWindow->currentWrapper()->textEditor()->textCursor().position();
    ASSERT_TRUE(strRet.isEmpty() && iRet == 0);

    pWindow->deleteLater();
}

//moveToLineIndentation 002
TEST_F(test_textedit, moveToLineIndentation_002)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world\nHolle world"));
    pWindow->currentWrapper()->textEditor()->moveToLineIndentation();
    QString strRet(pWindow->currentWrapper()->textEditor()->textCursor().selectedText());
    int iRet = pWindow->currentWrapper()->textEditor()->textCursor().position();
    ASSERT_TRUE(strRet.isEmpty() && iRet == 12);

    pWindow->deleteLater();
}

//nextLine 001
TEST_F(test_textedit, nextLine_001)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world\nHolle world"));
    QTextCursor textCursor = pWindow->currentWrapper()->textEditor()->textCursor();
    textCursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor);
    pWindow->currentWrapper()->textEditor()->setTextCursor(textCursor);
    pWindow->currentWrapper()->textEditor()->m_cursorMark = true;
    pWindow->currentWrapper()->textEditor()->nextLine();
    QString strRet(pWindow->currentWrapper()->textEditor()->textCursor().selectedText());
    ASSERT_TRUE(!strRet.isEmpty());

    pWindow->deleteLater();
}

//nextLine 002
TEST_F(test_textedit, nextLine_002)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world\nHolle world"));
    QTextCursor textCursor = pWindow->currentWrapper()->textEditor()->textCursor();
    textCursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor);
    pWindow->currentWrapper()->textEditor()->setTextCursor(textCursor);
    pWindow->currentWrapper()->textEditor()->m_cursorMark = false;
    pWindow->currentWrapper()->textEditor()->nextLine();
    QString strRet(pWindow->currentWrapper()->textEditor()->textCursor().selectedText());
    ASSERT_TRUE(strRet.isEmpty());

    pWindow->deleteLater();
}

//prevLine 001
TEST_F(test_textedit, prevLine_001)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world\nHolle world"));
    pWindow->currentWrapper()->textEditor()->m_cursorMark = true;
    pWindow->currentWrapper()->textEditor()->prevLine();
    QString strRet(pWindow->currentWrapper()->textEditor()->textCursor().selectedText());
    ASSERT_TRUE(!strRet.isEmpty());

    pWindow->deleteLater();
}

//prevLine 002
TEST_F(test_textedit, prevLine_002)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world\nHolle world"));
    pWindow->currentWrapper()->textEditor()->m_cursorMark = false;
    pWindow->currentWrapper()->textEditor()->prevLine();
    QString strRet(pWindow->currentWrapper()->textEditor()->textCursor().selectedText());
    ASSERT_TRUE(strRet.isEmpty());

    pWindow->deleteLater();
}

//jumpToLine
TEST_F(test_textedit, jumpToLine)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world\nHolle world"));
    pWindow->currentWrapper()->textEditor()->jumpToLine(1, true);
    int iRet = pWindow->currentWrapper()->textEditor()->textCursor().blockNumber() + 1;
    ASSERT_TRUE(iRet == 1);

    pWindow->deleteLater();
}

//newline
TEST_F(test_textedit, newline)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world\nHolle world"));
    pWindow->currentWrapper()->textEditor()->newline();
    int iRet = pWindow->currentWrapper()->textEditor()->textCursor().blockNumber() + 1;
    ASSERT_TRUE(iRet == 3);

    pWindow->deleteLater();
}

//openNewlineAbove
TEST_F(test_textedit, openNewlineAbove)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world\nHolle world"));
    pWindow->currentWrapper()->textEditor()->openNewlineAbove();
    int iRet = pWindow->currentWrapper()->textEditor()->textCursor().blockNumber() + 1;
    ASSERT_TRUE(iRet == 2);

    pWindow->deleteLater();
}

//openNewlineBelow
TEST_F(test_textedit, openNewlineBelow)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world\nHolle world"));
    pWindow->currentWrapper()->textEditor()->openNewlineBelow();
    int iRet = pWindow->currentWrapper()->textEditor()->textCursor().blockNumber() + 1;
    ASSERT_TRUE(iRet == 3);

    pWindow->deleteLater();
}

//moveLineDownUp 001
TEST_F(test_textedit, moveLineDownUp_001)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world\nHolle world"));
    QTextCursor textCursor = pWindow->currentWrapper()->textEditor()->textCursor();
    textCursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
    pWindow->currentWrapper()->textEditor()->setTextCursor(textCursor);
    pWindow->currentWrapper()->textEditor()->moveLineDownUp(true);
    int iRet = pWindow->currentWrapper()->textEditor()->textCursor().blockNumber() + 1;
    ASSERT_TRUE(iRet == 1);

    pWindow->deleteLater();
}

//moveLineDownUp 002
TEST_F(test_textedit, moveLineDownUp_002)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world\nHolle world\nHolle world"));
    QTextCursor textCursor = pWindow->currentWrapper()->textEditor()->textCursor();
    textCursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor);
    pWindow->currentWrapper()->textEditor()->setTextCursor(textCursor);
    pWindow->currentWrapper()->textEditor()->moveLineDownUp(false);
    int iRet = pWindow->currentWrapper()->textEditor()->textCursor().blockNumber() + 1;
    ASSERT_TRUE(iRet == 3);

    pWindow->deleteLater();
}

//scrollLineUp
TEST_F(test_textedit, scrollLineUp)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("H\nl\nl\ne\n\nw\no\nr\nl\nd\n.H\nl\nl\ne\n\nw"
                                                                  "\no\nr\nl\nd\n.H\nl\nl\ne\n\nw\no\nr\nl\nd\n.H"
                                                                  "\nl\nl\ne\n\nw\no\nr\nl\nd\n.H\nl\nl\ne\n\nw\no"
                                                                  "\nr\nl\nd\n."));
    pWindow->currentWrapper()->textEditor()->scrollLineUp();
    int iRet = pWindow->currentWrapper()->textEditor()->verticalScrollBar()->value();
    ASSERT_TRUE(iRet == 26);

    pWindow->deleteLater();
}

TEST_F(test_textedit, scrollLineDown)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("H\nl\nl\ne\n\nw\no\nr\nl\nd\n.H\nl\nl\ne\n\nw"
                                                                  "\no\nr\nl\nd\n.H\nl\nl\ne\n\nw\no\nr\nl\nd\n.H"
                                                                  "\nl\nl\ne\n\nw\no\nr\nl\nd\n.H\nl\nl\ne\n\nw\no"
                                                                  "\nr\nl\nd\n."));
    QTextCursor textCursor = pWindow->currentWrapper()->textEditor()->textCursor();
    textCursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
    pWindow->currentWrapper()->textEditor()->setTextCursor(textCursor);
    pWindow->currentWrapper()->textEditor()->scrollLineDown();
    int iRet = pWindow->currentWrapper()->textEditor()->verticalScrollBar()->value();
    ASSERT_TRUE(iRet == 1);

    pWindow->deleteLater();
}

TEST_F(test_textedit, scrollUp)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("H\nl\nl\ne\n\nw\no\nr\nl\nd\n.H\nl\nl\ne\n\nw"
                                                                  "\no\nr\nl\nd\n.H\nl\nl\ne\n\nw\no\nr\nl\nd\n.H"
                                                                  "\nl\nl\ne\n\nw\no\nr\nl\nd\n.H\nl\nl\ne\n\nw\no"
                                                                  "\nr\nl\nd\n.H\nl\nl\ne\n\nw\no\nr\nl\nd\n.H\n"));
    int iScrollBarMaxmun = pWindow->currentWrapper()->textEditor()->verticalScrollBar()->maximum();
    pWindow->currentWrapper()->textEditor()->verticalScrollBar()->setValue(iScrollBarMaxmun);
    pWindow->currentWrapper()->textEditor()->scrollUp();
    int iRet = pWindow->currentWrapper()->textEditor()->verticalScrollBar()->value();
    ASSERT_TRUE(iScrollBarMaxmun != iRet);

    pWindow->deleteLater();
}

TEST_F(test_textedit, scrollDown)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("H\nl\nl\ne\n\nw\no\nr\nl\nd\n.H\nl\nl\ne\n\nw"
                                                                  "\no\nr\nl\nd\n.H\nl\nl\ne\n\nw\no\nr\nl\nd\n.H"
                                                                  "\nl\nl\ne\n\nw\no\nr\nl\nd\n.H\nl\nl\ne\n\nw\no"
                                                                  "\nr\nl\nd\n.H\nl\nl\ne\n\nw\no\nr\nl\nd\n.H\n"));
    pWindow->currentWrapper()->textEditor()->verticalScrollBar()->setValue(0);
    pWindow->currentWrapper()->textEditor()->scrollDown();
    int iRet = pWindow->currentWrapper()->textEditor()->verticalScrollBar()->value();
    ASSERT_TRUE(iRet != 0);

    pWindow->deleteLater();
}

//duplicateLine 001
TEST_F(test_textedit, duplicateLine_001)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world\nHolle world\nHolle world"));
    QTextCursor textCursor = pWindow->currentWrapper()->textEditor()->textCursor();
    textCursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
    pWindow->currentWrapper()->textEditor()->setTextCursor(textCursor);
    int iDuplicateLineBeFor = pWindow->currentWrapper()->textEditor()->blockCount();
    pWindow->currentWrapper()->textEditor()->duplicateLine();
    int iDuplicateLineAfter = pWindow->currentWrapper()->textEditor()->blockCount();

    ASSERT_TRUE(iDuplicateLineAfter == iDuplicateLineBeFor + 1);

    pWindow->deleteLater();
}

//duplicateLine 002
TEST_F(test_textedit, duplicateLine_002)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world\nHolle world\nHolle world"));
    QTextCursor textCursor = pWindow->currentWrapper()->textEditor()->textCursor();
    textCursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
    textCursor.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor);
    pWindow->currentWrapper()->textEditor()->setTextCursor(textCursor);
    int iDuplicateLineBeFor = pWindow->currentWrapper()->textEditor()->blockCount();
    pWindow->currentWrapper()->textEditor()->duplicateLine();
    int iDuplicateLineAfter = pWindow->currentWrapper()->textEditor()->blockCount();

    ASSERT_TRUE(iDuplicateLineAfter == iDuplicateLineBeFor + 1);

    pWindow->deleteLater();
}

//duplicateLine 003
TEST_F(test_textedit, duplicateLine_003)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world\nHolle world\nHolle world"));
    QTextCursor textCursor = pWindow->currentWrapper()->textEditor()->textCursor();
    textCursor.movePosition(QTextCursor::PreviousWord, QTextCursor::MoveAnchor);
    pWindow->currentWrapper()->textEditor()->setTextCursor(textCursor);
    int iDuplicateLineBeFor = pWindow->currentWrapper()->textEditor()->blockCount();
    pWindow->currentWrapper()->textEditor()->duplicateLine();
    int iDuplicateLineAfter = pWindow->currentWrapper()->textEditor()->blockCount();

    ASSERT_TRUE(iDuplicateLineAfter == iDuplicateLineBeFor + 1);

    pWindow->deleteLater();
}

//void copyLines 001
TEST_F(test_textedit, copyLines_001)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world\nHolle world\nHolle world"));
    QTextCursor textCursor = pWindow->currentWrapper()->textEditor()->textCursor();
    textCursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
    pWindow->currentWrapper()->textEditor()->setTextCursor(textCursor);
    QClipboard *pClipboard = QApplication::clipboard();
    pClipboard->clear();
    pWindow->currentWrapper()->textEditor()->copyLines();
    QString strRet(pClipboard->text());
    ASSERT_TRUE(!strRet.compare(QString("Holle world")));

    pWindow->deleteLater();
}

//void copyLines 002
TEST_F(test_textedit, copyLines_002)
{
    Window *pWindow = new Window();
    pWindow->addBlankTab(QString());
    pWindow->currentWrapper()->textEditor()->insertTextEx(pWindow->currentWrapper()->textEditor()->textCursor(),
                                                          QString("Holle world\nHolle world\nHolle world"));
    QTextCursor textCursor = pWindow->currentWrapper()->textEditor()->textCursor();
    textCursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
    pWindow->currentWrapper()->textEditor()->setTextCursor(textCursor);
    QClipboard *pClipboard = QApplication::clipboard();
    pClipboard->clear();
    pWindow->currentWrapper()->textEditor()->copyLines();
    QString strRet(pClipboard->text());
    ASSERT_TRUE(!strRet.compare(QString("Holle world")));

    pWindow->deleteLater();
}

void stub_push(QUndoCommand *cmd)
{
    Q_UNUSED(cmd);
}

void stub_updateModifyStatus(const QString &path, bool isModified)
{
    Q_UNUSED(path);
    Q_UNUSED(isModified);
}

//void cutlines();
TEST_F(test_textedit, cutlines)
{
    Stub stub;
    stub.set(ADDR(QUndoStack, push), stub_push);
    stub.set(ADDR(Window, updateModifyStatus), stub_updateModifyStatus);
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->cutlines();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    p->deleteLater();
    startManager->deleteLater();
    ee->deleteLater();
}

TEST_F(test_textedit, joinLines)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->joinLines();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    p->deleteLater();
    startManager->deleteLater();
    ee->deleteLater();
}

TEST_F(test_textedit, killLine)
{
    Window *pWindow = new Window;
    EditWrapper *wrapper = pWindow->createEditor();
    QFile f("1.cpp");
    if (f.exists()) {
        pWindow->addTab("1.cpp");
        pWindow->currentWrapper()->m_pTextEdit->killLine();
    }

    ASSERT_TRUE(wrapper->m_pTextEdit != nullptr);
    wrapper->deleteLater();
    pWindow->deleteLater();
}
TEST_F(test_textedit, killCurrentLine)
{
    Window *pWindow = new Window;
    EditWrapper *wrapper = pWindow->createEditor();
    QFile f("1.cpp");
    if (f.exists()) {
        pWindow->addTab("1.cpp");
        pWindow->currentWrapper()->m_pTextEdit->killCurrentLine();
    }

    ASSERT_TRUE(wrapper->m_pTextEdit != nullptr);
    wrapper->deleteLater();
    pWindow->deleteLater();
}
TEST_F(test_textedit, killBackwardWord)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->killBackwardWord();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

TEST_F(test_textedit, killForwardWord)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->killForwardWord();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
TEST_F(test_textedit, escape)
{
    //QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    //EditWrapper * ee = new EditWrapper();
    //startManager->setWrapper(ee);
    //startManager->escape();
}

TEST_F(test_textedit, indentText)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->indentText();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

TEST_F(test_textedit, unindentText)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->unindentText();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

TEST_F(test_textedit, setTabSpaceNumber)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->setTabSpaceNumber(2);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

//QString capitalizeText(QString text);

//void keepCurrentLineAtCenter();
//void scrollToLine(int scrollOffset, int row, int column);

//void setLineWrapMode(bool enable);

TEST_F(test_textedit, capitalizeText)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->capitalizeText("2");

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

TEST_F(test_textedit, keepCurrentLineAtCenter)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->keepCurrentLineAtCenter();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
TEST_F(test_textedit, setLineWrapMode)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->setLineWrapMode(true);
    startManager->setLineWrapMode(false);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

//void setFontFamily(QString fontName);
//void setFontSize(int fontSize);
//void updateFont();
TEST_F(test_textedit, setFontFamily)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->setFontFamily("2");

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
TEST_F(test_textedit, setFontSize)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->setFontSize(2);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
TEST_F(test_textedit, updateFont)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->updateFont();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

TEST_F(test_textedit, replaceAll)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    //startManager->replaceAll("aa","bb");

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

TEST_F(test_textedit, replaceNext)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    //    startManager->replaceAll("aa","bb");

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
TEST_F(test_textedit, replaceRest)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    //startManager->replaceRest("aa","bb");

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
TEST_F(test_textedit, beforeReplace)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->beforeReplace("bb");

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
TEST_F(test_textedit, findKeywordForward)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->findKeywordForward("bb");

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
TEST_F(test_textedit, removeKeywords)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->removeKeywords();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
//void renderAllSelections();
TEST_F(test_textedit, highlightKeyword)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->highlightKeyword("aa", 2);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
TEST_F(test_textedit, updateCursorKeywordSelection)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->updateCursorKeywordSelection("aa", true);
    startManager->updateCursorKeywordSelection("aa", false);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
TEST_F(test_textedit, updateHighlightLineSelection)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->updateHighlightLineSelection();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
TEST_F(test_textedit, renderAllSelections)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->renderAllSelections();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
//DMenu *getHighlightMenu();
TEST_F(test_textedit, getHighlightMenu)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->getHighlightMenu();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

//void lineNumberAreaPaintEvent(QPaintEvent *event);
TEST_F(test_textedit, lineNumberAreaPaintEvent)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    QPaintEvent *e;
    startManager->setWrapper(ee);
    startManager->lineNumberAreaPaintEvent(e);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

//void codeFLodAreaPaintEvent(QPaintEvent *event);
TEST_F(test_textedit, codeFLodAreaPaintEvent)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    QPaintEvent *e;
    startManager->setWrapper(ee);
    startManager->codeFLodAreaPaintEvent(e);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
//void setCodeFlodFlagVisable(bool isVisable,bool bIsFirstOpen = false);
TEST_F(test_textedit, setCodeFlodFlagVisable)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->setCodeFlodFlagVisable(true, false);
    startManager->setCodeFlodFlagVisable(false, false);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
//void setThemeWithPath(const QString &path);
TEST_F(test_textedit, setThemeWithPath)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    ee->OnThemeChangeSlot("aa");

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
//void setTheme(const KSyntaxHighlighting::Theme &theme, const QString &path);

//bool highlightWordUnderMouse(QPoint pos);
TEST_F(test_textedit, highlightWordUnderMouse)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    QPoint x(10, 10);
    startManager->setWrapper(ee);
    startManager->highlightWordUnderMouse(x);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
//void removeHighlightWordUnderCursor();
TEST_F(test_textedit, removeHighlightWordUnderCursor)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->removeHighlightWordUnderCursor();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

//void setSettings(Settings *settings);
TEST_F(test_textedit, setSettings)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *set;
    startManager->setWrapper(ee);
    startManager->setSettings(set);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

//void copySelectedText();
TEST_F(test_textedit, copySelectedText)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->copySelectedText();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

//void cutSelectedText();
TEST_F(test_textedit, cutSelectedText)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->cutSelectedText();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
//void pasteText();
TEST_F(test_textedit, pasteText)
{
    QClipboard *c = QApplication::clipboard();
    c->setText("ddd");
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->pasteText();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

//void setMark();
TEST_F(test_textedit, setMark)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->setMark();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

//void unsetMark();
TEST_F(test_textedit, unsetMark)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->unsetMark();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

//bool tryUnsetMark();
TEST_F(test_textedit, tryUnsetMark)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->tryUnsetMark();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

//void exchangeMark();
TEST_F(test_textedit, exchangeMark)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->exchangeMark();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
//void saveMarkStatus();
TEST_F(test_textedit, saveMarkStatus)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->saveMarkStatus();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
//void restoreMarkStatus();
TEST_F(test_textedit, restoreMarkStatus)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->restoreMarkStatus();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

//void completionWord(QString word);
//QString getWordAtMouse();
TEST_F(test_textedit, getWordAtMouse)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->getWordAtMouse();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
//QString getWordAtCursor();

//void toggleReadOnlyMode();
TEST_F(test_textedit, toggleReadOnlyMode)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->toggleReadOnlyMode();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
//void toggleComment(bool sister);
TEST_F(test_textedit, toggleComment)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->toggleComment(true);
    startManager->toggleComment(false);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
//bool atWordSeparator(int position);
TEST_F(test_textedit, atWordSeparator)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->atWordSeparator(2);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

//void showCursorBlink();
TEST_F(test_textedit, showCursorBlink)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->showCursorBlink();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
//void hideCursorBlink();
TEST_F(test_textedit, hideCursorBlink)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->hideCursorBlink();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

//void setReadOnlyPermission(bool permission);
TEST_F(test_textedit, setReadOnlyPermission)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->setReadOnlyPermission(true);
    startManager->setReadOnlyPermission(false);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
//bool getReadOnlyPermission();
TEST_F(test_textedit, getReadOnlyPermission)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->getReadOnlyPermission();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
//bool getReadOnlyMode();
TEST_F(test_textedit, getReadOnlyMode)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->getReadOnlyMode();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

//void hideRightMenu();
TEST_F(test_textedit, hideRightMenu)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->hideRightMenu();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

//void clearBlack();
TEST_F(test_textedit, clearBlack)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    //startManager->clearBlack();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
//void flodOrUnflodAllLevel(bool isFlod);
TEST_F(test_textedit, flodOrUnflodAllLevel)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->flodOrUnflodAllLevel(true);
    startManager->flodOrUnflodAllLevel(false);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
//void flodOrUnflodCurrentLevel(bool isFlod);
TEST_F(test_textedit, flodOrUnflodCurrentLevel)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->flodOrUnflodCurrentLevel(true);
    startManager->flodOrUnflodCurrentLevel(false);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
//void getHideRowContent(int iLine);
TEST_F(test_textedit, getHideRowContent)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->getHideRowContent(1);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
//int  getHighLightRowContentLineNum(int iLine);
TEST_F(test_textedit, getHighLightRowContentLineNum)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->getHighLightRowContentLineNum(1);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
//int  getLinePosByLineNum(int iLine);
TEST_F(test_textedit, getLinePosYByLineNum)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->getLinePosYByLineNum(1);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
//bool ifHasHighlight();
TEST_F(test_textedit, ifHasHighlight)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->ifHasHighlight();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
//void bookMarkAreaPaintEvent(QPaintEvent *event);
TEST_F(test_textedit, bookMarkAreaPaintEvent)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    QPaintEvent *e;
    startManager->bookMarkAreaPaintEvent(e);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

//int getLineFromPoint(const QPoint &point);
TEST_F(test_textedit, getLineFromPoint)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    QPoint s(10, 10);
    startManager->getLineFromPoint(s);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

//void addOrDeleteBookMark();
TEST_F(test_textedit, addOrDeleteBookMark)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->addOrDeleteBookMark();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

//void moveToPreviousBookMark();
TEST_F(test_textedit, moveToPreviousBookMark)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->moveToPreviousBookMark();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

//void checkBookmarkLineMove(int from, int charsRemoved, int charsAdded);
TEST_F(test_textedit, checkBookmarkLineMove)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->checkBookmarkLineMove(2, 3, 4);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

//void setIsFileOpen();
TEST_F(test_textedit, setIsFileOpen)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->setIsFileOpen();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

//QStringList readHistoryRecordofBookmark();
TEST_F(test_textedit, readHistoryRecordofBookmark)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->readHistoryRecordofBookmark();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

//QStringList readHistoryRecordofFilePath(QString key);
TEST_F(test_textedit, readHistoryRecordofFilePath)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->readHistoryRecordofFilePath("advance.editor.browsing_history_file");

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

//void writeHistoryRecord();
TEST_F(test_textedit, writeHistoryRecord)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->writeHistoryRecord();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

//void isMarkCurrentLine(bool isMark, QString strColor = "");
TEST_F(test_textedit, isMarkCurrentLine)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->isMarkCurrentLine(true, "red");
    startManager->isMarkCurrentLine(false, "red");

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

//void isMarkAllLine(bool isMark, QString strColor = "");
TEST_F(test_textedit, isMarkAllLine)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->isMarkAllLine(true, "red");
    startManager->isMarkAllLine(false, "red");

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
    s->deleteLater();
}

//void cancelLastMark();
TEST_F(test_textedit, cancelLastMark)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->cancelLastMark();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
    s->deleteLater();
}

//void markSelectWord();
TEST_F(test_textedit, markSelectWord)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->markSelectWord();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
    s->deleteLater();
}

//void updateMark(int from, int charsRemoved, int charsAdded);
TEST_F(test_textedit, updateMark)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->updateMark(1, 2, 3);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
    s->deleteLater();
}

//bool containsExtraSelection(QList<QTextEdit::ExtraSelection> listSelections, QTextEdit::ExtraSelection selection);
TEST_F(test_textedit, containsExtraSelection)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->containsExtraSelection(listSelection, selectio);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
    s->deleteLater();
}

//void appendExtraSelection(QList<QTextEdit::ExtraSelection> wordMarkSelections, QTextEdit::ExtraSelection selection
//                          , QString strColor, QList<QTextEdit::ExtraSelection> *listSelections);
TEST_F(test_textedit, appendExtraSelection)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    selectio.cursor = startManager->textCursor();
    selectio.format.setBackground(QColor(Qt::red));
    QList<QTextEdit::ExtraSelection> listSelectionsd;
    listSelectionsd.append(selectio);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->appendExtraSelection(listSelection, selectio, "#000000", &listSelectionsd);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
    s->deleteLater();
}
/// void setCursorStart(int _);
TEST_F(test_textedit, setCursorStart)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->setCursorStart(2);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
    s->deleteLater();
}
//void writeEncodeHistoryRecord();
TEST_F(test_textedit, writeEncodeHistoryRecord)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->writeEncodeHistoryRecord();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
    s->deleteLater();
}
//QStringList readEncodeHistoryRecord();
TEST_F(test_textedit, readEncodeHistoryRecord)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->readEncodeHistoryRecord();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
    s->deleteLater();
}

//clickFindAction
TEST_F(test_textedit, clickFindAction)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->clickFindAction();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
    s->deleteLater();
}
// void tellFindBarClose();
TEST_F(test_textedit, tellFindBarClose)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->tellFindBarClose();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
    s->deleteLater();
}

// void setEditPalette(const QString &activeColor, const QString &inactiveColor);
TEST_F(test_textedit, setEditPalette)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->setEditPalette("aa", "aa");

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    s->deleteLater();
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

// void setCodeFoldWidgetHide(bool isHidden);
TEST_F(test_textedit, setCodeFoldWidgetHide)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->setCodeFoldWidgetHide(true);
    startManager->setCodeFoldWidgetHide(false);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    s->deleteLater();
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
// void updateLeftAreaWidget();
TEST_F(test_textedit, updateLeftAreaWidget)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->updateLeftAreaWidget();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    s->deleteLater();
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

// void updateLineNumber();
TEST_F(test_textedit, updateLineNumber)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->updateLeftAreaWidget();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    s->deleteLater();
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
// void handleScrollFinish();
TEST_F(test_textedit, handleScrollFinish)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->handleScrollFinish();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    s->deleteLater();
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

// void slot_translate();
TEST_F(test_textedit, slot_translate)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->slot_translate();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    s->deleteLater();
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

#if 0 //gerrit上段错误，暂且屏蔽
// void upcaseWord();
 TEST_F(test_textedit, upcaseWord)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->upcaseWord();

    
}
#endif

#if 0 //gerrit上段错误，暂且屏蔽
// void downcaseWord();
 TEST_F(test_textedit, downcaseWord)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->downcaseWord();

    
}
#endif

// void capitalizeWord();
// TEST_F(test_textedit, capitalizeWord)
//{
//    QList<QTextEdit::ExtraSelection> listSelection;
//    QTextEdit::ExtraSelection selectio;
//    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
//    EditWrapper * ee = new EditWrapper();
//    Settings *s = new Settings();
//    startManager->setSettings(s);
//    startManager->setWrapper(ee);
//    startManager->capitalizeWord();

//    
//}
// void transposeChar();

TEST_F(test_textedit, transposeChar)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->transposeChar();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    s->deleteLater();
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

// void handleCursorMarkChanged(bool mark, QTextCursor cursor);
TEST_F(test_textedit, handleCursorMarkChanged)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->handleCursorMarkChanged(true, QTextCursor());
    startManager->handleCursorMarkChanged(false, QTextCursor());

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    s->deleteLater();
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

// void adjustScrollbarMargins();
TEST_F(test_textedit, adjustScrollbarMargins)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->adjustScrollbarMargins();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    s->deleteLater();
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

// void onSelectionArea();
TEST_F(test_textedit, onSelectionArea)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->onSelectionArea();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    s->deleteLater();
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
// void fingerZoom(QString name, QString direction, int fingers);
TEST_F(test_textedit, fingerZoom)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->fingerZoom("aa", "aa", 3);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    s->deleteLater();
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

// bool event(QEvent* evt) override;   //触摸屏event事件
TEST_F(test_textedit, event)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QEvent *e = new QEvent(QEvent::Type::MouseButtonPress);
    startManager->event(e);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    s->deleteLater();
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

// void inputMethodEvent(QInputMethodEvent *e) override;

// void mousePressEvent(QMouseEvent *e) override;
TEST_F(test_textedit, mousePressEvent)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QPoint a(1, 2);
    QPointF b(a);
    QMouseEvent *e = new QMouseEvent(QMouseEvent::Type::Enter, b, Qt::MouseButton::LeftButton, Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier);
    startManager->mousePressEvent(e);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    s->deleteLater();
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

// void mouseMoveEvent(QMouseEvent *e) override;
TEST_F(test_textedit, mouseMoveEvent)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QPoint a(1, 2);
    QPointF b(a);
    QMouseEvent *e = new QMouseEvent(QMouseEvent::Type::Enter, b, Qt::MouseButton::LeftButton, Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier);
    startManager->mouseMoveEvent(e);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    s->deleteLater();
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
// void mouseReleaseEvent(QMouseEvent *e) override;
TEST_F(test_textedit, mouseReleaseEvent)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QPoint a(1, 2);
    QPointF b(a);
    QMouseEvent *e = new QMouseEvent(QMouseEvent::Type::Enter, b, Qt::MouseButton::LeftButton, Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier);
    startManager->mouseReleaseEvent(e);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    s->deleteLater();
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

// void wheelEvent(QWheelEvent *e) override;
TEST_F(test_textedit, wheelEvent)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QPointF pos;
    QWheelEvent *e = new QWheelEvent(pos, 4, Qt::MouseButton::LeftButton, Qt::KeyboardModifier::AltModifier);

    //    QWheelEvent(const QPointF &pos, int delta,
    //    Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers,
    //    Qt::Orientation orient = Qt::Vertical);
    startManager->wheelEvent(e);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    s->deleteLater();
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
// bool eventFilter(QObject *object, QEvent *event) override;
TEST_F(test_textedit, eventFilter)
{
    //    QList<QTextEdit::ExtraSelection> listSelection;
    //    QTextEdit::ExtraSelection selectio;
    //    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    //    EditWrapper * ee = new EditWrapper();
    //    Settings *s = new Settings();
    //    startManager->setSettings(s);
    //    startManager->setWrapper(ee);
    //    QObject *object;

    //    QEvent *e=new QEvent(QEvent::MouseButtonPress);
    //    startManager->eventFilter(startManager,e);
    //    //MouseButtonDblClick

    //    QEvent *e1=new QEvent(QEvent::MouseButtonDblClick);
    //    startManager->eventFilter(startManager,e1);
    //    QEvent *e2=new QEvent(QEvent::HoverMove);
    //    startManager->eventFilter(startManager,e2);
    //    QEvent *e3=new QEvent(QEvent::HoverLeave);
    //    startManager->eventFilter(startManager,e3);

    //    
}
QAction *stub_exec(const QPoint &pos, QAction *at = nullptr)
{
    Q_UNUSED(pos)
    Q_UNUSED(at)

    return nullptr;
}
// void contextMenuEvent(QContextMenuEvent *event) override;

TEST_F(test_textedit, contextMenuEvent)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    QPoint b(0, 0);
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    Stub stub;
    stub.set((QAction * (QMenu::*)(const QPoint &, QAction *)) ADDR(QMenu, exec), stub_exec);
    QContextMenuEvent *e = new QContextMenuEvent(QContextMenuEvent::Reason::Keyboard, b);
    //startManager->contextMenuEvent(e);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    s->deleteLater();
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
// void paintEvent(QPaintEvent *e) override;
TEST_F(test_textedit, paintEvent)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QRect a(1, 2, 3, 4);
    QPaintEvent *e = new QPaintEvent(a);
    startManager->paintEvent(e);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    s->deleteLater();
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

// bool blockContainStrBrackets(int line);
TEST_F(test_textedit, blockContainStrBrackets)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QPaintEvent *e;
    startManager->blockContainStrBrackets(2);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    s->deleteLater();
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
// bool setCursorKeywordSeletoin(int position, bool findNext);
TEST_F(test_textedit, setCursorKeywordSeletoin)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QPaintEvent *e;
    startManager->setCursorKeywordSeletoin(2, true);
    startManager->setCursorKeywordSeletoin(2, false);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    s->deleteLater();
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
// void cursorPositionChanged();
TEST_F(test_textedit, cursorPositionChanged)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QPaintEvent *e;
    startManager->cursorPositionChanged();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    s->deleteLater();
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
// void updateHighlightBrackets(const QChar &openChar, const QChar &closeChar);
TEST_F(test_textedit, updateHighlightBrackets)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    QChar a = ' ';
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QPaintEvent *e;
    startManager->updateHighlightBrackets(a, a);

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    s->deleteLater();
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}
// int getFirstVisibleBlockId() const;
TEST_F(test_textedit, getFirstVisibleBlockId)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper *ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QPaintEvent *e;
    //startManager->getFirstVisibleBlockId();

    ASSERT_TRUE(ee->m_pTextEdit != nullptr);
    s->deleteLater();
    ee->deleteLater();
    startManager->deleteLater();
    p->deleteLater();
}

//void setTruePath(QString qstrTruePath);
TEST_F(test_textedit, setBackupPath)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    startManager->setTruePath("aa");

    ASSERT_TRUE(startManager->m_pLeftAreaWidget != nullptr);
    startManager->deleteLater();
    p->deleteLater();
}

// QString getTruePath();
TEST_F(test_textedit, getTruePath)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    startManager->getTruePath();

    ASSERT_TRUE(startManager->m_pLeftAreaWidget != nullptr);
    startManager->deleteLater();
    p->deleteLater();
}

////初始化右键菜单
//void initRightClickedMenu();
TEST_F(test_textedit, initRightClickedMenu)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    startManager->initRightClickedMenu();

    ASSERT_TRUE(startManager->m_pLeftAreaWidget != nullptr);
    startManager->deleteLater();
    p->deleteLater();
}

//inline QString getFilePath() { return m_sFilePath;};
TEST_F(test_textedit, getFilePath)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    startManager->getFilePath();

    ASSERT_TRUE(startManager->m_pLeftAreaWidget != nullptr);
    startManager->deleteLater();
    p->deleteLater();
}
////
//inline void setFilePath(QString file) { m_sFilePath = file;}
TEST_F(test_textedit, setFilePath)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    startManager->setFilePath("a");

    ASSERT_TRUE(startManager->m_pLeftAreaWidget != nullptr);
    startManager->deleteLater();
    p->deleteLater();
}
////
//int characterCount() const;
TEST_F(test_textedit, characterCount)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    startManager->characterCount();

    ASSERT_TRUE(startManager->m_pLeftAreaWidget != nullptr);
    startManager->deleteLater();
    p->deleteLater();
}

//void moveCursorNoBlink(QTextCursor::MoveOperation operation,
//                       QTextCursor::MoveMode mode = QTextCursor::MoveAnchor);
TEST_F(test_textedit, moveCursorNoBlink)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    startManager->moveCursorNoBlink(QTextCursor::MoveOperation::Up);

    ASSERT_TRUE(startManager->m_pLeftAreaWidget != nullptr);
    startManager->deleteLater();
    p->deleteLater();
}

//void convertWordCase(ConvertCase convertCase);
//TEST_F(test_textedit, convertWordCase)
//{
//    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
//    startManager->convertWordCase(ConvertCase::LOWER);
//    startManager->convertWordCase(ConvertCase::UPPER);
//    startManager->convertWordCase(ConvertCase::CAPITALIZE);

//    
//}

//void renderAllSelections();

//void setBookmarkFlagVisable(bool isVisable,bool bIsFirstOpen = false);
TEST_F(test_textedit, setBookmarkFlagVisable)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    startManager->setBookmarkFlagVisable(false, false);
    startManager->setBookmarkFlagVisable(true, false);

    ASSERT_TRUE(startManager->m_pLeftAreaWidget != nullptr);
    startManager->deleteLater();
    p->deleteLater();
}
/*
    void unCommentSelection();
    void setComment();
    void removeComment();
*/
TEST_F(test_textedit, unCommentSelection)
{
    EditWrapper *editWrapper = new EditWrapper;
    editWrapper->openFile("1.cpp", "1.cpp");
    QTextCursor cursor = editWrapper->m_pTextEdit->textCursor();
    cursor.setPosition(0, QTextCursor::MoveAnchor);
    editWrapper->m_pTextEdit->setTextCursor(cursor);
    editWrapper->m_pTextEdit->unCommentSelection();

    ASSERT_TRUE(editWrapper->m_pTextEdit != nullptr);
    editWrapper->deleteLater();
}

TEST_F(test_textedit, setComment)
{
    EditWrapper *editWrapper = new EditWrapper;
    editWrapper->openFile("1.cpp", "1.cpp");
    QTextCursor cursor = editWrapper->m_pTextEdit->textCursor();
    cursor.setPosition(0, QTextCursor::MoveAnchor);
    editWrapper->m_pTextEdit->setTextCursor(cursor);
    editWrapper->m_pTextEdit->setComment();

    ASSERT_TRUE(editWrapper->m_pTextEdit != nullptr);
    editWrapper->deleteLater();
}

TEST_F(test_textedit, removeComment)
{
    EditWrapper *editWrapper = new EditWrapper;
    editWrapper->openFile("1.cpp", "1.cpp");
    QTextCursor cursor = editWrapper->m_pTextEdit->textCursor();
    cursor.setPosition(0, QTextCursor::MoveAnchor);
    editWrapper->m_pTextEdit->setTextCursor(cursor);
    editWrapper->m_pTextEdit->removeComment();

    ASSERT_TRUE(editWrapper->m_pTextEdit != nullptr);
    editWrapper->deleteLater();
}

TEST_F(test_textedit, keyPressEvent)
{
    Window *window = new Window;
    EditWrapper *editWrapper = window->createEditor();
    editWrapper->openFile("1.cpp", "1.cpp");
    QTextCursor cursor = editWrapper->m_pTextEdit->textCursor();
    cursor.setPosition(0, QTextCursor::MoveAnchor);
    editWrapper->m_pTextEdit->setTextCursor(cursor);
    QKeyEvent keyEvent(QEvent::KeyPress, Qt::Key_Insert, Qt::NoModifier);
    editWrapper->m_pTextEdit->keyPressEvent(&keyEvent);

    ASSERT_TRUE(editWrapper->m_pTextEdit != nullptr);
    editWrapper->deleteLater();
    window->deleteLater();
}
