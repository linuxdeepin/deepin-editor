// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VIEWMODEFSM_H
#define VIEWMODEFSM_H

#include "viewmode.h"

//
// ViewModeFsm —— 视图模式判定/切换合法性/置灰规则（纯函数，无 QWidget 依赖，100% 可测）
//
// 落地需求（§4.4 模式判定规则，2026-08-04 细化）：
//   - 入口始终显示（含新建 .txt）
//   - 非 md 文件下仅「实时预览」置灰，「编辑模式」「查看视图」可用
//   - md 文件默认 LivePreview；其他默认 Edit
//   - 语言切为 Markdown 时 Edit 自动跃迁 LivePreview（2026-08-19，对齐 md 默认视图）；
//     语言切走时 LivePreview 回退 Edit（对称）
//   - 「查看视图」在非 md 下走纯文本只读（沿用原只读模式语义），md 下走 Milkdown 渲染
//   - Wysiwyg 为阶段二预留，本期不可达
//
class ViewModeFsm
{
public:
    // 默认视图：md → LivePreview，非 md → Edit
    static ViewMode resolveDefaultMode(bool isMarkdown)
    {
        return isMarkdown ? ViewMode::LivePreview : ViewMode::Edit;
    }

    // 入口可用性（置灰规则）
    static bool isLivePreviewAvailable(bool isMarkdown) { return isMarkdown; }
    static bool isEditViewAvailable(bool) { return true; }
    static bool isReadViewAvailable(bool) { return true; }

    // 切换合法性：任意文件可切 Edit/ReadView；仅 md 可切 LivePreview；Wysiwyg 阶段二不可达
    static bool canSwitchTo(ViewMode target, bool isMarkdown)
    {
        switch (target) {
        case ViewMode::Edit:
        case ViewMode::ReadView:
            return true;
        case ViewMode::LivePreview:
            return isMarkdown;
        case ViewMode::Wysiwyg:
            return false;   // 阶段二预留
        }
        return false;
    }

    // 语言切走（md → 非 md）时的回退：LivePreview 退回 Edit，ReadView/Edit 保持
    static ViewMode fallbackWhenMarkdownLost(ViewMode current)
    {
        if (current == ViewMode::LivePreview) return ViewMode::Edit;
        return current;   // Edit/ReadView 不受影响（ReadView 对非 md 是纯文本只读）
    }

    // 语言切到（非 md → md）时的跃迁：Edit 自动切入 LivePreview（对齐「md 默认实时预览」，
    // 2026-08-19 需求：新建文件手动切语言为 Markdown 即入实时预览）；
    // ReadView/LivePreview 保持（ReadView 仅切换实现：渲染页 ↔ 纯文本只读）
    static ViewMode elevateWhenMarkdownGained(ViewMode current)
    {
        if (current == ViewMode::Edit) return ViewMode::LivePreview;
        return current;
    }

    // 「查看视图」是否走纯文本只读（ReadView + 非 md）：是则 EditWrapper 仅置 TextEdit 只读，不进渲染页
    static bool isReadOnlyTextMode(ViewMode mode, bool isMarkdown)
    {
        return mode == ViewMode::ReadView && !isMarkdown;
    }
};

#endif // VIEWMODEFSM_H
