// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// bridge.js —— QWebChannel 双向通信胶水
//
// 协议（与 C++ MarkdownBridge 一一对应）：
//   C++ → JS：bridge.{signal}.connect(fn)
//   JS → C++：bridge.onReady/onContentChanged/onScrollRatio/onOpenLink(...)
//
// 提供 setupBridge(handlers) 供 main.js 调用，在 QWebChannel 就绪后连接信号。

let bridgeReady = false;
let pendingHandlers = null;

function initChannel() {
    if (typeof QWebChannel === "undefined") {
        // WebChannel 未注入（非 QWebEngine 宿主，如本地浏览器调试），降级
        console.warn("QWebChannel unavailable; running in standalone mode");
        if (pendingHandlers) pendingHandlers.standalone && pendingHandlers.standalone();
        return;
    }
    // eslint-disable-next-line no-undef
    new QWebChannel(qt.webChannelTransport, function (channel) {
        window.bridge = channel.objects.bridge;
        bridgeReady = true;
        if (pendingHandlers) wireSignals(pendingHandlers);
    });
}

function wireSignals(h) {
    const b = window.bridge;
    if (!b) return;
    b.setMarkdownRequested.connect(function (md) { h.onSetMarkdown(md); });
    b.setModeRequested.connect(function (editable) { h.onSetMode(editable); });
    b.applyThemeRequested.connect(function (json, isDark) { h.onApplyTheme(json, isDark); });
    b.setLayoutRequested.connect(function (maxW, center) { h.onSetLayout(maxW, center); });
    b.scrollToRatioRequested.connect(function (ratio) { h.onScrollToRatio(ratio); });
}

// 供 main.js 调用：注册 C++→JS 信号处理器
export function setupBridge(handlers) {
    pendingHandlers = handlers;
    if (bridgeReady) wireSignals(handlers);
}

initChannel();
