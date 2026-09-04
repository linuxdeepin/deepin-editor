// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// DeleteBackCommand 单元测试（src/editor/deletebackcommond.cpp）。
// 退格/删除键的向后删除撤销项（单光标，m_edit 为 QPlainTextEdit*，undo/redo 无空判断，
// 必须传入真实控件——使用空壳 TextEdit，其行为等价 DPlainTextEdit）。
//
// 分支清单：
// CT: 构造 —— m_delText = cursor.selectedText()（选区/无选区）；delPos = min(anchor,position)
// R1: redo —— 选中 [delPos, delPos+size) 并 deleteChar；m_edit->setTextCursor
// U1: undo —— 在 insertPos 重新插入 delText；编辑器光标选中恢复的文本
//
// 用例映射：
// - Redo_WithSelection_DeletesAndUndoRestoresWithSelection   → CT/R1/U1（有选区，可逆主路径）
// - Redo_NoSelection_DocumentesForwardDeleteSemantics        → CT/R1/U1（无选区边界：
//     delText 为空但 redo 的 deleteChar 在无选区时前向删除一个字符——记录真实语义）
// - Redo_SelectionAtDocumentStart_DeletesPrefix              → CT（delPos=0 边界）
// - Redo_MultiByteSelection_ReversibleBothWays               → 中文选区（多字节等价类）
// - Redo_RepeatCycles_StaysReversible                        → 多轮 redo/undo 幂等
//
// 最小清单自检：1 每公开方法 ≥1 用例（ctor/dtor/undo/redo，dtor 由作用域析构覆盖）✔
// 2 输入等价类（有/无选区、ASCII/中文）✔ 3 边界（文档首、无选区）✔ 4 同质多组并入
// 多字节用例（断言逻辑不同的不参数化）✔ 5 分支映射 ✔ 6 全分支 ✔ 7 无异常路径 ✔
// 8 负面（无选区）✔ 9 双向可逆断言 ✔ 10 空壳接缝无 gMock ✔

#include "editor_undo_helpers.h"

#include "../../../src/editor/deletebackcommond.h"

class DeleteBackCommandTest : public EditorUndoTestBase
{
};

// ---- CT/R1/U1 主路径：选中文本删除且 undo 精确恢复（含选中态） ----
TEST_F(DeleteBackCommandTest, Redo_WithSelection_DeletesAndUndoRestoresWithSelection)
{
    // Arrange：选中 "world"
    edit->setPlainText("hello world");
    const QString snapshot = docText();
    QTextCursor cursor = ut::cursorAt(edit->document(), 6, 11);
    DeleteBackCommand cmd(cursor, edit);

    // Act
    cmd.redo();

    // Assert
    EXPECT_EQ(docText(), QString("hello "));                   // 选区被删除
    EXPECT_EQ(edit->textCursor().position(), 6);               // 删除后光标塌缩到删除点

    // Act & Assert：undo 在原位重插并重新选中
    cmd.undo();
    EXPECT_EQ(docText(), snapshot);                            // 状态可逆
    EXPECT_EQ(ut::toLf(edit->textCursor().selectedText()), QString("world"));  // 恢复选中态
}

// ---- CT/R1/U1 无选区边界：delText 为空；redo 的 deleteChar 前向删除一个字符（真实语义） ----
TEST_F(DeleteBackCommandTest, Redo_NoSelection_DocumentsForwardDeleteSemantics)
{
    // Arrange：光标 position=2，无选区
    edit->setPlainText("abc");
    QTextCursor cursor = ut::cursorAt(edit->document(), 2, 2);
    DeleteBackCommand cmd(cursor, edit);

    // Act
    cmd.redo();

    // Assert：QTextCursor::deleteChar 在无选区时删除光标处字符（'c'）
    EXPECT_EQ(docText(), QString("ab"));
    EXPECT_EQ(edit->textCursor().position(), 2);

    // Act & Assert：undo 插入空串，文档保持删除后状态（构造期未捕获字符，此为源码语义）
    cmd.undo();
    EXPECT_EQ(docText(), QString("ab"));
    EXPECT_EQ(edit->textCursor().position(), 2);               // 光标稳定
}

// ---- CT delPos=0 边界：文档首部选区 ----
TEST_F(DeleteBackCommandTest, Redo_SelectionAtDocumentStart_DeletesPrefix)
{
    // Arrange：选中 "he"
    edit->setPlainText("hello");
    QTextCursor cursor = ut::cursorAt(edit->document(), 0, 2);
    DeleteBackCommand cmd(cursor, edit);

    // Act
    cmd.redo();

    // Assert
    EXPECT_EQ(docText(), QString("llo"));
    EXPECT_EQ(edit->textCursor().position(), 0);               // 删除点为文档首
    cmd.undo();
    EXPECT_EQ(docText(), QString("hello"));                    // 状态可逆
    EXPECT_EQ(ut::toLf(edit->textCursor().selectedText()), QString("he"));
}

// ---- 中文选区（多字节等价类）双向可逆 ----
TEST_F(DeleteBackCommandTest, Redo_MultiByteSelection_ReversibleBothWays)
{
    // Arrange：选中 "中文"（2 个 QChar）
    edit->setPlainText(QString::fromUtf8("中文abc"));
    QTextCursor cursor = ut::cursorAt(edit->document(), 0, 2);
    DeleteBackCommand cmd(cursor, edit);

    // Act
    cmd.redo();

    // Assert
    EXPECT_EQ(docText(), QString("abc"));
    cmd.undo();
    EXPECT_EQ(docText(), QString::fromUtf8("中文abc"));        // 状态可逆（多字节）
    EXPECT_EQ(ut::toLf(edit->textCursor().selectedText()), QString::fromUtf8("中文"));
}

// ---- 多轮 redo/undo 幂等性 ----
TEST_F(DeleteBackCommandTest, Redo_RepeatCycles_StaysReversible)
{
    // Arrange
    edit->setPlainText("0123456789");
    QTextCursor cursor = ut::cursorAt(edit->document(), 3, 6);  // "345"
    DeleteBackCommand cmd(cursor, edit);

    // Act & Assert：三轮循环，每轮结束都应精确回到初始文本
    for (int round = 0; round < 3; ++round) {
        cmd.redo();
        EXPECT_EQ(docText(), QString("0126789")) << "round " << round;
        cmd.undo();
        EXPECT_EQ(docText(), QString("0123456789")) << "round " << round;
    }
}

// ---- D1：经基类指针虚析构删除（deleting destructor 路径），文档状态不受影响 ----
TEST_F(DeleteBackCommandTest, Destructor_DeleteViaBasePointer_ReleasesCleanly)
{
    // Arrange：堆上构造并执行一次 redo 建立有效状态
    edit->setPlainText("hello");
    QTextCursor cursor = ut::cursorAt(edit->document(), 0, 2);
    QUndoCommand *cmd = new DeleteBackCommand(cursor, edit);
    cmd->redo();
    EXPECT_EQ(docText(), QString("llo"));                      // 前提：命令已生效

    // Act：经 QUndoCommand* 虚析构删除（虚析构分派 → deleting dtor 路径）
    delete cmd;

    // Assert：析构仅释放命令对象，不触碰文档
    EXPECT_EQ(docText(), QString("llo"));
}
