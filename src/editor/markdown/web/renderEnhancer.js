// SPDX-FileCopyrightText: 2026 UnionCTechnology Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// renderEnhancer.js —— 渲染后处理
//
// §4.8.8：在 Milkdown 默认 DOM 之上做无副作用的幂等加工。
// wrapTables：表格包裹（横向滚动 + 容器查询锚点）。
// enhanceCodeBlocks：代码块操作表头（折叠按钮 + 语言标签 + 复制按钮）与折叠提示行。

const TABLE_WRAPPER_CLASS = "table-wrapper";
const CODE_BLOCK_CLASS = "code-block";

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

// 语言切换后重刷已渲染代码块上的动态文案（静态文案由 enhance() 重建时带上）
export function retranslateCodeBlocks(root) {
    if (!root) return;
    root.querySelectorAll("." + CODE_BLOCK_CLASS).forEach((wrapper) => {
        const pre = wrapper.querySelector("pre");
        const lineCount = pre ? countCodeLines(pre.textContent) : 0;
        wrapper.querySelector(".code-block-collapsed > span:first-child").textContent =
            uiText("collapsedLinesText").replace("%1", lineCount);
        wrapper.querySelector(".code-block-expand").textContent = uiText("expandText");
        const collapsed = wrapper.classList.contains("collapsed");
        const toggle = wrapper.querySelector(".code-block-toggle");
        toggle.setAttribute("aria-label", collapsed ? uiText("expandTooltip") : uiText("collapseTooltip"));
        const copy = wrapper.querySelector(".code-block-copy");
        copy.title = uiText("copyTooltip");
        copy.setAttribute("aria-label", uiText("copyTooltip"));
    });
}

// 把每个 <table> 包裹为 .table-block > (.table-block-header + .table-wrapper > table)
// .table-wrapper：横向滚动容器 + 容器查询锚点；.table-block-header：空表头装饰条
// 幂等：已包裹的表格跳过
export function wrapTables(root) {
    if (!root) return;
    const tables = root.querySelectorAll("table");
    tables.forEach((table) => {
        if (table.closest("." + TABLE_WRAPPER_CLASS)) return;
        const block = document.createElement("div");
        block.className = "table-block";
        const header = document.createElement("div");
        header.className = "table-block-header";
        const wrapper = document.createElement("div");
        wrapper.className = TABLE_WRAPPER_CLASS;
        table.parentNode.insertBefore(block, table);
        block.appendChild(header);
        block.appendChild(wrapper);
        wrapper.appendChild(table);
    });
}

// 统计代码行数（忽略末尾空行）
function countCodeLines(text) {
    const lines = String(text || "").split("\n");
    while (lines.length && lines[lines.length - 1].trim() === "") lines.pop();
    return lines.length;
}

// 语言名：Milkdown 7.x 渲染在 pre[data-language]；兼容旧式 code.language-xxx class
function detectLanguage(pre) {
    const lang = pre.getAttribute("data-language");
    if (lang && lang.length > 0) return lang;
    const code = pre.querySelector("code");
    if (code) {
        const m = code.className.match(/(?:^|\s)language-([\w#+-]+)/);
        if (m) return m[1];
    }
    return "";
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

// 代码块加工：包一层 .code-block（圆角半透明底 + 吸顶表头挂载点）
// 表头结构：[折叠箭头][语言标签]……[复制按钮]；提示行在块区内、不在表头内
// 幂等：已包裹的 pre 跳过
export function enhanceCodeBlocks(root) {
    if (!root) return;
    root.querySelectorAll("pre").forEach((pre) => {
        if (pre.closest("." + CODE_BLOCK_CLASS)) return;

        const lang = detectLanguage(pre);
        const codeEl = pre.querySelector("code");
        const lineCount = countCodeLines(codeEl ? codeEl.textContent : pre.textContent);

        const wrapper = document.createElement("div");
        wrapper.className = CODE_BLOCK_CLASS;
        // 空代码块（0 行）：加 empty 类，CSS 撤销 min-height 208px 设计最小高度，
        // 表头下仅保留 pre 自身的一行空白，避免大块空白
        if (lineCount === 0) wrapper.classList.add("empty");

        const header = document.createElement("div");
        header.className = "code-block-header";

        const toggleBtn = document.createElement("button");
        toggleBtn.type = "button";
        toggleBtn.className = "code-block-toggle";
        toggleBtn.innerHTML = ARROW_UP_SVG;
        toggleBtn.setAttribute("aria-label", uiText("collapseTooltip"));

        const langLabel = document.createElement("span");
        langLabel.className = "code-block-lang";
        langLabel.textContent = lang;

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
        hintText.textContent = uiText("collapsedLinesText").replace("%1", lineCount);
        const expandBtn = document.createElement("span");
        expandBtn.className = "code-block-expand";
        expandBtn.textContent = uiText("expandText");
        hint.appendChild(hintText);
        hint.appendChild(expandBtn);

        pre.parentNode.insertBefore(wrapper, pre);
        wrapper.appendChild(header);
        wrapper.appendChild(hint);
        wrapper.appendChild(pre);

        toggleBtn.addEventListener("click", () => {
            setCollapsed(wrapper, toggleBtn, !wrapper.classList.contains("collapsed"));
        });
        expandBtn.addEventListener("click", () => {
            setCollapsed(wrapper, toggleBtn, false);
        });
        copyBtn.addEventListener("click", () => {
            // 折叠态同样复制完整代码（表头常驻可见，pre 隐藏不影响取文本）
            const ok = copyTextToClipboard(codeEl ? codeEl.textContent : pre.textContent);
            copyBtn.innerHTML = ok ? CHECK_ICON_SVG : COPY_ICON_SVG;
            setTimeout(() => { copyBtn.innerHTML = COPY_ICON_SVG; }, COPY_FEEDBACK_MS);
        });
    });
}

// 统一入口：每次 Milkdown 重渲染后执行一次
export function enhance(root) {
    wrapTables(root);
    enhanceCodeBlocks(root);
}
