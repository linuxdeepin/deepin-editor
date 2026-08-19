// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MARKDOWNBRIDGE_H
#define MARKDOWNBRIDGE_H

#include <QObject>

//
// MarkdownBridge —— 暴露给 JS 的 WebChannel 桥接对象
//
// 协议（与 web/bridge.js 一一对应）：
//   JS → C++（槽）:onReady / onContentChanged / onScrollRatio / onOpenLink
//   C++ → JS（信号）:setMarkdownRequested / setModeRequested /
//                    applyThemeRequested / setLayoutRequested / scrollToRatioRequested
//
// 阶段一（只读预览）：仅 onReady / setMarkdownRequested / applyThemeRequested /
//                    setLayoutRequested / scrollToRatioRequested 实际使用；
//                    onContentChanged / onScrollRatio / setModeRequested 接口先到位，
//                    阶段二（WYSIWYG 编辑）启用，本期实现为转发信号但上层不连接。
//                    onOpenLink 由 JS 点击拦截调用（外部链接转系统浏览器）。
//
class MarkdownBridge : public QObject
{
    Q_OBJECT
public:
    explicit MarkdownBridge(QObject *parent = nullptr) : QObject(parent) {}

public slots:
    // JS → C++
    void onReady() { emit ready(); }
    void onContentChanged(const QString &md) { emit contentChanged(md); }
    void onScrollRatio(double ratio) { emit scrollRatioChanged(ratio); }
    void onOpenLink(const QString &url) { emit openLinkRequested(url); }

signals:
    // C++ → JS（JS 端 bridge.{signal}.connect(function(...){...}) 接收）
    void ready();
    void contentChanged(const QString &md);
    void scrollRatioChanged(double ratio);
    void openLinkRequested(const QString &url);

    void setMarkdownRequested(const QString &md);
    void setModeRequested(bool editable);
    void applyThemeRequested(const QString &jsonColors, bool isDark);
    void setLayoutRequested(int maxW, bool center);
    void scrollToRatioRequested(double ratio);
};

#endif // MARKDOWNBRIDGE_H
