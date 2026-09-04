// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// DeleteBackAltCommand 单元测试（src/editor/deletebackcommond.cpp）。
// 列编辑模式退格/删除撤销项。私有 DelNode 链表（空/单/多节点）经公开 ctor/redo/undo
// 间接覆盖（test_writer §4.0 第 5 条：private 逻辑必须经 public 路径覆盖分支）。
//
// 分支清单（ctor 循环体内）：
// B1: cursor.hasSelection() 真 → 直接取 selectedText
// B2: 假 && backward=true  → !atEnd 时向右扩展 1 字符
// B3: 假 && backward=false → !atStart 时向左扩展 1 字符（backspace）
// B4: 扩展后 text 为空（atStart 且 backspace / atEnd 且 delete）→ 不生成 DelNode（空链）
// B5: text 非空 → 生成 DelNode（delPos 含 sum 偏移累计）；leftToRight = anchor<position
// R: redo —— 逐节点 setPosition(delPos) 后 deleteChar × size；restoreColumnEditSelection
// U: undo —— 逐节点 insertPos 重插 delText；idInColumn 越界判断；方向恢复；restore
// ID: id() → Utils::IdColumnEditDelete
//
// 用例映射：
// - Redo_MultiSelections_DeletesAllThenUndoRestores       → B1×3 + R/U（三节点链）+ 副作用断言
// - Redo_MixedSelectionAndBareCursor_BuildsBothNodeKinds  → B1 + B3 混合（sum 偏移正确性）
// - Ctor_BareCursorBackspace_ExtendsBackwardChar          → B3（backspace 单字符）
// - Ctor_BareCursorDelete_ExtendsForwardChar              → B2（delete 单字符）
// - Ctor_CursorAtStartBackspace_EmptyNodeChainNoop        → B4（atStart 空链）
// - Ctor_CursorAtEndDelete_EmptyNodeChainNoop             → B4（atEnd 空链）
// - Redo_RightToLeftNode_KeepsOrientationOnUndo           → B5 leftToRight=false + U 方向恢复
// - Redo_MultiByteSelections_ReversibleBothWays           → B1 中文等价类
// - Redo_NullEdit_StillMutatesDocument                    → R/U 的 m_edit 空分支
// - Id_ColumnDelete_ReturnsIdColumnEditDelete             → ID
//
// 最小清单自检：1 每公开方法 ≥1 用例（ctor/dtor/undo/redo/id）✔ 2 输入等价类（选区/
// 裸光标×方向/多字节）✔ 3 边界（atStart/atEnd/单节点/三节点/空链）✔ 4 同质多组以
// 场景用例呈现（断言逻辑各异）✔ 5 分支映射 ✔ 6 全分支 ✔ 7 无异常路径 ✔
// 8 负面（空链 no-op）✔ 9 双向可逆 + 强异常安全 ✔ 10 空壳接缝无 gMock ✔

#include "editor_undo_helpers.h"

#include "../../../src/editor/deletebackcommond.h"

#include "../../../src/common/utils.h"

class DeleteBackAltCommandTest : public EditorUndoTestBase
{
};

// ---- B1×3 + R/U：三选区全删、undo 全恢复 + restoreColumnEditSelection 副作用 ----
TEST_F(DeleteBackAltCommandTest, Redo_MultiSelections_DeletesAllThenUndoRestores)
{
    // Arrange：三行各选中整行内容（aa/bb/cc）
    edit->setPlainText("aa\nbb\ncc");
    const QString snapshot = docText();
    QList<QTextEdit::ExtraSelection> selections;
    selections << ut::makeSelection(edit->document(), 0, 2)
               << ut::makeSelection(edit->document(), 3, 5)
               << ut::makeSelection(edit->document(), 6, 8);
    DeleteBackAltCommand cmd(selections, edit);

    // Act
    cmd.redo();

    // Assert：三个 DelNode 按 delPos 偏移累计逐字符删除
    EXPECT_EQ(docText(), QString("\n\n"));                     // 行内容全删，仅剩换行
    EXPECT_EQ(seam.restoreColumnEditSelectionCalls, 1);        // redo 末尾恢复列选区状态

    // Act & Assert：undo 依 insertPos 重插全部文本
    cmd.undo();
    EXPECT_EQ(docText(), snapshot);                            // 状态可逆
    EXPECT_EQ(seam.restoreColumnEditSelectionCalls, 2);        // undo 同样恢复列选区
    ASSERT_EQ(seam.lastRestoredSelections.size(), 3);
    EXPECT_EQ(ut::toLf(seam.lastRestoredSelections[0].cursor.selectedText()), QString("aa"));
    EXPECT_EQ(ut::toLf(seam.lastRestoredSelections[2].cursor.selectedText()), QString("cc"));
}

// ---- B1 + B3 混合：真实选区 + 裸光标（backspace 扩展），验证 sum 偏移 ----
TEST_F(DeleteBackAltCommandTest, Redo_MixedSelectionAndBareCursor_BuildsBothNodeKinds)
{
    // Arrange：sel0 裸光标 position=1（backspace 扩展为 'a'）；sel1 选 "d"
    edit->setPlainText("ab\ncd");
    const QString snapshot = docText();
    QList<QTextEdit::ExtraSelection> selections;
    selections << ut::makeSelection(edit->document(), 1, 1)    // 无选区，backward=false
               << ut::makeSelection(edit->document(), 3, 4);
    DeleteBackAltCommand cmd(selections, edit, false);

    // Act
    cmd.redo();

    // Assert：节点0 删 'a'（delPos=0），节点1 delPos=3-1=2（sum 偏移）删 'c'
    EXPECT_EQ(docText(), QString("b\nd"));
    cmd.undo();
    EXPECT_EQ(docText(), snapshot);                            // 状态可逆
}

// ---- B3：backspace 裸光标向左扩展单字符 ----
TEST_F(DeleteBackAltCommandTest, Ctor_BareCursorBackspace_ExtendsBackwardChar)
{
    // Arrange：光标 position=2，backward=false（backspace 语义）
    edit->setPlainText("abc");
    QList<QTextEdit::ExtraSelection> selections;
    selections << ut::makeSelection(edit->document(), 2, 2);
    DeleteBackAltCommand cmd(selections, edit, false);

    // Act
    cmd.redo();

    // Assert：向左扩展选中 'b' 并删除
    EXPECT_EQ(docText(), QString("ac"));
    cmd.undo();
    EXPECT_EQ(docText(), QString("abc"));                      // 状态可逆
    EXPECT_EQ(ut::toLf(seam.lastRestoredSelections[0].cursor.selectedText()), QString("b"));
}

// ---- B2：delete 裸光标向右扩展单字符 ----
TEST_F(DeleteBackAltCommandTest, Ctor_BareCursorDelete_ExtendsForwardChar)
{
    // Arrange：光标 position=1，backward=true（delete 语义）
    edit->setPlainText("abc");
    QList<QTextEdit::ExtraSelection> selections;
    selections << ut::makeSelection(edit->document(), 1, 1);
    DeleteBackAltCommand cmd(selections, edit, true);

    // Act
    cmd.redo();

    // Assert：向右扩展选中 'b' 并删除
    EXPECT_EQ(docText(), QString("ac"));
    cmd.undo();
    EXPECT_EQ(docText(), QString("abc"));                      // 状态可逆
}

// ---- B4 atStart 边界：backspace 于块首无法扩展 → 空链，redo/undo 均无操作 ----
TEST_F(DeleteBackAltCommandTest, Ctor_CursorAtStartBackspace_EmptyNodeChainNoop)
{
    // Arrange：光标 position=0（文档首即块首）
    edit->setPlainText("ab");
    QList<QTextEdit::ExtraSelection> selections;
    selections << ut::makeSelection(edit->document(), 0, 0);
    DeleteBackAltCommand cmd(selections, edit, false);

    // Act
    cmd.redo();

    // Assert：未生成 DelNode，文档无变化（空链边界）
    EXPECT_EQ(docText(), QString("ab"));
    EXPECT_EQ(seam.restoreColumnEditSelectionCalls, 1);        // 列选区状态仍被恢复

    // Act & Assert：undo 对空链同样无操作（强异常安全：状态未损坏）
    cmd.undo();
    EXPECT_EQ(docText(), QString("ab"));
}

// ---- B4 atEnd 边界：delete 于文档尾无法扩展 → 空链 ----
TEST_F(DeleteBackAltCommandTest, Ctor_CursorAtEndDelete_EmptyNodeChainNoop)
{
    // Arrange：光标 position=2（文档尾）
    edit->setPlainText("ab");
    QList<QTextEdit::ExtraSelection> selections;
    selections << ut::makeSelection(edit->document(), 2, 2);
    DeleteBackAltCommand cmd(selections, edit, true);

    // Act
    cmd.redo();

    // Assert
    EXPECT_EQ(docText(), QString("ab"));                       // 空链无删除
    cmd.undo();
    EXPECT_EQ(docText(), QString("ab"));                       // 状态未损坏
}

// ---- B5 leftToRight=false：反向选区的 DelNode 在 undo 后保持方向 ----
TEST_F(DeleteBackAltCommandTest, Redo_RightToLeftNode_KeepsOrientationOnUndo)
{
    // Arrange：反向选区 anchor=4 position=1（选中 "ell"）
    edit->setPlainText("hello");
    QList<QTextEdit::ExtraSelection> selections;
    selections << ut::makeSelection(edit->document(), 4, 1);
    DeleteBackAltCommand cmd(selections, edit);

    // Act
    cmd.redo();

    // Assert
    EXPECT_EQ(docText(), QString("ho"));
    cmd.undo();
    EXPECT_EQ(docText(), QString("hello"));                    // 状态可逆
    ASSERT_EQ(seam.lastRestoredSelections.size(), 1);
    const QTextCursor &restored = seam.lastRestoredSelections[0].cursor;
    EXPECT_GT(restored.anchor(), restored.position());         // 方向恢复：anchor(4) > position(1)
    EXPECT_EQ(ut::toLf(restored.selectedText()), QString("ell"));
}

// ---- B1 中文等价类：多字节选区双向可逆 ----
TEST_F(DeleteBackAltCommandTest, Redo_MultiByteSelections_ReversibleBothWays)
{
    // Arrange：两行中文，各选中全部字符
    edit->setPlainText(QString::fromUtf8("你好\n世界"));
    const QString snapshot = docText();
    QList<QTextEdit::ExtraSelection> selections;
    selections << ut::makeSelection(edit->document(), 0, 2)
               << ut::makeSelection(edit->document(), 3, 5);
    DeleteBackAltCommand cmd(selections, edit);

    // Act
    cmd.redo();

    // Assert
    EXPECT_EQ(docText(), QString("\n"));
    cmd.undo();
    EXPECT_EQ(docText(), snapshot);                            // 多字节状态可逆
    EXPECT_EQ(ut::toLf(seam.lastRestoredSelections[1].cursor.selectedText()),
              QString::fromUtf8("世界"));
}

// ---- R/U 的 m_edit 空分支：文档仍按节点修改，不触碰列选区恢复 ----
TEST_F(DeleteBackAltCommandTest, Redo_NullEdit_StillMutatesDocument)
{
    // Arrange
    edit->setPlainText("abc");
    QList<QTextEdit::ExtraSelection> selections;
    selections << ut::makeSelection(edit->document(), 0, 1);
    DeleteBackAltCommand cmd(selections, nullptr);

    // Act
    cmd.redo();

    // Assert
    EXPECT_EQ(docText(), QString("bc"));
    EXPECT_EQ(seam.restoreColumnEditSelectionCalls, 0);        // 空 edit 不触发列选区恢复
    cmd.undo();
    EXPECT_EQ(docText(), QString("abc"));                      // 状态可逆
    EXPECT_EQ(seam.restoreColumnEditSelectionCalls, 0);
}

// ---- ID ----
TEST_F(DeleteBackAltCommandTest, Id_ColumnDelete_ReturnsIdColumnEditDelete)
{
    // Arrange
    edit->setPlainText("ab");
    QList<QTextEdit::ExtraSelection> selections;
    selections << ut::makeSelection(edit->document(), 0, 1);
    DeleteBackAltCommand cmd(selections, edit);

    // Assert
    EXPECT_EQ(cmd.id(), Utils::IdColumnEditDelete);            // IdColumnEdit | IdDelete
    EXPECT_EQ(cmd.id(), Utils::IdColumnEdit | Utils::IdDelete);
}

// ---- D1：经基类指针虚析构删除（deleting destructor 路径） ----
TEST_F(DeleteBackAltCommandTest, Destructor_DeleteViaBasePointer_ReleasesCleanly)
{
    // Arrange
    edit->setPlainText("ab");
    QList<QTextEdit::ExtraSelection> selections;
    selections << ut::makeSelection(edit->document(), 0, 1);
    QUndoCommand *cmd = new DeleteBackAltCommand(selections, edit);
    cmd->redo();
    EXPECT_EQ(docText(), QString("b"));                        // 前提：命令已生效

    // Act
    delete cmd;

    // Assert：析构后文档保持命令执行后的状态
    EXPECT_EQ(docText(), QString("b"));
    EXPECT_EQ(seam.restoreColumnEditSelectionCalls, 1);        // 析构不再触发列选区恢复
}
