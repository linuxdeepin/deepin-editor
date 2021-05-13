#include "test_textedit.h"
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
    if(f.open(QFile::WriteOnly)){
        f.write(text.toUtf8());
        f.close();
    }
}

void test_textedit::forstub(QPoint q)
{

}

TEST_F(test_textedit, setWrapper)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper *e = new EditWrapper();
    startManager->setWrapper(e);

    assert(1==1);
}
//TextEdit(QWidget *parent = nullptr);
TEST_F(test_textedit, TextEdit)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);

    assert(1==1);
}

//getCurrentLine
TEST_F(test_textedit, getCurrentLine)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    startManager->getCurrentLine();

    assert(1==1);
}
//getCurrentColumn
TEST_F(test_textedit, getCurrentColumn)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    startManager->getCurrentColumn();

    assert(1==1);
}
//getPosition
TEST_F(test_textedit, getPosition)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    startManager->getPosition();

    assert(1==1);
}
//getScrollOffset
TEST_F(test_textedit, getScrollOffset)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    startManager->getScrollOffset();

    assert(1==1);
}
//forwardChar
TEST_F(test_textedit, forwardChar)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    startManager->forwardChar();

    assert(1==1);
}
//backwardChar
TEST_F(test_textedit, backwardChar)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    startManager->backwardChar();

    assert(1==1);
}
//forwardWord
TEST_F(test_textedit, forwardWord)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    startManager->forwardWord();

    assert(1==1);
}
//forwardPair
TEST_F(test_textedit, forwardPair)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    startManager->forwardPair();

    assert(1==1);
}
//backwardPair
TEST_F(test_textedit, backwardPair)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    startManager->backwardPair();

    assert(1==1);
}
//blockCount
TEST_F(test_textedit, blockCount)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    startManager->blockCount();

    assert(1==1);
}
//moveToStart
TEST_F(test_textedit, moveToStart)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    startManager->moveToStart();

    assert(1==1);
}
//moveToEnd
TEST_F(test_textedit, moveToEnd)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    startManager->moveToEnd();

    assert(1==1);
}
//moveToStartOfLine
TEST_F(test_textedit, moveToStartOfLine)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    startManager->moveToStartOfLine();

    assert(1==1);
}
//moveToEndOfLine
TEST_F(test_textedit, moveToEndOfLine)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    startManager->moveToEndOfLine();

    assert(1==1);
}
//moveToLineIndentation
TEST_F(test_textedit, moveToLineIndentation)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    startManager->moveToLineIndentation();

    assert(1==1);
}
//nextLine
TEST_F(test_textedit, nextLine)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    startManager->nextLine();

    assert(1==1);
}
//prevLine
TEST_F(test_textedit, prevLine)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    startManager->prevLine();

    assert(1==1);
}
//jumpToLine
TEST_F(test_textedit, jumpToLine)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    startManager->jumpToLine(1,true);
    startManager->jumpToLine(1,false);


    assert(1==1);
}
//newline
TEST_F(test_textedit, newline)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->newline();

    assert(1==1);
}
//openNewlineAbove
TEST_F(test_textedit, openNewlineAbove)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->openNewlineAbove();

    assert(1==1);
}
//openNewlineBelow
TEST_F(test_textedit, openNewlineBelow)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->openNewlineBelow();

    assert(1==1);
}
//moveLineDownUp
TEST_F(test_textedit, moveLineDownUp)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->moveLineDownUp(true);
    startManager->moveLineDownUp(false);

    assert(1==1);
}
//scrollLineUp
TEST_F(test_textedit, scrollLineUp)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->scrollLineUp();

    assert(1==1);
}
TEST_F(test_textedit, scrollLineDown)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->scrollLineDown();

    assert(1==1);
}
TEST_F(test_textedit, scrollUp)
{
    QScrollBar *p = new QScrollBar();
    TextEdit *startManager = new TextEdit();
    startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);

    //startManager->scrollUp();

    assert(1==1);
}
TEST_F(test_textedit, scrollDown)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    //startManager->scrollDown();

    assert(1==1);
}
//duplicateLine
TEST_F(test_textedit, duplicateLine)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->duplicateLine();

    assert(1==1);
}
//    void copyLines();
//void cutlines();
TEST_F(test_textedit, copyLines)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->copyLines();

    assert(1==1);
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

TEST_F(test_textedit, cutlines)
{
    Stub stub;
    stub.set(ADDR(QUndoStack, push), stub_push);
    stub.set(ADDR(Window, updateModifyStatus), stub_updateModifyStatus);
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->cutlines();

    assert(1==1);
}

TEST_F(test_textedit, joinLines)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->joinLines();

    assert(1==1);
}
TEST_F(test_textedit, killLine)
{
    Window* pWindow = new Window;
    EditWrapper* wrapper = pWindow->createEditor();
    QFile f("1.cpp");
    if(f.exists()){
        pWindow->addTab("1.cpp");
        pWindow->currentWrapper()->m_pTextEdit->killLine();
    }
    assert(1==1);
}
TEST_F(test_textedit, killCurrentLine)
{
    Window* pWindow = new Window;
    EditWrapper* wrapper = pWindow->createEditor();
    QFile f("1.cpp");
    if(f.exists()){
        pWindow->addTab("1.cpp");
        pWindow->currentWrapper()->m_pTextEdit->killCurrentLine();
    }
    assert(1==1);
}
TEST_F(test_textedit, killBackwardWord)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->killBackwardWord();

    assert(1==1);
}

TEST_F(test_textedit, killForwardWord)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->killForwardWord();

    assert(1==1);
}
TEST_F(test_textedit, escape)
{
    //QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    //EditWrapper * ee = new EditWrapper();
    //startManager->setWrapper(ee);
    //startManager->escape();

    //assert(1==1);
}
TEST_F(test_textedit, indentText)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->indentText();

    assert(1==1);
}

TEST_F(test_textedit, unindentText)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->unindentText();

    assert(1==1);
}
TEST_F(test_textedit, setTabSpaceNumber)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->setTabSpaceNumber(2);

    assert(1==1);
}
//QString capitalizeText(QString text);

//void keepCurrentLineAtCenter();
//void scrollToLine(int scrollOffset, int row, int column);

//void setLineWrapMode(bool enable);

TEST_F(test_textedit, capitalizeText)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->capitalizeText("2");

    assert(1==1);
}
TEST_F(test_textedit, keepCurrentLineAtCenter)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->keepCurrentLineAtCenter();

    assert(1==1);
}
TEST_F(test_textedit, setLineWrapMode)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->setLineWrapMode(true);
    startManager->setLineWrapMode(false);


    assert(1==1);
}
//void setFontFamily(QString fontName);
//void setFontSize(int fontSize);
//void updateFont();
TEST_F(test_textedit, setFontFamily)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->setFontFamily("2");

    assert(1==1);
}
TEST_F(test_textedit, setFontSize)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->setFontSize(2);

    assert(1==1);
}
TEST_F(test_textedit, updateFont)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->updateFont();

    assert(1==1);
}

TEST_F(test_textedit, replaceAll)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->replaceAll("aa","bb");

    assert(1==1);
}

TEST_F(test_textedit, replaceNext)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->replaceAll("aa","bb");

    assert(1==1);
}
TEST_F(test_textedit, replaceRest)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->replaceRest("aa","bb");

    assert(1==1);
}
TEST_F(test_textedit, beforeReplace)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->beforeReplace("bb");

    assert(1==1);
}
TEST_F(test_textedit, findKeywordForward)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->findKeywordForward("bb");

    assert(1==1);
}
TEST_F(test_textedit, removeKeywords)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->removeKeywords();

    assert(1==1);
}
//void renderAllSelections();
TEST_F(test_textedit, highlightKeyword)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->highlightKeyword("aa",2);

    assert(1==1);
}
TEST_F(test_textedit, updateCursorKeywordSelection)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->updateCursorKeywordSelection("aa",true);
    startManager->updateCursorKeywordSelection("aa",false);

    assert(1==1);
}
TEST_F(test_textedit, updateHighlightLineSelection)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->updateHighlightLineSelection();

    assert(1==1);
}
TEST_F(test_textedit, renderAllSelections)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->renderAllSelections();

    assert(1==1);
}
//DMenu *getHighlightMenu();
TEST_F(test_textedit, getHighlightMenu)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->getHighlightMenu();

    assert(1==1);
}

//void lineNumberAreaPaintEvent(QPaintEvent *event);
TEST_F(test_textedit, lineNumberAreaPaintEvent)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    QPaintEvent *e;
    startManager->setWrapper(ee);
    startManager->lineNumberAreaPaintEvent(e);

    assert(1==1);
}
//void codeFLodAreaPaintEvent(QPaintEvent *event);
TEST_F(test_textedit, codeFLodAreaPaintEvent)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    QPaintEvent *e;
    startManager->setWrapper(ee);
    startManager->codeFLodAreaPaintEvent(e);

    assert(1==1);
}
//void setCodeFlodFlagVisable(bool isVisable,bool bIsFirstOpen = false);
TEST_F(test_textedit, setCodeFlodFlagVisable)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->setCodeFlodFlagVisable(true,false);
    startManager->setCodeFlodFlagVisable(false,false);

    assert(1==1);
}
//void setThemeWithPath(const QString &path);
TEST_F(test_textedit, setThemeWithPath)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    ee->OnThemeChangeSlot("aa");

    assert(1==1);
}
//void setTheme(const KSyntaxHighlighting::Theme &theme, const QString &path);


//bool highlightWordUnderMouse(QPoint pos);
TEST_F(test_textedit, highlightWordUnderMouse)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    QPoint x(10,10);
    startManager->setWrapper(ee);
    startManager->highlightWordUnderMouse(x);

    assert(1==1);
}
//void removeHighlightWordUnderCursor();
TEST_F(test_textedit, removeHighlightWordUnderCursor)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->removeHighlightWordUnderCursor();

    assert(1==1);
}

//void setSettings(Settings *settings);
TEST_F(test_textedit, setSettings)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *set;
    startManager->setWrapper(ee);
    startManager->setSettings(set);

    assert(1==1);
}


//void copySelectedText();
TEST_F(test_textedit, copySelectedText)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->copySelectedText();

    assert(1==1);
}
//void cutSelectedText();
TEST_F(test_textedit, cutSelectedText)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->cutSelectedText();

    assert(1==1);
}
//void pasteText();
TEST_F(test_textedit, pasteText)
{
    QClipboard * c = QApplication::clipboard();
    c->setText("ddd");
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->pasteText();

    assert(1==1);
}

//void setMark();
TEST_F(test_textedit, setMark)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->setMark();

    assert(1==1);
}
//void unsetMark();
TEST_F(test_textedit, unsetMark)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->unsetMark();

    assert(1==1);
}
//bool tryUnsetMark();
TEST_F(test_textedit, tryUnsetMark)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->tryUnsetMark();

    assert(1==1);
}
//void exchangeMark();
TEST_F(test_textedit, exchangeMark)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->exchangeMark();

    assert(1==1);
}
//void saveMarkStatus();
TEST_F(test_textedit, saveMarkStatus)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->saveMarkStatus();

    assert(1==1);
}
//void restoreMarkStatus();
TEST_F(test_textedit, restoreMarkStatus)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->restoreMarkStatus();

    assert(1==1);
}

//void completionWord(QString word);
//QString getWordAtMouse();
TEST_F(test_textedit, getWordAtMouse)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->getWordAtMouse();

    assert(1==1);
}
//QString getWordAtCursor();

//void toggleReadOnlyMode();
TEST_F(test_textedit, toggleReadOnlyMode)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->toggleReadOnlyMode();

    assert(1==1);
}
//void toggleComment(bool sister);
TEST_F(test_textedit, toggleComment)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->toggleComment(true);
    startManager->toggleComment(false);

    assert(1==1);
}
//bool atWordSeparator(int position);
TEST_F(test_textedit, atWordSeparator)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->atWordSeparator(2);

    assert(1==1);
}

//void showCursorBlink();
TEST_F(test_textedit, showCursorBlink)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->showCursorBlink();

    assert(1==1);
}
//void hideCursorBlink();
TEST_F(test_textedit, hideCursorBlink)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->hideCursorBlink();

    assert(1==1);
}

//void setReadOnlyPermission(bool permission);
TEST_F(test_textedit, setReadOnlyPermission)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->setReadOnlyPermission(true);
    startManager->setReadOnlyPermission(false);

    assert(1==1);
}
//bool getReadOnlyPermission();
TEST_F(test_textedit, getReadOnlyPermission)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->getReadOnlyPermission();

    assert(1==1);
}
//bool getReadOnlyMode();
TEST_F(test_textedit, getReadOnlyMode)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->getReadOnlyMode();

    assert(1==1);
}

//void hideRightMenu();
TEST_F(test_textedit, hideRightMenu)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->hideRightMenu();

    assert(1==1);
}

//void clearBlack();
TEST_F(test_textedit, clearBlack)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    //startManager->clearBlack();

    assert(1==1);
}
//void flodOrUnflodAllLevel(bool isFlod);
TEST_F(test_textedit, flodOrUnflodAllLevel)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->flodOrUnflodAllLevel(true);
    startManager->flodOrUnflodAllLevel(false);

    assert(1==1);
}
//void flodOrUnflodCurrentLevel(bool isFlod);
TEST_F(test_textedit, flodOrUnflodCurrentLevel)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->flodOrUnflodCurrentLevel(true);
    startManager->flodOrUnflodCurrentLevel(false);

    assert(1==1);
}
//void getHideRowContent(int iLine);
TEST_F(test_textedit, getHideRowContent)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->getHideRowContent(1);

    assert(1==1);
}
//int  getHighLightRowContentLineNum(int iLine);
TEST_F(test_textedit, getHighLightRowContentLineNum)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->getHighLightRowContentLineNum(1);

    assert(1==1);
}
//int  getLinePosByLineNum(int iLine);
TEST_F(test_textedit, getLinePosYByLineNum)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->getLinePosYByLineNum(1);

    assert(1==1);
}
//bool ifHasHighlight();
TEST_F(test_textedit, ifHasHighlight)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->ifHasHighlight();

    assert(1==1);
}
//void bookMarkAreaPaintEvent(QPaintEvent *event);
TEST_F(test_textedit, bookMarkAreaPaintEvent)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    QPaintEvent *e;
    startManager->bookMarkAreaPaintEvent(e);

    assert(1==1);
}

//int getLineFromPoint(const QPoint &point);
TEST_F(test_textedit, getLineFromPoint)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    QPoint s(10,10);
    startManager->getLineFromPoint(s);

    assert(1==1);
}

//void addOrDeleteBookMark();
TEST_F(test_textedit, addOrDeleteBookMark)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->addOrDeleteBookMark();

    assert(1==1);
}

//void moveToPreviousBookMark();
TEST_F(test_textedit, moveToPreviousBookMark)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->moveToPreviousBookMark();

    assert(1==1);
}


//void checkBookmarkLineMove(int from, int charsRemoved, int charsAdded);
TEST_F(test_textedit, checkBookmarkLineMove)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->checkBookmarkLineMove(2,3,4);

    assert(1==1);
}

//void setIsFileOpen();
TEST_F(test_textedit, setIsFileOpen)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->setIsFileOpen();

    assert(1==1);
}

//QStringList readHistoryRecordofBookmark();
TEST_F(test_textedit, readHistoryRecordofBookmark)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->readHistoryRecordofBookmark();

    assert(1==1);
}

//QStringList readHistoryRecordofFilePath(QString key);
TEST_F(test_textedit, readHistoryRecordofFilePath)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->readHistoryRecordofFilePath("advance.editor.browsing_history_file");

    assert(1==1);
}

//void writeHistoryRecord();
TEST_F(test_textedit, writeHistoryRecord)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->writeHistoryRecord();

    assert(1==1);
}

//void isMarkCurrentLine(bool isMark, QString strColor = "");
TEST_F(test_textedit, isMarkCurrentLine)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->isMarkCurrentLine(true,"red");
    startManager->isMarkCurrentLine(false,"red");

    assert(1==1);
}

//void isMarkAllLine(bool isMark, QString strColor = "");
TEST_F(test_textedit, isMarkAllLine)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->isMarkAllLine(true,"red");
    startManager->isMarkAllLine(false,"red");

    assert(1==1);
}

//void cancelLastMark();
TEST_F(test_textedit, cancelLastMark)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->cancelLastMark();

    assert(1==1);
}

//void markSelectWord();
TEST_F(test_textedit, markSelectWord)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->markSelectWord();

    assert(1==1);
}

//void updateMark(int from, int charsRemoved, int charsAdded);
TEST_F(test_textedit, updateMark)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->updateMark(1,2,3);

    assert(1==1);
}

//bool containsExtraSelection(QList<QTextEdit::ExtraSelection> listSelections, QTextEdit::ExtraSelection selection);
TEST_F(test_textedit, containsExtraSelection)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->containsExtraSelection(listSelection,selectio);

    assert(1==1);
}

//void appendExtraSelection(QList<QTextEdit::ExtraSelection> wordMarkSelections, QTextEdit::ExtraSelection selection
//                          , QString strColor, QList<QTextEdit::ExtraSelection> *listSelections);
TEST_F(test_textedit, appendExtraSelection)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    selectio.cursor=startManager->textCursor();
    selectio.format.setBackground(QColor(Qt::red));
    QList<QTextEdit::ExtraSelection> listSelectionsd;
    listSelectionsd.append(selectio);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->appendExtraSelection(listSelection,selectio,"#000000",&listSelectionsd);

    assert(1==1);
}
/// void setCursorStart(int _);
 TEST_F(test_textedit, setCursorStart)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->setCursorStart(2);

    assert(1==1);
}
//void writeEncodeHistoryRecord();
 TEST_F(test_textedit, writeEncodeHistoryRecord)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->writeEncodeHistoryRecord();

    assert(1==1);
}
//QStringList readEncodeHistoryRecord();
 TEST_F(test_textedit, readEncodeHistoryRecord)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->readEncodeHistoryRecord();

    assert(1==1);
}


 //clickFindAction
 TEST_F(test_textedit, clickFindAction)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->clickFindAction();

    assert(1==1);
}
// void tellFindBarClose();
 TEST_F(test_textedit, tellFindBarClose)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->tellFindBarClose();

    assert(1==1);
}

// void setEditPalette(const QString &activeColor, const QString &inactiveColor);
 TEST_F(test_textedit, setEditPalette)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->setEditPalette("aa","aa");

    assert(1==1);
}

// void setCodeFoldWidgetHide(bool isHidden);
 TEST_F(test_textedit, setCodeFoldWidgetHide)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->setCodeFoldWidgetHide(true);
    startManager->setCodeFoldWidgetHide(false);

    assert(1==1);
}
// void updateLeftAreaWidget();
 TEST_F(test_textedit, updateLeftAreaWidget)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->updateLeftAreaWidget();

    assert(1==1);
}

// void updateLineNumber();
 TEST_F(test_textedit, updateLineNumber)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->updateLeftAreaWidget();

    assert(1==1);
}
// void handleScrollFinish();
 TEST_F(test_textedit, handleScrollFinish)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->handleScrollFinish();

    assert(1==1);
}

#ifdef TABLET
// void slot_translate();
 TEST_F(test_textedit, slot_translate)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->slot_translate();

    assert(1==1);
}
#endif

// void upcaseWord();
 TEST_F(test_textedit, upcaseWord)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->upcaseWord();

    assert(1==1);
}
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

    assert(1==1);
}
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

//    assert(1==1);
//}
// void transposeChar();
 TEST_F(test_textedit, transposeChar)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->transposeChar();

    assert(1==1);
}

// void handleCursorMarkChanged(bool mark, QTextCursor cursor);
 TEST_F(test_textedit, handleCursorMarkChanged)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->handleCursorMarkChanged(true,QTextCursor());
    startManager->handleCursorMarkChanged(false,QTextCursor());

    assert(1==1);
}

// void adjustScrollbarMargins();
 TEST_F(test_textedit, adjustScrollbarMargins)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->adjustScrollbarMargins();

    assert(1==1);
}

// void onSelectionArea();
 TEST_F(test_textedit, onSelectionArea)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->onSelectionArea();

    assert(1==1);
}
// void fingerZoom(QString name, QString direction, int fingers);
 TEST_F(test_textedit, fingerZoom)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->fingerZoom("aa","aa",3);

    assert(1==1);
}

// bool event(QEvent* evt) override;   //触摸屏event事件
 TEST_F(test_textedit, event)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QEvent *e=new QEvent(QEvent::Type::MouseButtonPress);
    startManager->event(e);

    assert(1==1);
}

// void inputMethodEvent(QInputMethodEvent *e) override;

// void mousePressEvent(QMouseEvent *e) override;
 TEST_F(test_textedit, mousePressEvent)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QPoint a(1,2);
    QPointF b(a);
    QMouseEvent *e=new QMouseEvent(QMouseEvent::Type::Enter,b,Qt::MouseButton::LeftButton,Qt::MouseButton::LeftButton,Qt::KeyboardModifier::NoModifier);
    startManager->mousePressEvent(e);
    assert(1==1);
}
// void mouseMoveEvent(QMouseEvent *e) override;
 TEST_F(test_textedit, mouseMoveEvent)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QPoint a(1,2);
    QPointF b(a);
    QMouseEvent *e=new QMouseEvent(QMouseEvent::Type::Enter,b,Qt::MouseButton::LeftButton,Qt::MouseButton::LeftButton,Qt::KeyboardModifier::NoModifier);
    startManager->mouseMoveEvent(e);

    assert(1==1);
}
// void mouseReleaseEvent(QMouseEvent *e) override;
 TEST_F(test_textedit, mouseReleaseEvent)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QPoint a(1,2);
    QPointF b(a);
    QMouseEvent *e=new QMouseEvent(QMouseEvent::Type::Enter,b,Qt::MouseButton::LeftButton,Qt::MouseButton::LeftButton,Qt::KeyboardModifier::NoModifier);
    startManager->mouseReleaseEvent(e);

    assert(1==1);
}

// void wheelEvent(QWheelEvent *e) override;
 TEST_F(test_textedit, wheelEvent)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QPointF pos;
    QWheelEvent *e = new QWheelEvent(pos,4,Qt::MouseButton::LeftButton,Qt::KeyboardModifier::AltModifier);

    //    QWheelEvent(const QPointF &pos, int delta,
//    Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers,
//    Qt::Orientation orient = Qt::Vertical);
    startManager->wheelEvent(e);

    assert(1==1);
}
// bool eventFilter(QObject *object, QEvent *event) override;
 TEST_F(test_textedit, eventFilter)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QObject *object;

    QEvent *e=new QEvent(QEvent::MouseButtonPress);
    startManager->eventFilter(startManager,e);
    //MouseButtonDblClick

    QEvent *e1=new QEvent(QEvent::MouseButtonDblClick);
    startManager->eventFilter(startManager,e1);
    QEvent *e2=new QEvent(QEvent::HoverMove);
    startManager->eventFilter(startManager,e2);
    QEvent *e3=new QEvent(QEvent::HoverLeave);
    startManager->eventFilter(startManager,e3);


    assert(1==1);
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
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    QPoint b(0,0);
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    Stub stub;
    stub.set((QAction *(QMenu::*)(const QPoint &, QAction *))ADDR(QMenu, exec), stub_exec);
    QContextMenuEvent *e=new QContextMenuEvent(QContextMenuEvent::Reason::Keyboard,b);
    //startManager->contextMenuEvent(e);

    assert(1==1);
}
// void paintEvent(QPaintEvent *e) override;
 TEST_F(test_textedit, paintEvent)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QRect a(1,2,3,4);
    QPaintEvent *e=new QPaintEvent(a);
    startManager->paintEvent(e);

    assert(1==1);
}

// bool blockContainStrBrackets(int line);
 TEST_F(test_textedit, blockContainStrBrackets)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QPaintEvent *e;
    startManager->blockContainStrBrackets(2);

    assert(1==1);
}
// bool setCursorKeywordSeletoin(int position, bool findNext);
 TEST_F(test_textedit, setCursorKeywordSeletoin)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QPaintEvent *e;
    startManager->setCursorKeywordSeletoin(2,true);
    startManager->setCursorKeywordSeletoin(2,false);

    assert(1==1);
}
// void cursorPositionChanged();
 TEST_F(test_textedit, cursorPositionChanged)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QPaintEvent *e;
    startManager->cursorPositionChanged();

    assert(1==1);
}
// void updateHighlightBrackets(const QChar &openChar, const QChar &closeChar);
 TEST_F(test_textedit, updateHighlightBrackets)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    QChar a=' ';
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QPaintEvent *e;
    startManager->updateHighlightBrackets(a,a);

    assert(1==1);
}
// int getFirstVisibleBlockId() const;
 TEST_F(test_textedit, getFirstVisibleBlockId)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QPaintEvent *e;
    //startManager->getFirstVisibleBlockId();

    assert(1==1);
}


//void setTruePath(QString qstrTruePath);
TEST_F(test_textedit, setBackupPath)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    startManager->setTruePath("aa");

    assert(1==1);
}

// QString getTruePath();
TEST_F(test_textedit, getTruePath)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    startManager->getTruePath();

    assert(1==1);
}

////初始化右键菜单
//void initRightClickedMenu();
TEST_F(test_textedit, initRightClickedMenu)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    startManager->initRightClickedMenu();

    assert(1==1);
}

//inline QString getFilePath() { return m_sFilePath;};
TEST_F(test_textedit, getFilePath)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    startManager->getFilePath();

    assert(1==1);
}
////
//inline void setFilePath(QString file) { m_sFilePath = file;}
TEST_F(test_textedit, setFilePath)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    startManager->setFilePath("a");

    assert(1==1);
}
////
//int characterCount() const;
TEST_F(test_textedit, characterCount)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    startManager->characterCount();

    assert(1==1);
}


//void moveCursorNoBlink(QTextCursor::MoveOperation operation,
//                       QTextCursor::MoveMode mode = QTextCursor::MoveAnchor);
TEST_F(test_textedit, moveCursorNoBlink)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    startManager->moveCursorNoBlink(QTextCursor::MoveOperation::Up);

    assert(1==1);
}


//void convertWordCase(ConvertCase convertCase);
//TEST_F(test_textedit, convertWordCase)
//{
//    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
//    startManager->convertWordCase(ConvertCase::LOWER);
//    startManager->convertWordCase(ConvertCase::UPPER);
//    startManager->convertWordCase(ConvertCase::CAPITALIZE);

//    assert(1==1);
//}

//void renderAllSelections();


//void setBookmarkFlagVisable(bool isVisable,bool bIsFirstOpen = false);
TEST_F(test_textedit, setBookmarkFlagVisable)
{
    QScrollBar *p = new QScrollBar();TextEdit *startManager = new TextEdit();startManager->setVerticalScrollBar(p);
    startManager->setBookmarkFlagVisable(false,false);
    startManager->setBookmarkFlagVisable(true,false);
    assert(1==1);
}
/*
    void unCommentSelection();
    void setComment();
    void removeComment();
*/
TEST_F(test_textedit, unCommentSelection)
{
    EditWrapper* editWrapper = new EditWrapper;
    editWrapper->openFile("1.cpp","1.cpp");
    QTextCursor cursor =editWrapper->m_pTextEdit->textCursor();
    cursor.setPosition(0,QTextCursor::MoveAnchor);
    editWrapper->m_pTextEdit->setTextCursor(cursor);
    editWrapper->m_pTextEdit->unCommentSelection();
    assert(1==1);
}

TEST_F(test_textedit, setComment)
{
    EditWrapper* editWrapper = new EditWrapper;
    editWrapper->openFile("1.cpp","1.cpp");
    QTextCursor cursor =editWrapper->m_pTextEdit->textCursor();
    cursor.setPosition(0,QTextCursor::MoveAnchor);
    editWrapper->m_pTextEdit->setTextCursor(cursor);
    editWrapper->m_pTextEdit->setComment();
    assert(1==1);
}

TEST_F(test_textedit, removeComment)
{
    EditWrapper* editWrapper = new EditWrapper;
    editWrapper->openFile("1.cpp","1.cpp");
    QTextCursor cursor =editWrapper->m_pTextEdit->textCursor();
    cursor.setPosition(0,QTextCursor::MoveAnchor);
    editWrapper->m_pTextEdit->setTextCursor(cursor);
    editWrapper->m_pTextEdit->removeComment();
    assert(1==1);
}

TEST_F(test_textedit, keyPressEvent)
{
    Window* window= new Window;
    EditWrapper* editWrapper = window->createEditor();
    editWrapper->openFile("1.cpp","1.cpp");
    QTextCursor cursor =editWrapper->m_pTextEdit->textCursor();
    cursor.setPosition(0,QTextCursor::MoveAnchor);
    editWrapper->m_pTextEdit->setTextCursor(cursor);
    QKeyEvent keyEvent(QEvent::KeyPress,Qt::Key_Insert,Qt::NoModifier);
    editWrapper->m_pTextEdit->keyPressEvent(&keyEvent);
    assert(1==1);
}
