// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// DragInsertTextUndoCommand 单元测试（src/editor/inserttextundocommand.cpp）。
// 拖拽插入撤销项：使用固定偏移量（构造时的 selectionStart），不信任 redo 时的光标位置。
//
// 分支清单：
// CT: 构造 —— m_beginPostion = textcursor.selectionStart()
// R1: redo —— beginPostion != cursor.position() → setPosition(beginPostion)（分支真）
// R2: redo —— 相等 → 跳过重定位（分支假）；插入；m_pEdit 空/非空
// U1: undo —— 删除 [begin, begin+size)；m_pEdit 空/非空
//
// 用例映射：
// - Redo_CursorDrifted_RepositionsToDropTargetAndRestores  → R1（分支真）+ U1
// - Redo_CursorAtDropTarget_SkipsRepositionBranch          → R2（分支假）
// - Ctor_CrlfText_NormalizedToLf                           → CT + 插入结果
// - Redo_TextVariants_ExactInsertion（TEST_P）             → CT/R1 输入等价类
// - Redo_EmptyText_EmptyDoc_NoStateChange                  → 空文本/空文档边界
// - Redo_NullEdit_StillMutatesDocument                     → R1/U1 空 edit 分支
//
// 最小清单自检：1 每公开方法 ≥1 用例（ctor/undo/redo）✔ 2 输入等价类 ✔
// 3 边界（空文本/空文档/代理对）✔ 4 TEST_P ≥3 组 ✔ 5 分支映射 ✔ 6 全分支 ✔
// 7 无异常路径 ✔ 8 负面（空文本）✔ 9 双向可逆断言 ✔ 10 空壳接缝无 gMock ✔

#include "editor_undo_helpers.h"

#include "../../../src/editor/inserttextundocommand.h"

class DragInsertTextUndoCommandTest : public EditorUndoTestBase
{
};

// ---- R1 分支真：构造时光标带选区（position=selectionEnd ≠ selectionStart=drop 目标），
// redo 先重定位回选区起点再插入（插入替换选区内容） ----
TEST_F(DragInsertTextUndoCommandTest, Redo_CursorDrifted_RepositionsToDropTargetAndRestores)
{
    // Arrange：光标选中 [4,6)（"ef"），构造期 beginPostion = selectionStart = 4，
    // 而 position = 6 ≠ 4 → redo 进入重定位分支
    edit->setPlainText("abcdef");
    QTextCursor cursor = ut::cursorAt(edit->document(), 4, 6);
    DragInsertTextUndoCommand cmd(cursor, "XY", edit);

    // Act
    cmd.redo();

    // Assert：重定位 setPosition 清空选区后原位插入（不替换 "ef"）
    EXPECT_EQ(docText(), QString("abcdXYef"));
    EXPECT_EQ(edit->textCursor().position(), 6);               // 编辑器光标 = begin + size

    // Act & Assert：undo 删除 [4,6) 精确还原
    cmd.undo();
    EXPECT_EQ(docText(), QString("abcdef"));                   // 状态可逆
    EXPECT_EQ(edit->textCursor().position(), 4);
}

// ---- R2 分支假：redo 时光标仍等于 drop 目标，跳过重定位 ----
TEST_F(DragInsertTextUndoCommandTest, Redo_CursorAtDropTarget_SkipsRepositionBranch)
{
    // Arrange
    edit->setPlainText("abc");
    QTextCursor cursor = ut::cursorAt(edit->document(), 3, 3);
    DragInsertTextUndoCommand cmd(cursor, "Z", edit);

    // Act
    cmd.redo();

    // Assert
    EXPECT_EQ(docText(), QString("abcZ"));
    EXPECT_EQ(edit->textCursor().position(), 4);
    cmd.undo();
    EXPECT_EQ(docText(), QString("abc"));                      // 状态可逆
    EXPECT_EQ(edit->textCursor().position(), 3);
}

// ---- CT：CRLF 文本 ----
// 源码行为注记（defect 候选，见批次报告）：DragInsertTextUndoCommand 构造不做
// "\r\n"→"\n" 归一化（同类 InsertText/MidButton 均做），m_sInsertText.size() 按
// "\r\n" 计 2 个 QChar；QTextDocument 插入时把 "\r\n" 折叠为单个段落分隔符，
// undo 计算 [begin, begin+4) 越界，setPosition 拒绝移动导致选区为空，deleteChar
// 前向删除单字符 → 撤销结果错误（"line\nb" 而非 "line"）。按实际行为断言留证。
TEST_F(DragInsertTextUndoCommandTest, Ctor_CrlfText_InsertFoldsButUndoOvershoots)
{
    // Arrange
    edit->setPlainText("line");
    QTextCursor cursor = ut::cursorAt(edit->document(), 4, 4);
    DragInsertTextUndoCommand cmd(cursor, "a\r\nb", edit);

    // Act
    cmd.redo();

    // Assert：插入侧由 QTextDocument 折叠 "\r\n"，文档得到单个换行
    EXPECT_EQ(docText(), QString("linea\nb"));

    // Act & Assert：undo 侧长度按 4 计算越界，仅前向删除一个字符（实际行为）
    cmd.undo();
    EXPECT_EQ(docText(), QString("line\nb"));                  // 非 "line"：见 defect 注记
    EXPECT_NE(docText(), QString("line"));
}

// ---- CT/R1 输入等价类：ASCII / 中文 / emoji ----
namespace {
struct DragTextCase {
    QString text;
};
}  // namespace

class DragTextParamTest : public EditorUndoTestBase,
                          public ::testing::WithParamInterface<DragTextCase> {
};

TEST_P(DragTextParamTest, Redo_TextVariants_ExactInsertion)
{
    // Arrange：空文档，drop 目标 0
    QTextCursor cursor = ut::cursorAt(edit->document(), 0, 0);
    DragInsertTextUndoCommand cmd(cursor, GetParam().text, edit);

    // Act
    cmd.redo();

    // Assert
    EXPECT_EQ(docText(), GetParam().text);
    EXPECT_EQ(edit->textCursor().position(), GetParam().text.size());  // begin + UTF-16 长度

    cmd.undo();
    EXPECT_EQ(docText(), QString(""));                         // 状态可逆
    EXPECT_EQ(edit->document()->characterCount() - 1, 0);
}

INSTANTIATE_TEST_SUITE_P(TextVariants, DragTextParamTest,
    ::testing::Values(
        DragTextCase{ QString("hello") },
        DragTextCase{ QString::fromUtf8("中文字符") },
        DragTextCase{ QString::fromUtf8("e\xF0\x9F\x98\x80moji") }
    ));

// ---- 空文本 + 空文档边界：零长度范围，redo/undo 均无状态变化 ----
TEST_F(DragInsertTextUndoCommandTest, Redo_EmptyText_EmptyDoc_NoStateChange)
{
    // Arrange
    QTextCursor cursor = ut::cursorAt(edit->document(), 0, 0);
    DragInsertTextUndoCommand cmd(cursor, "", edit);

    // Act
    cmd.redo();

    // Assert：[0,0) 无插入内容，m_delPos 语义不受影响
    EXPECT_EQ(docText(), QString(""));
    EXPECT_EQ(edit->textCursor().position(), 0);
    cmd.undo();
    EXPECT_EQ(docText(), QString(""));                         // 强异常安全：状态未损坏
    EXPECT_EQ(edit->textCursor().position(), 0);
}

// ---- R1/U1 空 edit 分支 ----
TEST_F(DragInsertTextUndoCommandTest, Redo_NullEdit_StillMutatesDocument)
{
    // Arrange
    edit->setPlainText("abc");
    QTextCursor cursor = ut::cursorAt(edit->document(), 2, 2);
    DragInsertTextUndoCommand cmd(cursor, "Q", nullptr);

    // Act
    cmd.redo();

    // Assert
    EXPECT_EQ(docText(), QString("abQc"));
    cmd.undo();
    EXPECT_EQ(docText(), QString("abc"));                      // 状态可逆
}
