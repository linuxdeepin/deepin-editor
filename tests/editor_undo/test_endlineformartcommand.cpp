// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// EndlineFormartCommand 单元测试（src/editor/endlineformatcommond.cpp）。
// 行尾格式切换撤销项：redo 通过“插入+删除空格”触发文档变更信号后设置底栏格式，
// undo 直接回设旧格式。BottomBar::setEndlineMenuText 为接缝记录桩。
//
// 分支清单：
// R1: redo —— 编辑器光标处插入 " " 再原位删除（文档净变化为零，触发变更信号）；
//     bar->setEndlineMenuText(m_to)
// U1: undo —— bar->setEndlineMenuText(m_from)
//
// 用例映射：
// - Redo_SetsBarToTargetFormat_KeepsDocumentAndCursor    → R1（文档/光标不变式）
// - Undo_DirectCall_SetsBarToSourceFormat                       → U1（redo→undo 序列）
// - Redo_AllFormatPairs_PropagatedExactly（TEST_P）      → R1/U1 枚举全值
//   （Unknow=-1 / Unix=0 / Windows=1 三组双向）
//
// 最小清单自检：1 每公开方法 ≥1 用例（ctor/dtor/undo/redo）✔ 2 输入等价类（枚举
// 全值）✔ 3 边界（Unknow 负值）✔ 4 TEST_P ≥3 组 ✔ 5 分支映射 ✔ 6 全分支 ✔
// 7 无异常路径 ✔ 8 负面（Unknow）✔ 9 状态可逆（undo 回旧格式）✔ 10 无 gMock ✔

#include "editor_undo_helpers.h"

#include "../../../src/editor/endlineformatcommond.h"

// undo/redo 为 protected（QUndoCommand 约定）：测试子类以 using 声明提升为可调用
//（test-types.md 抽象/受保护方法处理），不改变任何被测行为。
class TestableEndlineFormartCommand : public EndlineFormartCommand
{
public:
    using EndlineFormartCommand::EndlineFormartCommand;
    using EndlineFormartCommand::undo;
    using EndlineFormartCommand::redo;
};

class EndlineFormartCommandTest : public EditorUndoTestBase
{
};

// ---- R1：redo 设置目标格式且文档内容/光标位置净不变 ----
TEST_F(EndlineFormartCommandTest, Redo_SetsBarToTargetFormat_KeepsDocumentAndCursor)
{
    // Arrange：伪 BottomBar（非虚 setEndlineMenuText 调接缝记录桩）
    QWidget barHost;
    auto *bar = ut::fakeBottomBar(&barHost);
    edit->setPlainText("line1\nline2");
    edit->setTextCursor(ut::cursorAt(edit->document(), 3, 3));
    const int cursorBefore = edit->textCursor().position();
    TestableEndlineFormartCommand cmd(edit, bar, BottomBar::Unix, BottomBar::Windows);

    // Act
    cmd.redo();

    // Assert：文档内容不变（插入与删除相抵），接缝记录目标格式
    EXPECT_EQ(docText(), QString("line1\nline2"));             // 文档净变化为零
    EXPECT_EQ(edit->textCursor().position(), cursorBefore);    // 光标位置保持
    EXPECT_EQ(seam.setEndlineMenuTextCalls, 1);
    EXPECT_EQ(seam.lastEndlineFormat, static_cast<int>(BottomBar::Windows));

    // Act & Assert：undo 恢复源格式
    cmd.undo();
    EXPECT_EQ(docText(), QString("line1\nline2"));             // 状态未损坏
    EXPECT_EQ(seam.setEndlineMenuTextCalls, 2);
    EXPECT_EQ(seam.lastEndlineFormat, static_cast<int>(BottomBar::Unix));
}

// ---- U1：undo 单独验证（此前有其他 redo 的场景下回设 from） ----
TEST_F(EndlineFormartCommandTest, Undo_DirectCall_SetsBarToSourceFormat)
{
    // Arrange
    QWidget barHost;
    auto *bar = ut::fakeBottomBar(&barHost);
    TestableEndlineFormartCommand cmd(edit, bar, BottomBar::Windows, BottomBar::Unix);

    // Act
    cmd.undo();

    // Assert：直接 undo 亦设置源格式（不触碰文档）
    EXPECT_EQ(seam.setEndlineMenuTextCalls, 1);
    EXPECT_EQ(seam.lastEndlineFormat, static_cast<int>(BottomBar::Windows));
    EXPECT_EQ(docText(), QString(""));                         // 空文档未被触碰
}

// ---- 枚举全值 TEST_P：Unknow/Unix/Windows 三组 from→to 双向传播 ----
namespace {
struct EndlineCase {
    BottomBar::EndlineFormat from;
    BottomBar::EndlineFormat to;
};
}  // namespace

class EndlineParamTest : public EditorUndoTestBase,
                         public ::testing::WithParamInterface<EndlineCase> {
};

TEST_P(EndlineParamTest, Redo_AllFormatPairs_PropagatedExactly)
{
    // Arrange
    const EndlineCase &c = GetParam();
    QWidget barHost;
    auto *bar = ut::fakeBottomBar(&barHost);
    edit->setPlainText("x");
    TestableEndlineFormartCommand cmd(edit, bar, c.from, c.to);

    // Act
    cmd.redo();

    // Assert：redo 恰好传播 to；文档不变
    EXPECT_EQ(seam.setEndlineMenuTextCalls, 1);
    EXPECT_EQ(seam.lastEndlineFormat, static_cast<int>(c.to));
    EXPECT_EQ(docText(), QString("x"));

    // Act & Assert：undo 传播 from
    cmd.undo();
    EXPECT_EQ(seam.setEndlineMenuTextCalls, 2);
    EXPECT_EQ(seam.lastEndlineFormat, static_cast<int>(c.from));
    EXPECT_EQ(docText(), QString("x"));                        // 状态未损坏
}

INSTANTIATE_TEST_SUITE_P(FormatVariants, EndlineParamTest,
    ::testing::Values(
        EndlineCase{ BottomBar::Unknow, BottomBar::Unix },
        EndlineCase{ BottomBar::Unix, BottomBar::Windows },
        EndlineCase{ BottomBar::Windows, BottomBar::Unknow }
    ));

// ---- D1：经基类指针虚析构删除（deleting destructor 路径）----
// 注：undo/redo 为 protected，本用例只构造具体基类对象并删除（D0 分派要求动态类型
// 恰为 EndlineFormartCommand，不能经由测试子类）。
TEST_F(EndlineFormartCommandTest, Destructor_DeleteViaBasePointer_ReleasesCleanly)
{
    // Arrange：具体类堆上构造（不调用受保护的 undo/redo）
    QWidget barHost;
    auto *bar = ut::fakeBottomBar(&barHost);
    QUndoCommand *cmd = new EndlineFormartCommand(edit, bar, BottomBar::Unix, BottomBar::Windows);

    // Act
    delete cmd;

    // Assert：构造/析构全程不触碰文档与底栏接口
    EXPECT_EQ(docText(), QString(""));
    EXPECT_EQ(seam.setEndlineMenuTextCalls, 0);
}
