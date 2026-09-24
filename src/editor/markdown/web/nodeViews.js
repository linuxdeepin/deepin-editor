// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// nodeViews.js —— 表格/代码块 NodeView（§4.8.8 视觉包裹的渲染期实现）
//
// 背景（2026-09-24 表格闪烁根因）：原 renderEnhancer 在每次重渲染后用 setTimeout
// 包裹 <table>/<pre>。但 ProseMirror 同步视图更新（renderDescs）会把节点 DOM 恢复为
// .ProseMirror 的直接子节点、并删除它不追踪的中间包裹层——包裹层被拆后表格以无样式
// 中间态绘制一帧，再由定时器重新包裹，表现为表格闪烁。
// 根治：包裹结构由 NodeView 的 dom/contentDOM 承载，成为 ProseMirror 自己拥有的 DOM
// （经 matchesNode/spec.update 原地复用），不再有"拆了再包"的两个任务/两次绘制。
// DOM 结构与 theme.css 选择器的契约和原 renderEnhancer 输出保持一致。
//
// 注册说明：gfm 预设未注册 table 的 nodeView（仅 tableEditing，不含 columnResizing），
// 此处注册不与其冲突（prosemirror buildNodeViews 对同名节点取首个提供者）。
// 必须用 $viewAsync 而非 $view：$view 的 nodeViewCtx 注册不带定时器依赖，与
// EditorView 构造存在竞态（view 可能先建好，注册随后落地→包裹不生效）；
// $viewAsync 把注册挂进 editorViewTimerCtx，保证先注册后建 view。
// 必须传 <schema>.node 而非 <schema> 本体：$nodeSchema 组合对象的 .id 是构造期的
// 一次性拷贝（彼时 $node runner 尚未执行，值为 undefined → 注册键变成 "undefined"，
// 包裹静默失效）；.node 子插件的 .id 由 runner 在 SchemaReady 前赋值，运行期读取正确。

import { $viewAsync } from "@milkdown/kit/utils";
import { tableSchema } from "@milkdown/kit/preset/gfm";
import { codeBlockSchema } from "@milkdown/kit/preset/commonmark";

// 折叠箭头 SVG：两段折线的上/下箭头（stroke 线条，非实心三角）
const ARROW_UP_SVG = '<svg xmlns="http://www.w3.org/2000/svg" width="10" height="10" viewBox="0 0 10 10" fill="none" stroke="currentColor" stroke-width="1.2" stroke-linecap="round" stroke-linejoin="round"><path d="M1 7 L5 3 L9 7"/></svg>';
const ARROW_DOWN_SVG = '<svg xmlns="http://www.w3.org/2000/svg" width="10" height="10" viewBox="0 0 10 10" fill="none" stroke="currentColor" stroke-width="1.2" stroke-linecap="round" stroke-linejoin="round"><path d="M1 3 L5 7 L9 3"/></svg>';

// 复制按钮 SVG：两张交叠圆角纸（复制）/ 对勾（成功，2s 后恢复，与应用其他复制位置一致）
const COPY_ICON_SVG = '<svg xmlns="http://www.w3.org/2000/svg" width="14" height="14" viewBox="0 0 14 14" fill="none" stroke="currentColor" stroke-width="1.2" stroke-linecap="round" stroke-linejoin="round"><rect x="4.5" y="4.5" width="8" height="8" rx="1.5"/><path d="M9.5 4.5 V3 a1.5 1.5 0 0 0 -1.5 -1.5 H3 a1.5 1.5 0 0 0 -1.5 1.5 V8 a1.5 1.5 0 0 0 1.5 1.5 H4.5"/></svg>';
const CHECK_ICON_SVG = '<svg xmlns="http://www.w3.org/2000/svg" width="14" height="14" viewBox="0 0 14 14" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"><path d="M2.5 7.5 L5.5 10.5 L11.5 3.5"/></svg>';
const COPY_FEEDBACK_MS = 2000;

// UI 文案：来自 C++ MarkdownBridge 的 Q_PROPERTY（Qt 翻译体系），
// 无 bridge（本地浏览器调试）时用英文兜底；语言切换时经 retranslated 信号刷新
const FALLBACK_TEXTS = {
    collapseTooltip: "Collapse code block",
    expandTooltip: "Expand code block",
    copyTooltip: "Copy code",
    expandText: "Expand",
    collapsedLinesText: "%1 line(s) of code collapsed",
};

function uiText(key) {
    const b = window.bridge;
    return (b && typeof b[key] === "string" && b[key].length > 0) ? b[key] : FALLBACK_TEXTS[key];
}

// 统计代码行数（忽略末尾空行）
function countCodeLines(text) {
    const lines = String(text || "").split("\n");
    while (lines.length && lines[lines.length - 1].trim() === "") lines.pop();
    return lines.length;
}

function copyTextToClipboard(text) {
    const ta = document.createElement("textarea");
    ta.value = text;
    ta.setAttribute("readonly", "");
    ta.style.position = "fixed";
    ta.style.opacity = "0";
    document.body.appendChild(ta);
    ta.select();
    let ok = false;
    try { ok = document.execCommand("copy"); } catch (e) { ok = false; }
    document.body.removeChild(ta);
    return ok;
}

// 折叠/展开：折叠时隐藏 pre、显示“已折叠xx行代码”提示行，箭头方向切换
function setCollapsed(wrapper, toggleBtn, collapsed) {
    wrapper.classList.toggle("collapsed", collapsed);
    toggleBtn.innerHTML = collapsed ? ARROW_DOWN_SVG : ARROW_UP_SVG;
    toggleBtn.setAttribute("aria-label", collapsed ? uiText("expandTooltip") : uiText("collapseTooltip"));
}

// —— 活动实例注册表：语言切换后重刷动态文案（原 retranslateCodeBlocks 的 NodeView 化）——
const activeCodeBlockViews = new Set();

export function retranslateCodeBlocks() {
    activeCodeBlockViews.forEach((view) => view.retranslate());
}

// —— 表格 NodeView ——
// .table-block（外框卡片）> .table-block-header（表头装饰条）+ .table-wrapper
// （横向滚动 + 容器查询锚点）> table > tbody（contentDOM：行/单元格由 PM 填充，
// 默认渲染结构同为 table>tbody，保持一致）
export const tableView = $viewAsync(tableSchema.node, () => (node) => {
    const block = document.createElement("div");
    block.className = "table-block";
    const header = document.createElement("div");
    header.className = "table-block-header";
    const wrapper = document.createElement("div");
    wrapper.className = "table-wrapper";
    const table = document.createElement("table");
    const tbody = document.createElement("tbody");
    table.appendChild(tbody);
    wrapper.appendChild(table);
    block.appendChild(header);
    block.appendChild(wrapper);
    return {
        dom: block,
        contentDOM: tbody,
        // 同类型即原地更新（PM 已按 node.sameMarkup/content.eq 前置筛选）：
        // 行内容由 PM 重新渲染进 tbody，包裹层与滚动条状态保留
        update: (n) => n.type === node.type,
    };
});

// —— 代码块 NodeView ——
// .code-block（圆角半透明底）> .code-block-header（[折叠箭头][语言标签]…[复制按钮]）
// + .code-block-collapsed（折叠提示行）+ pre[data-language] > code（contentDOM）
export const codeBlockView = $viewAsync(codeBlockSchema.node, () => (node) => {
    let current = node;

    const wrapper = document.createElement("div");
    wrapper.className = "code-block";

    const header = document.createElement("div");
    header.className = "code-block-header";

    const toggleBtn = document.createElement("button");
    toggleBtn.type = "button";
    toggleBtn.className = "code-block-toggle";
    toggleBtn.innerHTML = ARROW_UP_SVG;
    toggleBtn.setAttribute("aria-label", uiText("collapseTooltip"));

    const langLabel = document.createElement("span");
    langLabel.className = "code-block-lang";

    const copyBtn = document.createElement("button");
    copyBtn.type = "button";
    copyBtn.className = "code-block-copy";
    copyBtn.innerHTML = COPY_ICON_SVG;
    copyBtn.title = uiText("copyTooltip");
    copyBtn.setAttribute("aria-label", uiText("copyTooltip"));

    header.appendChild(toggleBtn);
    header.appendChild(langLabel);
    header.appendChild(copyBtn);

    // 折叠提示行：“已折叠xx行代码  展开”，显示在代码块区域
    const hint = document.createElement("div");
    hint.className = "code-block-collapsed";
    const hintText = document.createElement("span");
    const expandBtn = document.createElement("span");
    expandBtn.className = "code-block-expand";
    expandBtn.textContent = uiText("expandText");
    hint.appendChild(hintText);
    hint.appendChild(expandBtn);

    const pre = document.createElement("pre");
    const code = document.createElement("code");
    pre.appendChild(code);

    wrapper.appendChild(header);
    wrapper.appendChild(hint);
    wrapper.appendChild(pre);

    // 语言标签/空块标记/折叠行数：从 ProseMirror 节点读取，不依赖 DOM 文本
    // （nodeView 构造时内容尚未渲染进 contentDOM，DOM 里还没有代码文本）
    const refreshMeta = () => {
        const lang = current.attrs.language || "";
        if (lang) pre.setAttribute("data-language", lang);
        else pre.removeAttribute("data-language");
        langLabel.textContent = lang;
        const lineCount = countCodeLines(current.textContent);
        // 空代码块（0 行）：加 empty 类，CSS 撤销 min-height 208px 设计最小高度
        wrapper.classList.toggle("empty", lineCount === 0);
        hintText.textContent = uiText("collapsedLinesText").replace("%1", lineCount);
    };
    refreshMeta();

    toggleBtn.addEventListener("click", () => {
        setCollapsed(wrapper, toggleBtn, !wrapper.classList.contains("collapsed"));
    });
    expandBtn.addEventListener("click", () => {
        setCollapsed(wrapper, toggleBtn, false);
    });
    copyBtn.addEventListener("click", () => {
        // 折叠态同样复制完整代码（表头常驻可见，读节点文本不受 pre 隐藏影响）
        const ok = copyTextToClipboard(current.textContent);
        copyBtn.innerHTML = ok ? CHECK_ICON_SVG : COPY_ICON_SVG;
        setTimeout(() => { copyBtn.innerHTML = COPY_ICON_SVG; }, COPY_FEEDBACK_MS);
    });

    const view = {
        dom: wrapper,
        contentDOM: code,
        // 同类型原地更新：刷新语言/行数等元信息；折叠状态与交互 DOM 保留
        update: (n) => {
            if (n.type !== current.type) return false;
            current = n;
            refreshMeta();
            return true;
        },
        retranslate: () => {
            refreshMeta();
            const collapsed = wrapper.classList.contains("collapsed");
            toggleBtn.setAttribute("aria-label", collapsed ? uiText("expandTooltip") : uiText("collapseTooltip"));
            expandBtn.textContent = uiText("expandText");
            copyBtn.title = uiText("copyTooltip");
            copyBtn.setAttribute("aria-label", uiText("copyTooltip"));
        },
        destroy: () => {
            activeCodeBlockViews.delete(view);
        },
    };
    activeCodeBlockViews.add(view);
    return view;
});
