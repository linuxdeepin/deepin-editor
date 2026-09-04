// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// UnindentTextCommand 单元测试（src/editor/indenttextcommond.cpp）。
// 多行去缩进（tab 整体删 1 个；空格删至多 tabSpaceNumber 个）撤销项。
//
// 分支清单（redo 逐行）：
// B1: 行首 '\t'           → deleteChar，removed=1
// B2: 行首 ' '            → 数至多 tabSpaceNumber 个连续空格并删除，removed=cnt
//     （cnt ∈ {1..tabSpaceNumber}；空格数不足 tabSpaceNumber 时只删现有数量）
// B3: 行首无缩进字符       → removed=0（不动）
// R3: 选区恢复 —— 单行：newStartPos = startpos - removed[0]（负数钳 0）+ 选整块；
//     多行：newStartPos/newEndPos 负值与倒置钳制
// U1: undo 逐行 —— removed==1 重插 "\t"；removed>1 重插 removed 个空格；removed==0 跳过
// U2: undo 选区恢复 —— [startpos, endpos)
//
// 用例映射：
// - Redo_TabIndented_RemovesTabEachLine                    → B1 + U1(==1)
// - Redo_SpaceIndented_RemovesUpToTabWidth                 → B2（4 空格 + 2 空格两种量）+ U1(>1)
// - Redo_SpacesMoreThanTabWidth_RemovesExactlyTabWidth     → B2 边界（6 空格只删 4）
// - Redo_NoIndent_NoChangeAndUndoStable                    → B3 + U1(==0)
// - Redo_MixedIndentTypes_HandledPerLine                   → B1+B2+B3 混合
// - Redo_SingleLineWithSelection_ClampsStartAndSelectsBlock → R3 单行（负值钳 0）
// - Redo_MultiLineContrivedRange_ClampsEndToStart          → R3 多行（newEndPos<newStartPos 钳制）
// - Redo_IndentStyles_ParamRoundTrip（TEST_P）             → tab/空格/无缩进等价类
//
// 最小清单自检：1 每公开方法 ≥1 用例（ctor/dtor/redo/undo）✔ 2 输入等价类 ✔
// 3 边界（0/1/4/6 空格、tab、钳 0、倒置）✔ 4 TEST_P ≥3 组 ✔ 5 分支映射 ✔
// 6 全分支 ✔ 7 无异常路径 ✔ 8 负面（无缩进行）✔ 9 双向可逆断言 ✔ 10 无 gMock ✔

#include "editor_undo_helpers.h"

#include "../../../src/editor/indenttextcommond.h"

class UnindentTextCommandTest : public EditorUndoTestBase
{
};

// ---- B1 + U1(==1)：tab 缩进行去缩进与还原 ----
TEST_F(UnindentTextCommandTest, Redo_TabIndented_RemovesTabEachLine)
{
    // Arrange：两行 tab 缩进，编辑器选区 [0,7)（"\taa\n\tbb"）
    edit->setPlainText("\taa\n\tbb");
    const QString snapshot = docText();
    edit->setTextCursor(ut::cursorAt(edit->document(), 0, 7));
    UnindentTextCommand cmd(edit, 0, 7, 0, 1, 4);

    // Act
    cmd.redo();

    // Assert：各行删 1 个 "\t"；R3 多行：[0-1→0, 7-2=5) 选区
    EXPECT_EQ(docText(), QString("aa\nbb"));
    EXPECT_EQ(edit->textCursor().selectionStart(), 0);
    EXPECT_EQ(edit->textCursor().selectionEnd(), 5);

    // Act & Assert：undo 重插 "\t" 并恢复原选区
    cmd.undo();
    EXPECT_EQ(docText(), snapshot);                            // 状态可逆
    EXPECT_EQ(edit->textCursor().selectionStart(), 0);
    EXPECT_EQ(edit->textCursor().selectionEnd(), 7);
}

// ---- B2 + U1(>1)：空格缩进（4 空格行删 4；2 空格行删 2） ----
TEST_F(UnindentTextCommandTest, Redo_SpaceIndented_RemovesUpToTabWidth)
{
    // Arrange
    edit->setPlainText("    aa\n  bb");
    const QString snapshot = docText();
    edit->setTextCursor(ut::cursorAt(edit->document(), 0, 11));
    UnindentTextCommand cmd(edit, 0, 11, 0, 1, 4);

    // Act
    cmd.redo();

    // Assert：第一行删 4 空格、第二行只有 2 个全删
    EXPECT_EQ(docText(), QString("aa\nbb"));
    cmd.undo();
    EXPECT_EQ(docText(), snapshot);                            // 状态可逆：4 空格 + 2 空格还原
}

// ---- B2 边界：空格数超过 tabSpaceNumber 只删 tabSpaceNumber 个 ----
TEST_F(UnindentTextCommandTest, Redo_SpacesMoreThanTabWidth_RemovesExactlyTabWidth)
{
    // Arrange：单行 6 个前导空格
    edit->setPlainText("      aa");
    const QString snapshot = docText();
    edit->setTextCursor(ut::cursorAt(edit->document(), 0, 8));
    UnindentTextCommand cmd(edit, 0, 8, 0, 0, 4);

    // Act
    cmd.redo();

    // Assert：只删 4 个空格，剩 2 个
    EXPECT_EQ(docText(), QString("  aa"));
    cmd.undo();
    EXPECT_EQ(docText(), snapshot);                            // 状态可逆：重插 4 个空格
}

// ---- B3 + U1(==0)：无缩进行不动、undo 稳定 ----
TEST_F(UnindentTextCommandTest, Redo_NoIndent_NoChangeAndUndoStable)
{
    // Arrange
    edit->setPlainText("aa\nbb");
    edit->setTextCursor(ut::cursorAt(edit->document(), 0, 5));
    UnindentTextCommand cmd(edit, 0, 5, 0, 1, 4);

    // Act
    cmd.redo();

    // Assert：removed=0，文档不变（强异常安全）
    EXPECT_EQ(docText(), QString("aa\nbb"));
    EXPECT_EQ(edit->textCursor().selectionStart(), 0);         // 选区 [0-0, 5-0)
    EXPECT_EQ(edit->textCursor().selectionEnd(), 5);
    cmd.undo();
    EXPECT_EQ(docText(), QString("aa\nbb"));                   // undo 同样无操作
}

// ---- B1+B2+B3 混合：逐行独立判定 ----
TEST_F(UnindentTextCommandTest, Redo_MixedIndentTypes_HandledPerLine)
{
    // Arrange：tab 行 / 2 空格行 / 无缩进行
    edit->setPlainText("\ta\n  b\nc");
    const QString snapshot = docText();
    edit->setTextCursor(ut::cursorAt(edit->document(), 0, 9));
    UnindentTextCommand cmd(edit, 0, 9, 0, 2, 4);

    // Act
    cmd.redo();

    // Assert
    EXPECT_EQ(docText(), QString("a\nb\nc"));
    cmd.undo();
    EXPECT_EQ(docText(), snapshot);                            // 各行按 removed 值独立还原
}

// ---- R3 单行：newStartPos 负值钳 0 + 选整块 ----
TEST_F(UnindentTextCommandTest, Redo_SingleLineWithSelection_ClampsStartAndSelectsBlock)
{
    // Arrange：选区 [0,6)（含 tab）；startpos=0 时 newStartPos = 0-1 = -1 → 钳 0
    edit->setPlainText("\thello");
    const QString snapshot = docText();
    edit->setTextCursor(ut::cursorAt(edit->document(), 0, 6));
    UnindentTextCommand cmd(edit, 0, 6, 0, 0, 4);

    // Act
    cmd.redo();

    // Assert：R3 单行分支——钳 0 后选整块 "hello"
    EXPECT_EQ(docText(), QString("hello"));
    EXPECT_EQ(ut::toLf(edit->textCursor().selectedText()), QString("hello"));
    EXPECT_EQ(edit->textCursor().selectionStart(), 0);

    cmd.undo();
    EXPECT_EQ(docText(), snapshot);                            // 状态可逆
    EXPECT_EQ(edit->textCursor().selectionStart(), 0);         // U2 恢复 [startpos,endpos)
    EXPECT_EQ(edit->textCursor().selectionEnd(), 6);
}

// ---- R3 多行倒置钳制：构造 endpos < totalRemoved 触发 newEndPos 钳到 newStartPos ----
TEST_F(UnindentTextCommandTest, Redo_MultiLineContrivedRange_ClampsEndToStart)
{
    // Arrange：两行 tab；endpos=1 < totalRemoved=2 → newEndPos=-1 → 钳为 newStartPos=0
    edit->setPlainText("\ta\n\tb");
    edit->setTextCursor(ut::cursorAt(edit->document(), 0, 1));
    UnindentTextCommand cmd(edit, 0, 1, 0, 1, 4);

    // Act
    cmd.redo();

    // Assert：选区塌缩为 [0,0)（钳制分支命中）
    EXPECT_EQ(docText(), QString("a\nb"));
    EXPECT_EQ(edit->textCursor().selectionStart(), 0);
    EXPECT_EQ(edit->textCursor().selectionEnd(), 0);
    cmd.undo();
    EXPECT_EQ(docText(), QString("\ta\n\tb"));                 // 状态可逆
}

// ---- 等价类 TEST_P：tab / 空格 / 无缩进三组同一往返断言 ----
namespace {
struct UnindentCase {
    QString docContent;
    QString expectedAfterRedo;
};
}  // namespace

class UnindentParamTest : public EditorUndoTestBase,
                          public ::testing::WithParamInterface<UnindentCase> {
};

TEST_P(UnindentParamTest, Redo_IndentStyles_ParamRoundTrip)
{
    // Arrange：单行，无选区路径（cursor 无选区）
    const UnindentCase &c = GetParam();
    edit->setPlainText(c.docContent);
    edit->setTextCursor(ut::cursorAt(edit->document(), 0, 0));
    UnindentTextCommand cmd(edit, 0, c.docContent.size(), 0, 0, 4);

    // Act
    cmd.redo();

    // Assert
    EXPECT_EQ(docText(), c.expectedAfterRedo);
    cmd.undo();
    EXPECT_EQ(docText(), c.docContent);                        // 状态可逆
}

INSTANTIATE_TEST_SUITE_P(IndentVariants, UnindentParamTest,
    ::testing::Values(
        UnindentCase{ QString("\ttabbed"), QString("tabbed") },
        UnindentCase{ QString("  twoSpaces"), QString("twoSpaces") },
        UnindentCase{ QString("noIndent"), QString("noIndent") }
    ));

// ---- D1：经基类指针虚析构删除（deleting destructor 路径） ----
TEST_F(UnindentTextCommandTest, Destructor_DeleteViaBasePointer_ReleasesCleanly)
{
    // Arrange
    edit->setPlainText("\tline");
    edit->setTextCursor(ut::cursorAt(edit->document(), 0, 5));
    QUndoCommand *cmd = new UnindentTextCommand(edit, 0, 5, 0, 0, 4);
    cmd->redo();
    EXPECT_EQ(docText(), QString("line"));                     // 前提：命令已生效

    // Act
    delete cmd;

    // Assert：析构不改变文档状态
    EXPECT_EQ(docText(), QString("line"));
}
