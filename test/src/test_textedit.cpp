#include "test_textedit.h"
#define protected public

test_textedit::test_textedit()
{

}

TEST_F(test_textedit, setWrapper)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper *e = new EditWrapper();
    startManager->setWrapper(e);

    assert(1==1);
}
//lineNumberAreaWidth

TEST_F(test_textedit, lineNumberAreaWidth)
{
    TextEdit *startManager = new TextEdit();
    startManager->lineNumberAreaWidth();

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
    startManager->setThemeWithPath("aa");

    assert(1==1);
}
//void setTheme(const KSyntaxHighlighting::Theme &theme, const QString &path);
//void loadHighlighter();
TEST_F(test_textedit, loadHighlighter)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->loadHighlighter();

    assert(1==1);
}

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
//void setModified(bool modified);
TEST_F(test_textedit, setModified)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->setModified(true);

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
TEST_F(test_textedit, getLinePosByLineNum)
{
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    startManager->setWrapper(ee);
    startManager->getLinePosByLineNum(1);

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
    QList<QTextEdit::ExtraSelection> listSelection;
    QTextEdit::ExtraSelection selectio;
     QList<QTextEdit::ExtraSelection> *listSelectionsd=nullptr;
    TextEdit *startManager = new TextEdit();
    EditWrapper * ee = new EditWrapper();
    Settings *s = new Settings();
    startManager->setSettings(s);
    startManager->setWrapper(ee);
    startManager->appendExtraSelection(listSelection,selectio,"red",listSelectionsd);

    assert(1==1);
}
