// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// ReplaceAllCommand 单元测试（src/editor/replaceallcommond.cpp）。
// 全部替换撤销项：redo/undo 均为“全选删除 + 插入”，完全对称。
//
// 分支清单：
// R1: redo —— setPosition(0) + End KeepAnchor → deleteChar → insertText(m_newText)
// U1: undo —— 同构，insertText(m_oldText)
// （两方法无条件分支；输入维度为 old/new 文本内容）
//
// 用例映射：
// - Redo_ReplacesEntireDocument_UndoRestoresOriginal    → R1/U1 主路径
// - Redo_EmptyNewText_ClearsDocument                    → R1（newText 空边界）
// - Redo_EmptyOldTextOnEmptyDoc_UndoClearsAgain         → U1（oldText 空边界）
// - Redo_TextVariants_RoundTripExact（TEST_P）          → ASCII/中文/emoji 等价类
// - Redo_RepeatCycles_StaysReversible                   → 多轮幂等
//
// 最小清单自检：1 每公开方法 ≥1 用例（ctor/dtor/redo/undo）✔ 2 输入等价类 ✔
// 3 边界（空文本/空文档）✔ 4 TEST_P ≥3 组 ✔ 5 分支映射 ✔ 6 全分支 ✔
// 7 无异常路径 ✔ 8 负面（空文本）✔ 9 双向可逆断言 ✔ 10 无 gMock ✔

#include "editor_undo_helpers.h"

#include "../../../src/editor/replaceallcommond.h"

class ReplaceAllCommandTest : public EditorUndoTestBase
{
};

// ---- R1/U1 主路径：整文档替换与还原 ----
TEST_F(ReplaceAllCommandTest, Redo_ReplacesEntireDocument_UndoRestoresOriginal)
{
    // Arrange：光标绑定编辑器文档
    edit->setPlainText("foo bar foo");
    QString oldText = docText();
    QString newText("baz");
    ReplaceAllCommand cmd(oldText, newText, edit->textCursor());

    // Act
    cmd.redo();

    // Assert：文档整体替换为新文本
    EXPECT_EQ(docText(), QString("baz"));
    EXPECT_EQ(edit->document()->characterCount() - 1, 3);

    // Act & Assert：undo 精确还原
    cmd.undo();
    EXPECT_EQ(docText(), QString("foo bar foo"));              // 状态可逆
    EXPECT_EQ(edit->document()->characterCount() - 1, 11);
}

// ---- R1 空 newText 边界：清空文档 ----
TEST_F(ReplaceAllCommandTest, Redo_EmptyNewText_ClearsDocument)
{
    // Arrange
    edit->setPlainText("content");
    QString oldText = docText();
    QString newText("");
    ReplaceAllCommand cmd(oldText, newText, edit->textCursor());

    // Act
    cmd.redo();

    // Assert
    EXPECT_EQ(docText(), QString(""));
    EXPECT_EQ(edit->document()->characterCount() - 1, 0);      // 仅剩末尾段落符
    cmd.undo();
    EXPECT_EQ(docText(), QString("content"));                  // 状态可逆
}

// ---- U1 空 oldText 边界：空文档起步，undo 清回空 ----
TEST_F(ReplaceAllCommandTest, Redo_EmptyOldTextOnEmptyDoc_UndoClearsAgain)
{
    // Arrange：空文档（oldText 为空）
    QString oldText("");
    QString newText("inserted");
    ReplaceAllCommand cmd(oldText, newText, edit->textCursor());

    // Act
    cmd.redo();

    // Assert：空文档上插入新文本
    EXPECT_EQ(docText(), QString("inserted"));
    cmd.undo();
    EXPECT_EQ(docText(), QString(""));                         // 状态可逆：清回空
    EXPECT_EQ(edit->document()->characterCount() - 1, 0);
}

// ---- 输入等价类 TEST_P：ASCII/中文/emoji（含代理对）整替往返 ----
namespace {
struct ReplaceAllCase {
    QString original;
    QString replacement;
};
}  // namespace

class ReplaceAllParamTest : public EditorUndoTestBase,
                            public ::testing::WithParamInterface<ReplaceAllCase> {
};

TEST_P(ReplaceAllParamTest, Redo_TextVariants_RoundTripExact)
{
    // Arrange
    const ReplaceAllCase &c = GetParam();
    edit->setPlainText(c.original);
    QString oldText = docText();
    QString newText = c.replacement;                           // 构造参数为非 const 引用
    ReplaceAllCommand cmd(oldText, newText, edit->textCursor());

    // Act
    cmd.redo();

    // Assert
    EXPECT_EQ(docText(), c.replacement);
    EXPECT_EQ(edit->document()->characterCount() - 1, c.replacement.size());

    cmd.undo();
    EXPECT_EQ(docText(), c.original);                          // 多字节往返精确
    EXPECT_EQ(edit->document()->characterCount() - 1, c.original.size());
}

INSTANTIATE_TEST_SUITE_P(TextVariants, ReplaceAllParamTest,
    ::testing::Values(
        ReplaceAllCase{ QString("abcabc"), QString("xyz") },
        ReplaceAllCase{ QString::fromUtf8("你好，世界"), QString::fromUtf8("再见") },
        ReplaceAllCase{ QString::fromUtf8("old\xF0\x9F\x98\x80"), QString::fromUtf8("new\xF0\x9F\x8E\x89") }
    ));

// ---- 多轮幂等 ----
TEST_F(ReplaceAllCommandTest, Redo_RepeatCycles_StaysReversible)
{
    // Arrange
    edit->setPlainText("0123456789");
    QString oldText = docText();
    QString newText("X");
    ReplaceAllCommand cmd(oldText, newText, edit->textCursor());

    // Act & Assert
    for (int round = 0; round < 3; ++round) {
        cmd.redo();
        EXPECT_EQ(docText(), QString("X")) << "round " << round;
        cmd.undo();
        EXPECT_EQ(docText(), QString("0123456789")) << "round " << round;
    }
}

// ---- D1：经基类指针虚析构删除（deleting destructor 路径） ----
TEST_F(ReplaceAllCommandTest, Destructor_DeleteViaBasePointer_ReleasesCleanly)
{
    // Arrange
    edit->setPlainText("origin");
    QString oldText = docText();
    QString newText("n");
    QUndoCommand *cmd = new ReplaceAllCommand(oldText, newText, edit->textCursor());
    cmd->redo();
    EXPECT_EQ(docText(), QString("n"));                        // 前提：命令已生效

    // Act
    delete cmd;

    // Assert：析构不改变文档状态
    EXPECT_EQ(docText(), QString("n"));
}
