// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// InsertBlockByTextCommand 单元测试（src/editor/insertblockbytextcommond.cpp）。
// 分块插入大文本的撤销项。EditWrapper/Window/BottomBar 以接缝伪指针注入
//（不构造实例；window()/bottomBar()/isQuit()/setPrintEnabled()/setChildEnabled()
// 均为接缝记录桩），TextEdit 为空壳真实控件。
//
// 分支清单：
// CT: 构造 —— m_edit 空 / m_text 空 / m_wrapper 空 → 提前返回（三分支）；
//     有选区 → 暂存 m_selected 与 m_insertPos
// R1: redo —— treat(true)（断开 cursorPositionChanged）；insertByBlock；treat(false)
// I1: insertByBlock —— size <= 1MB：不插入，仅记录 m_delPos = 光标位置
// I2: size > 1MB —— 循环整块插入（m_wrapper 非空且 !isQuit）+ processEvents；
//     余量 y 插入（y!=0）；isQuit=true 时跳过插入；y==0 时跳过余量分支
// U1: undo —— 删除 [m_delPos - size, m_delPos)；m_selected 非空 → 原位重插
// T: treat —— window/bar 空/非空（setPrintEnabled/setChildEnabled 副作用）
//
// 用例映射：
// - Redo_SmallText_SkipsBlockInsert_UndoTrimsTail        → I1 + U1（m_selected 空）
// - Redo_LargeText_InsertsAllBlocksAndUndoClears         → I2（n 循环 + y 余量）+ T(非空)
// - Redo_LargeText_ExactMultiple_NoRemainder             → I2（y==0 边界）
// - Redo_LargeText_QuitFlag_SkipsInsert                  → I2（isQuit 分支）
// - Redo_LargeTextWithSelection_FullyReversible          → CT(选区) + U1(重插)
// - Ctor_NullWrapper_SmallTextRedoSafe                   → CT(m_wrapper 空) + I1
// - Ctor_EmptyText_EarlyReturnThenRedoSafe               → CT(m_text 空) + I1
// - Ctor_NullEdit_EarlyReturnOnly                        → CT(m_edit 空；仅构造，undo/redo
//   会解引用 m_edit，不在本用例调用——源码契约：该构造参数组合仅供不入栈场景）
//
// 最小清单自检：1 每公开方法 ≥1 用例（ctor/dtor/redo/undo；私有 treat/insertByBlock
// 经 redo/undo 间接全覆盖）✔ 2 输入等价类（小/大/整除/退出/选区）✔ 3 边界（1MB 整除、
// y=0、y!=0、空文本）✔ 4 分组场景各异不参数化（块大小参数无法快速参数化）✔
// 5 分支映射 ✔ 6 全分支 ✔ 7 无异常路径 ✔ 8 负面（空文本/空 wrapper）✔
// 9 双向可逆断言 ✔ 10 无 gMock ✔

#include "editor_undo_helpers.h"

#include "../../../src/editor/insertblockbytextcommond.h"

class InsertBlockByTextCommandTest : public EditorUndoTestBase
{
protected:
    static constexpr int kBlockSize = 1 * 1024 * 1024;  // 与源码一致的分块阈值
};

// ---- I1 + U1（m_selected 空）：小文本 redo 不插入，undo 按固定区间裁剪 ----
TEST_F(InsertBlockByTextCommandTest, Redo_SmallText_SkipsBlockInsert_UndoTrimsTail)
{
    // Arrange：宿主 QWidget 充当伪 EditWrapper（非虚调用，安全）；window/bar 为空
    QWidget wrapperHost;
    auto *wrapper = reinterpret_cast<EditWrapper *>(&wrapperHost);
    edit->setPlainText("1234567890");
    edit->setTextCursor(ut::cursorAt(edit->document(), 10, 10));  // 光标在文档尾
    InsertBlockByTextCommand cmd("abc", edit, wrapper);

    // Act
    cmd.redo();

    // Assert：小文本不触发分块插入，文档不变；treat 的 bar 为空不调用 setChildEnabled
    EXPECT_EQ(docText(), QString("1234567890"));
    EXPECT_EQ(seam.setChildEnabledCalls, 0);
    EXPECT_EQ(seam.setPrintEnabledCalls, 0);

    // Act & Assert：undo 删除 [m_delPos-3, m_delPos) = [7,10)（源码固定偏移语义）
    cmd.undo();
    EXPECT_EQ(docText(), QString("1234567"));
}

// ---- I2 整块 + 余量 + T(非空)：大文本分块插入与清空还原 ----
TEST_F(InsertBlockByTextCommandTest, Redo_LargeText_InsertsAllBlocksAndUndoClears)
{
    // Arrange：1MB + 10 字符（n=1 整块 + y=10 余量）；window/bar 均为伪指针
    QWidget wrapperHost, windowHost, barHost;
    auto *wrapper = reinterpret_cast<EditWrapper *>(&wrapperHost);
    seam.fakeWindowObject = &windowHost;
    seam.fakeBottomBarObject = &barHost;
    const QString text = QString(kBlockSize + 10, 'a');
    edit->setTextCursor(ut::cursorAt(edit->document(), 0, 0));
    InsertBlockByTextCommand cmd(text, edit, wrapper);

    // Act
    cmd.redo();

    // Assert：全部插入；treat(true)→setPrintEnabled(false)/setChildEnabled(false)，
    // treat(false)→setPrintEnabled(true)/setChildEnabled(true)（各调用一次）
    EXPECT_EQ(edit->document()->characterCount() - 1, kBlockSize + 10);
    EXPECT_EQ(docText().at(0), QChar('a'));
    EXPECT_EQ(docText().at(docText().size() - 1), QChar('a'));
    EXPECT_EQ(seam.setPrintEnabledCalls, 2);
    EXPECT_TRUE(seam.lastPrintEnabled);                        // 最后一次为 treat(false)
    EXPECT_EQ(seam.setChildEnabledCalls, 2);
    EXPECT_TRUE(seam.lastChildEnabled);

    // Act & Assert：undo 删除全部插入内容
    cmd.undo();
    EXPECT_EQ(docText(), QString(""));                         // 状态可逆
    EXPECT_EQ(edit->document()->characterCount() - 1, 0);
}

// ---- I2 y==0 边界：恰好整数倍块大小（2 块），余量分支跳过 ----
// 注：源码条件为 size > 1MB（严格大于），恰好 1MB 不触发分块插入；取 2MB 覆盖
// n=2、y=0 的整除边界。
TEST_F(InsertBlockByTextCommandTest, Redo_LargeText_ExactMultiple_NoRemainder)
{
    // Arrange：恰好 2MB（n=2、y=0）
    QWidget wrapperHost;
    auto *wrapper = reinterpret_cast<EditWrapper *>(&wrapperHost);
    const QString text = QString(2 * kBlockSize, 'b');
    edit->setTextCursor(ut::cursorAt(edit->document(), 0, 0));
    InsertBlockByTextCommand cmd(text, edit, wrapper);

    // Act
    cmd.redo();

    // Assert
    EXPECT_EQ(edit->document()->characterCount() - 1, 2 * kBlockSize);
    EXPECT_EQ(docText(), text);
    cmd.undo();
    EXPECT_EQ(docText(), QString(""));                         // 状态可逆
}

// ---- I2 isQuit 分支：退出标志使分块与余量插入全部跳过 ----
TEST_F(InsertBlockByTextCommandTest, Redo_LargeText_QuitFlag_SkipsInsert)
{
    // Arrange：预填文档长度 = 文本长度 + 5，光标在文档尾；isQuit=true
    QWidget wrapperHost;
    auto *wrapper = reinterpret_cast<EditWrapper *>(&wrapperHost);
    seam.fakeIsQuit = true;
    const QString text = QString(kBlockSize + 5, 'x');
    edit->setPlainText(QString(text.size() + 5, 'c'));
    edit->setTextCursor(ut::cursorAt(edit->document(), text.size() + 5, text.size() + 5));
    InsertBlockByTextCommand cmd(text, edit, wrapper);

    // Act
    cmd.redo();

    // Assert：不插入，文档不变；m_delPos = 光标位置（文档尾）
    EXPECT_EQ(edit->document()->characterCount() - 1, text.size() + 5);

    // Act & Assert：undo 按固定区间裁剪 [len-size, len) → 剩 5 个字符
    cmd.undo();
    EXPECT_EQ(edit->document()->characterCount() - 1, 5);
    EXPECT_EQ(docText(), QString("ccccc"));
}

// ---- CT(选区) + U1(重插)：大文本替换选区后完全可逆 ----
TEST_F(InsertBlockByTextCommandTest, Redo_LargeTextWithSelection_FullyReversible)
{
    // Arrange：选中 "hello"，大文本插入将替换选区
    QWidget wrapperHost;
    auto *wrapper = reinterpret_cast<EditWrapper *>(&wrapperHost);
    edit->setPlainText("hello world");
    edit->setTextCursor(ut::cursorAt(edit->document(), 0, 5));
    const QString text = QString(kBlockSize + 10, 'z');
    InsertBlockByTextCommand cmd(text, edit, wrapper);

    // Act
    cmd.redo();

    // Assert：选区被替换为插入文本（insertText 在选区光标上执行）
    EXPECT_EQ(edit->document()->characterCount() - 1, text.size() + 6);  // z...z + " world"
    EXPECT_EQ(docText().right(6), QString(" world"));

    // Act & Assert：undo 删除插入文本并原位重插 "hello"
    cmd.undo();
    EXPECT_EQ(docText(), QString("hello world"));              // 完全可逆
}

// ---- CT(m_wrapper 空) + I1：空 wrapper 构造提前返回，redo/undo 仍走固定偏移 ----
TEST_F(InsertBlockByTextCommandTest, Ctor_NullWrapper_SmallTextRedoSafe)
{
    // Arrange
    edit->setPlainText("abcdef");
    edit->setTextCursor(ut::cursorAt(edit->document(), 6, 6));
    InsertBlockByTextCommand cmd("XY", edit, nullptr);

    // Act
    cmd.redo();

    // Assert：treat 的 m_wrapper 分支整体跳过
    EXPECT_EQ(docText(), QString("abcdef"));
    EXPECT_EQ(seam.setChildEnabledCalls, 0);
    cmd.undo();                                                // m_delPos 已由 redo 记录
    EXPECT_EQ(docText(), QString("abcd"));                     // [6-2,6) 被裁剪
}

// ---- CT(m_text 空)：空文本提前返回；redo 后 m_delPos 有值，undo 安全 ----
TEST_F(InsertBlockByTextCommandTest, Ctor_EmptyText_EarlyReturnThenRedoSafe)
{
    // Arrange
    QWidget wrapperHost;
    auto *wrapper = reinterpret_cast<EditWrapper *>(&wrapperHost);
    edit->setPlainText("keep");
    edit->setTextCursor(ut::cursorAt(edit->document(), 4, 4));
    InsertBlockByTextCommand cmd("", edit, wrapper);

    // Act
    cmd.redo();

    // Assert：空文本无插入；m_delPos = 光标位置 4
    EXPECT_EQ(docText(), QString("keep"));

    // Act & Assert：undo 删除 [4-0,4) 空范围 → 文档不变（强异常安全）
    cmd.undo();
    EXPECT_EQ(docText(), QString("keep"));
}

// ---- CT(m_edit 空)：仅覆盖构造提前返回（redo/undo 会解引用 m_edit，不调用） ----
TEST_F(InsertBlockByTextCommandTest, Ctor_NullEdit_EarlyReturnOnly)
{
    // Arrange & Act：空 edit 构造直接提前返回，无任何文档访问
    QWidget wrapperHost;
    auto *wrapper = reinterpret_cast<EditWrapper *>(&wrapperHost);
    InsertBlockByTextCommand cmd("text", nullptr, wrapper);

    // Assert：命令对象可用（id/text 等 QUndoCommand 基类接口不受影响）
    EXPECT_EQ(cmd.text(), QString(""));
    EXPECT_EQ(docText(), QString(""));                         // 编辑器状态未受影响
}

// ---- D1：经基类指针虚析构删除（deleting destructor 路径） ----
TEST_F(InsertBlockByTextCommandTest, Destructor_DeleteViaBasePointer_ReleasesCleanly)
{
    // Arrange：小文本 redo 记录 m_delPos 后保持文档不变
    QWidget wrapperHost;
    auto *wrapper = reinterpret_cast<EditWrapper *>(&wrapperHost);
    edit->setPlainText("12345");
    edit->setTextCursor(ut::cursorAt(edit->document(), 5, 5));
    QUndoCommand *cmd = new InsertBlockByTextCommand("ab", edit, wrapper);
    cmd->redo();
    EXPECT_EQ(docText(), QString("12345"));                    // 前提：小文本 redo 无插入

    // Act
    delete cmd;

    // Assert：析构不改变文档状态
    EXPECT_EQ(docText(), QString("12345"));
}
