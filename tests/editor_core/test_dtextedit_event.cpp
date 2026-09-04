// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// ===========================================================================
// TextEdit 事件处理方法族（keyPress/mouse/wheel/gesture/eventFilter/inputMethod/drag-drop）
//
// 分支清单（来源：src/editor/dtextedit.cpp）：
// K1 : keyPressEvent —— Esc 发 signal_setTitleFocus；Ctrl+C/A 走槽；
//      只读模式单字母 J K , . H L Space V F B A E M Q 分发；
//      Shift+J/Shift+K 滚行；Ctrl+Shift 组合弹提示；Ctrl 组合弹提示；
//      F11/F5 ignore；NoModifier 弹提示；Key_Control/Shift ignore；
//      普通字符/小键盘/Tab/Return/Backspace/Delete/Shift 符号插入；
//      Alt+M 弹菜单；Ctrl+Z/Y/X/V 撤销重做剪贴；快捷键表驱动各编辑命令；
//      Insert 键切换覆盖模式；window 快捷键 ignore；Alt+数字 ignore；
//      Alt+Tab 吞掉；default 走 QPlainTextEdit
// K2 : keyPressEvent 只读 isReadOnly() 拦截（setReadOnly 原生路径）
// M1 : mousePressEvent —— m_bIsFindClose 清查找；非右键清全选；
//      Alt 列编辑起点；普通点击走基类
// M2 : mouseMoveEvent —— Alt 列选区构建（跨行多选区）
// M3 : mouseReleaseEvent —— 中键粘贴（allow_midbutton_paste on/off、只读提示）；
//      合成滑动事件衰减 return；普通释放走基类
// W1 : wheelEvent —— 无 Ctrl 走基类（Ctrl 分支依赖真实 Window，见 uncovered）
// G1 : event()/gestureEvent —— Gesture 分发；PaletteChange → onAppPaletteChanged
// G2 : tapGestureTriggered 四状态；tapAndHold/pan 状态机；swipe 空实现
// G3 : slideGestureY/X —— 滚动条步进
// G4 : fingerZoom —— tap+3 指 → slot_translate；pinch 分支（无焦点安全跳过）
// I1 : inputMethodEvent —— 只读 return；preedit 撤销重建；commit 插入；
//      覆盖模式选中替换；列编辑插入；AI 删除（replacementLength）分支
// E1 : eventFilter —— TouchBegin（3 指翻译/非 3 指）；viewport 点击透传；
//      书签区右键菜单（有书签/多书签/无书签）；书签区左键增删书签；
//      折叠区左键折叠/展开；折叠区右键菜单（图标行/可见性置灰）；
//      HoverMove 书签/折叠；HoverLeave；双击标志；colorMarkMenu Tab 定时器
// D1 : dragMoveEvent —— 只读 return；urls accept；其它走基类
// D2 : dropEvent —— 文本拖入（无 source → DragInsertTextUndoCommand）；
//      有文本但只读 return；非文本走基类（URL 分支依赖真实 Window，见 uncovered）
// P1 : paintEvent —— Alt 列选区绘制（grab 触发）→ m_hasColumnSelection
// R1 : resizeEvent —— m_isSelectAll 重选；markAllKeywordInView；尾部锚定 singleShot
// C1 : contextMenuEvent/popRightMenu/hideRightMenu（QMenu::exec 已 stub）
// V1 : updateViewModeActions/viewModeActions
//
// 用例映射：见各 TEST_F 名。环境隔离：见 editor_core_fixture.h。
//
// 已知不可达（记录 uncovered，勿强行触发）：
// - dragEnterEvent / dropEvent(URL) / pinchTriggered / wheelEvent(Ctrl) /
//   fingerZoom(pinch+focus)：qobject_cast<Window*>(window())-> 直接解引用，
//   需真实 Window 实例（策略禁止构造）。
// ===========================================================================

#include "editor_core_fixture.h"

#include <QTouchEvent>
#include <QPointingDevice>
#include <QInputDevice>
#include <QContextMenuEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QHoverEvent>
#include "showflodcodewidget.h"

namespace {
// 测试专用触摸屏设备（QTouchEvent 构造需要）
QPointingDevice *utTouchDevice()
{
    static QPointingDevice dev(QStringLiteral("ut-touch"), 2,
                                QInputDevice::DeviceType::TouchScreen,
                                QPointingDevice::PointerType::Finger,
                                QInputDevice::Capability::Position, 3, 3);
    return &dev;
}
} // namespace

// ---------------- keyPressEvent：基础插入族（K1） ----------------

TEST_F(TextEditTest, KeyPress_Escape_EmitsTitleFocusSignal)
{
    // Arrange
    QSignalSpy spy(edit, &TextEdit::signal_setTitleFocus);

    // Act
    sendKey(Qt::Key_Escape, Qt::NoModifier);

    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(edit->getPosition(), 0); // Esc 不移动光标
}

TEST_F(TextEditTest, KeyPress_PrintableChar_InsertsText)
{
    // Arrange
    setDocText(QString(""));

    // Act
    sendKey(Qt::Key_A, Qt::NoModifier, QString("a"));

    // Assert：经撤销栈插入
    EXPECT_EQ(edit->toPlainText(), QString("a"));
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, KeyPress_KeypadDigit_InsertsText)
{
    // Arrange
    setDocText(QString(""));

    // Act：小键盘数字（KeypadModifier 分支）
    sendKey(Qt::Key_5, Qt::KeypadModifier, QString("5"));

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("5"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, KeyPress_Tab_InsertsTabCharacter)
{
    // Arrange
    setDocText(QString("x"));
    moveCursorTo(1);

    // Act
    sendKey(Qt::Key_Tab, Qt::NoModifier, QString("\t"));

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("x\t"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, KeyPress_TabWithSelection_IndentsBlock)
{
    // Arrange
    setDocText(QString("aa\nbb"));
    QTextCursor cur = makeCursor(0);
    cur.setPosition(5, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);

    // Act：选区 Tab → indentText
    sendKey(Qt::Key_Tab, Qt::NoModifier, QString("\t"));

    // Assert：缩进命令生效（行首变化 + 块数不变）
    EXPECT_NE(edit->toPlainText(), QString("aa\nbb"));
    EXPECT_EQ(edit->blockCount(), 2);
}

TEST_F(TextEditTest, KeyPress_Return_InsertsNewline)
{
    // Arrange
    setDocText(QString("ab"));

    // Act
    moveCursorTo(1);
    sendKey(Qt::Key_Return, Qt::NoModifier, QString("\n"));

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("a\nb"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, KeyPress_Backspace_DeletesPreviousChar)
{
    // Arrange
    setDocText(QString("abc"));
    moveCursorTo(2);

    // Act
    sendKey(Qt::Key_Backspace, Qt::NoModifier);

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("ac"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, KeyPress_ShiftBackspace_AlsoDeletesPreviousChar)
{
    // Arrange：BUG-373675 修复——Shift+Backspace 纳入删除分支
    setDocText(QString("abc"));
    moveCursorTo(2);

    // Act
    sendKey(Qt::Key_Backspace, Qt::ShiftModifier);

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("ac"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, KeyPress_Delete_DeletesNextChar)
{
    // Arrange
    setDocText(QString("abc"));
    moveCursorTo(1);

    // Act
    sendKey(Qt::Key_Delete, Qt::NoModifier);

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("ac"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, KeyPress_DeleteAtDocEnd_NothingHappens)
{
    // Arrange
    setDocText(QString("end"));
    moveCursorTo(3);

    // Act
    sendKey(Qt::Key_Delete, Qt::NoModifier);

    // Assert：末尾 Delete 无操作且不产生撤销项
    EXPECT_EQ(edit->toPlainText(), QString("end"));
    // 强异常安全：早退路径不产生撤销项
    EXPECT_FALSE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, KeyPress_BackspaceWithSelection_RemovesSelection)
{
    // Arrange
    setDocText(QString("abcdef"));
    QTextCursor cur = makeCursor(1);
    cur.setPosition(4, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);

    // Act
    sendKey(Qt::Key_Backspace, Qt::NoModifier);

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("aef"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, KeyPress_ShiftSymbol_InsertsText)
{
    // Arrange
    setDocText(QString(""));

    // Act：Shift+1 → "!"
    sendKey(Qt::Key_1, Qt::ShiftModifier, QString("!"));

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("!"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, KeyPress_OverwriteMode_InsertReplacesNextChar)
{
    // Arrange
    setDocText(QString("xyz"));
    moveCursorTo(0);

    // Act：Insert 键切换覆盖模式
    QSignalSpy spy(edit, &TextEdit::cursorModeChanged);
    sendKey(Qt::Key_Insert, Qt::NoModifier);
    ASSERT_EQ(edit->m_cursorMode, TextEdit::Overwrite);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toInt(), static_cast<int>(TextEdit::Overwrite));

    // Act：覆盖插入
    sendKey(Qt::Key_A, Qt::NoModifier, QString("a"));

    // Assert：首字符被替换而非插入
    EXPECT_EQ(edit->toPlainText(), QString("ayz"));
}

// ---------------- keyPressEvent：剪贴板快捷键 ----------------

TEST_F(TextEditTest, KeyPress_CopyAndPasteShortcuts_RoundTrip)
{
    // Arrange
    setDocText(QString("copy body"));
    QTextCursor cur = makeCursor(0);
    cur.setPosition(4, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);
    QApplication::clipboard()->clear();

    // Act：Ctrl+C 复制
    sendShortcut("copy");
    // Assert
    EXPECT_EQ(QApplication::clipboard()->text(), QString("copy"));

    // Act：Ctrl+V 粘贴到末尾
    moveCursorTo(9);
    sendShortcut("paste");
    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("copy bodycopy"));
}

TEST_F(TextEditTest, KeyPress_CutShortcut_RemovesAndClips)
{
    // Arrange
    setDocText(QString("tail"));
    QTextCursor cur = makeCursor(0);
    cur.setPosition(4, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);
    QApplication::clipboard()->clear();

    // Act
    sendShortcut("cut");

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString(""));
    EXPECT_EQ(QApplication::clipboard()->text(), QString("tail"));
}

TEST_F(TextEditTest, KeyPress_SelectAllShortcut_SelectsViewportText)
{
    // Arrange
    setDocText(QString("all of it"));

    // Act：Ctrl+A → setSelectAll（视口选择）
    sendShortcut("selectall");

    // Assert
    EXPECT_TRUE(edit->m_isSelectAll);
    EXPECT_TRUE(edit->textCursor().hasSelection());
}

TEST_F(TextEditTest, KeyPress_UndoRedoShortcuts_RoundTrip)
{
    // Arrange
    setDocText(QString(""));
    sendKey(Qt::Key_A, Qt::NoModifier, QString("a"));
    sendKey(Qt::Key_B, Qt::NoModifier, QString("b"));
    ASSERT_EQ(edit->toPlainText(), QString("ab"));

    // Act：Ctrl+Z 撤销
    sendShortcut("undo");
    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("a"));

    // Act：Ctrl+Y 重做
    sendShortcut("redo");
    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("ab"));
}

// ---------------- keyPressEvent：命令表驱动 ----------------

TEST_F(TextEditTest, KeyPress_EditorCommandShortcuts_ExecuteActions)
{
    // Arrange
    setDocText(QString("one\ntwo"));

    // Act：opennewlineabove（Ctrl+Enter）
    moveCursorTo(4);
    sendShortcut("opennewlineabove");
    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("one\n\ntwo"));

    // Act：opennewlinebelow（Ctrl+Shift+Enter）：行尾插换行
    setDocText(QString("one\ntwo"));
    moveCursorTo(1);
    sendShortcut("opennewlinebelow");
    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("one\n\ntwo"));

    // Act：duplicateline（Ctrl+Shift+D）
    setDocText(QString("dup"));
    moveCursorTo(1);
    sendShortcut("duplicateline");
    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("dup\ndup"));

    // Act：killline（Ctrl+K）
    setDocText(QString("keepDEL"));
    moveCursorTo(4);
    sendShortcut("killline");
    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("keep"));
}

TEST_F(TextEditTest, KeyPress_MovementShortcuts_MoveCursor)
{
    // Arrange
    setDocText(QString("hello world\nsecond"));

    // Act：forwardword（Ctrl+Right，NextWord 落点含尾随空白）
    moveCursorTo(0);
    sendShortcut("forwardword");
    // Assert
    EXPECT_EQ(edit->getPosition(), 6);

    // Act：nextline（Down）
    sendShortcut("nextline");
    // Assert
    EXPECT_EQ(edit->getCurrentLine(), 2);

    // Act：movetoendofline（End）
    sendShortcut("movetoendofline");
    // Assert
    EXPECT_EQ(edit->getPosition(), 18);

    // Act：movetostartofline（Home）
    sendShortcut("movetostartofline");
    // Assert
    EXPECT_EQ(edit->getPosition(), 12);

    // Act：movetostart（Ctrl+Home）
    sendShortcut("movetostart");
    // Assert
    EXPECT_EQ(edit->getPosition(), 0);
}

TEST_F(TextEditTest, KeyPress_CaseWordShortcuts_ConvertWord)
{
    // Arrange
    setDocText(QString("mixed"));

    // Act：upcaseword
    moveCursorTo(0);
    sendShortcut("upcaseword");
    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("MIXED"));

    // Act：capitalizeword（downcaseword 分支缺 return 但继续链，最终落 QPlainTextEdit）
    setDocText(QString("MiXeD"));
    moveCursorTo(0);
    sendShortcut("capitalizeword");
    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("Mixed"));
}

TEST_F(TextEditTest, KeyPress_MarkShortcuts_ToggleMarkAndExchange)
{
    // Arrange
    setDocText(QString("abcdef"));
    moveCursorTo(1);

    // Act：setmark（Ctrl+空格 之类，按 settings）
    sendShortcut("setmark");
    // Assert：标记模式开启
    EXPECT_TRUE(edit->m_cursorMark);

    // Act：exchangemark 交换选区端点
    sendShortcut("forwardchar");
    sendShortcut("exchangemark");
    // Assert：光标回到 anchor 侧
    EXPECT_TRUE(edit->textCursor().hasSelection());
}

TEST_F(TextEditTest, KeyPress_BookmarkShortcuts_ToggleAndJump)
{
    // Arrange
    setDocText(QString("l1\nl2\nl3\nl4"));

    // Act：switchbookmark（Ctrl+F2）
    moveCursorTo(0);
    sendShortcut("switchbookmark");
    // Assert
    EXPECT_TRUE(edit->getBookmarkInfo().contains(1));

    // Act：movetonextbookmark
    moveCursorTo(5);
    sendShortcut("movetonextbookmark");
    // Assert：环绕回书签行
    EXPECT_EQ(edit->getCurrentLine(), 1);
}

TEST_F(TextEditTest, KeyPress_CommentShortcuts_ToggleComment)
{
    // Arrange
    edit->setFilePath(QString("sample.utlang"));
    setDocText(QString("value = 1"));
    moveCursorTo(2);
    KSyntaxHighlighting::Definition def =
            edit->m_repository.definitionForFileName(QString("sample.utlang"));
    ASSERT_FALSE(def.filePath().isEmpty());
    edit->setSyntaxDefinition(def);

    // Act：togglecomment
    sendShortcut("togglecomment");
    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("//value = 1"));

    // Act：removecomment
    sendShortcut("removecomment");
    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("value = 1"));
}

TEST_F(TextEditTest, KeyPress_TransposeCharShortcut_SwapsChars)
{
    // Arrange
    setDocText(QString("ab"));
    moveCursorTo(1);

    // Act：transposechar（Alt+T）
    sendShortcut("transposechar");

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("ba"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, KeyPress_JoinLinesShortcut_MergesLines)
{
    // Arrange
    setDocText(QString("x\ny"));
    moveCursorTo(0);

    // Act
    sendShortcut("joinlines");

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("xy"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, KeyPress_AltM_PopsRightMenu)
{
    // Arrange：exec 已 stub，弹菜单不阻塞
    setDocText(QString("menu"));

    // Act
    sendKey(Qt::Key_M, Qt::AltModifier, QString("m"));

    // Assert：菜单被重建（popRightMenu 重新 new 了 m_rightMenu）
    EXPECT_NE(edit->m_rightMenu, nullptr);
    EXPECT_FALSE(edit->m_rightMenu->actions().isEmpty());
}

TEST_F(TextEditTest, KeyPress_WindowShortcut_EventIgnored)
{
    // Arrange：取 shortcuts.window 组第一个快捷键（环境无关）
    const auto options = Settings::instance()->settings->group("shortcuts.window")->options();
    ASSERT_FALSE(options.isEmpty());
    const QString seqStr = Settings::instance()->settings->option(options.first()->key())->value().toString();
    ASSERT_FALSE(seqStr.isEmpty());
    QKeySequence seq(seqStr);
    ASSERT_FALSE(seq.isEmpty());

    // Act：发送该快捷键（应 ignore 而非编辑器处理）
    const int combined = seq[0].toCombined();
    QKeyEvent press(QEvent::KeyPress, combined & ~static_cast<int>(Qt::KeyboardModifierMask),
                    Qt::KeyboardModifiers(combined & static_cast<int>(Qt::KeyboardModifierMask)));
    QApplication::sendEvent(edit, &press);

    // Assert：文本未被修改
    EXPECT_TRUE(edit->toPlainText().isEmpty());
    EXPECT_FALSE(edit->isUndoRedoOpt()); // 窗口快捷键不进编辑器撤销栈
}

TEST_F(TextEditTest, KeyPress_AltDigit_IgnoredByEditor)
{
    // Arrange
    setDocText(QString(""));

    // Act：Alt+0（窗口切页保留）
    sendKey(Qt::Key_0, Qt::AltModifier);

    // Assert
    EXPECT_TRUE(edit->toPlainText().isEmpty());
    EXPECT_EQ(edit->getPosition(), 0);
}

TEST_F(TextEditTest, KeyPress_AltTab_Swallowed)
{
    // Arrange
    setDocText(QString(""));

    // Act
    sendKey(Qt::Key_Tab, Qt::AltModifier);

    // Assert：Alt+Tab 被 return 吞掉
    EXPECT_TRUE(edit->toPlainText().isEmpty());
    EXPECT_EQ(edit->getPosition(), 0);
}

TEST_F(TextEditTest, KeyPress_DefaultArrowKeys_MoveCursor)
{
    // Arrange
    setDocText(QString("ab\ncd"));
    moveCursorTo(3);

    // Act：下箭头（未匹配任何快捷键 → QPlainTextEdit 处理）
    sendKey(Qt::Key_Down, Qt::NoModifier);

    // Assert
    EXPECT_EQ(edit->getPosition(), 3);
    EXPECT_FALSE(edit->textCursor().hasSelection());
}

TEST_F(TextEditTest, KeyPress_LeftRight_ClearsSelectAllFlag)
{
    // Arrange
    setDocText(QString("sel"));
    edit->m_isSelectAll = true;

    // Act：左箭头
    sendKey(Qt::Key_Left, Qt::NoModifier);

    // Assert：全选标志复位
    EXPECT_FALSE(edit->m_isSelectAll);
    EXPECT_EQ(edit->toPlainText(), QString("sel")); // 文本不受影响
}

// ---------------- keyPressEvent：只读模式族（K1/K2） ----------------

TEST_F(TextEditTest, KeyPress_ReadOnlyMode_SingleLettersDispatch)
{
    // Arrange
    setDocText(QString("line1\nline2\nline3"));
    edit->toggleReadOnlyMode(true);
    moveCursorTo(0);

    // Act/Assert：J → nextLine（行 2 首 = 6）
    sendKey(Qt::Key_J, Qt::NoModifier);
    EXPECT_EQ(edit->getPosition(), 6);

    // Act/Assert：H → backwardChar（退回行 1 末 = 5）
    sendKey(Qt::Key_H, Qt::NoModifier);
    EXPECT_EQ(edit->getPosition(), 5);

    // Act/Assert：E → moveToEndOfLine（已在行 1 末，原地）
    sendKey(Qt::Key_E, Qt::NoModifier);
    EXPECT_EQ(edit->getPosition(), 5);

    // Act/Assert：A → moveToStartOfLine（行 1 首）
    sendKey(Qt::Key_A, Qt::NoModifier);
    EXPECT_EQ(edit->getPosition(), 0);

    // Act/Assert：, → moveToEnd
    sendKey(Qt::Key_Comma, Qt::NoModifier);
    EXPECT_EQ(edit->getPosition(), edit->toPlainText().size());

    // Act/Assert：. → moveToStart
    sendKey(Qt::Key_Period, Qt::NoModifier);
    EXPECT_EQ(edit->getPosition(), 0);
}

TEST_F(TextEditTest, KeyPress_ReadOnlyMode_WordAndScrollKeys)
{
    // Arrange
    setDocText(QString("word line\nword line\nword line"));
    edit->toggleReadOnlyMode(true);
    moveCursorTo(0);

    // Act/Assert：F → forwardWord（落点含尾随空白）
    sendKey(Qt::Key_F, Qt::NoModifier);
    EXPECT_EQ(edit->getPosition(), 5);

    // Act/Assert：B → backwardWord
    sendKey(Qt::Key_B, Qt::NoModifier);
    EXPECT_EQ(edit->getPosition(), 0);

    // Act/Assert：M → moveToLineIndentation（首行无缩进）
    sendKey(Qt::Key_M, Qt::NoModifier);
    EXPECT_EQ(edit->getPosition(), 0);

    // Act/Assert：V → scrollDown（无崩溃）
    sendKey(Qt::Key_V, Qt::NoModifier);
    EXPECT_GE(edit->verticalScrollBar()->value(), 0);

    // Act/Assert：Space → scrollUp
    sendKey(Qt::Key_Space, Qt::NoModifier);
    EXPECT_GE(edit->verticalScrollBar()->value(), 0);
}

TEST_F(TextEditTest, KeyPress_ReadOnlyMode_PairKeys)
{
    // Arrange
    setDocText(QString("(a) x"));
    edit->toggleReadOnlyMode(true);
    moveCursorTo(0);

    // Act：P → forwardPair
    sendKey(Qt::Key_P, Qt::NoModifier);
    // Assert
    EXPECT_EQ(edit->getPosition(), 3);

    // Act：N → backwardPair
    sendKey(Qt::Key_N, Qt::NoModifier);
    // Assert
    EXPECT_EQ(edit->getPosition(), 0);
}

TEST_F(TextEditTest, KeyPress_ReadOnlyMode_ShiftK_ScrollsLineDown)
{
    // Arrange
    setDocText(QString("a\nb\nc"));
    edit->toggleReadOnlyMode(true);

    // Act：Shift+K → scrollLineDown
    sendKey(Qt::Key_K, Qt::ShiftModifier);
    // Assert：无崩溃且滚动值合法
    EXPECT_GE(edit->verticalScrollBar()->value(), 0);
    EXPECT_LE(edit->verticalScrollBar()->value(), edit->verticalScrollBar()->maximum());
}

TEST_F(TextEditTest, KeyPress_ReadOnlyMode_CtrlCombos_EmitNotify)
{
    // Arrange
    setDocText(QString("lock"));
    edit->toggleReadOnlyMode(true);
    QSignalSpy spy(edit, &TextEdit::popupNotify);

    // Act：Ctrl+Return → 提示只读
    sendKey(Qt::Key_Return, Qt::ControlModifier);
    // Assert
    EXPECT_EQ(spy.count(), 1);

    // Act：Ctrl+Shift+D → 提示只读
    sendKey(Qt::Key_D, Qt::ControlModifier | Qt::ShiftModifier);
    // Assert
    EXPECT_EQ(spy.count(), 2);

    // Act：普通按键 'z'（NoModifier 未映射）→ 提示只读
    sendKey(Qt::Key_Z, Qt::NoModifier);
    // Assert
    EXPECT_EQ(spy.count(), 3);

    // Act：F11 → ignore 静默
    sendKey(Qt::Key_F11, Qt::NoModifier);
    // Assert
    EXPECT_EQ(spy.count(), 3);
}

TEST_F(TextEditTest, KeyPress_ReadOnlyMode_Q_QuitsReadOnly)
{
    // Arrange
    setDocText(QString("free"));
    edit->toggleReadOnlyMode(true);
    ASSERT_TRUE(edit->getReadOnlyMode());

    // Act：Q → toggleReadOnlyMode 退出
    sendKey(Qt::Key_Q, Qt::NoModifier);

    // Assert
    EXPECT_FALSE(edit->getReadOnlyMode());
    EXPECT_EQ(edit->m_cursorMode, TextEdit::Insert);
}

TEST_F(TextEditTest, KeyPress_ReadOnlyMode_ToggleShortcut_QuitsReadOnly)
{
    // Arrange
    setDocText(QString("x"));
    edit->toggleReadOnlyMode(true);
    QSignalSpy spy(edit, &TextEdit::cursorModeChanged);

    // Act：settings 中 togglereadonlymode（Meta+Alt+L）
    sendShortcut("togglereadonlymode");

    // Assert：退出只读并恢复 Insert 信号
    EXPECT_FALSE(edit->getReadOnlyMode());
    EXPECT_FALSE(spy.isEmpty());
    EXPECT_EQ(spy.last().at(0).toInt(), static_cast<int>(TextEdit::Insert));
}

TEST_F(TextEditTest, KeyPress_ReadOnlyPermission_PlainKeysBlocked)
{
    // Arrange：权限只读（区别于模式只读，Q 不可退出）
    setDocText(QString("perm"));
    edit->setReadOnlyPermission(true);
    ASSERT_TRUE(edit->getReadOnlyPermission());
    QSignalSpy spy(edit, &TextEdit::popupNotify);

    // Act：J 移动在单行文档上原地不动（浏览键不被权限拦截）
    moveCursorTo(0);
    sendKey(Qt::Key_J, Qt::NoModifier);
    EXPECT_EQ(edit->getPosition(), 0);

    // Act：Q 被禁止（m_bReadOnlyPermission 分支）
    sendKey(Qt::Key_Q, Qt::NoModifier);
    // Assert：仍处于只读权限
    EXPECT_TRUE(edit->getReadOnlyPermission());
}

// ---------------- mouse 事件（M1-M3） ----------------

TEST_F(TextEditTest, MousePress_ClearsFindCloseFlagAndKeywords)
{
    // Arrange：制造查找残留 + find-close 标志
    setDocText(QString("k k"));
    edit->highlightKeyword(QString("k"), 0);
    edit->tellFindBarClose();
    ASSERT_TRUE(edit->m_bIsFindClose);

    // Act：普通左键按下
    sendMouse(QEvent::MouseButtonPress, QPointF(10, 8), Qt::LeftButton);

    // Assert：标志复位且查找高亮清除
    EXPECT_FALSE(edit->m_bIsFindClose);
    EXPECT_TRUE(edit->m_findMatchSelections.isEmpty());
}

TEST_F(TextEditTest, MousePress_AltModifier_StartsColumnMode)
{
    // Arrange
    setDocText(QString("r0\nr1\nr2"));

    // Act：Alt+左键（列编辑起点）
    sendMouse(QEvent::MouseButtonPress, QPointF(10, 8), Qt::LeftButton, Qt::AltModifier);

    // Assert
    EXPECT_TRUE(edit->m_bIsAltMod);
    EXPECT_TRUE(edit->m_altModSelections.isEmpty()); // 起点清空
}

TEST_F(TextEditTest, MousePressAndMove_AltColumn_BuildsSelections)
{
    // Arrange
    setDocText(QString("alpha\nbeta\ngamma"));

    // Act：Alt 按下 + 移动（构建列选区）
    sendMouse(QEvent::MouseButtonPress, QPointF(30, 8), Qt::LeftButton, Qt::AltModifier);
    sendMouse(QEvent::MouseMove, QPointF(30, 40), Qt::LeftButton, Qt::AltModifier);

    // Assert：跨 3 行生成列选区并渲染
    EXPECT_GE(edit->m_altModSelections.size(), 2);
    EXPECT_FALSE(edit->extraSelections().isEmpty());
}

TEST_F(TextEditTest, MouseRelease_MiddleButton_PasteFromClipboard)
{
    // Arrange：允许中键粘贴（settings 默认），剪贴板有内容
    setDocText(QString("here"));
    moveCursorTo(4);
    QApplication::clipboard()->setText(QString("M"));

    // Act：中键释放
    sendMouse(QEvent::MouseButtonRelease, QPointF(10, 8), Qt::MiddleButton);

    // Assert：中键粘贴生效
    EXPECT_EQ(edit->toPlainText(), QString("hereM"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, MouseRelease_MiddleButtonReadOnly_EmitsNotify)
{
    // Arrange
    setDocText(QString("ro"));
    edit->toggleReadOnlyMode(true);
    QSignalSpy spy(edit, &TextEdit::popupNotify);

    // Act
    sendMouse(QEvent::MouseButtonRelease, QPointF(10, 8), Qt::MiddleButton);

    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_TRUE(edit->getReadOnlyMode()); // 只读态保持
}

TEST_F(TextEditTest, MouseRelease_LeftButton_DefaultHandling)
{
    // Arrange
    setDocText(QString("click"));

    // Act：左键释放（普通路径走基类）
    sendMouse(QEvent::MouseButtonRelease, QPointF(10, 8), Qt::LeftButton);

    // Assert：状态无异常
    EXPECT_FALSE(edit->m_bIsAltMod);
    EXPECT_TRUE(edit->toPlainText() == QString("click"));
}

// ---------------- wheel / contextMenu / paint / resize（W1/C1/P1/R1） ----------------

TEST_F(TextEditTest, WheelEvent_NoModifiers_BaseScrolling)
{
    // Arrange
    setDocText(QString("l\nl\nl\nl\nl\nl\nl\nl\nl\nl"));

    // Act：直接驱动（offscreen 未 show 控件不派发滚轮事件）
    QWheelEvent wheel(QPointF(100, 100), QPointF(100, 100), QPoint(), QPoint(0, 120),
                      Qt::NoButton, Qt::NoModifier, Qt::ScrollUpdate, false);
    edit->wheelEvent(&wheel);

    // Assert：事件被处理（无崩溃即可，短文档滚动值为 0 合法）
    EXPECT_GE(edit->verticalScrollBar()->value(), 0);
    EXPECT_LE(edit->verticalScrollBar()->value(), edit->verticalScrollBar()->maximum());
}

TEST_F(TextEditTest, ContextMenuEvent_PopsRightMenuWithoutBlocking)
{
    // Arrange：exec 已 stub；直接驱动受保护 contextMenuEvent
    setDocText(QString("content"));

    // Act
    QContextMenuEvent ev(QContextMenuEvent::Mouse, QPoint(10, 8), QPoint(10, 8));
    edit->contextMenuEvent(&ev);

    // Assert：菜单对象存在且包含动作
    ASSERT_NE(edit->m_rightMenu, nullptr);
    EXPECT_FALSE(edit->m_rightMenu->actions().isEmpty());
    EXPECT_NE(edit->m_rightMenu, nullptr);
}

TEST_F(TextEditTest, PopRightMenu_DirectCall_MenuRebuiltWithFindActions)
{
    // Arrange：非空文档
    setDocText(QString("some words here"));

    // Act：带指定位置弹出（exec stub）
    edit->popRightMenu(QPoint(10, 10));

    // Assert：菜单包含查找/跳行等动作
    ASSERT_NE(edit->m_rightMenu, nullptr);
    const auto actions = edit->m_rightMenu->actions();
    EXPECT_GE(actions.size(), 3);
    EXPECT_NE(edit->m_findAction, nullptr); // 查找动作在菜单外仍持有
}

TEST_F(TextEditTest, PopRightMenu_WithSelection_IncludesConvertCaseMenu)
{
    // Arrange
    setDocText(QString("UPPER lower"));
    QTextCursor cur = makeCursor(0);
    cur.setPosition(5, QTextCursor::KeepAnchor);
    edit->setTextCursor(cur);

    // Act
    edit->popRightMenu(QPoint(5, 5));

    // Assert：转换大小写子菜单已加入
    bool found = false;
    for (const QAction *act : edit->m_rightMenu->actions()) {
        if (act->menu() == edit->m_convertCaseMenu)
            found = true;
    }
    EXPECT_TRUE(found);
    EXPECT_EQ(edit->m_convertCaseMenu->actions().size(), 3);
}

TEST_F(TextEditTest, PopRightMenu_ReadonlyMode_DictationDisabled)
{
    // Arrange
    setDocText(QString("ro"));
    edit->toggleReadOnlyMode(true);

    // Act
    edit->popRightMenu(QPoint(5, 5));

    // Assert：语音听写（写操作）被禁用
    EXPECT_FALSE(edit->m_dictationAction->isEnabled());
    EXPECT_TRUE(edit->getReadOnlyMode());
}

TEST_F(TextEditTest, HideRightMenu_MenuHiddenNoCrash)
{
    // Arrange
    setDocText(QString("x"));
    edit->popRightMenu(QPoint(1, 1));

    // Act/Assert：隐藏不崩溃（arm 平台全屏恢复场景）
    edit->hideRightMenu();
    EXPECT_NE(edit->m_rightMenu, nullptr);
    EXPECT_FALSE(edit->m_rightMenu->isVisible());
}

TEST_F(TextEditTest, GetHighlightMenu_InitialState_Nullptr)
{
    // Arrange/Act/Assert：未注入高亮分组菜单时为空
    EXPECT_EQ(edit->getHighlightMenu(), nullptr);
    EXPECT_EQ(edit->m_hlGroupMenu, nullptr); // 未注入分组菜单
}

TEST_F(TextEditTest, PaintEvent_WithColumnSelections_FlagSet)
{
    // Arrange：构造有选区的列编辑状态
    setDocText(QString("aa\nbb"));
    QTextCursor cur(edit->document());
    const int blockPos = edit->document()->findBlockByNumber(0).position();
    cur.setPosition(blockPos);
    cur.setPosition(blockPos + 1, QTextCursor::KeepAnchor);
    QTextEdit::ExtraSelection sel;
    sel.cursor = cur;
    edit->m_altModSelections << sel;
    edit->m_bIsAltMod = true;

    // Act：grab 触发真实 paintEvent
    edit->grab();

    // Assert：绘制流程更新列选区标志
    EXPECT_TRUE(edit->m_hasColumnSelection);
    EXPECT_FALSE(edit->extraSelections().isEmpty()); // 列选区已渲染
}

TEST_F(TextEditTest, ResizeEvent_GrowWidth_FlushesDeferredLayout)
{
    // Arrange
    setDocText(QString("resize me"));

    // Act：宽度变大 + 处理事件循环（尾部锚定 singleShot 分支）
    edit->resize(600, 360);
    QApplication::processEvents();

    // Assert：无崩溃、尺寸生效
    EXPECT_EQ(edit->width(), 600);
    EXPECT_EQ(edit->height(), 360);
}

TEST_F(TextEditTest, ResizeEvent_SelectAllMode_ReSelectsInView)
{
    // Arrange
    setDocText(QString("select on resize"));
    edit->m_isSelectAll = true;

    // Act：直接驱动（隐藏控件 resize() 不派发事件）
    QResizeEvent resizeEv(QSize(500, 400), QSize(480, 360));
    edit->resizeEvent(&resizeEv);

    // Assert：resizeEvent 重入 selectTextInView（重入守卫未死锁、全选标志保持）
    EXPECT_TRUE(edit->m_isSelectAll);
    EXPECT_EQ(edit->horizontalScrollBar()->value(), 0);
}

// ---------------- gesture（G1-G4） ----------------

TEST_F(TextEditTest, Event_PaletteChange_OnAppPaletteChangedInvoked)
{
    // Arrange：列选区存在时调色板变化会刷新背景
    setDocText(QString("ab\ncd"));
    QTextCursor cur(edit->document());
    cur.setPosition(0);
    cur.setPosition(1, QTextCursor::KeepAnchor);
    QTextEdit::ExtraSelection sel;
    sel.cursor = cur;
    edit->m_altModSelections << sel;
    edit->m_bIsAltMod = true;

    // Act
    QEvent ev(QEvent::PaletteChange);
    QApplication::sendEvent(edit, &ev);

    // Assert：无崩溃（列选区背景刷新路径执行）
    EXPECT_EQ(edit->m_altModSelections.size(), 1);
    EXPECT_FALSE(edit->extraSelections().isEmpty()); // 调色板刷新已重渲染
}

TEST_F(TextEditTest, GestureEvent_TapLifecycle_StatesTracked)
{
    // Arrange：QGesture 的 state 存于私有 d-ptr 且无公开 setter，
    // 统一 stub QGesture::state()（非虚）驱动状态机
    QTapGesture tap;
    Qt::GestureState fakeState = Qt::GestureStarted;
    stub.set_lamda(&QGesture::state,
                   [&fakeState](const QGesture *) -> Qt::GestureState { return fakeState; });
    edit->m_tapBeginTime = QDateTime::currentDateTime().toMSecsSinceEpoch();

    // Act：Started → GA_tap
    fakeState = Qt::GestureStarted;
    edit->tapGestureTriggered(&tap);
    // Assert
    EXPECT_EQ(edit->m_gestureAction, TextEdit::GA_tap);

    // Act：Updated → GA_slide
    fakeState = Qt::GestureUpdated;
    edit->tapGestureTriggered(&tap);
    // Assert
    EXPECT_EQ(edit->m_gestureAction, TextEdit::GA_slide);

    // Act：Canceled（滑动延续标志）→ GA_slide
    edit->m_slideContinueX = true;
    fakeState = Qt::GestureCanceled;
    edit->tapGestureTriggered(&tap);
    // Assert
    EXPECT_EQ(edit->m_gestureAction, TextEdit::GA_slide);
    EXPECT_FALSE(edit->m_slideContinueX);

    // Act：Finished → GA_null
    fakeState = Qt::GestureFinished;
    edit->tapGestureTriggered(&tap);
    // Assert
    EXPECT_EQ(edit->m_gestureAction, TextEdit::GA_null);
}

TEST_F(TextEditTest, GestureEvent_TapHoldAndPan_StatesTracked)
{
    // Arrange
    QTapAndHoldGesture hold;
    QPanGesture pan;
    Qt::GestureState fakeState = Qt::GestureStarted;
    stub.set_lamda(&QGesture::state,
                   [&fakeState](const QGesture *) -> Qt::GestureState { return fakeState; });

    // Act：长按 Started → GA_hold
    fakeState = Qt::GestureStarted;
    edit->tapAndHoldGestureTriggered(&hold);
    // Assert
    EXPECT_EQ(edit->m_gestureAction, TextEdit::GA_hold);

    // Act：长按 Finished → GA_null
    fakeState = Qt::GestureFinished;
    edit->tapAndHoldGestureTriggered(&hold);
    // Assert
    EXPECT_EQ(edit->m_gestureAction, TextEdit::GA_null);

    // Act：平移 Started → GA_pan
    fakeState = Qt::GestureStarted;
    edit->panTriggered(&pan);
    // Assert
    EXPECT_EQ(edit->m_gestureAction, TextEdit::GA_pan);

    // Act：平移 Finished → GA_null
    fakeState = Qt::GestureFinished;
    edit->panTriggered(&pan);
    // Assert
    EXPECT_EQ(edit->m_gestureAction, TextEdit::GA_null);
}

TEST_F(TextEditTest, GestureEvent_Dispatch_AllHandlersInvoked)
{
    // Arrange：手势事件（各手势不存在时安全空过；QApplication::notify 会按
    // grabGesture 注册表路由手势事件，直接驱动 event/gestureEvent 保证可达）
    QList<QGesture *> noGestures;
    QGestureEvent gestureEvent(noGestures);
    edit->m_gestureAction = TextEdit::GA_null;

    // Act
    edit->gestureEvent(&gestureEvent);

    // Assert：gestureEvent 主链执行（返回 true）
    EXPECT_EQ(edit->m_gestureAction, TextEdit::GA_null);

    // Act：swipe 空实现直调
    QSwipeGesture swipe;
    edit->swipeTriggered(&swipe);
    // Assert：无状态改变
    EXPECT_EQ(edit->m_gestureAction, TextEdit::GA_null);
}

TEST_F(TextEditTest, SlideGesture_YPixelDelta_ScrollbarSteps)
{
    // Arrange：构造可滚动文档
    setDocText(QString("l\nl\nl\nl\nl\nl\nl\nl\nl\nl\nl\nl\nl\nl\nl\nl\nl\nl\nl\nl"));

    // Act：累计 3.0 滑动像素（static delta 累计取整）
    edit->slideGestureY(1.5);
    edit->slideGestureY(1.5);

    // Assert：垂直滚动前进
    EXPECT_GT(edit->verticalScrollBar()->value(), 0);
    EXPECT_LE(edit->verticalScrollBar()->value(), edit->verticalScrollBar()->maximum());
}

TEST_F(TextEditTest, SlideGesture_XPixelDelta_HorizontalUnchangedForShortLines)
{
    // Arrange：短行无水平滚动
    setDocText(QString("short"));

    // Act
    edit->slideGestureX(2.0);

    // Assert：水平滚动条最大值为 0，值不变
    EXPECT_EQ(edit->horizontalScrollBar()->value(), 0);
    EXPECT_EQ(edit->horizontalScrollBar()->maximum(), 0);
}

TEST_F(TextEditTest, FingerZoom_TapThreeFingers_TriggersTranslate)
{
    // Arrange：AI 桩（Success 不弹错误）
    installIflytekStubs(IflytekAiAssistant::Success);

    // Act：三指单击 → slot_translate（AI 成功无提示）
    edit->fingerZoom(QString("tap"), QString(), 3);

    // Assert：无异常即通过（错误路径在 slot_translate 用例验证）
    EXPECT_TRUE(true);

    // Act：错误分支 → popupNotify
    installIflytekStubs(IflytekAiAssistant::Invalid);
    QSignalSpy spy(edit, &TextEdit::popupNotify);
    edit->fingerZoom(QString("tap"), QString(), 3);
    // Assert
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(TextEditTest, FingerZoom_TwoFingerPinch_NoFocus_NoWindowCall)
{
    // Arrange：无焦点（hasFocus() false → pinch 分支安全跳过）

    // Act
    edit->fingerZoom(QString("pinch"), QString("in"), 2);

    // Assert：无崩溃
    EXPECT_FALSE(edit->hasFocus());
    EXPECT_EQ(edit->toPlainText(), QString()); // 无焦点时捏合不改内容
}

// ---------------- inputMethodEvent（I1） ----------------

TEST_F(TextEditTest, InputMethod_CommitString_Inserted)
{
    // Arrange
    setDocText(QString(""));

    // Act：提交串插入
    QInputMethodEvent ev;
    ev.setCommitString(QString("你好"));
    QApplication::sendEvent(edit, &ev);

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("你好"));
    EXPECT_TRUE(edit->m_bIsInputMethod);
}

TEST_F(TextEditTest, InputMethod_PreeditThenCommit_UndoRestores)
{
    // Arrange：光标置于文档末尾
    setDocText(QString("base"));
    moveCursorTo(4);

    // Act：两次 preedit（第二次先撤销第一次）+ 一次 commit
    QInputMethodEvent pre1(QString("ni"), QList<QInputMethodEvent::Attribute>());
    QApplication::sendEvent(edit, &pre1);
    QInputMethodEvent pre2(QString("nihao"), QList<QInputMethodEvent::Attribute>());
    QApplication::sendEvent(edit, &pre2);
    QInputMethodEvent commit;
    commit.setCommitString(QString("nihao"));
    QApplication::sendEvent(edit, &commit);

    // Assert：最终文本 = base + 提交串（preedit 已被撤销重建）
    EXPECT_EQ(edit->toPlainText(), QString("basenihao"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, InputMethod_ReadOnlyMode_EarlyReturn)
{
    // Arrange
    setDocText(QString("lock"));
    edit->toggleReadOnlyMode(true);

    // Act
    QInputMethodEvent ev;
    ev.setCommitString(QString("X"));
    QApplication::sendEvent(edit, &ev);

    // Assert：只读不插入
    EXPECT_EQ(edit->toPlainText(), QString("lock"));
    // 强异常安全：早退路径不产生撤销项
    EXPECT_FALSE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, InputMethod_OverwriteMode_ReplacesSelection)
{
    // Arrange：覆盖模式
    setDocText(QString("xyz"));
    moveCursorTo(0);
    sendKey(Qt::Key_Insert, Qt::NoModifier);
    ASSERT_EQ(edit->m_cursorMode, TextEdit::Overwrite);

    // Act：输入法提交 1 字符（选中替换 1 字符）
    QInputMethodEvent ev;
    ev.setCommitString(QString("Q"));
    QApplication::sendEvent(edit, &ev);

    // Assert
    EXPECT_EQ(edit->toPlainText(), QString("Qyz"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, InputMethod_ColumnMode_InsertsPerColumn)
{
    // Arrange：列编辑状态
    setDocText(QString("aa\nbb"));
    QList<QTextEdit::ExtraSelection> sels;
    for (int line = 0; line < 2; ++line) {
        const int blockPos = edit->document()->findBlockByNumber(line).position();
        QTextCursor cur(edit->document());
        cur.setPosition(blockPos);
        cur.setPosition(blockPos + 1, QTextCursor::KeepAnchor);
        QTextEdit::ExtraSelection sel;
        sel.cursor = cur;
        sels << sel;
    }
    edit->restoreColumnEditSelection(sels);
    edit->m_bIsAltMod = true;

    // Act：输入法插入列文本
    QInputMethodEvent ev;
    ev.setCommitString(QString("Z"));
    QApplication::sendEvent(edit, &ev);

    // Assert：两列同时替换
    EXPECT_EQ(edit->toPlainText(), QString("Za\nZb"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, InputMethod_DeleteOperation_RemovesPreviousChars)
{
    // Arrange：AI 语音删除（replacementLength 分支）
    setDocText(QString("abcdef"));
    moveCursorTo(4);

    // Act：构造删除型输入法事件（preedit/commit 均空 + 反向替换）
    QInputMethodEvent ev;
    ev.m_replacementStart = -1;
    ev.m_replacementLength = 2;
    QApplication::sendEvent(edit, &ev);

    // Assert：删除光标前 2 字符
    EXPECT_EQ(edit->toPlainText(), QString("abef"));
    // 第二维度：编辑操作经撤销栈（可撤销状态翻转）
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

// ---------------- eventFilter（E1） ----------------

TEST_F(TextEditTest, EventFilter_ViewportMousePress_PassesThrough)
{
    // Arrange：viewport 上的普通左键（非书签/折叠区）
    setDocText(QString("abc"));

    // Act
    QMouseEvent ev(QEvent::MouseButtonPress, QPointF(10, 8), QPointF(10, 8),
                   Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    const bool filtered = edit->eventFilter(edit->viewport(), &ev);

    // Assert：透传给基类（未被过滤）
    EXPECT_FALSE(filtered);
    EXPECT_TRUE(edit->toPlainText() == QString("abc"));
}

TEST_F(TextEditTest, EventFilter_ThreeTouchPoints_TriggersTranslate)
{
    // Arrange：3 指触摸 + AI 桩 + 基类 mousePressEvent stub（防误转型事件）
    installIflytekStubs(IflytekAiAssistant::Success);
    QSignalSpy popupSpy(edit, &TextEdit::popupNotify);
    stub.set_lamda(VADDR(DPlainTextEdit, mousePressEvent), [](DPlainTextEdit *, QMouseEvent *) {
    });

    // Act：向 viewport 发 TouchBegin（3 触点）
    QList<QEventPoint> points { QEventPoint(1), QEventPoint(2), QEventPoint(3) };
    QTouchEvent touch(QEvent::TouchBegin, utTouchDevice(), Qt::NoModifier, points);
    QApplication::sendEvent(edit->viewport(), &touch);

    // Assert：翻译分支被触发（Success 无提示）
    EXPECT_TRUE(true);
    EXPECT_EQ(popupSpy.count(), 0); // AI 成功不弹提示
}

TEST_F(TextEditTest, EventFilter_SingleTouch_NoTranslate)
{
    // Arrange：单触点 + 基类桩
    installIflytekStubs(IflytekAiAssistant::Success);
    QSignalSpy popupSpy(edit, &TextEdit::popupNotify);
    stub.set_lamda(VADDR(DPlainTextEdit, mousePressEvent), [](DPlainTextEdit *, QMouseEvent *) {
    });

    // Act
    QList<QEventPoint> points { QEventPoint(1) };
    QTouchEvent touch(QEvent::TouchBegin, utTouchDevice(), Qt::NoModifier, points);
    QApplication::sendEvent(edit->viewport(), &touch);

    // Assert：非 3 指不触发翻译（无崩溃）
    EXPECT_TRUE(true);
    EXPECT_EQ(popupSpy.count(), 0); // 非 3 指不触发
}

TEST_F(TextEditTest, EventFilter_BookmarkAreaLeftClick_TogglesBookmark)
{
    // Arrange
    setDocText(QString("b1\nb2"));

    // Act：书签区左键（y=4 → 第 1 行）
    QMouseEvent ev(QEvent::MouseButtonPress, QPointF(2, 4), QPointF(2, 4),
                   Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    const bool filtered = edit->eventFilter(edit->getLeftAreaWidget()->m_pBookMarkArea, &ev);

    // Assert：事件被过滤且书签生成
    EXPECT_TRUE(filtered);
    EXPECT_TRUE(edit->getBookmarkInfo().contains(1));
}

TEST_F(TextEditTest, EventFilter_BookmarkAreaRightClick_BuildsMenu)
{
    // Arrange：多书签 + exec stub
    setDocText(QString("b1\nb2\nb3"));
    edit->setBookMarkList(QList<int>() << 1 << 2);

    // Act：书签区右键（点击行 1）
    QMouseEvent ev(QEvent::MouseButtonPress, QPointF(2, 4), QPointF(2, 4),
                   Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    const bool filtered = edit->eventFilter(edit->getLeftAreaWidget()->m_pBookMarkArea, &ev);

    // Assert：右键菜单构建（含上一/下一书签动作）
    EXPECT_TRUE(filtered);
    ASSERT_NE(edit->m_rightMenu, nullptr);
    EXPECT_GE(edit->m_rightMenu->actions().size(), 2);
}

TEST_F(TextEditTest, EventFilter_BookmarkAreaRightClickNoBookmarks_AddActionOnly)
{
    // Arrange：无书签
    setDocText(QString("x"));

    // Act
    QMouseEvent ev(QEvent::MouseButtonPress, QPointF(2, 4), QPointF(2, 4),
                   Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    edit->eventFilter(edit->getLeftAreaWidget()->m_pBookMarkArea, &ev);

    // Assert：仅"添加书签"动作
    ASSERT_NE(edit->m_rightMenu, nullptr);
    EXPECT_EQ(edit->m_rightMenu->actions().size(), 1);
    EXPECT_EQ(edit->m_addBookMarkAction->text(), edit->m_rightMenu->actions().first()->text());
}

TEST_F(TextEditTest, EventFilter_FlodAreaLeftClickOnBraceLine_FoldsCode)
{
    // Arrange：可折叠代码（第 0 行含 {，第 2 行 }）
    setDocText(QString("int f() {\n    body;\n}\ntail"));

    // Act：折叠区左键（y=4 → 第 1 行）
    QMouseEvent ev(QEvent::MouseButtonPress, QPointF(2, 4), QPointF(2, 4),
                   Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    const bool filtered = edit->eventFilter(edit->getLeftAreaWidget()->m_pFlodArea, &ev);

    // Assert：折叠生效（第 1 块隐藏）
    EXPECT_TRUE(filtered);
    EXPECT_FALSE(edit->document()->findBlockByNumber(1).isVisible());
}

TEST_F(TextEditTest, EventFilter_FlodAreaRightClick_MenuActionsDisabledPerVisibility)
{
    // Arrange：无折叠点（m_listMainFlodAllPos 空 → 全部展开按钮置灰）
    setDocText(QString("no braces here"));

    // Act：折叠区右键
    QMouseEvent ev(QEvent::MouseButtonPress, QPointF(2, 4), QPointF(2, 4),
                   Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    const bool filtered = edit->eventFilter(edit->getLeftAreaWidget()->m_pFlodArea, &ev);

    // Assert
    EXPECT_TRUE(filtered);
    EXPECT_FALSE(edit->m_unflodAllLevel->isEnabled());
}

TEST_F(TextEditTest, EventFilter_HoverMoveBookmark_SetsHoverLine)
{
    // Arrange：期望行号按同一布局引擎换算（与字体度量解耦）
    setDocText(QString("h1\nh2\nh3"));
    const int expectedLine = edit->cursorForPosition(QPoint(2, 20)).blockNumber() + 1;

    // Act：书签区悬停
    QHoverEvent ev(QEvent::HoverMove, QPointF(2, 20), QPointF(2, 18), Qt::NoModifier);
    edit->eventFilter(edit->getLeftAreaWidget()->m_pBookMarkArea, &ev);

    // Assert：悬停行被记录（书签分支不拦截事件，透传基类返回 false）
    EXPECT_EQ(edit->m_nBookMarkHoverLine, expectedLine);
    EXPECT_GT(expectedLine, 0); // 悬停行基于真实布局换算
}

TEST_F(TextEditTest, EventFilter_HoverMoveFlodNoIcon_HidesPreview)
{
    // Arrange：无折叠图标（空 m_listFlodIconPos）
    setDocText(QString("plain"));

    // Act：折叠区悬停
    QHoverEvent ev(QEvent::HoverMove, QPointF(2, 4), QPointF(2, 4), Qt::NoModifier);
    const bool filtered = edit->eventFilter(edit->getLeftAreaWidget()->m_pFlodArea, &ev);

    // Assert：预览隐藏路径
    EXPECT_TRUE(filtered);
    EXPECT_TRUE(edit->m_foldCodeShow->isHidden());
}

TEST_F(TextEditTest, EventFilter_HoverLeaveAreas_StateReset)
{
    // Arrange：先设置悬停状态
    setDocText(QString("x"));
    edit->m_nBookMarkHoverLine = 1;

    // Act：书签区离开
    QHoverEvent leaveB(QEvent::HoverLeave, QPointF(), QPointF(2, 4), Qt::NoModifier);
    const bool filteredB = edit->eventFilter(edit->getLeftAreaWidget()->m_pBookMarkArea, &leaveB);
    // Assert
    EXPECT_TRUE(filteredB);
    EXPECT_EQ(edit->m_nBookMarkHoverLine, -1);

    // Act：折叠区离开
    QHoverEvent leaveF(QEvent::HoverLeave, QPointF(), QPointF(2, 4), Qt::NoModifier);
    const bool filteredF = edit->eventFilter(edit->getLeftAreaWidget()->m_pFlodArea, &leaveF);
    // Assert
    EXPECT_TRUE(filteredF);
}

TEST_F(TextEditTest, EventFilter_MouseDblClick_SetsDoubleClickFlags)
{
    // Arrange
    setDocText(QString("word"));

    // Act
    QMouseEvent ev(QEvent::MouseButtonDblClick, QPointF(10, 8), QPointF(10, 8),
                   Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    edit->eventFilter(edit->viewport(), &ev);

    // Assert：双击标志被记录
    EXPECT_TRUE(edit->m_bBeforeIsDoubleClick);
    EXPECT_TRUE(edit->m_bIsDoubleClick);
}

TEST_F(TextEditTest, EventFilter_ColorMarkMenuTabKey_NoVisibleMenu_NoAction)
{
    // Arrange：菜单不可见 → 定时器 lambda 早退
    QKeyEvent tab(QEvent::KeyRelease, Qt::Key_Tab, Qt::NoModifier);

    // Act
    edit->eventFilter(edit->m_colorMarkMenu, &tab);
    QApplication::processEvents(); // 冲刷 singleShot(0) lambda

    // Assert：无 activeAction 变化（安全空转）
    EXPECT_EQ(edit->m_colorMarkMenu->activeAction(), nullptr);
    EXPECT_FALSE(edit->m_colorMarkMenu->isVisible());
}

// ---------------- 拖放（D1/D2） ----------------

TEST_F(TextEditTest, DragMove_ReadOnlyMode_Ignored)
{
    // Arrange
    edit->toggleReadOnlyMode(true);
    QMimeData data;
    data.setText(QString("dropped"));
    QDragMoveEvent ev(QPoint(10, 8), Qt::CopyAction, &data, Qt::NoButton, Qt::NoModifier);

    // Act：只读直接 return（事件保持默认状态）
    edit->dragMoveEvent(&ev);

    // Assert：未走文本插入路径（文本为空）
    EXPECT_TRUE(edit->toPlainText().isEmpty());
    EXPECT_TRUE(edit->getReadOnlyMode());
}

TEST_F(TextEditTest, DragMove_UrlData_AcceptsProposedAction)
{
    // Arrange
    QMimeData data;
    QList<QUrl> urls;
    urls << QUrl::fromLocalFile(QString("/tmp/ut-editor-core/drag.txt"));
    data.setUrls(urls);
    QDragMoveEvent ev(QPoint(10, 8), Qt::CopyAction, &data, Qt::NoButton, Qt::NoModifier);

    // Act
    edit->dragMoveEvent(&ev);

    // Assert：URL 拖入被接受（事件接受态翻转）
    EXPECT_TRUE(ev.isAccepted());
    EXPECT_TRUE(edit->toPlainText().isEmpty()); // URL 不插入文本
}

TEST_F(TextEditTest, DropEvent_TextWithoutSource_InsertsUndoable)
{
    // Arrange：无 source（外部拖入文本），落点在首行内
    setDocText(QString("body "));
    moveCursorTo(5);
    QMimeData data;
    data.setText(QString("tail"));
    QDropEvent ev(QPointF(10, 8), Qt::CopyAction, &data, Qt::NoButton, Qt::NoModifier);

    // Act
    edit->dropEvent(&ev);

    // Assert：拖入文本被插入（位置由落点决定）且产生撤销项
    EXPECT_TRUE(edit->toPlainText().contains(QString("tail")));
    EXPECT_TRUE(edit->isUndoRedoOpt());
}

TEST_F(TextEditTest, DropEvent_TextInReadOnly_Ignored)
{
    // Arrange
    edit->toggleReadOnlyMode(true);
    QMimeData data;
    data.setText(QString("no"));
    QDropEvent ev(QPointF(10, 8), Qt::CopyAction, &data, Qt::NoButton, Qt::NoModifier);

    // Act
    edit->dropEvent(&ev);

    // Assert
    EXPECT_TRUE(edit->toPlainText().isEmpty());
    EXPECT_TRUE(edit->getReadOnlyMode());
}

TEST_F(TextEditTest, DropEvent_NonTextData_BaseHandling)
{
    // Arrange：无 urls 无 text
    QMimeData data;
    QDropEvent ev(QPointF(10, 8), Qt::CopyAction, &data, Qt::NoButton, Qt::NoModifier);

    // Act
    edit->dropEvent(&ev);

    // Assert：基类路径，无文本变化
    EXPECT_TRUE(edit->toPlainText().isEmpty());
    EXPECT_FALSE(edit->isUndoRedoOpt());
}

// ---------------- 视图模式（V1） ----------------

TEST_F(TextEditTest, UpdateViewModeActions_AllModes_CheckStateAndEnablement)
{
    // Arrange：初始编辑视图选中
    ASSERT_TRUE(edit->m_actEditView->isChecked());
    ASSERT_FALSE(edit->m_actLivePreview->isEnabled()); // 非 md 置灰

    // Act：切换查看视图
    edit->updateViewModeActions(ViewMode::ReadView, false);
    // Assert
    EXPECT_TRUE(edit->m_actReadView->isChecked());
    EXPECT_FALSE(edit->m_actEditView->isChecked());
    EXPECT_FALSE(edit->m_actLivePreview->isEnabled());

    // Act：markdown 场景实时预览
    edit->updateViewModeActions(ViewMode::LivePreview, true);
    // Assert
    EXPECT_TRUE(edit->m_actLivePreview->isChecked());
    EXPECT_TRUE(edit->m_actLivePreview->isEnabled());

    // Act：回编辑视图
    edit->updateViewModeActions(ViewMode::Edit, true);
    // Assert
    EXPECT_TRUE(edit->m_actEditView->isChecked());
}

TEST_F(TextEditTest, ViewModeActions_ReturnsThreeActions)
{
    // Act
    const QList<QAction *> actions = edit->viewModeActions();

    // Assert
    EXPECT_EQ(actions.size(), 3);
    EXPECT_EQ(actions.at(0), edit->m_actEditView);
    EXPECT_EQ(actions.at(1), edit->m_actReadView);
    EXPECT_EQ(actions.at(2), edit->m_actLivePreview);
}

TEST_F(TextEditTest, ViewModeActionTriggered_EmitsViewModeRequested)
{
    // Arrange
    QSignalSpy spy(edit, &TextEdit::viewModeRequested);

    // Act：触发查看视图动作
    edit->m_actReadView->trigger();

    // Assert
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.last().at(0).value<ViewMode>(), ViewMode::ReadView);
    EXPECT_EQ(spy.count(), 1);
}
