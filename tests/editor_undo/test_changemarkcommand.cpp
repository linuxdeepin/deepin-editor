// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// ChangeMarkCommand 单元测试（src/editor/changemarkcommand.cpp）。
// 颜色标记变更撤销项：undo/redo 先执行子命令（QUndoCommand 基类），再经
// convertReplaceToMark + manualUpdateAllMark（均为接缝记录桩）刷新标记。
//
// 分支清单：
// U1: undo —— QUndoCommand::undo()（子命令逆序）；m_EditPtr 空 / 旧标记空 → 跳过；
//     否则 convert(m_oldMarkReplace) → manualUpdateAllMark
// R1: redo —— 同构，使用 m_newMarkReplace
//
// 用例映射：
// - Redo_NewMarks_ConvertedAndApplied                   → R1(正常)
// - Undo_OldMarks_ConvertedAndApplied                   → U1(正常)（含 redo 后回退）
// - Redo_NullEdit_SkipsMarkUpdate                       → R1/U1(m_EditPtr 空)
// - Redo_EmptyMarkLists_SkipsMarkUpdate                 → R1/U1(列表空)
// - Redo_WithChildCommand_ChildRunsBeforeMark           → R1/U1 子命令先于标记刷新
// - Redo_MarkOperationFields_PropagateThroughConvert    → R1 字段透传（类型/颜色/匹配文本）
//
// 最小清单自检：1 每公开方法 ≥1 用例（ctor/dtor/undo/redo）✔ 2 输入等价类（空/单/多
// 标记）✔ 3 边界（空列表/空指针/多元素）✔ 4 字段参数化并入透传用例 ✔ 5 分支映射 ✔
// 6 全分支 ✔ 7 无异常路径 ✔ 8 负面（空指针/空列表）✔ 9 状态可逆（undo 后记录回旧值）✔
// 10 无 gMock（TextEdit 不可注入虚接口，用接缝记录桩）✔

#include "editor_undo_helpers.h"

#include "../../../src/editor/changemarkcommand.h"

class ChangeMarkCommandTest : public EditorUndoTestBase
{
protected:
    static QList<TextEdit::MarkReplaceInfo> makeMarks(int count, const QString &color)
    {
        QList<TextEdit::MarkReplaceInfo> marks;
        for (int i = 0; i < count; ++i) {
            TextEdit::MarkReplaceInfo info;
            info.start = i * 10;
            info.end = i * 10 + 5;
            info.time = 1000 + i;
            info.opt.type = TextEdit::MarkOnce;
            info.opt.color = color;
            marks.append(info);
        }
        return marks;
    }
};

// ---- R1 正常路径：新标记经 convert 后送 manualUpdateAllMark ----
TEST_F(ChangeMarkCommandTest, Redo_NewMarks_ConvertedAndApplied)
{
    // Arrange
    QList<TextEdit::MarkReplaceInfo> oldMark = makeMarks(1, "red");
    QList<TextEdit::MarkReplaceInfo> newMark = makeMarks(2, "blue");
    ChangeMarkCommand cmd(QPointer<TextEdit>(edit), oldMark, newMark);

    // Act
    cmd.redo();

    // Assert：convert 恰好一次；manual 收到的列表与 newMark 元素一一对应
    EXPECT_EQ(seam.convertReplaceToMarkCalls, 1);
    EXPECT_EQ(seam.manualUpdateAllMarkCalls, 1);
    ASSERT_EQ(seam.lastManualMarks.size(), 2);
    EXPECT_EQ(seam.lastManualMarks.at(0).first.color, QString("blue"));
    EXPECT_EQ(seam.lastManualMarks.at(0).second, qint64(1000));  // time 透传
    EXPECT_EQ(seam.lastManualMarks.at(1).second, qint64(1001));
}

// ---- U1 正常路径：undo 恢复旧标记 ----
TEST_F(ChangeMarkCommandTest, Undo_OldMarks_ConvertedAndApplied)
{
    // Arrange
    QList<TextEdit::MarkReplaceInfo> oldMark = makeMarks(1, "red");
    QList<TextEdit::MarkReplaceInfo> newMark = makeMarks(2, "blue");
    ChangeMarkCommand cmd(QPointer<TextEdit>(edit), oldMark, newMark);

    // Act
    cmd.redo();
    cmd.undo();

    // Assert：undo 后最后一次 manual 使用旧标记（单元素、红色、时间回退）
    EXPECT_EQ(seam.convertReplaceToMarkCalls, 2);
    EXPECT_EQ(seam.manualUpdateAllMarkCalls, 2);
    ASSERT_EQ(seam.lastManualMarks.size(), 1);
    EXPECT_EQ(seam.lastManualMarks.at(0).first.color, QString("red"));
    EXPECT_EQ(seam.lastManualMarks.at(0).second, qint64(1000));
}

// ---- R1/U1 m_EditPtr 空分支 ----
TEST_F(ChangeMarkCommandTest, Redo_NullEdit_SkipsMarkUpdate)
{
    // Arrange：非空标记 + 空编辑器指针
    QList<TextEdit::MarkReplaceInfo> oldMark = makeMarks(1, "red");
    QList<TextEdit::MarkReplaceInfo> newMark = makeMarks(1, "blue");
    ChangeMarkCommand cmd(QPointer<TextEdit>(), oldMark, newMark);

    // Act
    cmd.redo();
    cmd.undo();

    // Assert：空指针分支不触发转换与刷新
    EXPECT_EQ(seam.convertReplaceToMarkCalls, 0);
    EXPECT_EQ(seam.manualUpdateAllMarkCalls, 0);
}

// ---- R1/U1 标记列表空分支 ----
TEST_F(ChangeMarkCommandTest, Redo_EmptyMarkLists_SkipsMarkUpdate)
{
    // Arrange：编辑器有效但两侧标记均为空
    ChangeMarkCommand cmd(QPointer<TextEdit>(edit), {}, {});

    // Act
    cmd.redo();
    cmd.undo();

    // Assert
    EXPECT_EQ(seam.convertReplaceToMarkCalls, 0);
    EXPECT_EQ(seam.manualUpdateAllMarkCalls, 0);
}

// ---- R1/U1 子命令先于标记刷新（顺序验证） ----
TEST_F(ChangeMarkCommandTest, Redo_WithChildCommand_ChildRunsBeforeMark)
{
    // Arrange：QUndoCommand 子项机制——子命令以父命令指针构造挂入 child_list；
    // ChangeMarkCommand::redo 内先调 QUndoCommand::redo()（遍历子命令）再刷新标记
    QStringList orderLog;
    ChangeMarkCommand *cmd = new ChangeMarkCommand(QPointer<TextEdit>(edit),
                                                   makeMarks(1, "red"),
                                                   makeMarks(1, "blue"));
    new RecordingCommand(&orderLog, "child", cmd);             // 以 cmd 为 parent 挂为子命令

    // Act
    cmd->redo();

    // Assert：子命令 redo 先执行，其后标记刷新
    ASSERT_EQ(orderLog.size(), 1);
    EXPECT_EQ(orderLog.at(0), QString("child:redo"));
    EXPECT_EQ(seam.manualUpdateAllMarkCalls, 1);               // 子命令之后标记刷新发生

    // Act & Assert：undo 同样子命令先行（逆序遍历，单子命令等价）
    cmd->undo();
    ASSERT_EQ(orderLog.size(), 2);
    EXPECT_EQ(orderLog.at(1), QString("child:undo"));
    EXPECT_EQ(seam.manualUpdateAllMarkCalls, 2);               // undo/redo 各一次刷新

    delete cmd;                                                // 父命令析构连带子命令
}

// ---- R1 字段透传：MarkOperationType / color / matchText 经 convert 保留 ----
TEST_F(ChangeMarkCommandTest, Redo_MarkOperationFields_PropagateThroughConvert)
{
    // Arrange：两个不同类型的标记操作
    QList<TextEdit::MarkReplaceInfo> newMark;
    TextEdit::MarkReplaceInfo markAll;
    markAll.opt.type = TextEdit::MarkAll;
    markAll.opt.color = "green";
    markAll.opt.matchText = "TODO";
    markAll.time = 42;
    newMark.append(markAll);
    ChangeMarkCommand cmd(QPointer<TextEdit>(edit), {}, newMark);

    // Act
    cmd.redo();

    // Assert：接缝 convert 逐字段透传
    ASSERT_EQ(seam.lastManualMarks.size(), 1);
    EXPECT_EQ(seam.lastManualMarks.at(0).first.type, TextEdit::MarkAll);
    EXPECT_EQ(seam.lastManualMarks.at(0).first.color, QString("green"));
    EXPECT_EQ(seam.lastManualMarks.at(0).first.matchText, QString("TODO"));
    EXPECT_EQ(seam.lastManualMarks.at(0).second, qint64(42));
}
