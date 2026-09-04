// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// editor_undo 批次（B5）链接接缝（link seam）状态头。
//
// 被测的 9 个 QUndoCommand 源文件（src/editor/*command*.cpp、undolist.cpp）引用了
// TextEdit / EditWrapper / Window / BottomBar 的少量成员函数。本测试模块不编译这些
// 类的真实实现（dtextedit.cpp 9589 行 + editwrapper/window/bottombar 会级联拖入整个
// 应用），而是在 editor_undo_seam.cpp 中给出“空壳”定义（链接接缝）：
//   - TextEdit：构造/析构 + 14 个事件 override 全部委托 DPlainTextEdit 基类实现，
//     使其行为等价于一个真实可用的 DPlainTextEdit（文档/光标操作全部走 Qt 真实路径）；
//     restoreColumnEditSelection / convertReplaceToMark / manualUpdateAllMark 为可观测
//     记录桩（写入 EditorUndoSeam 状态）。
//   - EditWrapper / Window / BottomBar：测试中从不构造实例，仅以“伪指针”形式传入
//     （reinterpret_cast 自普通 QWidget），被调用的非虚成员 window()/bottomBar()/
//     isQuit()/setPrintEnabled()/setChildEnabled()/setEndlineMenuText() 为记录桩。
// 本模块所有目标均不开 AUTOMOC（Q_OBJECT 声明只需编译通过，不实例化其元对象特性；
// TextEdit 的 12 个信号与 Q_OBJECT 声明的 metaObject/qt_metacast/qt_metacall 由
// 接缝 TU 提供空壳定义）。
//
// 状态归属约定（对应 test-types.md §7 / self_checker 5b“计数器为夹具成员”）：
// EditorUndoSeam 实例由各测试夹具持有（fixture 成员），SetUp 中重置并经
// ut_install_seam(&seam) 安装，TearDown 中 ut_install_seam(nullptr) 卸载；
// 文件内的 g_seam 指针仅是接缝定义到夹具成员的管道，不承载用例间可残留状态。

#ifndef AUTOTESTS_EDITOR_UNDO_SEAM_H
#define AUTOTESTS_EDITOR_UNDO_SEAM_H

#include <QList>
#include <QPair>
#include <QString>
#include <QTextEdit>

#include "dtextedit.h"

// 接缝观测状态：所有计数器/记录字段在夹具 SetUp() 中随实例重建自动归零。
struct EditorUndoSeam {
    // TextEdit::restoreColumnEditSelection
    int restoreColumnEditSelectionCalls = 0;
    QList<QTextEdit::ExtraSelection> lastRestoredSelections;

    // TextEdit::convertReplaceToMark（static）与 manualUpdateAllMark
    int convertReplaceToMarkCalls = 0;
    int manualUpdateAllMarkCalls = 0;
    QList<QPair<TextEdit::MarkOperation, qint64>> lastManualMarks;

    // EditWrapper::window() / bottomBar() / isQuit()
    void *fakeWindowObject = nullptr;      // 实际为 Window*（避免头文件闭包扩散，用 void* 传递）
    void *fakeBottomBarObject = nullptr;   // 实际为 BottomBar*
    bool fakeIsQuit = false;

    // Window::setPrintEnabled
    int setPrintEnabledCalls = 0;
    bool lastPrintEnabled = false;

    // BottomBar::setChildEnabled / setEndlineMenuText
    int setChildEnabledCalls = 0;
    bool lastChildEnabled = false;
    int setEndlineMenuTextCalls = 0;
    int lastEndlineFormat = 0;             // BottomBar::EndlineFormat 的 int 值
};

// 安装/卸载接缝状态；seam 为 nullptr 时所有接缝记录桩退化为无操作。
void ut_install_seam(EditorUndoSeam *seam);

#endif  // AUTOTESTS_EDITOR_UNDO_SEAM_H
