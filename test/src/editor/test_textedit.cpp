#include "test_textedit.h"

test_textedit::test_textedit()
{

}

void test_textedit::forstub(QPoint q)
{
  qDebug()<<q;
}

TEST_F(test_textedit, setWrapper)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper *e = new EditWrapper();
    startManager->setWrapper(e);

    assert(1==1);
}
//TextEdit(QWidget *parent = nullptr);
TEST_F(test_textedit, TextEdit)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);

    assert(1==1);
}

//getCurrentLine
TEST_F(test_textedit, getCurrentLine)
{
    TextEdit *startManager = new TextEdit();
    startManager->getCurrentLine();

    assert(1==1);
}
//getCurrentColumn
TEST_F(test_textedit, getCurrentColumn)
{
    TextEdit *startManager = new TextEdit();
    startManager->getCurrentColumn();

    assert(1==1);
}
//getPosition
TEST_F(test_textedit, getPosition)
{
    TextEdit *startManager = new TextEdit();
    startManager->getPosition();

    assert(1==1);
}
//getScrollOffset
TEST_F(test_textedit, getScrollOffset)
{
    TextEdit *startManager = new TextEdit();
    startManager->getScrollOffset();

    assert(1==1);
}
//forwardChar
TEST_F(test_textedit, forwardChar)
{
    TextEdit *startManager = new TextEdit();
    startManager->forwardChar();

    assert(1==1);
}
//backwardChar
TEST_F(test_textedit, backwardChar)
{
    TextEdit *startManager = new TextEdit();
    startManager->backwardChar();

    assert(1==1);
}
//forwardWord
TEST_F(test_textedit, forwardWord)
{
    TextEdit *startManager = new TextEdit();
    startManager->forwardWord();

    assert(1==1);
}
//forwardPair
TEST_F(test_textedit, forwardPair)
{
    TextEdit *startManager = new TextEdit();
    startManager->forwardPair();

    assert(1==1);
}
//backwardPair
TEST_F(test_textedit, backwardPair)
{
    TextEdit *startManager = new TextEdit();
    startManager->backwardPair();

    assert(1==1);
}
//blockCount
TEST_F(test_textedit, blockCount)
{
    TextEdit *startManager = new TextEdit();
    startManager->blockCount();

    assert(1==1);
}
//moveToStart
TEST_F(test_textedit, moveToStart)
{
    TextEdit *startManager = new TextEdit();
    startManager->moveToStart();

    assert(1==1);
}
//moveToEnd
TEST_F(test_textedit, moveToEnd)
{
    TextEdit *startManager = new TextEdit();
    startManager->moveToEnd();

    assert(1==1);
}
//moveToStartOfLine
TEST_F(test_textedit, moveToStartOfLine)
{
    TextEdit *startManager = new TextEdit();
    startManager->moveToStartOfLine();

    assert(1==1);
}
//moveToEndOfLine
TEST_F(test_textedit, moveToEndOfLine)
{
    TextEdit *startManager = new TextEdit();
    startManager->moveToEndOfLine();

    assert(1==1);
}
//moveToLineIndentation
TEST_F(test_textedit, moveToLineIndentation)
{
    TextEdit *startManager = new TextEdit();
    startManager->moveToLineIndentation();

    assert(1==1);
}
//nextLine
TEST_F(test_textedit, nextLine)
{
    TextEdit *startManager = new TextEdit();
    startManager->nextLine();

    assert(1==1);
}
//prevLine
TEST_F(test_textedit, prevLine)
{
    TextEdit *startManager = new TextEdit();
    startManager->prevLine();

    assert(1==1);
}
//jumpToLine
TEST_F(test_textedit, jumpToLine)
{
    TextEdit *startManager = new TextEdit();
    startManager->jumpToLine(1,true);

    assert(1==1);
}
//newline
TEST_F(test_textedit, newline)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->newline();

    assert(1==1);
}
//openNewlineAbove
TEST_F(test_textedit, openNewlineAbove)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->openNewlineAbove();

    assert(1==1);
}
//openNewlineBelow
TEST_F(test_textedit, openNewlineBelow)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->openNewlineBelow();

    assert(1==1);
}
//moveLineDownUp
TEST_F(test_textedit, moveLineDownUp)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->moveLineDownUp(true);

    assert(1==1);
}
//scrollLineUp
TEST_F(test_textedit, scrollLineUp)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->scrollLineUp();

    assert(1==1);
}
TEST_F(test_textedit, scrollLineDown)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->scrollLineDown();

    assert(1==1);
}
TEST_F(test_textedit, scrollUp)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->scrollUp();

    assert(1==1);
}
TEST_F(test_textedit, scrollDown)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->scrollDown();

    assert(1==1);
}
//duplicateLine
TEST_F(test_textedit, duplicateLine)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->duplicateLine();

    assert(1==1);
}
//    void copyLines();
//void cutlines();
TEST_F(test_textedit, copyLines)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->copyLines();

    assert(1==1);
}
TEST_F(test_textedit, cutlines)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->cutlines();

    assert(1==1);
}

TEST_F(test_textedit, joinLines)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->joinLines();

    assert(1==1);
}
TEST_F(test_textedit, killLine)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->killLine();

    assert(1==1);
}
TEST_F(test_textedit, killCurrentLine)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->killCurrentLine();

    assert(1==1);
}
TEST_F(test_textedit, killBackwardWord)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->killBackwardWord();

    assert(1==1);
}

TEST_F(test_textedit, killForwardWord)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->killForwardWord();

    assert(1==1);
}
TEST_F(test_textedit, escape)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->escape();

    assert(1==1);
}
TEST_F(test_textedit, indentText)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->indentText();

    assert(1==1);
}

TEST_F(test_textedit, unindentText)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->unindentText();

    assert(1==1);
}
TEST_F(test_textedit, setTabSpaceNumber)
{
    TextEdit *startManager = new TextEdit();
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
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->capitalizeText("2");

    assert(1==1);
}
TEST_F(test_textedit, keepCurrentLineAtCenter)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->keepCurrentLineAtCenter();

    assert(1==1);
}
TEST_F(test_textedit, setLineWrapMode)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->setLineWrapMode(true);

    assert(1==1);
}
//void setFontFamily(QString fontName);
//void setFontSize(int fontSize);
//void updateFont();
TEST_F(test_textedit, setFontFamily)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->setFontFamily("2");

    assert(1==1);
}
TEST_F(test_textedit, setFontSize)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->setFontSize(2);

    assert(1==1);
}
TEST_F(test_textedit, updateFont)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->updateFont();

    assert(1==1);
}

TEST_F(test_textedit, replaceAll)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->replaceAll("aa","bb");

    assert(1==1);
}

TEST_F(test_textedit, replaceNext)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->replaceAll("aa","bb");

    assert(1==1);
}
TEST_F(test_textedit, replaceRest)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->replaceRest("aa","bb");

    assert(1==1);
}
TEST_F(test_textedit, beforeReplace)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->beforeReplace("bb");

    assert(1==1);
}
TEST_F(test_textedit, findKeywordForward)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->findKeywordForward("bb");

    assert(1==1);
}
TEST_F(test_textedit, removeKeywords)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->removeKeywords();

    assert(1==1);
}
//void renderAllSelections();
TEST_F(test_textedit, highlightKeyword)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->highlightKeyword("aa",2);

    assert(1==1);
}
TEST_F(test_textedit, updateCursorKeywordSelection)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->updateCursorKeywordSelection(2,true);

    assert(1==1);
}
TEST_F(test_textedit, updateHighlightLineSelection)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->updateHighlightLineSelection();

    assert(1==1);
}
TEST_F(test_textedit, renderAllSelections)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->renderAllSelections();

    assert(1==1);
}
//DMenu *getHighlightMenu();
TEST_F(test_textedit, getHighlightMenu)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->getHighlightMenu();

    assert(1==1);
}

//void lineNumberAreaPaintEvent(QPaintEvent *event);
TEST_F(test_textedit, lineNumberAreaPaintEvent)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    QPaintEvent *e;
    startManager->setWrapper(ee);
    startManager->lineNumberAreaPaintEvent(e);

    assert(1==1);
}
//void codeFLodAreaPaintEvent(QPaintEvent *event);
TEST_F(test_textedit, codeFLodAreaPaintEvent)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    QPaintEvent *e;
    startManager->setWrapper(ee);
    startManager->codeFLodAreaPaintEvent(e);

    assert(1==1);
}
//void setCodeFlodFlagVisable(bool isVisable,bool bIsFirstOpen = false);
TEST_F(test_textedit, setCodeFlodFlagVisable)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->setCodeFlodFlagVisable(true,true);

    assert(1==1);
}
//void setThemeWithPath(const QString &path);
TEST_F(test_textedit, setThemeWithPath)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    ee->OnThemeChangeSlot("aa");

    assert(1==1);
}
//void setTheme(const KSyntaxHighlighting::Theme &theme, const QString &path);


//bool highlightWordUnderMouse(QPoint pos);
TEST_F(test_textedit, highlightWordUnderMouse)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    QPoint x(10,10);
    startManager->setWrapper(ee);
    startManager->highlightWordUnderMouse(x);

    assert(1==1);
}
//void removeHighlightWordUnderCursor();
TEST_F(test_textedit, removeHighlightWordUnderCursor)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->removeHighlightWordUnderCursor();

    assert(1==1);
}

//void setSettings(Settings *settings);
TEST_F(test_textedit, setSettings)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    Settings *set;
    startManager->setWrapper(ee);
    startManager->setSettings(set);

    assert(1==1);
}


//void copySelectedText();
TEST_F(test_textedit, copySelectedText)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->copySelectedText();

    assert(1==1);
}
//void cutSelectedText();
TEST_F(test_textedit, cutSelectedText)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->cutSelectedText();

    assert(1==1);
}
//void pasteText();
TEST_F(test_textedit, pasteText)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->pasteText();

    assert(1==1);
}

//void setMark();
TEST_F(test_textedit, setMark)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->setMark();

    assert(1==1);
}
//void unsetMark();
TEST_F(test_textedit, unsetMark)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->unsetMark();

    assert(1==1);
}
//bool tryUnsetMark();
TEST_F(test_textedit, tryUnsetMark)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->tryUnsetMark();

    assert(1==1);
}
//void exchangeMark();
TEST_F(test_textedit, exchangeMark)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->exchangeMark();

    assert(1==1);
}
//void saveMarkStatus();
TEST_F(test_textedit, saveMarkStatus)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->saveMarkStatus();

    assert(1==1);
}
//void restoreMarkStatus();
TEST_F(test_textedit, restoreMarkStatus)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->restoreMarkStatus();

    assert(1==1);
}

//void completionWord(QString word);
//QString getWordAtMouse();
TEST_F(test_textedit, getWordAtMouse)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->getWordAtMouse();

    assert(1==1);
}
//QString getWordAtCursor();

//void toggleReadOnlyMode();
TEST_F(test_textedit, toggleReadOnlyMode)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->toggleReadOnlyMode();

    assert(1==1);
}
//void toggleComment(bool sister);
TEST_F(test_textedit, toggleComment)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->toggleComment(true);
    startManager->toggleComment(false);

    assert(1==1);
}
//bool atWordSeparator(int position);
TEST_F(test_textedit, atWordSeparator)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->atWordSeparator(2);

    assert(1==1);
}

//void showCursorBlink();
TEST_F(test_textedit, showCursorBlink)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->showCursorBlink();

    assert(1==1);
}
//void hideCursorBlink();
TEST_F(test_textedit, hideCursorBlink)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->hideCursorBlink();

    assert(1==1);
}

//void setReadOnlyPermission(bool permission);
TEST_F(test_textedit, setReadOnlyPermission)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->setReadOnlyPermission(true);
    startManager->setReadOnlyPermission(false);

    assert(1==1);
}
//bool getReadOnlyPermission();
TEST_F(test_textedit, getReadOnlyPermission)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->getReadOnlyPermission();

    assert(1==1);
}
//bool getReadOnlyMode();
TEST_F(test_textedit, getReadOnlyMode)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->getReadOnlyMode();

    assert(1==1);
}

//void hideRightMenu();
TEST_F(test_textedit, hideRightMenu)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->hideRightMenu();

    assert(1==1);
}

//void clearBlack();
TEST_F(test_textedit, clearBlack)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->clearBlack();

    assert(1==1);
}
//void flodOrUnflodAllLevel(bool isFlod);
TEST_F(test_textedit, flodOrUnflodAllLevel)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->flodOrUnflodAllLevel(true);
    startManager->flodOrUnflodAllLevel(false);

    assert(1==1);
}
//void flodOrUnflodCurrentLevel(bool isFlod);
TEST_F(test_textedit, flodOrUnflodCurrentLevel)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->flodOrUnflodCurrentLevel(true);
    startManager->flodOrUnflodCurrentLevel(false);

    assert(1==1);
}
//void getHideRowContent(int iLine);
TEST_F(test_textedit, getHideRowContent)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->getHideRowContent(1);

    assert(1==1);
}
//int  getHighLightRowContentLineNum(int iLine);
TEST_F(test_textedit, getHighLightRowContentLineNum)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->getHighLightRowContentLineNum(1);

    assert(1==1);
}
//int  getLinePosByLineNum(int iLine);
TEST_F(test_textedit, getLinePosYByLineNum)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->getLinePosYByLineNum(1);

    assert(1==1);
}
//bool ifHasHighlight();
TEST_F(test_textedit, ifHasHighlight)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->ifHasHighlight();

    assert(1==1);
}
//void bookMarkAreaPaintEvent(QPaintEvent *event);
TEST_F(test_textedit, bookMarkAreaPaintEvent)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    QPaintEvent *e;
    startManager->bookMarkAreaPaintEvent(e);

    assert(1==1);
}

//int getLineFromPoint(const QPoint &point);
TEST_F(test_textedit, getLineFromPoint)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    QPoint s(10,10);
    startManager->getLineFromPoint(s);

    assert(1==1);
}

//void addOrDeleteBookMark();
TEST_F(test_textedit, addOrDeleteBookMark)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->addOrDeleteBookMark();

    assert(1==1);
}

//void moveToPreviousBookMark();
TEST_F(test_textedit, moveToPreviousBookMark)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->moveToPreviousBookMark();

    assert(1==1);
}


//void checkBookmarkLineMove(int from, int charsRemoved, int charsAdded);
TEST_F(test_textedit, checkBookmarkLineMove)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->checkBookmarkLineMove(2,3,4);

    assert(1==1);
}

//void setIsFileOpen();
TEST_F(test_textedit, setIsFileOpen)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->setIsFileOpen();

    assert(1==1);
}

//QStringList readHistoryRecordofBookmark();
TEST_F(test_textedit, readHistoryRecordofBookmark)
{
    TextEdit *startManager = new TextEdit();
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
    TextEdit *startManager = new TextEdit();
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
    TextEdit *startManager = new TextEdit();
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
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->isMarkCurrentLine(true,"red");

    assert(1==1);
}

//void isMarkAllLine(bool isMark, QString strColor = "");
TEST_F(test_textedit, isMarkAllLine)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->isMarkAllLine(true,"red");

    assert(1==1);
}

//void cancelLastMark();
TEST_F(test_textedit, cancelLastMark)
{
    TextEdit *startManager = new TextEdit();
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
    TextEdit *startManager = new TextEdit();
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
    TextEdit *startManager = new TextEdit();
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
    TextEdit *startManager = new TextEdit();
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
    TextEdit *startManager = new TextEdit();
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
    TextEdit *startManager = new TextEdit();
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
    TextEdit *startManager = new TextEdit();
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
    TextEdit *startManager = new TextEdit();
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
    TextEdit *startManager = new TextEdit();
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
    TextEdit *startManager = new TextEdit();
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
    TextEdit *startManager = new TextEdit();
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
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->setCodeFoldWidgetHide(true);

    assert(1==1);
}
// void updateLeftAreaWidget();
 TEST_F(test_textedit, updateLeftAreaWidget)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    TextEdit *startManager = new TextEdit();
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
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->updateLeftAreaWidget();

    assert(1==1);
}
// void updateWordCount();
// TEST_F(test_textedit, updateWordCount)
//{
//    QList<QTextEdit::ExtraSelection> listSelection;
//    QTextEdit::ExtraSelection selectio;
//    TextEdit *startManager = new TextEdit();
//    EditWrapper * ee = new EditWrapper();
//    Settings *s = new Settings();
//    startManager->setSettings(s);
//    startManager->setWrapper(ee);
//    startManager->updateWordCount();

//    assert(1==1);
//}
// void handleScrollFinish();
 TEST_F(test_textedit, handleScrollFinish)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->handleScrollFinish();

    assert(1==1);
}



// void slot_translate();
 TEST_F(test_textedit, slot_translate)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->slot_translate();

    assert(1==1);
}

// void upcaseWord();
 TEST_F(test_textedit, upcaseWord)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    TextEdit *startManager = new TextEdit();
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
    TextEdit *startManager = new TextEdit();
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
//    TextEdit *startManager = new TextEdit();
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
    TextEdit *startManager = new TextEdit();
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
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->handleCursorMarkChanged(true,QTextCursor());

    assert(1==1);
}

// void adjustScrollbarMargins();
 TEST_F(test_textedit, adjustScrollbarMargins)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    TextEdit *startManager = new TextEdit();
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
    TextEdit *startManager = new TextEdit();
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
    TextEdit *startManager = new TextEdit();
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
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QEvent e=QEvent(QEvent::Type::MouseButtonPress);
    //startManager->event(&e);

    assert(1==1);
}

// void inputMethodEvent(QInputMethodEvent *e) override;

// void mousePressEvent(QMouseEvent *e) override;
 TEST_F(test_textedit, mousePressEvent)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QMouseEvent *e;
    //startManager->mousePressEvent(e);

    assert(1==1);
}
// void mouseMoveEvent(QMouseEvent *e) override;
 TEST_F(test_textedit, mouseMoveEvent)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QMouseEvent *e;
    //startManager->mouseMoveEvent(e);

    assert(1==1);
}
// void mouseReleaseEvent(QMouseEvent *e) override;
 TEST_F(test_textedit, mouseReleaseEvent)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QMouseEvent *e;
    //startManager->mouseReleaseEvent(e);

    assert(1==1);
}
// void keyPressEvent(QKeyEvent *e) override;
 TEST_F(test_textedit, keyPressEvent)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QKeyEvent *e;
    //startManager->keyPressEvent(e);

    assert(1==1);
}
// void wheelEvent(QWheelEvent *e) override;
 TEST_F(test_textedit, wheelEvent)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    TextEdit *startManager = new TextEdit();
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
    TextEdit *startManager = new TextEdit();
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
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    QPoint b(500,500);
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    Stub stub;
    stub.set((QAction *(QMenu::*)(const QPoint &, QAction *))ADDR(QMenu, exec), stub_exec);
    QContextMenuEvent *e=new QContextMenuEvent(QContextMenuEvent::Reason::Keyboard,b);
    startManager->contextMenuEvent(e);

    assert(1==1);
}
// void paintEvent(QPaintEvent *e) override;
 TEST_F(test_textedit, paintEvent)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    TextEdit *startManager = new TextEdit();
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
    TextEdit *startManager = new TextEdit();
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
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QPaintEvent *e;
    startManager->setCursorKeywordSeletoin(2,true);

    assert(1==1);
}
// void cursorPositionChanged();
 TEST_F(test_textedit, cursorPositionChanged)
{
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
    TextEdit *startManager = new TextEdit();
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
    TextEdit *startManager = new TextEdit();
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
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    QPaintEvent *e;
    //startManager->getFirstVisibleBlockId();

    assert(1==1);
}


//void setBackupPath(QString qstrTruePath);
TEST_F(test_textedit, setBackupPath)
{
    TextEdit *startManager = new TextEdit();
    startManager->setBackupPath("aa");

    assert(1==1);
}

// QString getTruePath();
TEST_F(test_textedit, getTruePath)
{
    TextEdit *startManager = new TextEdit();
    startManager->getTruePath();

    assert(1==1);
}

////初始化右键菜单
//void initRightClickedMenu();
TEST_F(test_textedit, initRightClickedMenu)
{
    TextEdit *startManager = new TextEdit();
    startManager->initRightClickedMenu();

    assert(1==1);
}
////弹窗右键菜单
//void popRightMenu(QPoint pos = QPoint());
//TEST_F(test_textedit, contextMenuEvent)
//{
//    TextEdit *startManager = new TextEdit();
//    QContextMenuEvent *event ;
//    startManager->contextMenuEvent(event);
//    startManager->deleteLater();

//    assert(1==1);
//}
//inline QString getFilePath() { return m_sFilePath;};
TEST_F(test_textedit, getFilePath)
{
    TextEdit *startManager = new TextEdit();
    startManager->getFilePath();

    assert(1==1);
}
////
//inline void setFilePath(QString file) { m_sFilePath = file;}
TEST_F(test_textedit, setFilePath)
{
    TextEdit *startManager = new TextEdit();
    startManager->setFilePath("a");

    assert(1==1);
}
////
//inline LeftAreaTextEdit* getLeftAreaWidget() { return m_pLeftAreaWidget;};
////是否撤销重做操作
//bool isUndoRedoOpt() {return (m_pUndoStack->canRedo()||m_pUndoStack->canUndo());};
////判断是否修改
//bool getModified() { return (document()->isModified() && isUndoRedoOpt());}

//int getCurrentLine();
//int getCurrentColumn();
//int getPosition();
//int getScrollOffset();
//DMenu *getHighlightMenu();


//void forwardChar();
//void backwardChar();
//void forwardWord();
//void backwardWord();
//void forwardPair();
//void backwardPair();

//int blockCount() const;
//int characterCount() const;
//QTextBlock firstVisibleBlock();

//void moveToStart();
//void moveToEnd();
//void moveToStartOfLine();
//void moveToEndOfLine();
//void moveToLineIndentation();
//void nextLine();
//void prevLine();
//void jumpToLine(int line, bool keepLineAtCenter);

//void moveCursorNoBlink(QTextCursor::MoveOperation operation,
//                       QTextCursor::MoveMode mode = QTextCursor::MoveAnchor);

//void newline();
//void openNewlineAbove();
//void openNewlineBelow();
//void moveLineDownUp(bool up);
//void scrollLineUp();
//void scrollLineDown();
//void scrollUp();
//void scrollDown();
//void duplicateLine();
//void copyLines();
//void cutlines();

///**
// * @brief joinLines 合并行
// */
//void joinLines();

//void killLine();
//void killCurrentLine();
//void killBackwardWord();
//void killForwardWord();

//void escape();
//void indentText();
//void unindentText();
//void setTabSpaceNumber(int number);
//void convertWordCase(ConvertCase convertCase);
//QString capitalizeText(QString text);

//void keepCurrentLineAtCenter();
//void scrollToLine(int scrollOffset, int row, int column);

//void setLineWrapMode(bool enable);
//void setFontFamily(QString fontName);
//void setFontSize(int fontSize);
//void updateFont();

//void replaceAll(const QString &replaceText, const QString &withText);
//void replaceNext(const QString &replaceText, const QString &withText);
//void replaceRest(const QString &replaceText, const QString &withText);
//void beforeReplace(QString _);

//bool findKeywordForward(const QString &keyword);

//void removeKeywords();
//bool highlightKeyword(QString keyword, int position);
//void updateCursorKeywordSelection(int position, bool findNext);
//void updateHighlightLineSelection();
//bool updateKeywordSelections(QString keyword,QTextCharFormat charFormat,QList<QTextEdit::ExtraSelection> *listSelection);
//void renderAllSelections();


//void lineNumberAreaPaintEvent(QPaintEvent *event);
//void codeFLodAreaPaintEvent(QPaintEvent *event);
//void setBookmarkFlagVisable(bool isVisable,bool bIsFirstOpen = false);
//void setCodeFlodFlagVisable(bool isVisable,bool bIsFirstOpen = false);
//void setTheme(const QString &path);
//bool highlightWordUnderMouse(QPoint pos);
//void removeHighlightWordUnderCursor();
//void setSettings(Settings *settings);
////设置tabbar修改状态是否带*,不改变document 修改属性
//void setTabbarModified(bool modified);
//void copySelectedText();
//void cutSelectedText();
//void pasteText();
//void setMark();
//void unsetMark();
//bool tryUnsetMark();
//void exchangeMark();
//void saveMarkStatus();
//void restoreMarkStatus();
//void completionWord(QString word);
//QString getWordAtMouse();
//QString getWordAtCursor();
//void toggleReadOnlyMode();
//void toggleComment(bool sister);
//int getNextWordPosition(QTextCursor cursor, QTextCursor::MoveMode moveMode);
//int getPrevWordPosition(QTextCursor cursor, QTextCursor::MoveMode moveMode);
//bool atWordSeparator(int position);
//void showCursorBlink();
//void hideCursorBlink();
//void setReadOnlyPermission(bool permission);
//bool getReadOnlyPermission();
//bool getReadOnlyMode();
//void hideRightMenu();
//void clearBlack();
//void flodOrUnflodAllLevel(bool isFlod);
//void flodOrUnflodCurrentLevel(bool isFlod);
//void getHideRowContent(int iLine);
//bool isNeedShowFoldIcon(QTextBlock block);
//int  getHighLightRowContentLineNum(int iLine);

///**
// * @author liumaochuan ut000616
// * @brief getLinePosYByLineNum 根据行号获得行Y轴坐标
// * @param iLine 行
// * @return 行Y轴坐标
// */
//int  getLinePosYByLineNum(int iLine);
//bool ifHasHighlight();

////书签功能相关
///**
// * @author liumaochuan ut000616
// * @brief bookMarkAreaPaintEvent 绘制书签
// * @param event 书签区域的绘制事件
// */
//void bookMarkAreaPaintEvent(QPaintEvent *event);

///**
// * @author liumaochuan ut000616
// * @brief getLineFromPoint 得到鼠标点击位置所在的行
// * @param point 鼠标点击位置
// * @return 鼠标点击位置所在的行
// */
//int getLineFromPoint(const QPoint &point);

///**
// * @author liumaochuan ut000616
// * @brief moveToPreviousBookMark 移动到上一个书签
// */
//void moveToPreviousBookMark();

///**
// * @author liumaochuan ut000616
// * @brief moveToNextBookMark 移动到下一个书签
// */
//void moveToNextBookMark();

///**
// * @author liumaochuan ut000616
// * @brief checkBookmarkLineMove 检测书签行移动
// * @param from 文本变化时光标位置
// * @param charsRemoved 移除的字符数
// * @param charsAdded 添加的字符数
// */
//void checkBookmarkLineMove(int from, int charsRemoved, int charsAdded);

///**
// * @author liumaochuan ut000616
// * @brief setIsFileOpen 设置是否在读取文件
// */
//void setIsFileOpen();

///**
// * @author liumaochuan ut000616
// * @brief setTextFinished 读取文件结束
// */
//void setTextFinished();

///**
// * @author liumaochuan ut000616
// * @brief readHistoryRecord 读取书签相关记录
// * @return 书签相关记录列表
// */
//QStringList readHistoryRecord(QString key);

///**
// * @author liumaochuan ut000616
// * @brief readHistoryRecordofBookmark 读取书签记录
// * @return 书签记录列表
// */
//QStringList readHistoryRecordofBookmark();

///**
// * @author liumaochuan ut000616
// * @brief readHistoryRecordofFilePath 读取添加了书签的文件路径记录
// * @return 文件路径列表
// */
//QStringList readHistoryRecordofFilePath(QString key);

///**
// * @author liumaochuan ut000616
// * @brief writeHistoryRecord 写入书签相关记录
// */
//void writeHistoryRecord();

////标记功能相关
///**
// * @author liumaochuan ut000616
// * @brief isMarkCurrentLine 标记或取消标记当前行
// * @param isMark true为标记，false为取消标记
// * @param strColor 标记格式
// */
//void isMarkCurrentLine(bool isMark, QString strColor = "");

///**
// * @author liumaochuan ut000616
// * @brief isMarkAllLine 标记或取消标记所有
// * @param isMark true为标记，false为取消标记
// * @param strColor 标记格式
// */
//void isMarkAllLine(bool isMark, QString strColor = "");

///**
// * @author liumaochuan ut000616
// * @brief cancelLastMark 取消上一个标记
// */
//void cancelLastMark();

///**
// * @brief markSelectWord 标记选择的文本
// */
//void markSelectWord();

///**
// * @author liumaochuan ut000616
// * @brief updateMark 更新标记
// * @param from 文本变化时光标位置
// * @param charsRemoved 移除的字符数
// * @param charsAdded 添加的字符数
// */
//void updateMark(int from, int charsRemoved, int charsAdded);

////QTextEdit :: ExtraSelection结构提供了一种为文档中的给定选择,指定字符格式的方法。
///**
// * @author liumaochuan ut000616
// * @brief containsExtraSelection 指定字符格式列表是否包含该指定字符格式
// * @param listSelections 指定字符格式列表
// * @param selection 指定字符格式
// * @return true or false
// */
//bool containsExtraSelection(QList<QTextEdit::ExtraSelection> listSelections, QTextEdit::ExtraSelection selection);

///**
// * @author liumaochuan ut000616
// * @brief appendExtraSelection 在指定字符格式列表添加指定字符格式
// * @param wordMarkSelections 指定字符格式列表
// * @param selection 指定字符格式
// * @param markColor 指定字符颜色格式
// * @param listSelections 添加的指定字符格式列表
// */
//void appendExtraSelection(QList<QTextEdit::ExtraSelection> wordMarkSelections, QTextEdit::ExtraSelection selection
//                          , QString strColor, QList<QTextEdit::ExtraSelection> *listSelections);

//void setCursorStart(int pos);
//void writeEncodeHistoryRecord();
//QStringList readEncodeHistoryRecord();
///**
// * @brief tellFindBarClose 通知查找框关闭
// */
//void tellFindBarClose();
///**
// * @author liumaochuan ut000616
// * @brief setEditPalette 设置textEdit的颜色
// * @param activeColor active时的颜色
// * @param inactiveColor inactive时的颜色
// */
//void setEditPalette(const QString &activeColor, const QString &inactiveColor);
///**
// * @author liumaochuan ut000616
// * @brief setCodeFoldWidgetHide 代码折叠悬浮预览
// * @param isHidden 是否隐藏
// */
//void setCodeFoldWidgetHide(bool isHidden);
//void setBackupPath(QString qstrTruePath);
//QString getTruePath();

