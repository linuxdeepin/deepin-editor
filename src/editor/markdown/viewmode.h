// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VIEWMODE_H
#define VIEWMODE_H

#include <QMetaType>

//
// ViewMode —— 编辑器视图模式枚举
//
// 阶段一（本期）实际使用：Edit / ReadView / LivePreview
// Wysiwyg 为阶段二（WYSIWYG 富文本编辑）预留，本期不可达（ViewModeFsm 拒绝切换）。
//
enum class ViewMode {
    Edit,         // 编辑视图（纯文本源码，现状）
    ReadView,     // 查看视图（md：Milkdown 只读渲染；非 md：TextEdit 只读）
    LivePreview,  // 实时阅览（左源码 + 右只读渲染，md 专属）
    Wysiwyg       // 【预留】阶段二单面板所见即所得编辑
};

Q_DECLARE_METATYPE(ViewMode)

#endif // VIEWMODE_H
