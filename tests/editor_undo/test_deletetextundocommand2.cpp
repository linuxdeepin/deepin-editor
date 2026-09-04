// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// DeleteTextUndoCommand2 单元测试（src/editor/deletetextundocommand.cpp）。
// Ctrl+K / Ctrl+Shift+K 删除到行尾、删除整行的撤销项。
//
// 分支清单（redo 单光标）：
// B1: m_iscurrLine=true        → 选整块 + NextCharacter（整行删除）
// B2: isEmptyLine（构造文本空）|| atBlockEnd → NextCharacter（选换行符）
// B3: isBlankLine && atBlockStart            → 选整块（删除纯空白行）
// B4: 其余                                     → 选至 EndOfBlock（删到行尾）
// 注：m_sInsertText 在 redo 中被实际删除内容覆盖；构造文本只决定 B2-B4 分支选择。
// R2/U2: 列编辑变体 —— 逐选区 deletePreviousChar / 以 m_beginPostion 重插
// CT: 两个构造重载（单光标/列编辑）均含 "\r\n"→"\n" 归一化
//
// 用例映射：
// - Redo_CurrLineTrue_DeletesWholeLine                    → B1
// - Redo_NotCurrLine_AtBlockEnd_DeletesNewline            → B2（atBlockEnd 侧）
// - Redo_NotCurrLine_EmptyCtorText_DeletesNextChar        → B2（isEmptyLine 短路侧）
// - Redo_NotCurrLine_BlankLineAtStart_DeletesWholeBlock   → B3
// - Redo_NotCurrLine_MidLine_DeletesToEndOfBlock          → B4
// - Ctor_CrlfText_NormalizedAffectsBranchOnly             → CT 归一化
// - Redo_ColumnSingleSelection_ReversibleBothWays         → R2/U2（单选区）
// - Redo_ColumnMultiSelection_UndoUsesLastBeginPos        → R2/U2（多选区；见 defect 注记）
// - Redo_TextVariants_RoundTripExact（TEST_P）            → 中文/emoji/ASCII 等价类
//
// 最小清单自检：1 每公开方法 ≥1 用例（ctor×2/undo/redo）✔ 2 输入等价类（分支文本/
// 多字节）✔ 3 边界（块首/块尾/空构造文本）✔ 4 TEST_P ≥3 组 ✔ 5 分支映射 ✔
// 6 全分支 ✔ 7 无异常路径 ✔ 8 负面（空文本）✔ 9 双向可逆断言 ✔ 10 无 gMock ✔

#include "editor_undo_helpers.h"

#include "../../../src/editor/deletetextundocommand.h"

class DeleteTextUndoCommand2Test : public EditorUndoTestBase
{
};

// ---- B1：删除整行（含行尾换行符），undo 精确重建 ----
TEST_F(DeleteTextUndoCommand2Test, Redo_CurrLineTrue_DeletesWholeLine)
{
    // Arrange：光标位于第二行中部 position=4（"cd" 中间）
    edit->setPlainText("ab\ncd\nef");
    const QString snapshot = docText();
    QTextCursor cursor = ut::cursorAt(edit->document(), 4, 4);
    DeleteTextUndoCommand2 cmd(cursor, "xy", edit, true);

    // Act
    cmd.redo();

    // Assert：整行 "cd\n" 被删除
    EXPECT_EQ(docText(), QString("ab\nef"));
    EXPECT_EQ(edit->document()->blockCount(), 2);

    // Act & Assert：undo 在 beginPostion 重插 "cd"+段落分隔符
    cmd.undo();
    EXPECT_EQ(docText(), snapshot);                            // 状态可逆
    EXPECT_EQ(edit->document()->blockCount(), 3);
}

// ---- B2 atBlockEnd 侧：行尾删除换行符（行合并） ----
TEST_F(DeleteTextUndoCommand2Test, Redo_NotCurrLine_AtBlockEnd_DeletesNewline)
{
    // Arrange：光标位于第二行行尾 position=5
    edit->setPlainText("ab\ncd\nef");
    QTextCursor cursor = ut::cursorAt(edit->document(), 5, 5);
    DeleteTextUndoCommand2 cmd(cursor, "xy", edit, false);

    // Act
    cmd.redo();

    // Assert：选中并删除行尾换行符 → 与下一行合并
    EXPECT_EQ(docText(), QString("ab\ncdef"));
    cmd.undo();
    EXPECT_EQ(docText(), QString("ab\ncd\nef"));               // 状态可逆
}

// ---- B2 isEmptyLine 短路侧：构造文本为空 → 删除光标后一个字符 ----
TEST_F(DeleteTextUndoCommand2Test, Redo_NotCurrLine_EmptyCtorText_DeletesNextChar)
{
    // Arrange：光标 position=2，构造文本为空串
    edit->setPlainText("abcdef");
    QTextCursor cursor = ut::cursorAt(edit->document(), 2, 2);
    DeleteTextUndoCommand2 cmd(cursor, "", edit, false);

    // Act
    cmd.redo();

    // Assert：NextCharacter 选中 'c' 并删除
    EXPECT_EQ(docText(), QString("abdef"));
    cmd.undo();
    EXPECT_EQ(docText(), QString("abcdef"));                   // 状态可逆
}

// ---- B3：纯空白行且光标在块首 → 删除整块 ----
TEST_F(DeleteTextUndoCommand2Test, Redo_NotCurrLine_BlankLineAtStart_DeletesWholeBlock)
{
    // Arrange：第二行为三个空格，光标在其块首 position=3；构造文本非空但 trim 后为空
    edit->setPlainText("ab\n   \ncd");
    QTextCursor cursor = ut::cursorAt(edit->document(), 3, 3);
    DeleteTextUndoCommand2 cmd(cursor, "   ", edit, false);

    // Act
    cmd.redo();

    // Assert：整块 "   " 被删除（保留换行符）
    EXPECT_EQ(docText(), QString("ab\n\ncd"));
    cmd.undo();
    EXPECT_EQ(docText(), QString("ab\n   \ncd"));              // 状态可逆
}

// ---- B4：行中删除到行尾 ----
TEST_F(DeleteTextUndoCommand2Test, Redo_NotCurrLine_MidLine_DeletesToEndOfBlock)
{
    // Arrange
    edit->setPlainText("abcdef");
    QTextCursor cursor = ut::cursorAt(edit->document(), 2, 2);
    DeleteTextUndoCommand2 cmd(cursor, "xy", edit, false);

    // Act
    cmd.redo();

    // Assert："cdef" 被删除
    EXPECT_EQ(docText(), QString("ab"));
    cmd.undo();
    EXPECT_EQ(docText(), QString("abcdef"));                   // 状态可逆
}

// ---- CT：CRLF 归一化（归一化后非空白 → 走 B4 删到行尾分支） ----
TEST_F(DeleteTextUndoCommand2Test, Ctor_CrlfText_NormalizedAffectsBranchOnly)
{
    // Arrange
    edit->setPlainText("abcdef");
    QTextCursor cursor = ut::cursorAt(edit->document(), 2, 2);
    DeleteTextUndoCommand2 cmd(cursor, "a\r\nb", edit, false);

    // Act
    cmd.redo();

    // Assert："a\r\nb" 归一化为 "a\nb"（非空非纯空白）→ B4 分支删到行尾
    EXPECT_EQ(docText(), QString("ab"));
    cmd.undo();
    EXPECT_EQ(docText(), QString("abcdef"));
}

// ---- R2/U2 单选区：正确的双向可逆 ----
TEST_F(DeleteTextUndoCommand2Test, Redo_ColumnSingleSelection_ReversibleBothWays)
{
    // Arrange：单个列选区 [0,1)
    edit->setPlainText("aa\nbb");
    const QString snapshot = docText();
    QList<QTextEdit::ExtraSelection> selections;
    selections << ut::makeSelection(edit->document(), 0, 1);
    DeleteTextUndoCommand2 cmd(selections, "t", edit, false);

    // Act
    cmd.redo();

    // Assert：选区被 deletePreviousChar 删除
    EXPECT_EQ(docText(), QString("a\nbb"));
    cmd.undo();
    EXPECT_EQ(docText(), snapshot);                            // 状态可逆
}

// ---- R2/U2 多选区：undo 以最后一次 redo 记录的 m_beginPostion 统一重插 ----
// 源码行为注记（defect 候选，见批次报告）：redo 列编辑循环里 m_beginPostion 被
// 每个选区覆盖，undo 循环对所有选区统一使用最后写入值，导致多选区场景文本被
// 恢复到最后一个选区起点而非各自起点；本用例按实际行为断言并保留可逆性失败的证据。
TEST_F(DeleteTextUndoCommand2Test, Redo_ColumnMultiSelection_UndoUsesLastBeginPos)
{
    // Arrange：两个列选区 [0,1) 与 [3,4)
    edit->setPlainText("aa\nbb");
    QList<QTextEdit::ExtraSelection> selections;
    selections << ut::makeSelection(edit->document(), 0, 1)
               << ut::makeSelection(edit->document(), 3, 4);
    DeleteTextUndoCommand2 cmd(selections, "t", edit, false);

    // Act
    cmd.redo();

    // Assert：redo 删除两个选区内容
    EXPECT_EQ(docText(), QString("a\nb"));

    // Act & Assert：undo 将两段文本都插到最后一个选区起点 position=3（实际行为）
    cmd.undo();
    EXPECT_EQ(docText(), QString("a\nbab"));                   // 非 "aa\nbb"：见 defect 注记
    EXPECT_NE(docText(), QString("aa\nbb"));
}

// ---- R2/U2 列编辑裸光标（块中/块首）：ctor2 捕获前字符与 "\n" 两分支 ----
TEST_F(DeleteTextUndoCommand2Test, Redo_ColumnBareCursorMidBlock_DeletesPreviousChar)
{
    // Arrange：块中裸光标 position=2（"abc" 中 'b' 之前）→ selectTextList 记录 'b'
    edit->setPlainText("abc");
    QList<QTextEdit::ExtraSelection> selections;
    selections << ut::makeSelection(edit->document(), 2, 2);
    DeleteTextUndoCommand2 cmd(selections, "t", edit, false);

    // Act
    cmd.redo();

    // Assert：deletePreviousChar 删除 'b'
    EXPECT_EQ(docText(), QString("ac"));
    cmd.undo();
    EXPECT_EQ(docText(), QString("acb"));                      // 同前述重插位置偏移语义
}

// ---- R2/U2 列编辑裸光标（块首）：捕获 "\n" ----
TEST_F(DeleteTextUndoCommand2Test, Redo_ColumnBareCursorAtBlockStart_DeletesNewline)
{
    // Arrange：裸光标位于第二行行首 position=3 → posInBlock-1 < 0 → 记录 "\n"
    edit->setPlainText("ab\ncd");
    QList<QTextEdit::ExtraSelection> selections;
    selections << ut::makeSelection(edit->document(), 3, 3);
    DeleteTextUndoCommand2 cmd(selections, "t", edit, false);

    // Act
    cmd.redo();

    // Assert：删除上一行换行符，两行合并
    EXPECT_EQ(docText(), QString("abcd"));
    EXPECT_EQ(edit->document()->blockCount(), 1);
    cmd.undo();
    EXPECT_EQ(docText(), QString("abc\nd"));                   // undo 重插换行（位置同上偏移语义）
}

// ---- 输入等价类 TEST_P：中文/emoji/ASCII 内容的整行删除往返 ----
namespace {
struct Delete2Case {
    QString docContent;
    int cursorPos;
    bool currLine;
    QString expectedAfterRedo;
};
}  // namespace

class Delete2ParamTest : public EditorUndoTestBase,
                         public ::testing::WithParamInterface<Delete2Case> {
};

TEST_P(Delete2ParamTest, Redo_TextVariants_RoundTripExact)
{
    // Arrange
    const Delete2Case &c = GetParam();
    edit->setPlainText(c.docContent);
    QTextCursor cursor = ut::cursorAt(edit->document(), c.cursorPos, c.cursorPos);
    DeleteTextUndoCommand2 cmd(cursor, "xy", edit, c.currLine);

    // Act
    cmd.redo();

    // Assert
    EXPECT_EQ(docText(), c.expectedAfterRedo);
    cmd.undo();
    EXPECT_EQ(docText(), c.docContent);                        // 多字节往返精确
    EXPECT_EQ(edit->document()->characterCount() - 1, c.docContent.size());
}

INSTANTIATE_TEST_SUITE_P(TextVariants, Delete2ParamTest,
    ::testing::Values(
        Delete2Case{ QString("first\nsecond"), 7, false, QString("first\ns") },        // 行中删到行尾
        Delete2Case{ QString::fromUtf8("第一行\n第二行"), 4, true,
                     QString::fromUtf8("第一行\n") },                                   // 删整行（末行无换行可带）
        Delete2Case{ QString::fromUtf8("a\xF0\x9F\x98\x80" "\nz"), 0, false,
                     QString("\nz") }                                                    // emoji 行删到行尾
    ));
