// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// main.js —— Milkdown 实例装配（移植自 uos-ai MarkdownEditor.tsx，去 Vue 包装，裸挂载）
//
// 插件链顺序与 uos-ai 一致：
//   commonmark → gfm → history → indent → listener → clipboard → mathPlugins
// 本期固定 editable:false（只读预览）。阶段二切 editable:true 即得 WYSIWYG。

import { Editor, rootCtx, defaultValueCtx, editorViewOptionsCtx } from "@milkdown/kit/core";
import { commonmark } from "@milkdown/kit/preset/commonmark";
import { gfm } from "@milkdown/kit/preset/gfm";
import { history } from "@milkdown/kit/plugin/history";
import { indent } from "@milkdown/kit/plugin/indent";
import { listener, listenerCtx } from "@milkdown/kit/plugin/listener";
import { clipboard } from "@milkdown/kit/plugin/clipboard";
import { replaceAll } from "@milkdown/kit/utils";
import { mathPlugins, normalizeMathDelimiters } from "./milkdownMathPlugins.js";
import { setupBridge } from "./bridge.js";
import { enhance, retranslateCodeBlocks } from "./renderEnhancer.js";

import "katex/dist/katex.min.css";
import "./theme.css";

const ROOT_ID = "app";
let editor = null;
let lastValue = "";

// —— C++ → JS 处理器 ——
let lastRequestedRatio = 0;   // 最近一次 C++ 请求的滚动比例；重渲染后重放（§4.6 初始对齐）

function renderMarkdown(md) {
    if (!editor) return;
    const normalized = normalizeMathDelimiters(md == null ? "" : String(md));
    if (normalized === lastValue) return;
    lastValue = normalized;
    editor.action(replaceAll(normalized));
    // 重渲染后做静态后处理（表格包裹等）并重放滚动比例：
    // 首次渲染完成前 scrollToRatio 因 max=0 被跳过，此处对齐左右初始位置
    const rootEl = document.getElementById(ROOT_ID);
    setTimeout(() => {
        enhance(rootEl);
        reapplyScroll();
    }, 0);
}

function reapplyScroll() {
    const max = document.documentElement.scrollHeight - window.innerHeight;
    if (max > 0) {
        __programmaticScroll = true;
        window.scrollTo(0, lastRequestedRatio * max);
        setTimeout(() => { __programmaticScroll = false; }, 0);
    }
}

function applyTheme(jsonColors, isDark) {
    try {
        const colors = JSON.parse(jsonColors || "{}");
        const root = document.documentElement;
        for (const [k, v] of Object.entries(colors)) {
            root.style.setProperty(k, v);
        }
        root.setAttribute("data-theme", isDark ? "dark" : "light");
    } catch (e) {
        console.error("applyTheme failed:", e);
    }
}

function applyLayout(maxContentWidth, center) {
    const root = document.documentElement;
    root.style.setProperty("--content-max-width", maxContentWidth > 0 ? maxContentWidth + "px" : "none");
    root.style.setProperty("--content-center", center ? "1" : "0");
}

function scrollToRatio(ratio) {
    // §4.6 滚动到比例（0~1）
    // 根因修复：ProseMirror 元素本身不可滚动（无 overflow:auto），页面滚动在 window/documentElement。
    ratio = Math.max(0, Math.min(1, ratio));
    lastRequestedRatio = ratio;
    const max = document.documentElement.scrollHeight - window.innerHeight;
    console.log("[md] scrollToRatio", ratio, "max", max);
    if (max > 0) {
        // 标记来自 C++ 的程序触发，window scroll 事件里跳过反向通知
        __programmaticScroll = true;
        window.scrollTo(0, ratio * max);
        setTimeout(() => { __programmaticScroll = false; }, 0);
    }
}

// 右栏用户滚动 → 通知 C++（反向同步）
let __programmaticScroll = false;
let __lastNotifiedRatio = -1;
window.addEventListener("scroll", () => {
    if (__programmaticScroll) return;
    const max = document.documentElement.scrollHeight - window.innerHeight;
    if (max <= 0) return;
    const ratio = Math.max(0, Math.min(1, window.scrollY / max));
    // 钳制微小抖动
    if (Math.abs(ratio - __lastNotifiedRatio) < 0.001) return;
    __lastNotifiedRatio = ratio;
    if (window.bridge && typeof window.bridge.onScrollRatio === "function") {
        window.bridge.onScrollRatio(ratio);
    }
}, { passive: true });

// 外部链接拦截：http/https 链接不在预览内导航，经 bridge.onOpenLink → C++ QDesktopServices
// 转系统浏览器打开（捕获阶段先行，preventDefault 阻止 Chromium 导航；页内锚点 # 不受影响）
document.addEventListener("click", (event) => {
    const target = event.target;
    const anchor = target && target.closest ? target.closest("a[href]") : null;
    if (!anchor) return;
    const href = anchor.href || "";
    if (!/^https?:\/\//i.test(href)) return;
    event.preventDefault();
    event.stopPropagation();
    if (window.bridge && typeof window.bridge.onOpenLink === "function") {
        window.bridge.onOpenLink(href);
    } else {
        console.warn("[md] bridge unavailable, external link not opened:", href);
    }
}, true);

// 启动 Milkdown
async function boot() {
    const rootEl = document.getElementById(ROOT_ID);
    editor = await Editor.make()
        .config((ctx) => {
            ctx.set(rootCtx, rootEl);
            ctx.set(defaultValueCtx, "");
            ctx.update(editorViewOptionsCtx, (prev) => ({
                ...prev,
                editable: () => false,
                attributes: { class: "milkdown-read-only" },
            }));
            ctx.get(listenerCtx).markdownUpdated((_ctx, markdown) => {
                // 预留：阶段二编辑模式回写源码
                if (window.bridge && typeof window.bridge.onContentChanged === "function") {
                    window.bridge.onContentChanged(markdown);
                }
            });
        })
        .use(commonmark)
        .use(gfm)
        .use(history)
        .use(indent)
        .use(listener)
        .use(clipboard)
        .use(mathPlugins)
        .create();

    setupBridge({
        onSetMarkdown: renderMarkdown,
        onSetMode: () => { /* 阶段二 */ },
        onApplyTheme: applyTheme,
        onSetLayout: applyLayout,
        onScrollToRatio: scrollToRatio,
        onRetranslate: () => retranslateCodeBlocks(document.getElementById(ROOT_ID)),
    });

    console.log("[md] boot done, bridge=", typeof window.bridge, "onReady=", window.bridge ? typeof window.bridge.onReady : "n/a");
    if (window.bridge && typeof window.bridge.onReady === "function") {
        window.bridge.onReady();
    } else {
        // bridge 尚未就绪（QWebChannel 回调竞态），延迟重试
        const retry = () => {
            if (window.bridge && typeof window.bridge.onReady === "function") {
                window.bridge.onReady();
                console.log("[md] ready notified (after retry)");
            } else {
                setTimeout(retry, 50);
            }
        };
        setTimeout(retry, 50);
    }
}

boot().catch((e) => console.error("Milkdown boot failed:", e));
