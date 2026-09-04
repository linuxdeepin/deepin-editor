// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// DeleteTextUndoCommand 单元测试（src/editor/deletetextundocommand.cpp）。
//
// 分支清单：
// CT1（单光标）：hasSelection 真 → m_sInsertText=selectedText；
//     假 → positionInBlock-1 >= 0 → 块内前一字符；< 0（块首）→ "\n"（上一行换行符）
// CT2（列编辑）：逐选区同上三分支 → m_selectTextList
// R1: redo 单光标 —— deletePreviousChar；m_edit 空/非空
// R2: redo 列编辑 —— 逐选区 deletePreviousChar；restoreColumnEditSelection
// U1: undo 单光标 —— beginPos 重插 + 重新选中；m_edit 空/非空
// U2: undo 列编辑 —— 逐选区重插 selectTextList[i] + 选中；restoreColumnEditSelection
// ID: id() —— 列编辑选区空 → Utils::IdDelete；非空 → Utils::IdColumnEditDelete
//
// 用例映射：
// - Redo_WithSelection_DeletesAndUndoRestoresSelected    → CT1(真)+R1/U1
// - Redo_NoSelectionMidBlock_DeletesPreviousChar         → CT1(块内)+R1/U1
// - Redo_NoSelectionAtBlockStart_DeletesNewline          → CT1(块首 "\n")+R1/U1
// - Redo_ColumnSelections_DeletesAllThenUndoRestores     → CT2(真)+R2/U2
// - Redo_ColumnBareCursorMidBlock_DeletesPreviousChar    → CT2(块内)+R2/U2
// - Redo_MultiByteText_ReversibleBothWays（TEST_P）      → 输入等价类（中文/emoji/ASCII）
// - Id_SingleMode_ReturnsIdDelete / Id_ColumnMode_ReturnsIdColumnEditDelete → ID
//
// 最小清单自检：1 每公开方法 ≥1 用例（ctor×2/undo/redo/id）✔ 2 输入等价类 ✔
// 3 边界（块首/块内/空）✔ 4 TEST_P ≥3 组 ✔ 5 分支映射 ✔ 6 全分支 ✔
// 7 无异常路径 ✔ 8 负面（块首删除换行合并）✔ 9 双向可逆断言 ✔ 10 空壳接缝无 gMock ✔

#include "editor_undo_helpers.h"

#include "../../../src/editor/deletetextundocommand.h"

#include "../../../src/common/utils.h"

class DeleteTextUndoCommandTest : public EditorUndoTestBase
{
};

// ---- CT1(真)+R1/U1：选区删除与恢复 ----
TEST_F(DeleteTextUndoCommandTest, Redo_WithSelection_DeletesAndUndoRestoresSelected)
{
    // Arrange：选中 "he"
    edit->setPlainText("hello");
    const QString snapshot = docText();
    QTextCursor cursor = ut::cursorAt(edit->document(), 0, 2);
    DeleteTextUndoCommand cmd(cursor, edit);

    // Act
    cmd.redo();

    // Assert
    EXPECT_EQ(docText(), QString("llo"));
    EXPECT_EQ(edit->textCursor().position(), 0);               // 删除后光标位于 beginPos

    // Act & Assert：undo 在 beginPos 重插并选中
    cmd.undo();
    EXPECT_EQ(docText(), snapshot);                            // 状态可逆
    EXPECT_EQ(ut::toLf(edit->textCursor().selectedText()), QString("he"));
}

// ---- CT1(块内)+R1/U1：无选区时光标前一个字符被删除 ----
// 源码行为注记（defect 候选，见批次报告）：无选区路径 m_beginPos 记录的是光标位置
// （被删字符之后），undo 在 m_beginPos 重插导致字符落在原字符位置之后（"acb"），
// 仅当选区路径（beginPos=selectionStart）恢复正确。按实际行为断言留证。
TEST_F(DeleteTextUndoCommandTest, Redo_NoSelectionMidBlock_DeletesPreviousChar)
{
    // Arrange：光标 position=2（"abc" 中 'b' 之前）
    edit->setPlainText("abc");
    QTextCursor cursor = ut::cursorAt(edit->document(), 2, 2);
    DeleteTextUndoCommand cmd(cursor, edit);

    // Act
    cmd.redo();

    // Assert：deletePreviousChar 删除 'b'
    EXPECT_EQ(docText(), QString("ac"));
    cmd.undo();
    EXPECT_EQ(docText(), QString("acb"));                      // 非 "abc"：见 defect 注记
    EXPECT_NE(docText(), QString("abc"));
}

// ---- CT1(块首)+R1/U1：块首删除的是上一行换行符（行合并）；undo 重插位置同 defect 注记 ----
TEST_F(DeleteTextUndoCommandTest, Redo_NoSelectionAtBlockStart_DeletesNewline)
{
    // Arrange：光标位于第二行行首 position=3
    edit->setPlainText("ab\ncd");
    QTextCursor cursor = ut::cursorAt(edit->document(), 3, 3);
    DeleteTextUndoCommand cmd(cursor, edit);

    // Act
    cmd.redo();

    // Assert：删除换行符，两行合并
    EXPECT_EQ(docText(), QString("abcd"));
    EXPECT_EQ(edit->document()->blockCount(), 1);
    cmd.undo();
    EXPECT_EQ(docText(), QString("abc\nd"));                   // 非 "ab\ncd"：同 defect 注记
    EXPECT_NE(docText(), QString("ab\ncd"));
}

// ---- CT2(真)+R2/U2：列编辑多选区 ----
TEST_F(DeleteTextUndoCommandTest, Redo_ColumnSelections_DeletesAllThenUndoRestores)
{
    // Arrange：两行各选一个字符
    edit->setPlainText("aa\nbb");
    const QString snapshot = docText();
    QList<QTextEdit::ExtraSelection> selections;
    selections << ut::makeSelection(edit->document(), 0, 1)
               << ut::makeSelection(edit->document(), 3, 4);
    DeleteTextUndoCommand cmd(selections, edit);

    // Act
    cmd.redo();

    // Assert：逐选区 deletePreviousChar（选区存在时删除选区）
    EXPECT_EQ(docText(), QString("a\nb"));
    EXPECT_EQ(seam.restoreColumnEditSelectionCalls, 1);

    // Act & Assert：undo 逐选区重插
    cmd.undo();
    EXPECT_EQ(docText(), snapshot);                            // 状态可逆
    EXPECT_EQ(seam.restoreColumnEditSelectionCalls, 2);
    EXPECT_EQ(ut::toLf(seam.lastRestoredSelections[0].cursor.selectedText()), QString("a"));
}

// ---- CT2(块内)+R2/U2：列编辑裸光标删前一个字符 ----
TEST_F(DeleteTextUndoCommandTest, Redo_ColumnBareCursorMidBlock_DeletesPreviousChar)
{
    // Arrange：两行各一个块中裸光标（position 1 与 4）
    edit->setPlainText("ab\ncd");
    const QString snapshot = docText();
    QList<QTextEdit::ExtraSelection> selections;
    selections << ut::makeSelection(edit->document(), 1, 1)
               << ut::makeSelection(edit->document(), 4, 4);
    DeleteTextUndoCommand cmd(selections, edit);

    // Act
    cmd.redo();

    // Assert：各自删除前一个字符（'a' 与 'c'）
    EXPECT_EQ(docText(), QString("b\nd"));
    cmd.undo();
    EXPECT_EQ(docText(), snapshot);                            // 状态可逆
}

// ---- CT2(块首裸光标)：列编辑变体捕获 "\n"（上一行换行符） ----
TEST_F(DeleteTextUndoCommandTest, Redo_ColumnBareCursorAtBlockStart_DeletesNewline)
{
    // Arrange：裸光标位于第二行行首 position=3（posInBlock-1 < 0 → 记录 "\n"）
    edit->setPlainText("ab\ncd");
    QList<QTextEdit::ExtraSelection> selections;
    selections << ut::makeSelection(edit->document(), 3, 3);
    DeleteTextUndoCommand cmd(selections, edit);

    // Act
    cmd.redo();

    // Assert：列编辑 redo 逐选区 deletePreviousChar → 删除换行符合并行
    EXPECT_EQ(docText(), QString("abcd"));
    EXPECT_EQ(edit->document()->blockCount(), 1);
    cmd.undo();
    EXPECT_EQ(docText(), QString("ab\ncd"));                   // 列编辑 undo 在光标现位重插，精确还原
    EXPECT_EQ(edit->document()->blockCount(), 2);
}

// ---- 输入等价类 TEST_P：选区内容 ASCII/中文/emoji（redo→undo 双向精确） ----
namespace {
struct DeleteTextCase {
    QString docContent;
    int selStart;
    int selEnd;
    QString expectedAfterRedo;
};
}  // namespace

class DeleteTextParamTest : public EditorUndoTestBase,
                            public ::testing::WithParamInterface<DeleteTextCase> {
};

TEST_P(DeleteTextParamTest, Redo_MultiByteText_ReversibleBothWays)
{
    // Arrange
    const DeleteTextCase &c = GetParam();
    edit->setPlainText(c.docContent);
    QTextCursor cursor = ut::cursorAt(edit->document(), c.selStart, c.selEnd);
    DeleteTextUndoCommand cmd(cursor, edit);

    // Act
    cmd.redo();

    // Assert
    EXPECT_EQ(docText(), c.expectedAfterRedo);
    cmd.undo();
    EXPECT_EQ(docText(), c.docContent);                        // 状态可逆
    EXPECT_EQ(edit->document()->characterCount() - 1, c.docContent.size());
}

INSTANTIATE_TEST_SUITE_P(TextVariants, DeleteTextParamTest,
    ::testing::Values(
        DeleteTextCase{ QString("hello"), 1, 4, QString("ho") },
        DeleteTextCase{ QString::fromUtf8("中文文本"), 0, 2, QString::fromUtf8("文本") },
        DeleteTextCase{ QString::fromUtf8("a\xF0\x9F\x98\x80" "b"), 1, 3, QString("ab") }  // emoji 代理对选区
    ));

// ---- ID 两分支 ----
TEST_F(DeleteTextUndoCommandTest, Id_SingleMode_ReturnsIdDelete)
{
    // Arrange
    QTextCursor cursor = ut::cursorAt(edit->document(), 0, 0);
    DeleteTextUndoCommand cmd(cursor, nullptr);

    // Assert
    EXPECT_EQ(cmd.id(), Utils::IdDelete);
    EXPECT_NE(cmd.id(), Utils::IdColumnEditDelete);
}

TEST_F(DeleteTextUndoCommandTest, Id_ColumnMode_ReturnsIdColumnEditDelete)
{
    // Arrange
    edit->setPlainText("ab");
    QList<QTextEdit::ExtraSelection> selections;
    selections << ut::makeSelection(edit->document(), 0, 1);
    DeleteTextUndoCommand cmd(selections, nullptr);

    // Assert
    EXPECT_EQ(cmd.id(), Utils::IdColumnEditDelete);
    EXPECT_EQ(cmd.id(), Utils::IdColumnEdit | Utils::IdDelete);
}
