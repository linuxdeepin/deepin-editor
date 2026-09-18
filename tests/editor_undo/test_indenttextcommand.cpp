// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// IndentTextCommand 单元测试（src/editor/indenttextcommond.cpp）。
// 多行前加 "\t" 缩进的撤销项。构造函数无条件解引用 m_edit->textCursor()，
// 必须传入真实（空壳）TextEdit。
//
// 分支清单：
// CT: 构造 —— m_hasselected = edit->textCursor().hasSelection()（编辑器选区状态前置）
// R1: redo 循环 —— startline..endline 逐行 StartOfBlock 插 "\t"（单行/多行边界；
//     末行 NextBlock 失败无害）
// R2: redo 选区恢复 —— !m_hasselected 跳过；单行：选整块；多行：[startpos+1,
//     endpos+(endline-startline)+1)
// U1: undo 循环 —— 逐行 StartOfBlock deleteChar（删 "\t"）
// U2: undo 选区恢复 —— !m_hasselected 跳过；有选区：[startpos, endpos)
//
// 用例映射：
// - Redo_SingleLineNoSelection_AddsTab                     → CT(假)+R1/U1（无选区路径）
// - Redo_MultiLineNoSelection_IndentsEveryLine             → R1 多行循环
// - Redo_MultiLineWithSelection_IndentsAndRestoresRange    → CT(真)+R2 多行公式 + U2
// - Redo_SingleLineWithSelection_SelectsWholeBlock         → R2 单行分支 + U2
// - Redo_MultiByteContent_IndentsPreserveText（TEST_P）    → 中文/emoji 内容等价类
// - Undo_AfterRedo_SelectionExactlyRestored                → U2 精确选区
//
// 最小清单自检：1 每公开方法 ≥1 用例（ctor/dtor/redo/undo）✔ 2 输入等价类 ✔
// 3 边界（单行/多行/末行）✔ 4 TEST_P ≥3 组 ✔ 5 分支映射 ✔ 6 全分支 ✔
// 7 无异常路径 ✔ 8 负面并入无选区边界 ✔ 9 双向可逆断言 ✔ 10 无 gMock ✔

#include "editor_undo_helpers.h"

#include "../../../src/editor/indenttextcommond.h"

class IndentTextCommandTest : public EditorUndoTestBase
{
};

// ---- CT(假)+R1/U1：无选区单行缩进/还原 ----
TEST_F(IndentTextCommandTest, Redo_SingleLineNoSelection_AddsTab)
{
    // Arrange：编辑器光标无选区（position 0）
    edit->setPlainText("abc");
    edit->setTextCursor(ut::cursorAt(edit->document(), 0, 0));
    IndentTextCommand cmd(edit, 0, 3, 0, 0);

    // Act
    cmd.redo();

    // Assert：行首插入 "\t"
    EXPECT_EQ(docText(), QString("\tabc"));
    cmd.undo();
    EXPECT_EQ(docText(), QString("abc"));                      // 状态可逆
}

// ---- R1 多行循环：每行加 "\t"，末行 NextBlock 失败不影响结果 ----
TEST_F(IndentTextCommandTest, Redo_MultiLineNoSelection_IndentsEveryLine)
{
    // Arrange
    edit->setPlainText("aa\nbb\ncc");
    const QString snapshot = docText();
    edit->setTextCursor(ut::cursorAt(edit->document(), 0, 0));
    IndentTextCommand cmd(edit, 0, 8, 0, 2);

    // Act
    cmd.redo();

    // Assert：三行行首各一个 "\t"（含最后一行）
    EXPECT_EQ(docText(), QString("\taa\n\tbb\n\tcc"));
    cmd.undo();
    EXPECT_EQ(docText(), snapshot);                            // 状态可逆：逐行删除 "\t"
}

// ---- CT(真)+R2 多行公式 + U2：带选区缩进后选区按源码公式恢复 ----
TEST_F(IndentTextCommandTest, Redo_MultiLineWithSelection_IndentsAndRestoresRange)
{
    // Arrange：编辑器选区 [0,8)（覆盖全部三行）
    edit->setPlainText("aa\nbb\ncc");
    const QString snapshot = docText();
    edit->setTextCursor(ut::cursorAt(edit->document(), 0, 8));
    IndentTextCommand cmd(edit, 0, 8, 0, 2);

    // Act
    cmd.redo();

    // Assert
    EXPECT_EQ(docText(), QString("\taa\n\tbb\n\tcc"));
    // R2 多行公式：[startpos+1, endpos + (endline-startline) + 1) = [1, 11)
    EXPECT_EQ(edit->textCursor().selectionStart(), 1);
    EXPECT_EQ(edit->textCursor().selectionEnd(), 8 + 2 + 1);

    // Act & Assert：undo 删 "\t" 并按 [startpos, endpos) 恢复选区
    cmd.undo();
    EXPECT_EQ(docText(), snapshot);                            // 状态可逆
    EXPECT_EQ(edit->textCursor().selectionStart(), 0);         // U2 恢复原始选区
    EXPECT_EQ(edit->textCursor().selectionEnd(), 8);
}

// ---- R2 单行分支：选整块 ----
TEST_F(IndentTextCommandTest, Redo_SingleLineWithSelection_SelectsWholeBlock)
{
    // Arrange：选中 "ello" [1,5)
    edit->setPlainText("hello");
    edit->setTextCursor(ut::cursorAt(edit->document(), 1, 5));
    IndentTextCommand cmd(edit, 1, 5, 0, 0);

    // Act
    cmd.redo();

    // Assert：R2 单行分支选整块 "\thello"
    EXPECT_EQ(docText(), QString("\thello"));
    EXPECT_EQ(ut::toLf(edit->textCursor().selectedText()), QString("\thello"));
    cmd.undo();
    EXPECT_EQ(docText(), QString("hello"));
    EXPECT_EQ(ut::toLf(edit->textCursor().selectedText()), QString("ello"));  // U2 原选区
}

// ---- 内容等价类 TEST_P：缩进/还原保持多字节文本 ----
namespace {
struct IndentCase {
    QString lineText;
};
}  // namespace

class IndentParamTest : public EditorUndoTestBase,
                        public ::testing::WithParamInterface<IndentCase> {
};

TEST_P(IndentParamTest, Redo_MultiByteContent_IndentsPreserveText)
{
    // Arrange：单行多字节内容，无选区
    edit->setPlainText(GetParam().lineText);
    edit->setTextCursor(ut::cursorAt(edit->document(), 0, 0));
    IndentTextCommand cmd(edit, 0, GetParam().lineText.size(), 0, 0);

    // Act
    cmd.redo();

    // Assert
    EXPECT_EQ(docText(), QString("\t") + GetParam().lineText);
    cmd.undo();
    EXPECT_EQ(docText(), GetParam().lineText);                 // 状态可逆
    EXPECT_EQ(edit->document()->characterCount() - 1, GetParam().lineText.size());
}

INSTANTIATE_TEST_SUITE_P(ContentVariants, IndentParamTest,
    ::testing::Values(
        IndentCase{ QString("plain ascii") },
        IndentCase{ QString::fromUtf8("中文缩进行") },
        IndentCase{ QString::fromUtf8("\xF0\x9F\x98\x80 emoji line") }
    ));

// ---- U2 精确性独立用例：中途选区（非整行）恢复原样 ----
TEST_F(IndentTextCommandTest, Undo_AfterRedo_SelectionExactlyRestored)
{
    // Arrange：三行文档，选区 [2,7)（跨行中部）
    edit->setPlainText("aa\nbb\ncc");
    const QString snapshot = docText();
    edit->setTextCursor(ut::cursorAt(edit->document(), 2, 7));
    IndentTextCommand cmd(edit, 2, 7, 0, 2);

    // Act & Assert
    cmd.redo();
    EXPECT_EQ(docText(), QString("\taa\n\tbb\n\tcc"));
    cmd.undo();
    EXPECT_EQ(docText(), snapshot);                            // 状态可逆
    EXPECT_EQ(edit->textCursor().selectionStart(), 2);         // U2 精确恢复中途选区
    EXPECT_EQ(edit->textCursor().selectionEnd(), 7);
}

// ---- D1：经基类指针虚析构删除（deleting destructor 路径） ----
TEST_F(IndentTextCommandTest, Destructor_DeleteViaBasePointer_ReleasesCleanly)
{
    // Arrange
    edit->setPlainText("line");
    edit->setTextCursor(ut::cursorAt(edit->document(), 0, 0));
    QUndoCommand *cmd = new IndentTextCommand(edit, 0, 4, 0, 0);
    cmd->redo();
    EXPECT_EQ(docText(), QString("\tline"));                   // 前提：命令已生效

    // Act
    delete cmd;

    // Assert：析构不改变文档状态
    EXPECT_EQ(docText(), QString("\tline"));
}
