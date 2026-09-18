// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef IMARKDOWNRENDERER_H
#define IMARKDOWNRENDERER_H

#include "viewmode.h"

#include <QString>
#include <QVariantMap>

//
// IMarkdownRenderer —— Markdown 渲染器接口（供 EditWrapper 依赖接口编程 / 测试用 Mock 替换）
//
// 业务层（EditWrapper）不直接依赖 MarkdownView（QWebEngineView 子类），而依赖此接口，
// 便于在完全不启动 WebEngine 的前提下用 GMock 替身验证编排逻辑（见 §5'.4）。
//
class IMarkdownRenderer
{
public:
    virtual ~IMarkdownRenderer() = default;

    // 渲染内核是否就绪（Milkdown 创建完成）。未就绪时 setMarkdown 会被缓存。
    virtual bool isReady() const = 0;

    // 单向（本期）：把 markdown 源码推给渲染内核。
    virtual void setMarkdown(const QString &md) = 0;
    // 切换只读/可编辑。本期固定 ReadOnly（Editable 为阶段二预留）。
    // mode 取 MarkdownView::Mode（ReadOnly=0/Editable=1），用 int 避免循环 include。
    virtual void setMode(int mode) = 0;
    // 注入深浅色与配色。
    virtual void applyTheme(const QVariantMap &themeMap) = 0;
    // 内容区布局：maxContentWidth=0 表示不限最大宽（实时阅览半幅），>0 表示居中约束（查看视图 800）。
    virtual void setLayout(int maxContentWidth, bool center) = 0;
    // 滚动同步：ratio ∈ [0,1]，由左侧 TextEdit 滚动驱动。
    virtual void scrollToRatio(double ratio) = 0;
};

#endif // IMARKDOWNRENDERER_H
