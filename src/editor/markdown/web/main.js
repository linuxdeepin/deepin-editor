// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// main.js —— Milkdown 实例装配（移植自 uos-ai MarkdownEditor.tsx，去 Vue 包装，裸挂载）
//
// 插件链顺序与 uos-ai 一致：
//   commonmark → gfm → history → indent → clipboard → mathPlugins
// 本期固定 editable:false（只读预览）。阶段二切 editable:true 即得 WYSIWYG
//（届时需恢复 listener 插件与 markdownUpdated 回写，见 boot() 内注释）。

import { Editor, rootCtx, defaultValueCtx, editorViewOptionsCtx, editorViewCtx, parserCtx } from "@milkdown/kit/core";
import { commonmark } from "@milkdown/kit/preset/commonmark";
import { gfm } from "@milkdown/kit/preset/gfm";
import { history } from "@milkdown/kit/plugin/history";
import { indent } from "@milkdown/kit/plugin/indent";
import { clipboard } from "@milkdown/kit/plugin/clipboard";
import { replaceAll } from "@milkdown/kit/utils";
import { mathPlugins, normalizeMathDelimiters } from "./milkdownMathPlugins.js";
import { setupBridge } from "./bridge.js";
import { enhance, enhanceNodes, retranslateCodeBlocks } from "./renderEnhancer.js";

import "katex/dist/katex.min.css";
import "./theme.css";

const ROOT_ID = "app";
let editor = null;
let lastValue = "";

// —— C++ → JS 处理器 ——
let lastRequestedRatio = 0;   // 最近一次 C++ 请求的滚动比例；重渲染后重放（§4.6 初始对齐）

// —— 渐进渲染（大文件分段上屏，性能优化 2026-09-16）——
// 整篇 replaceAll 需先完成全文 ProseMirror 建模才一次性上屏，大文档耗时超线性
// （实测 5MB≈50s、10MB≈242s，期间预览空白）。超过阈值改为按顶层块边界切块：
// 首块 replaceAll 立即上屏，其余块解析后 tr.insert 追加到文档末尾，块间让出
// 事件循环——首屏秒级可见、内容渐进填充、渲染进程保持响应。
// 语义边界情况：跨块的引用式链接定义、松散列表会被就近截断（预览可接受的误差）。
const PROGRESSIVE_THRESHOLD = 64 * 1024;   // 触发渐进的最小字符数（约 1~2 屏正文）
let renderGeneration = 0;                  // 代际号：新内容到达时作废进行中的渐进渲染

// —— 渐进构建期的滚动比例补偿 ——
// 比例式同步假设两栏代表同一篇完整文档；渐进填充期间右栏仅有部分内容，
// 直接换算会系统性偏移且随右栏增高持续漂移。以已渲染字符占比 f 近似高度占比：
// C++ 全文档比例 r → 右栏内比例 r/f（钳制 [0,1]，未渲染区停在已渲染底部），
// 反向通知乘回 f。f→1 时收敛为精确比例；字符占比≠高度占比，属近似（±10% 级）。
let buildProgress = null;        // { fraction }：渐进构建进度；null=完成/非渐进
let userScrolledDuringBuild = false;   // 构建期用户滚过右栏：完成后不再强制对齐（尊重用户位置）

function renderMarkdown(md) {
    if (!editor) return;
    const normalized = normalizeMathDelimiters(md == null ? "" : String(md));
    if (normalized === lastValue) return;
    lastValue = normalized;
    renderGeneration++;
    buildProgress = null;
    userScrolledDuringBuild = false;
    if (normalized.length <= PROGRESSIVE_THRESHOLD) {
        editor.action(replaceAll(normalized));
        // 重渲染后做静态后处理（表格包裹等）并重放滚动比例：
        // 首次渲染完成前 scrollToRatio 因 max=0 被跳过，此处对齐左右初始位置
        const rootEl = document.getElementById(ROOT_ID);
        setTimeout(() => {
            enhance(rootEl);
            reapplyScroll();
        }, 0);
        return;
    }
    renderProgressively(normalized, renderGeneration);
}

// 顶层块切块：在代码围栏（```/~~~）与数学块（$$）之外累积空行分割点，
// 达到目标块长即切断。无法安全切断时（超长围栏等）保守并入下一块。
function splitTopLevelBlocks(md, targetSize) {
    const chunks = [];
    const total = md.length;
    let chunkStart = 0;
    let cut = -1;          // 最近一个安全切断点（空行后的位置）
    let pos = 0;
    let fence = null;      // { marker, len } 围栏 / { marker: "$$" } 数学块
    while (pos < total) {
        let lineEnd = md.indexOf("\n", pos);
        if (lineEnd === -1) lineEnd = total;
        const trimmed = md.slice(pos, lineEnd).trim();
        if (fence) {
            if (fence.marker === "$$") {
                if (trimmed === "$$") fence = null;
            } else if (trimmed.length >= fence.len && trimmed.startsWith(fence.marker)) {
                fence = null;
            }
        } else if (trimmed.startsWith("```") || trimmed.startsWith("~~~")) {
            const ch = trimmed.charAt(0);
            let len = 0;
            while (len < trimmed.length && trimmed.charAt(len) === ch) len++;
            fence = { marker: ch.repeat(len), len };
        } else if (trimmed.startsWith("$$")) {
            // 单行 $$...$$ 自闭合；跨行块由后续整行 $$ 关闭
            fence = (trimmed.length > 4 && trimmed.endsWith("$$")) ? null : { marker: "$$" };
        } else if (trimmed === "" && lineEnd < total) {
            cut = lineEnd + 1;
        }
        pos = lineEnd + 1;
        if (!fence && cut > chunkStart && pos - chunkStart >= targetSize) {
            chunks.push(md.slice(chunkStart, cut));
            chunkStart = cut;
        }
    }
    if (chunkStart < total) chunks.push(md.slice(chunkStart));
    return chunks;
}

function renderProgressively(md, gen) {
    const chunks = splitTopLevelBlocks(md, PROGRESSIVE_THRESHOLD);
    const totalChars = md.length;
    let renderedChars = 0;
    let index = 0;
    buildProgress = { fraction: 0 };
    const step = () => {
        // 代际失效：期间有新内容/新请求到达，本轮渐进渲染作废
        if (gen !== renderGeneration || !editor) return;
        const chunk = chunks[index++];
        renderedChars += chunk.length;
        if (index === 1) {
            // 首块：清空重建 + 后处理 + 初始滚动对齐（对齐时机在后续块增高文档之前）
            editor.action(replaceAll(chunk));
            setTimeout(() => {
                if (gen !== renderGeneration) return;
                enhance(document.getElementById(ROOT_ID));
                reapplyScroll();
            }, 0);
        } else {
            appendChunk(chunk);
        }
        buildProgress.fraction = renderedChars / totalChars;
        if (index < chunks.length) {
            setTimeout(step, 0);
        } else {
            // 构建完成：f=1，后续同步恢复精确比例。若用户未动过右栏，
            // 按左栏最近请求做最终对齐（消除补偿近似误差）；动过则尊重其位置。
            buildProgress = null;
            if (!userScrolledDuringBuild) reapplyScroll();
        }
    };
    step();   // 首块同步渲染，抢最快首屏
}

// 解析单块并追加到文档末尾；仅对新增 DOM 做后处理
// （全树扫描在"块数×总节点数"下会重新引入超线性，renderEnhancer.enhanceNodes）
function appendChunk(chunk) {
    editor.action((ctx) => {
        const view = ctx.get(editorViewCtx);
        const parsed = ctx.get(parserCtx)(chunk);
        if (!parsed) return;
        const dom = view.dom;
        const before = dom.childNodes.length;
        view.dispatch(view.state.tr.insert(view.state.doc.content.size, parsed.content));
        enhanceNodes(Array.from(dom.childNodes).slice(before));
    });
}

// 全文档比例 → 当前（可能部分构建的）右栏内比例
function toRenderedRatio(ratio) {
    if (!buildProgress || buildProgress.fraction <= 0) return ratio;
    return Math.max(0, Math.min(1, ratio / buildProgress.fraction));
}

function reapplyScroll() {
    const max = document.documentElement.scrollHeight - window.innerHeight;
    if (max > 0) {
        applyProgrammaticScroll(toRenderedRatio(lastRequestedRatio) * max);
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

// 程序化滚动与用户滚动的可靠区分（2026-09-16 根因修复）：
// Chromium 的 scroll 事件在渲染步骤【异步】派发：仅靠同步标记会被定时器提前复位误判；
// 仅靠位置判定在渐进构建期也不可靠——补偿系数 f 随块追加变化，两次前向同步的目标
// 位置跳变可超容差，异步事件仍会被误判为用户滚动，形成"左→右→左"回声放大。
// 双保险：位置判定（±2px）+ 时间窗判定（前向同步后 200ms 内不回传，覆盖事件派发延迟）。
let __programmaticScroll = false;
let __lastProgrammaticY = null;
let __programmaticUntil = 0;

function applyProgrammaticScroll(y) {
    __lastProgrammaticY = y;
    __programmaticUntil = performance.now() + 200;
    __programmaticScroll = true;
    window.scrollTo(0, y);
    setTimeout(() => { __programmaticScroll = false; }, 0);
}

function isProgrammaticPosition() {
    return __lastProgrammaticY !== null
           && Math.abs(window.scrollY - __lastProgrammaticY) < 2;
}

function scrollToRatio(ratio) {
    // §4.6 滚动到比例（0~1）
    // 根因修复：ProseMirror 元素本身不可滚动（无 overflow:auto），页面滚动在 window/documentElement。
    ratio = Math.max(0, Math.min(1, ratio));
    lastRequestedRatio = ratio;
    // 渐进构建期：ratio 是全文档比例，先补偿映射到已渲染范围
    const target = toRenderedRatio(ratio);
    const max = document.documentElement.scrollHeight - window.innerHeight;
    if (max > 0) {
        applyProgrammaticScroll(target * max);
    }
}

// 右栏用户滚动 → 通知 C++（反向同步）
let __lastNotifiedRatio = -1;
let __lastNotifiedY = 0;
window.addEventListener("scroll", () => {
    // 程序化产物（同步请求的滚动、其异步事件、回声窗口期）不回传，避免左→右→左回环
    if (__programmaticScroll || isProgrammaticPosition() || performance.now() < __programmaticUntil)
        return;
    __lastProgrammaticY = null;   // 用户已接管滚动位置
    const max = document.documentElement.scrollHeight - window.innerHeight;
    if (max <= 0) return;
    let ratio = Math.max(0, Math.min(1, window.scrollY / max));
    // 渐进构建期：右栏内比例换算回全文档比例（f<1），左栏才能落在对应位置
    if (buildProgress) {
        ratio = Math.min(1, ratio * buildProgress.fraction);
        userScrolledDuringBuild = true;
    }
    // 去抖阈值须像素感知：固定 0.001 比例对超大文档（30 万行级）相当于数屏死区，
    // 用户翻页会被吞掉；构建期补偿后的通知值更小，同样会被吞。任一维度显著变化即通知。
    if (Math.abs(window.scrollY - __lastNotifiedY) < 40 && Math.abs(ratio - __lastNotifiedRatio) < 0.001)
        return;
    __lastNotifiedRatio = ratio;
    __lastNotifiedY = window.scrollY;
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
            // 阶段一只读预览不注册 listener/markdownUpdated：每次文档变更会把整篇 doc
            // 全量序列化回 markdown（渐进渲染的逐块追加将放大该开销），只读路径无消费者。
            // 阶段二（WYSIWYG 编辑回写）恢复 .use(listener) + markdownUpdated 注册。
        })
        .use(commonmark)
        .use(gfm)
        .use(history)
        .use(indent)
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
