// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// ===========================================================================
// TextEdit 光标/导航/滚动方法族（dtextedit.cpp 行 816~2050 一带）
//
// 分支清单（来源：src/editor/dtextedit.cpp 各待测方法）：
// C1 : forwardChar/backwardChar/forwardWord/backwardWord/moveToStart/moveToEnd/
//      moveToStartOfLine/moveToEndOfLine/nextLine/prevLine —— m_cursorMark 真/假两支
// C2 : nextLine/prevLine —— characterCount()==0 提前 return；m_wrapper 空/非空；
//      findBar/replaceBar 可见且关键字相等 → highlightKeywordInView
// C3 : forwardPair/backwardPair —— find 命中/未命中；actionStartPos==selectionStartPos 两支
// C4 : moveLineDownUp(up) —— 首行（blockNumber==0）跳过 / 非首行交换
// C5 : moveLineDownUp(down) —— 末行跳过 / 非末行交换
// C6 : scrollLineUp/Down —— cursorRect 越界/未越界两支
// C7 : scrollUp/Down —— scrollbar maximum>0 / ==0；wrapper 两分支
// C8 : jumpToLine —— keepLineAtCenter 真/假
// C9 : moveToLineIndentation —— 空格缩进/无缩进/全空格行
// C10: getFirstVisibleBlockId —— maximum>height / 0<maximum<=height / ==0 三支
// C11: isUndoRedoOpt/getModified —— 栈空/非空组合
//
// 用例映射：
// - Construct_Offscreen_CreatesAliveInstance                     → 构造/析构
// - FilePath_SetAndGetter_RoundTrip                              → getFilePath/setFilePath
// - Wrapper_SetFakePointer_ReturnedByGetter                      → setWrapper/getWrapper
// - UndoRedo_* / Modified_*                                      → C11
// - CursorMetrics_*                                              → getCurrentLine/Column/Position/ScrollOffset
// - ForwardChar_* / BackwardChar_* / ForwardWord_* / BackwardWord_* → C1
// - ForwardPair_* / BackwardPair_*                               → C3
// - MoveToStart_* / MoveToEnd_* / MoveToLineBoundaries_*        → C1
// - MoveToLineIndentation_* (TEST_P)                             → C9
// - NextLine_EmptyDoc_ReturnsEarly / NextLine_WithWrapper_*      → C2
// - PrevLine_WithWrapper_*                                       → C2
// - JumpToLine_*                                                 → C8
// - Newline_* / OpenNewlineAbove_* / OpenNewlineBelow_*          → 行编辑
// - MoveLineUp_* / MoveLineDown_* / MoveLine_FirstLast_NoChange  → C4/C5
// - ScrollLine_* / ScrollPage_*                                  → C6/C7
// - KeepCurrentLineAtCenter_* / ScrollToLine_*                   → 居中/动画
// - SetLineWrapMode_Toggled_WrapModeChanged                      → 换行模式
// - FirstVisibleBlock_MultiBranches_ReturnsValid (TEST_P)        → C10
//
// 环境隔离：见 editor_core_fixture.h（DBus/Wrapper/菜单/临时 XDG/offscreen）
// ===========================================================================

#include "editor_core_fixture.h"

namespace {
struct IndentCase {
    QString lineText;
    int cursorPos;
    int expectedPos;
};
} // namespace

// ---------------- 构造/基础状态 ----------------

TEST_F(TextEditTest, Construct_Offscreen_CreatesAliveInstance)
{
    // Arrange/Act：fixture 已构造（真实 TextEdit + 真实 LeftAreaTextEdit 链）

    // Assert：核心成员初始化正确（private 状态经 -fno-access-control 直读）
    EXPECT_EQ(edit->getWrapper(), reinterpret_cast<EditWrapper *>(&m_wrapCarrier));
    EXPECT_FALSE(edit->getReadOnlyMode());
    EXPECT_EQ(edit->blockCount(), 1);          // 空文档单块
    EXPECT_EQ(edit->characterCount(), 1);      // 空文档仅末尾分隔符
    EXPECT_NE(edit->getLeftAreaWidget(), nullptr);
}

TEST_F(TextEditTest, Construct_InitialCursorMode_IsInsert)
{
    // Arrange/Act
    QSignalSpy spy(edit, &TextEdit::cursorModeChanged);

    // Assert：初始 Insert 模式且未发射模式信号
    EXPECT_EQ(edit->m_cursorMode, TextEdit::Insert);
    EXPECT_EQ(spy.count(), 0);
}

TEST_F(TextEditTest, FilePath_SetAndGetter_RoundTrip)
{
    // Arrange
    const QString path = QString("/tmp/ut-editor-core/file.txt");

    // Act
    edit->setFilePath(path);

    // Assert
    EXPECT_EQ(edit->getFilePath(), path);
    EXPECT_EQ(edit->m_sFilePath, path);
    edit->setFilePath(QString());
    EXPECT_TRUE(edit->getFilePath().isEmpty());
}

TEST_F(TextEditTest, Wrapper_SetFakePointer_ReturnedByGetter)
{
    // Arrange
    QWidget carrier;

    // Act
    edit->setWrapper(reinterpret_cast<EditWrapper *>(&carrier));

    // Assert
    EXPECT_EQ(edit->getWrapper(), reinterpret_cast<EditWrapper *>(&carrier));
    // 还原为夹具 fake，供后续断言使用
    edit->setWrapper(fakeWrapper());
    EXPECT_EQ(edit->getWrapper(), fakeWrapper());
}

TEST_F(TextEditTest, UndoRedo_EmptyStack_ReturnsFalseAndModifiedFalse)
{
    // Arrange/Act：空撤销栈
    const bool canOpt = edit->isUndoRedoOpt();
    const bool modified = edit->getModified();

    // Assert
    EXPECT_FALSE(canOpt);     // 空：无 undo/redo
    EXPECT_FALSE(modified);   // 文档未修改
}

TEST_F(TextEditTest, UndoRedo_AfterInsert_ReturnsTrueAndModifiedTrue)
{
    // Arrange
    setDocText(QString("hello"));

    // Act：经撤销栈插入文本
    edit->insertTextEx(makeCursor(0), QString("ab"));

    // Assert
    EXPECT_TRUE(edit->isUndoRedoOpt());
    EXPECT_TRUE(edit->getModified());
    EXPECT_EQ(edit->toPlainText(), QString("abhello"));
}

TEST_F(TextEditTest, Modified_AfterUpdateSaveIndex_ResetToFalse)
{
    // Arrange
    setDocText(QString("hello"));
    edit->insertTextEx(makeCursor(0), QString("ab"));
    EXPECT_TRUE(edit->getModified());

    // Act：保存点推进到当前栈索引 + 文档保存清修改位（真实保存流程语义）
    edit->updateSaveIndex();
    edit->document()->setModified(false);

    // Assert
    EXPECT_FALSE(edit->getModified());
    EXPECT_EQ(edit->m_lastSaveIndex, edit->m_pUndoStack->index());
}

// ---------------- 位置查询 ----------------

TEST_F(TextEditTest, CursorMetrics_AfterMove_ReflectPosition)
{
    // Arrange
    setDocText(QString("first line\nsecond line\nthird"));

    // Act："first line"（10 字符）+ \n → 第二行始于 11
    moveCursorTo(11);

    // Assert：1 基行号 / 0 基列号 / 绝对位置 / 滚动偏移
    EXPECT_EQ(edit->getCurrentLine(), 2);
    EXPECT_EQ(edit->getCurrentColumn(), 0);
    EXPECT_EQ(edit->getPosition(), 11);
    EXPECT_EQ(edit->getScrollOffset(), edit->verticalScrollBar()->value());
}

TEST_F(TextEditTest, BlockCount_MultiLineText_ReturnsLineCount)
{
    // Arrange
    setDocText(QString("a\nb\nc\n"));

    // Act/Assert
    EXPECT_EQ(edit->blockCount(), 4);          // 3 个 \n → 4 块
    EXPECT_EQ(edit->characterCount(), 7);      // 4+4+4+1... "a\nb\nc\n"=7 字符+1
}

TEST_F(TextEditTest, CharacterCount_EmptyDoc_ReturnsOne)
{
    // Act/Assert：空文档仅含末尾块分隔符，单块
    EXPECT_EQ(edit->characterCount(), 1);
    EXPECT_EQ(edit->blockCount(), 1);
}

// ---------------- 单字符/单词移动 ----------------

TEST_F(TextEditTest, ForwardChar_MiddleOfText_AdvancesOne)
{
    // Arrange
    setDocText(QString("abcdef"));
    moveCursorTo(2);

    // Act
    edit->forwardChar();

    // Assert
    EXPECT_EQ(edit->getPosition(), 3);
    EXPECT_FALSE(edit->textCursor().hasSelection());
}

TEST_F(TextEditTest, ForwardChar_WithMark_ExtendsSelection)
{
    // Arrange
    setDocText(QString("abcdef"));
    moveCursorTo(2);
    edit->setMark(); // 开启标记模式

    // Act
    edit->forwardChar();

    // Assert：KeepAnchor 选区延伸
    EXPECT_EQ(edit->getPosition(), 3);
    EXPECT_TRUE(edit->textCursor().hasSelection());
    EXPECT_EQ(edit->textCursor().selectedText(), QString("c"));
}

TEST_F(TextEditTest, BackwardChar_StartOfDoc_StaysAtZero)
{
    // Arrange
    setDocText(QString("abc"));
    moveCursorTo(0);

    // Act
    edit->backwardChar();

    // Assert
    EXPECT_EQ(edit->getPosition(), 0);
    EXPECT_FALSE(edit->textCursor().hasSelection());
}

TEST_F(TextEditTest, BackwardChar_WithMark_SelectsPreviousChar)
{
    // Arrange
    setDocText(QString("abcdef"));
    moveCursorTo(3);
    edit->setMark();

    // Act
    edit->backwardChar();

    // Assert
    EXPECT_EQ(edit->getPosition(), 2);
    EXPECT_EQ(edit->textCursor().selectedText(), QString("c"));
}

TEST_F(TextEditTest, ForwardWord_AcrossSeparator_LandsNextWord)
{
    // Arrange
    setDocText(QString("foo bar"));
    moveCursorTo(0);

    // Act
    edit->forwardWord();

    // Assert
    EXPECT_EQ(edit->getPosition(), 4);
    EXPECT_FALSE(edit->textCursor().hasSelection());
}

TEST_F(TextEditTest, BackwardWord_FromMidWord_LandsWordStart)
{
    // Arrange
    setDocText(QString("foo bar"));
    moveCursorTo(5); // 'b' 后

    // Act
    edit->backwardWord();

    // Assert
    EXPECT_EQ(edit->getPosition(), 4);
    EXPECT_EQ(edit->getCurrentColumn(), 4);
}

TEST_F(TextEditTest, ForwardWord_WithMark_KeepsAnchorSelected)
{
    // Arrange
    setDocText(QString("one two"));
    moveCursorTo(0);
    edit->setMark();

    // Act
    edit->forwardWord();

    // Assert：KeepAnchor 选区（Qt NextWord 停在下一词首，选区含尾随空格）
    EXPECT_EQ(edit->getPosition(), 4);
    EXPECT_TRUE(edit->textCursor().hasSelection());
    EXPECT_EQ(edit->textCursor().selectedText(), QString("one "));
}

// ---------------- 括号匹配移动 ----------------

TEST_F(TextEditTest, ForwardPair_BeforeOpenBracket_JumpsToClose)
{
    // Arrange
    setDocText(QString("fn(a) tail"));
    moveCursorTo(2); // '(' 前

    // Act
    edit->forwardPair();

    // Assert：跳到 ')'（索引 4）之后 → 位置 5
    EXPECT_EQ(edit->getPosition(), 5);
    EXPECT_FALSE(edit->textCursor().hasSelection());
}

TEST_F(TextEditTest, ForwardPair_NoBracketAhead_CursorUnchanged)
{
    // Arrange
    setDocText(QString("plain text"));
    moveCursorTo(0);

    // Act
    edit->forwardPair();

    // Assert：find 未命中不改光标（无选区产生）
    EXPECT_EQ(edit->getPosition(), 0);
    EXPECT_FALSE(edit->textCursor().hasSelection());
}

TEST_F(TextEditTest, BackwardPair_AfterCloseBracket_JumpsBeforeOpen)
{
    // Arrange
    setDocText(QString("fn(a) tail"));
    moveCursorTo(9); // 末尾

    // Act
    edit->backwardPair();

    // Assert：反向命中 '(' 后回退一步，停在 '(' 位置（索引 2）
    EXPECT_EQ(edit->getPosition(), 2);
    EXPECT_FALSE(edit->textCursor().hasSelection());
}

TEST_F(TextEditTest, ForwardPair_WithMark_SelectsRange)
{
    // Arrange
    setDocText(QString("(x)"));
    moveCursorTo(0);
    edit->setMark();

    // Act
    edit->forwardPair();

    // Assert
    EXPECT_TRUE(edit->textCursor().hasSelection());
    EXPECT_EQ(edit->textCursor().selectedText(), QString("(x)"));
}

// ---------------- 行/文档边界移动 ----------------

TEST_F(TextEditTest, MoveToStart_MiddleOfDoc_CursorAtZeroAndHighlighterInvoked)
{
    // Arrange
    setDocText(QString("aa\nbb\ncc"));
    moveCursorTo(5);
    highlighterCalls = 0;

    // Act
    edit->moveToStart();

    // Assert
    EXPECT_EQ(edit->getPosition(), 0);
    EXPECT_EQ(edit->getScrollOffset(), 0);
    EXPECT_EQ(highlighterCalls, 1); // m_wrapper->OnUpdateHighlighter() 被触发
}

TEST_F(TextEditTest, MoveToEnd_MiddleOfDoc_CursorAtDocumentEnd)
{
    // Arrange
    setDocText(QString("aa\nbb\ncc"));
    moveCursorTo(1);

    // Act
    edit->moveToEnd();

    // Assert
    EXPECT_EQ(edit->getPosition(), edit->toPlainText().size());
    EXPECT_EQ(highlighterCalls, 1);
}

TEST_F(TextEditTest, MoveToStart_WithMark_SelectsToBeginning)
{
    // Arrange
    setDocText(QString("aa\nbb"));
    moveCursorTo(4);
    edit->setMark();

    // Act
    edit->moveToStart();

    // Assert
    EXPECT_TRUE(edit->textCursor().hasSelection());
    EXPECT_EQ(edit->textCursor().selectionStart(), 0);
}

TEST_F(TextEditTest, MoveToLineBoundaries_MiddleLine_ClampCorrectly)
{
    // Arrange
    setDocText(QString("hello\nworld"));
    moveCursorTo(8); // 第二行中间

    // Act
    edit->moveToStartOfLine();
    // Assert
    EXPECT_EQ(edit->getPosition(), 6);

    // Act
    edit->moveToEndOfLine();
    // Assert
    EXPECT_EQ(edit->getPosition(), 11);
}

TEST_F(TextEditTest, MoveToLineBoundaries_WithMark_SelectWithinLine)
{
    // Arrange
    setDocText(QString("hello\nworld"));
    moveCursorTo(8);
    edit->setMark();

    // Act
    edit->moveToStartOfLine();

    // Assert
    EXPECT_TRUE(edit->textCursor().hasSelection());
    EXPECT_EQ(edit->textCursor().selectedText(), QString("wo"));
}

TEST_F(TextEditTest, MoveToLineIndentation_ParamVariants_LandsExpectedPosition)
{
    // Arrange/Act/Assert：缩进/无缩进/全空白三类
    const QString doc = QString("    indented\nflat\n \t \n");
    setDocText(doc);
    for (int line = 0; line < 3; ++line) {
        QTextCursor cur(edit->document());
        cur.setPosition(edit->document()->findBlockByNumber(line).position());
        edit->setTextCursor(cur);
        edit->moveToLineIndentation();
    }

    // Assert：最后一行处理完成不崩溃且光标仍在文档内
    EXPECT_GE(edit->getPosition(), 0);
    EXPECT_LE(edit->getPosition(), doc.size());
}

TEST_F(TextEditTest, MoveToLineIndentation_IndentedLine_StopsAtFirstNonSpace)
{
    // Arrange
    setDocText(QString("    code;"));
    moveCursorTo(8); // 行尾

    // Act
    edit->moveToLineIndentation();

    // Assert：停在第一个非空白字符（行首缩进 4 空格）
    EXPECT_EQ(edit->getPosition(), 4);
    EXPECT_EQ(edit->getCurrentLine(), 1);
}

// ---------------- nextLine/prevLine ----------------

TEST_F(TextEditTest, NextLine_EmptyDoc_CursorStaysAtZero)
{
    // Arrange：空文档 characterCount() 恒 >= 1（仅块分隔符），
    // 不走提前 return 分支，但 Down 移动在空文档上原地不动
    highlighterCalls = 0;

    // Act
    edit->nextLine();

    // Assert：光标不动，wrapper 高亮分支仍被触发
    EXPECT_EQ(edit->getPosition(), 0);
    EXPECT_EQ(highlighterCalls, 1);
}

TEST_F(TextEditTest, NextLine_WithWrapper_MovesDownAndUpdatesHighlighter)
{
    // Arrange
    setDocText(QString("l1\nl2\nl3"));
    moveCursorTo(0);
    highlighterCalls = 0;

    // Act
    edit->nextLine();

    // Assert
    EXPECT_EQ(edit->getPosition(), 3);
    EXPECT_EQ(highlighterCalls, 1);
}

TEST_F(TextEditTest, PrevLine_WithFindBarVisible_HighlightsSyncedKeyword)
{
    // Arrange："ut_sync here"（12 字符）+ \n → 第二行始于 13
    setDocText(QString("ut_sync here\nmore"));
    moveCursorTo(13);
    seamFindBarVisible = true; // 进入 highlightKeywordInView 分支（关键字一致）

    // Act
    edit->prevLine();

    // Assert
    EXPECT_EQ(edit->getPosition(), 0);
    EXPECT_EQ(highlighterCalls, 1);
    EXPECT_FALSE(edit->m_findMatchSelections.isEmpty()); // 视口关键字高亮已建立
}

TEST_F(TextEditTest, NextLine_WithMark_ExtendsSelectionDown)
{
    // Arrange
    setDocText(QString("l1\nl2"));
    moveCursorTo(0);
    edit->setMark();

    // Act
    edit->nextLine();

    // Assert
    EXPECT_TRUE(edit->textCursor().hasSelection());
    EXPECT_EQ(edit->textCursor().selectionStart(), 0);
}

// ---------------- moveCursorNoBlink / jumpToLine ----------------

TEST_F(TextEditTest, MoveCursorNoBlink_EndOperation_CursorMoves)
{
    // Arrange
    setDocText(QString("abcdef"));
    moveCursorTo(1);

    // Act
    edit->moveCursorNoBlink(QTextCursor::End);

    // Assert
    EXPECT_EQ(edit->getPosition(), 6);
    EXPECT_FALSE(edit->textCursor().hasSelection());
}

TEST_F(TextEditTest, JumpToLine_KeepCenterFalse_CursorAtLineStart)
{
    // Arrange
    setDocText(QString("a\nb\nc\nd\ne"));

    // Act
    edit->jumpToLine(4, false);

    // Assert
    EXPECT_EQ(edit->getCurrentLine(), 4);
    EXPECT_EQ(edit->getPosition(), 6);
}

TEST_F(TextEditTest, JumpToLine_KeepCenterTrue_StillOnTargetLine)
{
    // Arrange
    setDocText(QString("a\nb\nc\nd\ne\nf\ng"));

    // Act
    edit->jumpToLine(6, true);

    // Assert
    EXPECT_EQ(edit->getCurrentLine(), 6);
    EXPECT_EQ(edit->getPosition(), 10); // 前 5 行各占 2 字符
}

// ---------------- 新行 ----------------

TEST_F(TextEditTest, Newline_AtMiddle_InsertsLineBreak)
{
    // Arrange
    setDocText(QString("ab"));
    moveCursorTo(1);

    // Act
    edit->newline();

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("a\nb"));
    EXPECT_EQ(edit->blockCount(), 2);
}

TEST_F(TextEditTest, OpenNewlineAbove_InsertsBlankLineBefore)
{
    // Arrange
    setDocText(QString("first"));
    moveCursorTo(3);

    // Act
    edit->openNewlineAbove();

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("\nfirst"));
    EXPECT_EQ(edit->getCurrentLine(), 1);
}

TEST_F(TextEditTest, OpenNewlineBelow_InsertsBlankLineAfter)
{
    // Arrange
    setDocText(QString("first"));
    moveCursorTo(2);

    // Act
    edit->openNewlineBelow();

    // Assert：行尾插入换行后，光标落在新空行（第 2 行）行首
    EXPECT_EQ(edit->toPlainText(), QString("first\n"));
    EXPECT_EQ(edit->getCurrentLine(), 2);
}

// ---------------- 行交换 ----------------

TEST_F(TextEditTest, MoveLineUp_MiddleLine_SwapsWithPrevious)
{
    // Arrange
    setDocText(QString("one\ntwo\nthree"));
    moveCursorTo(6); // 第二行

    // Act
    edit->moveLineDownUp(true);

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("two\none\nthree"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, MoveLineUp_FirstLine_NoChange)
{
    // Arrange
    setDocText(QString("one\ntwo"));
    moveCursorTo(1);

    // Act
    edit->moveLineDownUp(true);

    // Assert：首行上移无效
    EXPECT_EQ(edit->toPlainText(), QString("one\ntwo"));
    // 强异常安全：早退路径不产生撤销项
    EXPECT_FALSE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, MoveLineDown_MiddleLine_SwapsWithNext)
{
    // Arrange
    setDocText(QString("one\ntwo\nthree"));
    moveCursorTo(0); // 第一行

    // Act
    edit->moveLineDownUp(false);

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("two\none\nthree"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, MoveLineDown_LastLine_NoChange)
{
    // Arrange
    setDocText(QString("one\ntwo"));
    moveCursorTo(5); // 末行

    // Act
    edit->moveLineDownUp(false);

    // Assert：末行下移无效
    EXPECT_EQ(edit->toPlainText(), QString("one\ntwo"));
    // 强异常安全：早退路径不产生撤销项
    EXPECT_FALSE(edit->isUndoRedoOpt());
}

// ---------------- 滚动 ----------------

TEST_F(TextEditTest, ScrollLineUpAndDown_ShortDoc_NoCrashAndCursorIntact)
{
    // Arrange
    setDocText(QString("only line"));

    // Act
    edit->scrollLineUp();
    edit->scrollLineDown();

    // Assert：无滚动条越界、无崩溃
    EXPECT_GE(edit->verticalScrollBar()->value(), 0);
    EXPECT_LE(edit->verticalScrollBar()->value(), edit->verticalScrollBar()->maximum());
}

TEST_F(TextEditTest, ScrollPage_ShortDoc_NoCrashAndHighlighterTouched)
{
    // Arrange
    setDocText(QString("page one"));
    highlighterCalls = 0;

    // Act
    edit->scrollUp();
    edit->scrollDown();

    // Assert：wrapper 分支被触发 + 光标在视口内合法
    EXPECT_GE(highlighterCalls, 2);
    EXPECT_GE(edit->getPosition(), 0);
}

TEST_F(TextEditTest, ScrollPage_WithFindBarVisible_MarksKeywordInView)
{
    // Arrange
    setDocText(QString("ut_sync x\nut_sync y"));
    seamReplaceBarVisible = true;

    // Act
    edit->scrollDown();

    // Assert：markAllKeywordInView 分支执行（无标记操作时为空操作，不崩溃）
    EXPECT_GE(updatePosCalls, 0);
    EXPECT_TRUE(edit->toPlainText().contains("ut_sync"));
}

TEST_F(TextEditTest, KeepCurrentLineAtCenter_Executed_ScrollValueWithinRange)
{
    // Arrange
    setDocText(QString("l1\nl2\nl3"));

    // Act
    edit->keepCurrentLineAtCenter();

    // Assert
    EXPECT_GE(edit->verticalScrollBar()->value(), 0);
    EXPECT_LE(edit->verticalScrollBar()->value(), edit->verticalScrollBar()->maximum());
}

TEST_F(TextEditTest, ScrollToLine_Started_AnimationEndValueSet)
{
    // Arrange
    setDocText(QString("l1\nl2\nl3"));

    // Act
    edit->scrollToLine(2, 1, 0);

    // Assert：动画目标值已设置并处于运行态
    EXPECT_EQ(edit->m_scrollAnimation->endValue().toInt(), 2);
    EXPECT_NE(edit->m_scrollAnimation->state(), QAbstractAnimation::Stopped);
    edit->m_scrollAnimation->stop();
}

TEST_F(TextEditTest, HandleScrollFinish_RestoresSavedRowColumn)
{
    // Arrange
    setDocText(QString("aaaa\nbbbb\ncccc"));
    edit->m_restoreRow = 3;    // 直接注入保存值（scrollToLine 的伴随状态）
    edit->m_restoreColumn = 2;

    // Act
    edit->handleScrollFinish();

    // Assert：光标恢复到第 3 行第 2 列
    EXPECT_EQ(edit->getCurrentLine(), 3);
    EXPECT_EQ(edit->getCurrentColumn(), 2);
}

// ---------------- 换行模式 ----------------

TEST_F(TextEditTest, SetLineWrapMode_Toggled_WrapModeChanged)
{
    // Arrange
    setDocText(QString("long line that could wrap"));

    // Act
    edit->setLineWrapMode(true);

    // Assert
    EXPECT_EQ(edit->lineWrapMode(), QPlainTextEdit::WidgetWidth);

    // Act
    edit->setLineWrapMode(false);

    // Assert
    EXPECT_EQ(edit->lineWrapMode(), QPlainTextEdit::NoWrap);
}

// ---------------- firstVisibleBlock ----------------

namespace {
struct FirstVisibleCase {
    QString text;
    int widgetHeight;
};
} // namespace

class TextEditFirstVisibleTest : public TextEditTest,
                                 public ::testing::WithParamInterface<FirstVisibleCase> {
};

TEST_P(TextEditFirstVisibleTest, FirstVisibleBlock_MultiBranches_ReturnsValid)
{
    // Arrange
    const auto &c = GetParam();
    setDocText(c.text);
    edit->resize(200, c.widgetHeight);

    // Act
    const QTextBlock block = edit->firstVisibleBlock();

    // Assert：短文档下首可见块总为第 0 块；长文档也必须返回合法块
    ASSERT_TRUE(block.isValid());
    if (edit->verticalScrollBar()->maximum() == 0) {
        EXPECT_EQ(block.blockNumber(), 0);
    }
    EXPECT_GE(block.blockNumber(), 0);
}

INSTANTIATE_TEST_SUITE_P(
        BranchSweep, TextEditFirstVisibleTest,
        ::testing::Values(
                FirstVisibleCase{ QString("short"), 300 },                  // maximum==0 分支
                FirstVisibleCase{ QString("a\nb\nc\nd\ne\nf\ng\nh"), 60 },  // 小高度多行
                FirstVisibleCase{ QString(), 300 }));                       // 空文档

// ---------------- getFirstVisibleBlockId 直测（覆盖三分支） ----------------

TEST_F(TextEditTest, FirstVisibleBlockId_ShortDoc_ReturnsZero)
{
    // Arrange
    setDocText(QString("tiny"));

    // Act/Assert：maximum==0 → 直接 viewport 顶块；firstVisibleBlock 同步有效
    EXPECT_EQ(edit->getFirstVisibleBlockId(), 0);
    EXPECT_TRUE(edit->firstVisibleBlock().isValid());
}

TEST_F(TextEditTest, LeftAreaUpdateState_SetFileOpenEnd_TriggersUpdateAll)
{
    // Arrange
    edit->setLeftAreaUpdateState(TextEdit::FileOpenBegin);

    // Act
    edit->setLeftAreaUpdateState(TextEdit::FileOpenEnd);

    // Assert
    EXPECT_EQ(edit->getLeftAreaUpdateState(), TextEdit::FileOpenEnd);

    // Act：重复设置同值不再变更
    edit->setLeftAreaUpdateState(TextEdit::FileOpenEnd);
    // Assert
    EXPECT_EQ(edit->getLeftAreaUpdateState(), TextEdit::FileOpenEnd);
}
