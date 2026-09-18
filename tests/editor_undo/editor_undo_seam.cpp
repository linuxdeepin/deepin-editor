// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// editor_undo 批次（B5）链接接缝实现：见 editor_undo_seam.h 头部说明。
// 本文件提供 src/editor 被测撤销命令源文件引用的 TextEdit / EditWrapper / Window /
// BottomBar 成员符号的空壳/记录定义。真实实现（dtextedit.cpp、editwrapper.cpp、
// window.cpp、bottombar.cpp）不参与本测试目标链接，因此无重复符号。

#include "editor_undo_seam.h"

#include "editwrapper.h"
#include "FlashTween.h"
#include "uncommentselection.h"
#include "../widgets/bottombar.h"
#include "../widgets/window.h"

namespace {
// 接缝状态指针：仅由 ut_install_seam 设置，指向夹具持有的 EditorUndoSeam 实例。
// 未安装（nullptr）时所有记录桩为无操作，保证接缝函数独立安全。
EditorUndoSeam *g_seam = nullptr;
}  // namespace

void ut_install_seam(EditorUndoSeam *seam)
{
    g_seam = seam;
}

// ==================== TextEdit 值成员符号空壳 ====================
// TextEdit 以值成员形式持有 FlashTween（Q_OBJECT，无自定义虚函数/信号）与
// Comment::CommentDefinition，其构造/析构符号在真实实现 cpp 中；此处空壳提供，
// 使空壳 TextEdit 构造可链接（这些成员从不被使用，空行为安全）。

FlashTween::FlashTween()
{
}

FlashTween::~FlashTween()
{
}

const QMetaObject *FlashTween::metaObject() const
{
    return &QObject::staticMetaObject;
}

void *FlashTween::qt_metacast(const char *cname)
{
    return QObject::qt_metacast(cname);
}

int FlashTween::qt_metacall(QMetaObject::Call call, int id, void **args)
{
    return QObject::qt_metacall(call, id, args);
}

Comment::CommentDefinition::CommentDefinition()
{
}

// ==================== TextEdit 空壳（行为等价于 DPlainTextEdit） ====================
// 构造/析构 + 事件 override 委托基类，文档与光标操作走 Qt 真实路径；
// 被测命令专用的三个成员为可观测记录桩。Q_OBJECT 声明的三个虚函数同样给出
// 委托定义（vtable 需要），metaObject 汇报基类元对象即可满足本批次全部用法。

TextEdit::TextEdit(QWidget *parent)
    : DPlainTextEdit(parent)
{
}

TextEdit::~TextEdit()
{
    // 空壳：真实析构中的子控件清理不参与测试；成员中非平凡对象由编译器自动析构。
}

const QMetaObject *TextEdit::metaObject() const
{
    return &DPlainTextEdit::staticMetaObject;
}

void *TextEdit::qt_metacast(const char *cname)
{
    return DPlainTextEdit::qt_metacast(cname);
}

int TextEdit::qt_metacall(QMetaObject::Call call, int id, void **args)
{
    return DPlainTextEdit::qt_metacall(call, id, args);
}

// staticMetaObject：Qt6 PMF 连接的 assertObjectType<TextEdit> 会 ODR 使用该符号。
// 空壳直接复制基类元对象（本批次不使用 TextEdit 自有元方法）。
const QMetaObject TextEdit::staticMetaObject = DPlainTextEdit::staticMetaObject;

// cursorPositionChanged 槽：insertblockbytextcommond.cpp treat() 以 PMF 形式
// connect/disconnect 该成员（receiver 侧可为任意成员函数），空壳实现即可。
void TextEdit::cursorPositionChanged()
{
}

bool TextEdit::event(QEvent *evt)
{
    return DPlainTextEdit::event(evt);
}

void TextEdit::dragEnterEvent(QDragEnterEvent *event)
{
    DPlainTextEdit::dragEnterEvent(event);
}

void TextEdit::dragMoveEvent(QDragMoveEvent *event)
{
    DPlainTextEdit::dragMoveEvent(event);
}

void TextEdit::dropEvent(QDropEvent *event)
{
    DPlainTextEdit::dropEvent(event);
}

void TextEdit::inputMethodEvent(QInputMethodEvent *e)
{
    DPlainTextEdit::inputMethodEvent(e);
}

void TextEdit::mousePressEvent(QMouseEvent *e)
{
    DPlainTextEdit::mousePressEvent(e);
}

void TextEdit::mouseMoveEvent(QMouseEvent *e)
{
    DPlainTextEdit::mouseMoveEvent(e);
}

void TextEdit::mouseReleaseEvent(QMouseEvent *e)
{
    DPlainTextEdit::mouseReleaseEvent(e);
}

void TextEdit::keyPressEvent(QKeyEvent *e)
{
    DPlainTextEdit::keyPressEvent(e);
}

void TextEdit::wheelEvent(QWheelEvent *e)
{
    DPlainTextEdit::wheelEvent(e);
}

bool TextEdit::eventFilter(QObject *object, QEvent *event)
{
    return DPlainTextEdit::eventFilter(object, event);
}

void TextEdit::contextMenuEvent(QContextMenuEvent *event)
{
    DPlainTextEdit::contextMenuEvent(event);
}

void TextEdit::paintEvent(QPaintEvent *e)
{
    DPlainTextEdit::paintEvent(e);
}

void TextEdit::resizeEvent(QResizeEvent *e)
{
    DPlainTextEdit::resizeEvent(e);
}

// 列编辑选区恢复：记录桩（被 InsertTextUndoCommand / DeleteBackAltCommand /
// DeleteTextUndoCommand 的列编辑撤销/重做路径调用）。
void TextEdit::restoreColumnEditSelection(const QList<QTextEdit::ExtraSelection> &selections)
{
    if (g_seam) {
        ++g_seam->restoreColumnEditSelectionCalls;
        g_seam->lastRestoredSelections = selections;
    }
}

// MarkReplaceInfo -> (MarkOperation, time) 的确定性映射，供 ChangeMarkCommand 断言透传。
QList<QPair<TextEdit::MarkOperation, qint64>> TextEdit::convertReplaceToMark(
    const QList<TextEdit::MarkReplaceInfo> &replaceInfo)
{
    if (g_seam)
        ++g_seam->convertReplaceToMarkCalls;

    QList<QPair<TextEdit::MarkOperation, qint64>> marks;
    marks.reserve(replaceInfo.size());
    for (const TextEdit::MarkReplaceInfo &info : replaceInfo)
        marks.append(qMakePair(info.opt, info.time));
    return marks;
}

// 颜色标记整体刷新：记录桩（ChangeMarkCommand undo/redo 调用）。
void TextEdit::manualUpdateAllMark(const QList<QPair<TextEdit::MarkOperation, qint64>> &markInfo)
{
    if (g_seam) {
        ++g_seam->manualUpdateAllMarkCalls;
        g_seam->lastManualMarks = markInfo;
    }
}

// ==================== EditWrapper / Window / BottomBar 记录桩 ====================
// 测试中不构造这三者的实例：EditWrapper* / Window* / BottomBar* 以伪指针传入
// （reinterpret_cast 自普通 QWidget），下列非虚成员被直接调用并记录。

Window *EditWrapper::window()
{
    return g_seam ? reinterpret_cast<Window *>(g_seam->fakeWindowObject) : nullptr;
}

BottomBar *EditWrapper::bottomBar()
{
    return g_seam ? reinterpret_cast<BottomBar *>(g_seam->fakeBottomBarObject) : nullptr;
}

bool EditWrapper::isQuit()
{
    return g_seam ? g_seam->fakeIsQuit : false;
}

void Window::setPrintEnabled(bool enabled)
{
    if (g_seam) {
        ++g_seam->setPrintEnabledCalls;
        g_seam->lastPrintEnabled = enabled;
    }
}

void BottomBar::setChildEnabled(bool enabled)
{
    if (g_seam) {
        ++g_seam->setChildEnabledCalls;
        g_seam->lastChildEnabled = enabled;
    }
}

void BottomBar::setEndlineMenuText(BottomBar::EndlineFormat format)
{
    if (g_seam) {
        ++g_seam->setEndlineMenuTextCalls;
        g_seam->lastEndlineFormat = static_cast<int>(format);
    }
}
