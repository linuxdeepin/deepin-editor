// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// UndoList 单元测试（src/editor/undolist.cpp）。
// 组合命令：appendCom 追加（忽略空指针）；undo 逆序执行；redo 正序执行；
// 析构逐个 delete 并置空。
//
// 分支清单：
// A1: appendCom —— com 非空 → 追加；空 → 忽略
// U1: undo —— 逆序遍历（rbegin..rend）；processed%10==0 || processed==total 进度日志分支
// R1: redo —— 正序遍历（foreach）；同上进度分支
// D1: 析构 —— 逐个非空判断 + delete + 置空 + clear
//
// 用例映射：
// - Undo_ThreeCommands_ExecutesInReverseOrder       → U1（逆序核心语义）
// - Redo_ThreeCommands_ExecutesInForwardOrder       → R1（正序核心语义）
// - Undo_EmptyList_Noop                             → U1（0 次循环边界）
// - AppendCom_NullPointer_Ignored                   → A1（空指针分支）
// - Undo_TenCommands_CoversProgressLogBranch        → U1/R1（processed%10==0 分支）
// - Destructor_AppendedCommands_AllDeleted           → D1（实例存活计数归零）
// - UndoRedo_RoundTrip_OrderStableAfterCycles       → U1+R1 组合往返
//
// 最小清单自检：1 每公开方法 ≥1 用例（ctor/dtor/appendCom/undo/redo）✔
// 2 输入等价类（0/1/3/10 条命令、空指针）✔ 3 边界（空列表、10 条、重复追加）✔
// 4 同质多组以数量维度并入专用用例 ✔ 5 分支映射 ✔ 6 全分支 ✔ 7 无异常路径 ✔
// 8 负面（空指针/空列表）✔ 9 状态可逆（undo/redo 往返顺序稳定）✔
// 10 无 gMock（QUndoCommand 组合，用记录子命令）✔

#include "editor_undo_helpers.h"

#include "../../../src/editor/undolist.h"

// undo/redo 为 protected（QUndoCommand 约定）：测试子类以 using 声明提升为可调用。
class TestableUndoList : public UndoList
{
public:
    using UndoList::UndoList;
    using UndoList::undo;
    using UndoList::redo;
};

#include <QStringList>

namespace {
// 存活计数子命令：构造/析构维护 fixture 成员 aliveCount（非 static，无跨用例污染）。
class CountingCommand : public QUndoCommand
{
public:
    explicit CountingCommand(int *aliveCount)
        : m_aliveCount(aliveCount)
    {
        ++(*m_aliveCount);
    }

    ~CountingCommand() override { --(*m_aliveCount); }

    void undo() override {}
    void redo() override {}

private:
    int *m_aliveCount = nullptr;
};
}  // namespace

class UndoListTest : public EditorUndoTestBase
{
};

// ---- U1：三条命令逆序撤销 ----
TEST_F(UndoListTest, Undo_ThreeCommands_ExecutesInReverseOrder)
{
    // Arrange
    QStringList orderLog;
    TestableUndoList list;
    list.appendCom(new RecordingCommand(&orderLog, "first"));
    list.appendCom(new RecordingCommand(&orderLog, "second"));
    list.appendCom(new RecordingCommand(&orderLog, "third"));

    // Act
    list.undo();

    // Assert：逆序 third → second → first
    ASSERT_EQ(orderLog.size(), 3);
    EXPECT_EQ(orderLog.at(0), QString("third:undo"));
    EXPECT_EQ(orderLog.at(1), QString("second:undo"));
    EXPECT_EQ(orderLog.at(2), QString("first:undo"));
}

// ---- R1：三条命令正序重做 ----
TEST_F(UndoListTest, Redo_ThreeCommands_ExecutesInForwardOrder)
{
    // Arrange
    QStringList orderLog;
    TestableUndoList list;
    list.appendCom(new RecordingCommand(&orderLog, "first"));
    list.appendCom(new RecordingCommand(&orderLog, "second"));
    list.appendCom(new RecordingCommand(&orderLog, "third"));

    // Act
    list.redo();

    // Assert：正序 first → second → third
    ASSERT_EQ(orderLog.size(), 3);
    EXPECT_EQ(orderLog.at(0), QString("first:redo"));
    EXPECT_EQ(orderLog.at(1), QString("second:redo"));
    EXPECT_EQ(orderLog.at(2), QString("third:redo"));
}

// ---- U1/R1 0 次循环边界：空列表安全无操作 ----
TEST_F(UndoListTest, Undo_EmptyList_Noop)
{
    // Arrange
    TestableUndoList list;

    // Act：空列表 undo/redo 不应崩溃或产生任何副作用
    list.undo();
    list.redo();

    // Assert：编辑器状态未受影响（强异常安全）
    EXPECT_EQ(docText(), QString(""));
    EXPECT_EQ(edit->textCursor().position(), 0);               // 光标亦未被触碰
}

// ---- A1 空指针分支 ----
TEST_F(UndoListTest, AppendCom_NullPointer_Ignored)
{
    // Arrange
    QStringList orderLog;
    TestableUndoList list;
    list.appendCom(nullptr);                                   // 空指针被忽略
    list.appendCom(new RecordingCommand(&orderLog, "only"));

    // Act
    list.undo();
    list.redo();

    // Assert：仅有一条有效命令被执行（undo 一次 + redo 一次）
    ASSERT_EQ(orderLog.size(), 2);
    EXPECT_EQ(orderLog.at(0), QString("only:undo"));
    EXPECT_EQ(orderLog.at(1), QString("only:redo"));
}

// ---- U1/R1 进度日志分支（processed%10==0 与 processed==total 同时命中） ----
TEST_F(UndoListTest, Undo_TenCommands_CoversProgressLogBranch)
{
    // Arrange：恰好 10 条命令（第 10 条时 processed%10==0 与 ==total 均为真）
    QStringList orderLog;
    TestableUndoList list;
    for (int i = 0; i < 10; ++i)
        list.appendCom(new RecordingCommand(&orderLog, QString("c%1").arg(i)));

    // Act
    list.undo();

    // Assert：10 条全部逆序执行
    ASSERT_EQ(orderLog.size(), 10);
    EXPECT_EQ(orderLog.at(0), QString("c9:undo"));
    EXPECT_EQ(orderLog.at(9), QString("c0:undo"));

    // Act & Assert：redo 正序 10 条
    orderLog.clear();
    list.redo();
    ASSERT_EQ(orderLog.size(), 10);
    EXPECT_EQ(orderLog.at(0), QString("c0:redo"));
    EXPECT_EQ(orderLog.at(9), QString("c9:redo"));
}

// ---- D1：析构逐个释放全部追加命令 ----
TEST_F(UndoListTest, Destructor_AppendedCommands_AllDeleted)
{
    // Arrange
    int aliveCount = 0;                                        // 用例局部存活计数
    {
        TestableUndoList list;
        list.appendCom(new CountingCommand(&aliveCount));
        list.appendCom(nullptr);                               // 混入空指针验证析构判空分支
        list.appendCom(new CountingCommand(&aliveCount));
        list.appendCom(new CountingCommand(&aliveCount));
        EXPECT_EQ(aliveCount, 3);                              // 3 个子命令存活

        // Act：作用域结束触发 ~UndoList
    }

    // Assert：全部子命令被析构删除，计数归零
    EXPECT_EQ(aliveCount, 0);
}

// ---- U1+R1 组合往返：多轮撤销/重做顺序稳定 ----
TEST_F(UndoListTest, UndoRedo_RoundTrip_OrderStableAfterCycles)
{
    // Arrange
    QStringList orderLog;
    TestableUndoList list;
    list.appendCom(new RecordingCommand(&orderLog, "A"));
    list.appendCom(new RecordingCommand(&orderLog, "B"));

    // Act & Assert：两轮往返，每轮 undo 逆序、redo 正序
    for (int round = 0; round < 2; ++round) {
        orderLog.clear();
        list.undo();
        ASSERT_EQ(orderLog.size(), 2) << "round " << round;
        EXPECT_EQ(orderLog.at(0), QString("B:undo")) << "round " << round;
        EXPECT_EQ(orderLog.at(1), QString("A:undo")) << "round " << round;

        orderLog.clear();
        list.redo();
        ASSERT_EQ(orderLog.size(), 2) << "round " << round;
        EXPECT_EQ(orderLog.at(0), QString("A:redo")) << "round " << round;
        EXPECT_EQ(orderLog.at(1), QString("B:redo")) << "round " << round;
    }
}

// ---- D1：经基类指针虚析构删除 UndoList 自身（deleting destructor 路径） ----
TEST_F(UndoListTest, Destructor_DeleteViaBasePointer_ReleasesCleanly)
{
    // Arrange：具体类堆上构造（D0 分派要求动态类型恰为 UndoList）
    QUndoCommand *cmd = new UndoList();
    EXPECT_EQ(docText(), QString(""));                         // 前提：无副作用

    // Act
    delete cmd;

    // Assert：析构安全，文档状态不变
    EXPECT_EQ(docText(), QString(""));
}
