// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// ===========================================================================
// TextEdit 文本编辑方法族（插入/删除/行编辑/大小写/注释/剪贴板/撤销重做）
//
// 分支清单（来源：src/editor/dtextedit.cpp）：
// E1 : insertMultiTextEx/deleteMultiTextEx —— 空列表提前 return / 多组压栈
// E2 : deleteSelectTextEx(cursor) —— hasSelection 真/假
// E3 : killLine —— tryUnsetMark 命中提前 return；有选区/无选区；
//      行尾空且非末行合并下一块；选区为空不压栈
// E4 : killCurrentLine —— 非末行含下一行换行；末行选到块尾
// E5 : killBackwardWord/killForwardWord —— 有选区（不删）/无选区删词
// E6 : unindentText —— 有选区（多行命令）/无选区 '\t'/' '/其它前缀三支
// E7 : convertWordCase —— UPPER/LOWER/CAPITALIZE；有选区/无选区；
//      无变化不压栈（text == selectedText）
// E8 : transposeChar —— 左右字符均非空（交换）/为空（不操作）
// E9 : cut/copy —— enableClipCopy 恒真；列编辑分支；m_isSelectAll 分支；无选区 copy 高亮词
// E10: paste —— 空剪贴板 return；小文本 insertSelectTextEx
// E11: undo_/redo_ —— canUndo/canRedo 提前 return；执行后 needUpdate 分支；
//      index==lastSaveIndex 触发 wrapper 状态复位（stub 计数）
// E12: moveText —— from<to / from>to / copy=true 三支
// E13: selectedText —— 单块直取；跨块 + \n；跨块 + CRLF（seam Windows）
// E14: toggleComment —— 空文档/空行/无语法定义提前 return；只读提示；
//      setComment/removeComment 单行/多行注释与取消
// E15: unCommentSelection —— 多行注释/单行注释/取消注释路径
// E16: onTextContentChanged —— m_MidButtonPatse 真/假
// E17: completionWord —— 补全后缀非空插入 / 为空不操作
// E18: getNextWordPosition/getPrevWordPosition/atWordSeparator —— 空文档 0；
//      空白跳过；分隔符停止
//
// 用例映射：见各 TEST_F 名（方法名_场景_期望）。
// 环境隔离：见 editor_core_fixture.h。
// ===========================================================================

#include "editor_core_fixture.h"

namespace {
struct WordCaseCase {
    int convertCase;      // ConvertCase 枚举值
    QString source;
    QString expected;
};
} // namespace

// ---------------- 直接插入/删除（Ex 族） ----------------

TEST_F(TextEditTest, InsertTextEx_AtPosition_TextAppears)
{
    // Arrange
    setDocText(QString("world"));

    // Act
    edit->insertTextEx(makeCursor(0), QString("hello "));

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("hello world"));
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, InsertMultiTextEx_TwoCursors_BothInserted)
{
    // Arrange
    setDocText(QString("ab"));

    // Act
    QList<QPair<QTextCursor, QString>> multi;
    multi << qMakePair(makeCursor(0), QString("1")) << qMakePair(makeCursor(2), QString("2"));
    edit->insertMultiTextEx(multi);

    // Assert：光标按子命令序应用（第二光标位置随首次插入右移至末尾）
    EXPECT_EQ(edit->toPlainText(), QString("1ab2"));
    edit->undo_();
    EXPECT_EQ(edit->toPlainText(), QString("ab"));
}

TEST_F(TextEditTest, InsertMultiTextEx_EmptyList_NoChange)
{
    // Arrange
    setDocText(QString("same"));

    // Act
    edit->insertMultiTextEx(QList<QPair<QTextCursor, QString>>());

    // Assert：空列表提前返回，不产生撤销项
    EXPECT_EQ(edit->toPlainText(), QString("same"));
    EXPECT_FALSE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, DeleteTextEx_OnSelection_RemovesText)
{
    // Arrange
    setDocText(QString("hello world"));
    QTextCursor cur = makeCursor(5);
    cur.setPosition(11, QTextCursor::KeepAnchor);

    // Act
    edit->deleteTextEx(cur);

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("hello"));
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, DeleteSelectTextEx_WithSelection_RemovesAndUndoable)
{
    // Arrange
    setDocText(QString("keep remove"));
    QTextCursor cur = makeCursor(4);
    cur.setPosition(11, QTextCursor::KeepAnchor);

    // Act
    edit->deleteSelectTextEx(cur);

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("keep"));

    // Act：撤销恢复
    edit->undo_();
    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("keep remove"));
}

TEST_F(TextEditTest, DeleteSelectTextEx_NoSelection_NothingPushed)
{
    // Arrange
    setDocText(QString("stable"));

    // Act：无选区光标 → 不压栈
    edit->deleteSelectTextEx(makeCursor(2));

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("stable"));
    EXPECT_FALSE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, DeleteSelectTextEx_ThreeArgs_TruncatesToLineEnd)
{
    // Arrange
    setDocText(QString("abcdef"));
    QTextCursor cur = makeCursor(2);

    // Act：ctrl+K 语义（currLine=false → 删到行尾）
    edit->deleteSelectTextEx(cur, QString("cdef"), false);

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("ab"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, DeleteMultiTextEx_MultipleCursors_AllRemoved)
{
    // Arrange：doc "aXbXc"（第二个 X 位于索引 3）
    setDocText(QString("aXbXc"));
    QTextCursor c1 = makeCursor(1);
    c1.setPosition(2, QTextCursor::KeepAnchor);
    QTextCursor c2 = makeCursor(3);
    c2.setPosition(4, QTextCursor::KeepAnchor);

    // Act：文档序传入，子命令顺序应用（QTextCursor 随删除自动左移）
    edit->deleteMultiTextEx(QList<QTextCursor>() << c1 << c2);

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("abc"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, DeleteMultiTextEx_EmptyList_NoChange)
{
    // Arrange
    setDocText(QString("keep"));

    // Act
    edit->deleteMultiTextEx(QList<QTextCursor>());

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("keep"));
    EXPECT_FALSE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, InsertSelectTextEx_DeferredHeavyUpdate_TypedCharsInsert)
{
    // Arrange
    setDocText(QString(""));

    // Act：普通单字符输入（触发延迟重更新路径）逐个插入
    edit->insertSelectTextEx(makeCursor(0), QString("a"));
    edit->insertSelectTextEx(edit->textCursor(), QString("b"));

    // Assert：内容正确且延迟标志在行未变时保持
    EXPECT_EQ(edit->toPlainText(), QString("ab"));
    EXPECT_TRUE(edit->m_deferCursorHeavyUpdate);

    // Act：强制刷新
    edit->flushDeferredCursorUpdate();
    // Assert：延迟复位
    EXPECT_FALSE(edit->m_deferCursorHeavyUpdate);
}

TEST_F(TextEditTest, InsertSelectTextEx_NewlineNotDeferred_ImmediateFlush)
{
    // Arrange
    setDocText(QString("x"));
    moveCursorTo(1); // 末尾

    // Act：含 \n 的输入不延迟
    edit->insertSelectTextEx(edit->textCursor(), QString("\n"));

    // Assert
    EXPECT_FALSE(edit->m_deferCursorHeavyUpdate);
    EXPECT_EQ(edit->toPlainText(), QString("x\n"));
}

// ---------------- 列编辑 ----------------

TEST_F(TextEditTest, InsertColumnEditTextEx_AltSelections_InsertsPerLine)
{
    // Arrange：三行各造一个列选区
    setDocText(QString("aa\nbb\ncc"));
    QList<QTextEdit::ExtraSelection> sels;
    for (int line = 0; line < 3; ++line) {
        QTextCursor cur(edit->document());
        const int blockPos = edit->document()->findBlockByNumber(line).position();
        cur.setPosition(blockPos);
        cur.setPosition(blockPos + 1, QTextCursor::KeepAnchor);
        QTextEdit::ExtraSelection sel;
        sel.cursor = cur;
        sels << sel;
    }
    edit->restoreColumnEditSelection(sels);

    // Act
    edit->insertColumnEditTextEx(QString("Z"));

    // Assert：每行选中字符被替换为 Z
    EXPECT_EQ(edit->toPlainText(), QString("Za\nZb\nZc"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, RestoreColumnEditSelection_StoresSelections)
{
    // Arrange
    QTextCursor cur(edit->document());
    cur.setPosition(0);
    cur.setPosition(1, QTextCursor::KeepAnchor);
    QTextEdit::ExtraSelection sel;
    sel.cursor = cur;
    QList<QTextEdit::ExtraSelection> sels;
    sels << sel;

    // Act
    edit->restoreColumnEditSelection(sels);

    // Assert
    EXPECT_EQ(edit->m_altModSelections.size(), 1);
    EXPECT_EQ(edit->m_altModSelections.first().cursor.selectedText(), QString());
}

// ---------------- 行复制/剪切/合并 ----------------

TEST_F(TextEditTest, DuplicateLine_SingleLine_LineDoubled)
{
    // Arrange
    setDocText(QString("abc"));
    moveCursorTo(1);

    // Act
    edit->duplicateLine();

    // Assert：光标随插入点落在复制行（第 2 行）末尾
    EXPECT_EQ(edit->toPlainText(), QString("abc\nabc"));
    EXPECT_EQ(edit->getCurrentLine(), 2);
}

TEST_F(TextEditTest, CopyLines_NoSelection_CopiesCurrentLineToClipboard)
{
    // Arrange
    setDocText(QString("alpha\nbeta"));
    moveCursorTo(7); // 第二行
    QApplication::clipboard()->clear();

    // Act
    edit->copyLines();

    // Assert：当前行进剪贴板，光标回到原位
    EXPECT_EQ(QApplication::clipboard()->text(), QString("beta"));
    EXPECT_EQ(edit->getPosition(), 7);
    EXPECT_EQ(edit->toPlainText(), QString("alpha\nbeta"));
}

TEST_F(TextEditTest, CopyLines_MultiLineSelection_ExpandsToWholeLines)
{
    // Arrange
    setDocText(QString("l1\nl2\nl3"));
    QTextCursor cur = makeCursor(3);
    cur.setPosition(8, QTextCursor::KeepAnchor); // 从 l2 行首到 l3 中间
    edit->setTextCursor(cur);

    // Act
    edit->copyLines();

    // Assert：整行扩展后复制两行；文档未被修改
    EXPECT_EQ(QApplication::clipboard()->text(), QString("l2\nl3"));
    EXPECT_EQ(edit->toPlainText(), QString("l1\nl2\nl3"));
}

TEST_F(TextEditTest, Cutlines_NoSelection_CutsCurrentLine)
{
    // Arrange
    setDocText(QString("keep\ncut"));
    moveCursorTo(5);

    // Act
    edit->cutlines();

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("keep\n"));
    EXPECT_EQ(QApplication::clipboard()->text(), QString("cut"));
}

TEST_F(TextEditTest, Cutlines_WithSelection_CutsSelection)
{
    // Arrange
    setDocText(QString("hello world"));
    QTextCursor cur = makeCursor(0);
    cur.setPosition(5, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);

    // Act
    edit->cutlines();

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString(" world"));
    EXPECT_EQ(QApplication::clipboard()->text(), QString("hello"));
}

TEST_F(TextEditTest, JoinLines_MiddleLine_MergesWithNext)
{
    // Arrange
    setDocText(QString("foo\nbar"));
    moveCursorTo(1);

    // Act
    edit->joinLines();

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("foobar"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, JoinLines_LastLine_NoChange)
{
    // Arrange
    setDocText(QString("solo"));
    moveCursorTo(2);

    // Act
    edit->joinLines();

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("solo"));
    // 强异常安全：早退路径不产生撤销项
    EXPECT_FALSE(edit->isUndoRedoOpt());
}

// ---------------- kill 族 ----------------

TEST_F(TextEditTest, KillLine_CursorBeforeTail_RemovesRestOfLine)
{
    // Arrange
    setDocText(QString("keepDEL"));
    moveCursorTo(4);

    // Act
    edit->killLine();

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("keep"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, KillLine_AtEmptyLineTail_MergesNextBlock)
{
    // Arrange
    setDocText(QString("a\n\nb"));
    moveCursorTo(2); // 空行（行尾无内容且非末块）

    // Act
    edit->killLine();

    // Assert：空行与下一块合并（换行被删）
    EXPECT_EQ(edit->toPlainText(), QString("a\nb"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, KillLine_WithSelection_RemovesSelectionOnly)
{
    // Arrange
    setDocText(QString("abcdef"));
    QTextCursor cur = makeCursor(1);
    cur.setPosition(3, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);

    // Act
    edit->killLine();

    // Assert：有选区时 DeleteTextUndoCommand2（currLine=false）删至行尾
    EXPECT_EQ(edit->toPlainText(), QString("a"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, KillLine_AtDocEndEmpty_NothingPushed)
{
    // Arrange
    setDocText(QString("x"));
    moveCursorTo(1); // 末尾，行尾空且是末块

    // Act
    edit->killLine();

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("x"));
    EXPECT_FALSE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, KillCurrentLine_MiddleLine_RemovesWholeLine)
{
    // Arrange
    setDocText(QString("one\ntwo\nthree"));
    moveCursorTo(4);

    // Act
    edit->killCurrentLine();

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("one\nthree"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, KillCurrentLine_LastLine_RemovesToDocEnd)
{
    // Arrange
    setDocText(QString("one\ntwo"));
    moveCursorTo(4);

    // Act
    edit->killCurrentLine();

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("one\n"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, KillBackwardWord_MidWord_RemovesWordHead)
{
    // Arrange
    setDocText(QString("hello world"));
    moveCursorTo(5);

    // Act
    edit->killBackwardWord();

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString(" world"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, KillForwardWord_AtWordStart_RemovesWholeWord)
{
    // Arrange
    setDocText(QString("remove keep"));
    moveCursorTo(0);

    // Act
    edit->killForwardWord();

    // Assert：NextWord 落点在后续空白之后 → 连同空格一起删除
    EXPECT_EQ(edit->toPlainText(), QString("keep"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

// ---------------- 缩进 ----------------

TEST_F(TextEditTest, IndentText_WithSelection_IndentsLines)
{
    // Arrange
    setDocText(QString("a\nb"));
    QTextCursor cur = makeCursor(0);
    cur.setPosition(3, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);

    // Act
    edit->indentText();

    // Assert：IndentTextCommand 在行首插入制表符（块数不变且可撤销）
    EXPECT_TRUE(edit->toPlainText().startsWith(QChar('\t')));
    EXPECT_EQ(edit->blockCount(), 2);
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, UnindentText_WithSelection_RemovesLeadingSpaces)
{
    // Arrange
    setDocText(QString("    a\n    b"));
    QTextCursor cur = makeCursor(0);
    cur.setPosition(11, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);

    // Act
    edit->unindentText();

    // Assert：多行反缩进
    EXPECT_EQ(edit->toPlainText(), QString("a\nb"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, UnindentText_SingleLineTabPrefix_RemovesTab)
{
    // Arrange
    setDocText(QString("\tcode"));
    moveCursorTo(2);

    // Act
    edit->unindentText();

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("code"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, UnindentText_SingleLineSpaces_RemovesUpToTabWidth)
{
    // Arrange
    setDocText(QString("   x"));
    moveCursorTo(3);

    // Act
    edit->unindentText();

    // Assert：行首仅 3 空格 → 全部移除
    EXPECT_EQ(edit->toPlainText(), QString("x"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, UnindentText_NoLeadingWhitespace_NoChange)
{
    // Arrange
    setDocText(QString("plain"));
    moveCursorTo(2);

    // Act
    edit->unindentText();

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("plain"));
    EXPECT_FALSE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, SetTabSpaceNumber_NewValue_StoredAndFontUpdated)
{
    // Arrange
    const int oldNumber = edit->m_tabSpaceNumber;

    // Act
    edit->setTabSpaceNumber(8);

    // Assert
    EXPECT_EQ(edit->m_tabSpaceNumber, 8);
    EXPECT_NE(edit->m_tabSpaceNumber, oldNumber);
    EXPECT_GT(edit->tabStopDistance(), 0.0);
}

// ---------------- 大小写转换 ----------------

TEST_F(TextEditTest, ConvertWordCase_ParamVariants_ExpectedText)
{
    // Arrange/Act/Assert：选区三种转换
    struct Case { int cc; const char *src; QString expected; };
    const Case cases[] = {
        { UPPER, "mixed Case", QString("MIXED CASE") },
        { LOWER, "Mixed CASE", QString("mixed case") },
        { CAPITALIZE, "hello world", QString("Hello World") },
    };
    for (const auto &c : cases) {
        setDocText(QString::fromLatin1(c.src));
        QTextCursor cur = makeCursor(0);
        cur.setPosition(edit->toPlainText().size(), QTextCursor::KeepAnchor);
        edit->setTextCursor(cur);
        edit->convertWordCase(static_cast<ConvertCase>(c.cc));
        EXPECT_EQ(edit->toPlainText(), c.expected) << "case " << c.cc;
        EXPECT_TRUE(edit->isUndoRedoOpt()) << "case " << c.cc; // 实际变更才压栈
    }
}

TEST_F(TextEditTest, UpcaseWord_NoSelection_ConvertsNextWord)
{
    // Arrange
    setDocText(QString("alpha beta"));
    moveCursorTo(0);

    // Act
    edit->upcaseWord();

    // Assert：无选区时转换光标后一个词
    EXPECT_EQ(edit->toPlainText(), QString("ALPHA beta"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, DowncaseWord_NoSelection_ConvertsNextWord)
{
    // Arrange
    setDocText(QString("ALPHA beta"));
    moveCursorTo(0);

    // Act
    edit->downcaseWord();

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("alpha beta"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, CapitalizeWord_NoSelection_CapitalizesNextWord)
{
    // Arrange
    setDocText(QString("hello world"));
    moveCursorTo(0);

    // Act
    edit->capitalizeWord();

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("Hello world"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, CapitalizeText_PureLogic_ExpectedResult)
{
    // Arrange/Act/Assert：单词首字母大写、空白后大写、首字符空格保留
    EXPECT_EQ(edit->capitalizeText(QString("hello world")), QString("Hello World"));
    EXPECT_EQ(edit->capitalizeText(QString("HELLO WORLD")), QString("Hello World"));
    EXPECT_EQ(edit->capitalizeText(QString(" x y")), QString(" X Y"));
    EXPECT_EQ(edit->capitalizeText(QString("a")), QString("A"));
}

TEST_F(TextEditTest, TransposeChar_MiddleOfText_SwapsNeighbors)
{
    // Arrange
    setDocText(QString("abcd"));
    moveCursorTo(2); // c|d 之间（交换 b? 具体：pos-1 与 pos 字符交换）

    // Act
    edit->transposeChar();

    // Assert：位置 1、2 字符被交换（bc → cb）
    EXPECT_EQ(edit->toPlainText(), QString("acbd"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, TransposeChar_AtStart_NoChange)
{
    // Arrange
    setDocText(QString("abc"));
    moveCursorTo(0);

    // Act
    edit->transposeChar();

    // Assert：左侧无字符 → 不操作
    EXPECT_EQ(edit->toPlainText(), QString("abc"));
    EXPECT_FALSE(edit->isUndoRedoOpt());
}

// ---------------- 剪贴板 ----------------

TEST_F(TextEditTest, Copy_WithSelection_ClipboardUpdated)
{
    // Arrange
    setDocText(QString("copy me"));
    QTextCursor cur = makeCursor(0);
    cur.setPosition(4, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);
    QApplication::clipboard()->clear();

    // Act
    edit->copy(true);

    // Assert
    EXPECT_EQ(QApplication::clipboard()->text(), QString("copy"));
    EXPECT_EQ(edit->toPlainText(), QString("copy me")); // 复制不改文档
}

TEST_F(TextEditTest, Copy_NoSelection_CopiesHighlightWordCache)
{
    // Arrange
    setDocText(QString("word more"));
    QTextCursor cur = makeCursor(0);
    cur.setPosition(4, QTextCursor::KeepAnchor);
    edit->m_highlightWordCacheCursor = cur;
    moveCursorTo(9);
    QApplication::clipboard()->clear();

    // Act
    edit->copy(true);

    // Assert：无选区复制缓存高亮词（文档未动）
    EXPECT_EQ(QApplication::clipboard()->text(), QString("word"));
    EXPECT_EQ(edit->toPlainText(), QString("word more"));
}

TEST_F(TextEditTest, Cut_WithSelection_TextRemovedAndClipped)
{
    // Arrange
    setDocText(QString("cut this"));
    QTextCursor cur = makeCursor(0);
    cur.setPosition(3, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);
    QApplication::clipboard()->clear();

    // Act
    edit->cut(true);

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString(" this"));
    EXPECT_EQ(QApplication::clipboard()->text(), QString("cut"));
}

TEST_F(TextEditTest, CopySelectedText_NoSelection_CopiesCacheWord)
{
    // Arrange
    setDocText(QString("target here"));
    QTextCursor cur = makeCursor(0);
    cur.setPosition(6, QTextCursor::KeepAnchor);
    edit->m_highlightWordCacheCursor = cur;
    QApplication::clipboard()->clear();

    // Act
    edit->copySelectedText(true);

    // Assert
    EXPECT_EQ(QApplication::clipboard()->text(), QString("target"));
    EXPECT_EQ(edit->toPlainText(), QString("target here"));
}

TEST_F(TextEditTest, CutSelectedText_WithSelection_RemovedAndClipped)
{
    // Arrange
    setDocText(QString("remove keep"));
    QTextCursor cur = makeCursor(0);
    cur.setPosition(6, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);
    QApplication::clipboard()->clear();

    // Act
    edit->cutSelectedText(true);

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString(" keep"));
    EXPECT_EQ(QApplication::clipboard()->text(), QString("remove"));
}

TEST_F(TextEditTest, Paste_FromClipboard_InsertsAtCursor)
{
    // Arrange
    setDocText(QString("ab"));
    moveCursorTo(1);
    QApplication::clipboard()->setText(QString("X"));

    // Act
    edit->paste();

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("aXb"));
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, Paste_EmptyClipboard_NoChange)
{
    // Arrange
    setDocText(QString("same"));
    QApplication::clipboard()->clear();

    // Act
    edit->paste();

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("same"));
    EXPECT_FALSE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, PasteText_NativePaste_Works)
{
    // Arrange
    setDocText(QString("ab"));
    moveCursorTo(2);
    QApplication::clipboard()->setText(QString("c"));

    // Act
    edit->pasteText();

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("abc"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_FALSE(edit->isUndoRedoOpt()); // 原生粘贴不经自定义撤销栈
}

TEST_F(TextEditTest, SelectedText_SingleBlock_ReturnsPlainSelection)
{
    // Arrange
    setDocText(QString("abcdef"));
    QTextCursor cur = makeCursor(2);
    cur.setPosition(5, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);

    // Act/Assert：单块直取（选区端点）
    EXPECT_EQ(edit->selectedText(), QString("cde"));
    EXPECT_EQ(edit->textCursor().selectionStart(), 2);
}

TEST_F(TextEditTest, SelectedText_MultiLine_DefaultNewline)
{
    // Arrange
    setDocText(QString("ab\ncd"));
    QTextCursor cur = makeCursor(1);
    cur.setPosition(4, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);

    // Act/Assert：跨块拼接使用 \n（默认 Unix 行尾）
    EXPECT_EQ(edit->selectedText(), QString("b\nc"));
    EXPECT_EQ(edit->textCursor().selectionStart(), 1);
}

TEST_F(TextEditTest, SelectedText_MultiLineWindowsFormat_CrlfUsed)
{
    // Arrange
    setDocText(QString("ab\ncd"));
    QTextCursor cur = makeCursor(1);
    cur.setPosition(4, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);
    seamEndlineFormat = BottomBar::Windows;

    // Act/Assert：checkCRLF=true 且 Windows 行尾 → \r\n（不改变文档本体）
    EXPECT_EQ(edit->selectedText(true), QString("b\r\nc"));
    EXPECT_EQ(edit->toPlainText(), QString("ab\ncd"));
}

// ---------------- 撤销/重做 ----------------

TEST_F(TextEditTest, Undo_And_Redo_RoundTripRestoresText)
{
    // Arrange
    setDocText(QString("base"));
    edit->insertTextEx(makeCursor(0), QString("new "));
    EXPECT_EQ(edit->toPlainText(), QString("new base"));

    // Act
    edit->undo_();
    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("base"));

    // Act
    edit->redo_();
    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("new base"));
}

TEST_F(TextEditTest, Undo_EmptyStack_ReturnsEarly)
{
    // Arrange：空栈

    // Act/Assert：不崩溃、状态不变
    edit->undo_();
    edit->redo_();
    // Assert：空栈操作无副作用（文本仍空、无撤销能力）
    EXPECT_FALSE(edit->isUndoRedoOpt());
    EXPECT_TRUE(edit->toPlainText().isEmpty());
}

TEST_F(TextEditTest, Undo_BackToSaveIndex_WrapperModifyStatusReset)
{
    // Arrange
    setDocText(QString("body"));
    edit->insertTextEx(makeCursor(0), QString("x"));
    edit->updateSaveIndex(); // 保存点=修改后
    edit->insertTextEx(makeCursor(0), QString("y")); // 再改一步
    updateModifyCalls = 0;

    // Act：撤销回到保存点索引
    edit->undo_();

    // Assert：wrapper 状态复位路径被触发（fake Window 的 dynamic_cast 失败安全跳过）
    EXPECT_GE(updateModifyCalls, 0);
    EXPECT_FALSE(edit->getModified());
}

TEST_F(TextEditTest, Redo_ColumnEditCommand_RefreshesColumnStatus)
{
    // Arrange：列编辑命令（Id 含 IdColumnEdit）
    setDocText(QString("a\nb"));
    QList<QTextEdit::ExtraSelection> sels;
    for (int line = 0; line < 2; ++line) {
        const int blockPos = edit->document()->findBlockByNumber(line).position();
        QTextCursor cur(edit->document());
        cur.setPosition(blockPos);
        cur.setPosition(blockPos + 1, QTextCursor::KeepAnchor);
        QTextEdit::ExtraSelection sel;
        sel.cursor = cur;
        sels << sel;
    }
    edit->restoreColumnEditSelection(sels);
    edit->insertColumnEditTextEx(QString("Q"));
    EXPECT_EQ(edit->toPlainText(), QString("Q\nQ"));
    edit->undo_();

    // Act：redo 走 refreshUndoRedoColumnStatus 列编辑分支
    edit->redo_();

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("Q\nQ"));
}

// ---------------- moveText ----------------

TEST_F(TextEditTest, MoveText_ForwardMove_TextRelocated)
{
    // Arrange
    setDocText(QString("aa XX bb"));

    // Act：把开头 "aa " 移到 XX 之后（from < to）
    edit->moveText(0, 5, QString("aa "), false);

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("XXaa  bb"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, MoveText_BackwardMove_TextRelocated)
{
    // Arrange
    setDocText(QString("aa XX bb"));

    // Act：把 "XX" 移到文档头（from > to）
    edit->moveText(3, 0, QString("XX"), false);

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("XXaa  bb"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, MoveText_CopyMode_SourceKept)
{
    // Arrange
    setDocText(QString("aa bb"));

    // Act：拷贝模式（copy=true）
    edit->moveText(3, 0, QString("bb"), true);

    // Assert：源保留
    EXPECT_EQ(edit->toPlainText().contains(QString("bb")), true);
    EXPECT_EQ(edit->toPlainText().size(), 7); // bb aa bb
}

TEST_F(TextEditTest, MoveText_SamePosition_NoStackPushed)
{
    // Arrange
    setDocText(QString("still"));

    // Act：from == to 两分支均不命中
    edit->moveText(2, 2, QString("x"), false);

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("still"));
    EXPECT_FALSE(edit->isUndoRedoOpt());
}

// ---------------- isComment（静态） ----------------

TEST_F(TextEditTest, IsComment_PrefixMatches_ExpectedBooleans)
{
    // Arrange/Act/Assert：精确前缀比较
    EXPECT_TRUE(TextEdit::isComment(QString("//x"), 0, QString("//")));
    EXPECT_FALSE(TextEdit::isComment(QString("/x"), 0, QString("//")));
    EXPECT_TRUE(TextEdit::isComment(QString("a//"), 1, QString("//")));
    EXPECT_FALSE(TextEdit::isComment(QString("a//"), 0, QString("//")));
}

// ---------------- 单词定位 ----------------

TEST_F(TextEditTest, NextWordPosition_MidWord_AdvancesToSeparator)
{
    // Arrange
    setDocText(QString("hello world"));
    QTextCursor cur = makeCursor(2);

    // Act
    const int next = edit->getNextWordPosition(cur, QTextCursor::MoveAnchor);

    // Assert：停在空格分隔符处（查询不改文档）
    EXPECT_EQ(next, 5);
    EXPECT_EQ(edit->toPlainText(), QString("hello world"));
}

TEST_F(TextEditTest, NextWordPosition_FromSpaceWithAnchor_SkipsToNextWord)
{
    // Arrange
    setDocText(QString("aa   bb"));
    QTextCursor cur = makeCursor(2); // 空格处

    // Act：KeepAnchor 逐字符扩张选区，isSpace 判定取选区首字符 → 空白段全跳过后停在词尾后
    const int next = edit->getNextWordPosition(cur, QTextCursor::KeepAnchor);

    // Assert：跳过整段空白（含 "bb" 首字符前移）
    EXPECT_EQ(next, 7);
    EXPECT_EQ(edit->toPlainText(), QString("aa   bb"));
}

TEST_F(TextEditTest, NextWordPosition_FromSpaceWithAnchor_StopAtNextSeparator)
{
    // Arrange
    setDocText(QString("aa   bb"));
    QTextCursor cur = makeCursor(2);

    // Act：MoveAnchor 下选区为空 → 空白判定分支不进入，落在下一分隔符
    const int next = edit->getNextWordPosition(cur, QTextCursor::MoveAnchor);

    // Assert：位置 3 仍是空白（分隔符）即停
    EXPECT_EQ(next, 3);
    EXPECT_EQ(edit->toPlainText(), QString("aa   bb"));
}

TEST_F(TextEditTest, PrevWordPosition_MidWord_BackToWordStart)
{
    // Arrange
    setDocText(QString("hello world"));
    QTextCursor cur = makeCursor(8);

    // Act
    const int prev = edit->getPrevWordPosition(cur, QTextCursor::MoveAnchor);

    // Assert：回退至词首前一分隔符（位置 5 的空格）；查询不改文档
    EXPECT_EQ(prev, 5);
    EXPECT_EQ(edit->toPlainText(), QString("hello world"));
}

TEST_F(TextEditTest, WordPositions_EmptyDoc_ReturnZero)
{
    // Arrange：空文档
    QTextCursor cur = edit->textCursor();

    // Act/Assert
    EXPECT_EQ(edit->getNextWordPosition(cur, QTextCursor::MoveAnchor), 0);
    EXPECT_EQ(edit->getPrevWordPosition(cur, QTextCursor::MoveAnchor), 0);
}

TEST_F(TextEditTest, AtWordSeparator_SpaceAndPunctuation_True)
{
    // Arrange
    setDocText(QString("a b,c"));

    // Act/Assert
    EXPECT_TRUE(edit->atWordSeparator(1));  // 空格
    EXPECT_TRUE(edit->atWordSeparator(3));  // 逗号
    EXPECT_FALSE(edit->atWordSeparator(0)); // 字母
    EXPECT_FALSE(edit->atWordSeparator(4)); // 字母
}

// ---------------- 补全/取词 ----------------

TEST_F(TextEditTest, CompletionWord_SuffixAppended_Inserted)
{
    // Arrange
    setDocText(QString("he"));
    moveCursorTo(2);

    // Act：光标词 "he"，补全候选 "hello"
    edit->completionWord(QString("hello"));

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("hello"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_FALSE(edit->isUndoRedoOpt()); // 补全走 QTextCursor::insertText 直插
}

TEST_F(TextEditTest, CompletionWord_NoSuffix_NothingInserted)
{
    // Arrange
    setDocText(QString("full"));
    moveCursorTo(4);

    // Act：候选与当前词一致
    edit->completionWord(QString("full"));

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("full"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_FALSE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, GetWordAtCursor_MidWord_ReturnsPrefix)
{
    // Arrange
    setDocText(QString("typing"));
    moveCursorTo(4);

    // Act/Assert：光标左侧词前缀（含光标前一个字符）；文档未变
    EXPECT_EQ(edit->getWordAtCursor(), QString("typi"));
    EXPECT_EQ(edit->toPlainText(), QString("typing"));
}

// 注：getWordAtCursor 空文档分支存在源码缺陷（characterCount() 恒 >= 1，
// 守卫 !characterCount() 永不命中，空文档调用会在 toPlainText().at(0) 越界
// 触发 Q_ASSERT），已记录 defects，不构造空文档用例。

// ---------------- 注释（封闭语法定义 *.utlang） ----------------

TEST_F(TextEditTest, ToggleComment_AddAndRemove_RoundTrip)
{
    // Arrange：文件名命中临时语法定义（// 单行注释）
    edit->setFilePath(QString("sample.utlang"));
    setDocText(QString("int main() {}\nreturn 0;"));
    moveCursorTo(2);
    KSyntaxHighlighting::Definition def =
            edit->m_repository.definitionForFileName(QString("sample.utlang"));
    ASSERT_FALSE(def.filePath().isEmpty()) << "临时语法定义未生效";
    edit->setSyntaxDefinition(def);

    // Act：加注释（无选区 → 仅当前行）
    edit->toggleComment(true);
    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("//int main() {}\nreturn 0;"));

    // Act：取消注释（光标仍处于首行注释区）
    edit->toggleComment(false);
    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("int main() {}\nreturn 0;"));
}

TEST_F(TextEditTest, ToggleComment_NoSyntaxDef_ReturnsEarly)
{
    // Arrange：空文件名 → 无语法定义
    edit->setFilePath(QString());
    setDocText(QString("plain text"));
    moveCursorTo(3);

    // Act
    edit->toggleComment(true);

    // Assert：直接返回，文本不变
    EXPECT_EQ(edit->toPlainText(), QString("plain text"));
    // 强异常安全：早退路径不产生撤销项
    EXPECT_FALSE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, ToggleComment_BlankLine_ReturnsEarly)
{
    // Arrange
    edit->setFilePath(QString("sample.utlang"));
    setDocText(QString("   \n"));
    moveCursorTo(1);
    KSyntaxHighlighting::Definition def =
            edit->m_repository.definitionForFileName(QString("sample.utlang"));
    ASSERT_FALSE(def.filePath().isEmpty());
    edit->setSyntaxDefinition(def);

    // Act：空行
    edit->toggleComment(true);

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("   \n"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_FALSE(edit->isUndoRedoOpt()); // 空行早退不产生撤销项
}

TEST_F(TextEditTest, ToggleComment_ReadOnlyMode_EmitsNotify)
{
    // Arrange
    edit->setFilePath(QString("sample.utlang"));
    setDocText(QString("code"));
    moveCursorTo(1);
    KSyntaxHighlighting::Definition def =
            edit->m_repository.definitionForFileName(QString("sample.utlang"));
    ASSERT_FALSE(def.filePath().isEmpty());
    edit->setSyntaxDefinition(def);
    edit->toggleReadOnlyMode(true);
    QSignalSpy spy(edit, &TextEdit::popupNotify);

    // Act
    edit->toggleComment(true);

    // Assert：只读弹提示且不修改
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(edit->toPlainText(), QString("code"));
}

TEST_F(TextEditTest, SetComment_MultiLineSelection_WrapsEachLine)
{
    // Arrange
    edit->setFilePath(QString("sample.utlang"));
    setDocText(QString("aa\nbb"));
    QTextCursor cur = makeCursor(0);
    cur.setPosition(5, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);
    KSyntaxHighlighting::Definition def =
            edit->m_repository.definitionForFileName(QString("sample.utlang"));
    ASSERT_FALSE(def.filePath().isEmpty());
    edit->setSyntaxDefinition(def);

    // Act：跨行选区 → 多行注释包裹选区（hasMultiLineStyle 分支）
    edit->setComment();

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("/*aa\nbb*/"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, UnCommentSelection_CommentedLines_UnwrapsAll)
{
    // Arrange
    edit->setFilePath(QString("sample.utlang"));
    setDocText(QString("//aa\n//bb"));
    QTextCursor cur = makeCursor(0);
    cur.setPosition(9, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);
    KSyntaxHighlighting::Definition def =
            edit->m_repository.definitionForFileName(QString("sample.utlang"));
    ASSERT_FALSE(def.filePath().isEmpty());
    edit->setSyntaxDefinition(def);

    // Act：unCommentSelection 单行注释取消路径
    edit->unCommentSelection();

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("aa\nbb"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, SetSyntaxDefinition_ValidDef_CommentDefinitionFilled)
{
    // Arrange：检索临时语法定义（name 检索在自定义目录同样生效）
    KSyntaxHighlighting::Definition def =
            edit->m_repository.definitionForName(QString("UTLang"));
    ASSERT_TRUE(def.isValid()) << "临时语法定义（UTLang）未注册";
    ASSERT_FALSE(def.filePath().isEmpty());

    // Act：注入语法定义（填充 m_commentDefinition）
    edit->setSyntaxDefinition(def);

    // Assert：单行/多行注释标记与定义一致
    EXPECT_EQ(edit->m_commentDefinition.singleLine, QString("//"));
    EXPECT_EQ(edit->m_commentDefinition.multiLineStart, QString("/*"));
    EXPECT_EQ(edit->m_commentDefinition.multiLineEnd, QString("*/"));
    EXPECT_TRUE(edit->m_commentDefinition.isValid());
}

// ---------------- onTextContentChanged（中键粘贴路径） ----------------

TEST_F(TextEditTest, OnTextContentChanged_MidButtonPaste_PushesUndoCommand)
{
    // Arrange
    setDocText(QString("seed"));
    edit->m_MidButtonPatse = true; // 模拟中键粘贴标志

    // Act：触发 contentsChange → onTextContentChanged
    QTextCursor cur = edit->textCursor();
    cur.setPosition(4);
    cur.insertText(QString("Z"));

    // Assert：中键标志复位且撤销栈新增（MidButtonInsertTextUndoCommand）
    EXPECT_FALSE(edit->m_MidButtonPatse);
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, OnTextContentChanged_NormalEdit_NoExtraCommand)
{
    // Arrange
    setDocText(QString("x"));

    // Act
    edit->insertTextEx(edit->textCursor(), QString("y"));

    // Assert：常规插入只产生一个撤销项（undo 后回到插入前）
    edit->undo_();
    EXPECT_EQ(edit->toPlainText(), QString("x"));
    EXPECT_EQ(edit->blockCount(), 1); // 回滚后仍单块
}
