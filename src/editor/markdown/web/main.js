// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// main.js —— Milkdown 实例装配（移植自 uos-ai MarkdownEditor.tsx，去 Vue 包装，裸挂载）
//
// 插件链顺序与 uos-ai 一致：
//   commonmark → gfm → history → indent → clipboard → mathPlugins
//   另注册 tableView/codeBlockView（nodeViews.js）：表格/代码块视觉包裹走 NodeView
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
import { tableView, codeBlockView, retranslateCodeBlocks } from "./nodeViews.js";
import { unified } from "unified";
import remarkParse from "remark-parse";
import remarkGfm from "remark-gfm";
import remarkMath from "remark-math";

import "katex/dist/katex.min.css";
import "./theme.css";

const ROOT_ID = "app";
let editor = null;
let lastValue = "";

// —— C++ → JS 处理器 ——
let lastRequestedLine = 0;   // 最近一次 C++ 请求的源码行号（0 基顶部可见行）；重渲染后重放（§4.6 初始对齐）

// —— 渐进渲染（大文件分段上屏，性能优化 2026-09-16）——
// 整篇 replaceAll 需先完成全文 ProseMirror 建模才一次性上屏，大文档耗时超线性
// （实测 5MB≈50s、10MB≈242s，期间预览空白）。超过阈值改为整篇解析后分批插入：
// 先 parserCtx(md) 整篇解析确保跨块语义完整，再将顶层节点按 nodeSize 分批
// （首批 ~256KB、后续每批 ~1MB）tr.insert 追加到文档末尾，批间让出事件循环
// ——首屏秒级可见、内容渐进填充、渲染进程保持响应。
const PROGRESSIVE_THRESHOLD = 1024 * 1024; // 触发渐进的最小字符数（约 1MB）
const PROGRESSIVE_FIRST_BATCH = 256 * 1024; // 首批 nodeSize 上限（约 256KB，尽快出首屏）
const PROGRESSIVE_BATCH_SIZE = 1024 * 1024; // 后续批 nodeSize 上限（约 1MB，压低批数/分发开销）
let renderGeneration = 0;                  // 代际号：新内容到达时作废进行中的渐进渲染
let anchorMap = null;                      // [{sourceLine, previewY}, ...]：锚点映射表；null=未构建/降级
let anchorBuildHeight = -1;                // 锚点表构建时的页面 scrollHeight（惰性失效判据）
let lastUserScrollAt = 0;                  // 最近一次用户滚动右栏的时间戳（布局重排对齐护栏）

// —— 锚点映射（anchor-based line mapping，PMS 378227 修复）——
// VS Code markdown preview 同款行号同步：两栏按"顶部可见源码行号"对齐，而非滚动比例
//（比例假设"源码第 N% 位置 = 预览第 N% 位置"，且左栏滚动条比例 value/max 与行号比例
// line/总行数天然错位——max = 总行数 - 可见行数，中段滚动偏差可达半个视口）。
// 渲染后按 remark AST 顶层块（与 DOM 子节点同源一一对应）建立 {源码行号→预览像素Y}
// 锚点表，滚动时通过锚点段内线性插值定位，C++ 侧协议传行号（scrollToLine/onScrollLine）。

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
    anchorMap = null;                 // 旧锚点表作废，待 reapplyScroll/buildAnchorMap 重建
    userScrolledDuringBuild = false;
    if (normalized.length <= PROGRESSIVE_THRESHOLD) {
        editor.action(replaceAll(normalized));
        // 表格/代码块的视觉包裹由 NodeView 随文档同步生成（不再事后 enhance），
        // 渲染完成后重放滚动行号：首次渲染完成前 scrollToLine 因 max=0 被跳过，
        // 此处对齐左右初始位置；搜索高亮同样重放（replaceAll 重建了全部 DOM 节点）
        setTimeout(() => {
            reapplyScroll();
            reapplySearchHighlight();
        }, 0);
        return;
    }
    renderProgressively(normalized, renderGeneration);
}

function renderProgressively(md, gen) {
    // 整篇解析后再分批插入：确保跨块 markdown 语义（引用式链接定义、
    // 表格、松散列表等）完整解析，避免分块独立解析导致的格式丢失。
    let parsed = null;
    editor.action((ctx) => {
        parsed = ctx.get(parserCtx)(md);
    });
    if (!parsed) return;

    // 按 nodeSize 分批：首批小批抢首屏，后续大批减少批数。不可逐节点一批——
    // 万级节点的文档每批一次 dispatch + setTimeout 间隙，1.4MB 填充实测超 2 分钟；
    // 按大小分批后同为秒级。单个超限节点（超长表格/围栏）独立成批，不做拆分。
    const batches = [];
    let cur = [];
    let curSize = 0;
    let limit = PROGRESSIVE_FIRST_BATCH;
    parsed.content.forEach((node) => {
        cur.push(node);
        curSize += node.nodeSize;
        if (curSize >= limit) {
            batches.push(cur);
            cur = [];
            curSize = 0;
            limit = PROGRESSIVE_BATCH_SIZE;
        }
    });
    if (cur.length > 0) batches.push(cur);

    const totalSize = parsed.content.size;
    let renderedSize = 0;
    let index = 0;
    buildProgress = { fraction: 0 };
    const step = () => {
        // 代际失效：期间有新内容/新请求到达，本轮渐进渲染作废
        if (gen !== renderGeneration || !editor) return;
        const batch = batches[index++];
        if (!batch) {
            buildProgress = null;
            reapplySearchHighlight();
            if (!userScrolledDuringBuild) reapplyScroll();
            return;
        }
        for (const node of batch) renderedSize += node.nodeSize;
        if (index === 1) {
            // 首批：清空重建 + 初始滚动对齐（对齐时机在后续批增高文档之前）
            replaceWithNodes(batch);
            setTimeout(() => {
                if (gen !== renderGeneration) return;
                reapplyScroll();
                reapplySearchHighlight();
            }, 0);
        } else {
            appendChunk(batch);
            buildAnchorMap();
        }
        buildProgress.fraction = totalSize > 0 ? renderedSize / totalSize : 1;
        if (index < batches.length) {
            setTimeout(step, 0);
        } else {
            // 构建完成：f=1，后续同步恢复精确比例。若用户未动过右栏，
            // 按左栏最近请求做最终对齐（消除补偿近似误差）；动过则尊重其位置。
            buildProgress = null;
            reapplySearchHighlight();   // 完整 DOM 就绪后重建全部命中高亮
            if (!userScrolledDuringBuild) {
                buildAnchorMap();
                reapplyScroll();
            }
        }
    };
    step();   // 首批同步渲染，抢最快首屏
}

// 用已解析节点清空并重建文档内容
function replaceWithNodes(nodes) {
    editor.action((ctx) => {
        const view = ctx.get(editorViewCtx);
        const tr = view.state.tr;
        view.dispatch(tr.replaceWith(0, tr.doc.content.size, nodes));
    });
}

// 将已解析节点追加到文档末尾；表格/代码块外观由 NodeView 随节点创建自动生成
function appendChunk(nodes) {
    editor.action((ctx) => {
        const view = ctx.get(editorViewCtx);
        view.dispatch(view.state.tr.insert(view.state.doc.content.size, nodes));
    });
}

// 全文档比例 → 当前（可能部分构建的）右栏内比例
function toRenderedRatio(ratio) {
    if (!buildProgress || buildProgress.fraction <= 0) return ratio;
    return Math.max(0, Math.min(1, ratio / buildProgress.fraction));
}

// —— 锚点源行解析（与渲染管线同源的 remark AST）——
// milkdown 渲染链 = remarkParse+gfm+math → ProseMirror → DOM，顶层 AST 节点与
// .ProseMirror 直接子节点一一对应；仅 definition（引用式链接定义）被解析器
// 消费、不产生 DOM，需跳过。
// 旧方案按空行切分源码估算行号，在以下场景与渲染结构错位、配对逐块偏移，漂移
// 累积可达整节（V-5278 复测反馈）：
//   连续空行 → 产生无对应 DOM 的"幻影"块
//   松散列表（项间空行）→ 源码多块、渲染合并为一个 ul
//   definition / 引用式链接定义 → 源码有块、渲染无节点
// 故改为直接取 AST position 的精确起始行（remark 与渲染器共用，结构天然一致）。
const mdBlockParser = unified().use(remarkParse).use(remarkGfm).use(remarkMath);
let mdBlockCache = { value: null, blocks: null };   // 渐进渲染逐块重建锚点，缓存避免重复全文解析

// 返回可渲染顶层块：[{ type, startLine }...]（startLine 为 0 基源码行号）；解析失败返回 null
function topRenderedBlocks(md) {
    if (mdBlockCache.value !== md) {
        let blocks = null;
        try {
            const ast = mdBlockParser.parse(md);
            blocks = ast.children
                .filter((node) => node.type !== "definition" && node.position)
                .map((node) => ({ type: node.type, startLine: node.position.start.line - 1 }));
        } catch (e) {
            console.error("[md] remark parse for anchors failed:", e);
        }
        mdBlockCache = { value: md, blocks };
    }
    return mdBlockCache.blocks;
}

// 构建锚点映射表：将源码行号与预览像素 Y 对应。
// 首选全量配对：AST 顶层块 i ↔ .ProseMirror 子节点 i（数量相等时逐块精确；
// 整篇解析渲染完成后两者结构天然一致）。数量不等时（渐进构建期间 DOM 仅含
// 已插入的批次）退化为标题锚点：DOM 与 AST 中的标题序列仍按序一一对应；
// 标题锚点不足 2 个则置 null，调用方降级为纯比例映射。
// previewY 取 getBoundingClientRect + scrollY 的文档绝对 Y（与 window.scrollTo
// 同一坐标系；offsetTop 依赖 offsetParent 链，居中布局/外边距下不可靠）。
let lastAnchorMode = null;   // "zip" | "headings" | null，诊断用
const docY = (el) => el.getBoundingClientRect().top + window.scrollY;
function buildAnchorMap() {
    // 快照基准高度：此后 scrollHeight 变化即视为锚点过期（图片/字体/公式异步加载，
    // 实测 d_editor.md 60 张图加载使高度翻倍、行 279 目标 Y 漂移 9000px+，右栏大幅落后）
    anchorBuildHeight = document.documentElement.scrollHeight;
    const pm = document.querySelector(".ProseMirror");
    if (!pm || !lastValue) { anchorMap = null; lastAnchorMode = null; return; }
    const children = Array.from(pm.children);
    const blocks = topRenderedBlocks(lastValue);
    if (!blocks || children.length === 0) { anchorMap = null; lastAnchorMode = null; return; }

    const anchors = [];
    if (blocks.length === children.length) {
        lastAnchorMode = "zip";
        for (let i = 0; i < children.length; i++) {
            anchors.push({ sourceLine: blocks[i].startLine, previewY: docY(children[i]) });
        }
    } else {
        const astHeadings = blocks.filter((b) => b.type === "heading");
        const domHeadings = children.filter((el) => /^H[1-6]$/.test(el.tagName));
        const pairCount = Math.min(astHeadings.length, domHeadings.length);
        for (let i = 0; i < pairCount; i++) {
            anchors.push({ sourceLine: astHeadings[i].startLine, previewY: docY(domHeadings[i]) });
        }
        lastAnchorMode = anchors.length >= 2 ? "headings" : null;
        if (!buildProgress) {
            console.warn("[md] anchor block/dom mismatch:", blocks.length, "vs", children.length,
                "→ heading anchors:", anchors.length);
        }
    }
    // 哨兵：文档末尾（源码行号=总行数，预览Y=最后一个子节点底部）
    const lastChild = children[children.length - 1];
    anchors.push({ sourceLine: lastValue.split("\n").length,
                   previewY: lastChild.getBoundingClientRect().bottom + window.scrollY });
    anchorMap = anchors.length >= 2 ? anchors : null;
    if (anchorMap === null) lastAnchorMode = null;
}

// 源码行号 → 预览像素 Y（二分查找锚点段，段内线性插值）
// 锚点表不可用时降级为"行号/总行数 ≈ 滚动比例"近似
function lineToPreviewY(line) {
    if (!anchorMap || anchorMap.length < 2) {
        const max = document.documentElement.scrollHeight - window.innerHeight;
        const totalLines = lastValue ? lastValue.split("\n").length : 1;
        return (totalLines > 0 ? line / totalLines : 0) * max;
    }
    let lo = 0, hi = anchorMap.length - 1;
    while (lo < hi - 1) {
        const mid = (lo + hi) >> 1;
        if (anchorMap[mid].sourceLine <= line) lo = mid;
        else hi = mid;
    }
    const a0 = anchorMap[lo];
    const a1 = anchorMap[Math.min(lo + 1, anchorMap.length - 1)];
    const lineSpan = a1.sourceLine - a0.sourceLine;
    if (lineSpan <= 0) return a0.previewY;
    let t = (line - a0.sourceLine) / lineSpan;
    t = Math.max(0, Math.min(1, t));
    return a0.previewY + t * (a1.previewY - a0.previewY);
}

// 预览像素 Y → 源码行号（二分查找锚点段，段内线性插值，可返回小数）
// 锚点表不可用时降级为"滚动比例 × 总行数"近似（渐进期经 buildProgress 补偿）
function previewYToLine(scrollY) {
    if (!anchorMap || anchorMap.length < 2) {
        const max = document.documentElement.scrollHeight - window.innerHeight;
        let ratio = max > 0 ? scrollY / max : 0;
        if (buildProgress) ratio = Math.min(1, ratio * buildProgress.fraction);
        const totalLines = lastValue ? lastValue.split("\n").length : 1;
        return ratio * totalLines;
    }
    let lo = 0, hi = anchorMap.length - 1;
    while (lo < hi - 1) {
        const mid = (lo + hi) >> 1;
        if (anchorMap[mid].previewY <= scrollY) lo = mid;
        else hi = mid;
    }
    const a0 = anchorMap[lo];
    const a1 = anchorMap[Math.min(lo + 1, anchorMap.length - 1)];
    const ySpan = a1.previewY - a0.previewY;
    if (ySpan <= 0) return a0.sourceLine;
    let t = (scrollY - a0.previewY) / ySpan;
    t = Math.max(0, Math.min(1, t));
    return a0.sourceLine + t * (a1.sourceLine - a0.sourceLine);
}

// 惰性失效：协议入口换算前核对 scrollHeight，与快照不符即重建锚点表
//（兜底路径——即使 ResizeObserver 回调遗漏/时序错后，下一次同步仍然正确；
//  与 VS Code 每次滚动实时查询元素位置同效，成本为一次 scrollHeight 读取）
function ensureAnchorsFresh() {
    if (document.documentElement.scrollHeight !== anchorBuildHeight) {
        buildAnchorMap();
    }
}

function reapplyScroll() {
    buildAnchorMap();
    const max = document.documentElement.scrollHeight - window.innerHeight;
    if (max <= 0) return;
    if (anchorMap && anchorMap.length >= 2) {
        applyProgrammaticScroll(lineToPreviewY(lastRequestedLine));
    } else {
        // 降级：行号按总行数折算比例（渐进构建期先补偿到已渲染范围）
        const totalLines = lastValue ? lastValue.split("\n").length : 1;
        const ratio = totalLines > 0 ? lastRequestedLine / totalLines : 0;
        applyProgrammaticScroll(toRenderedRatio(ratio) * max);
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

// —— 搜索高亮（CSS Custom Highlight API，对齐左侧编辑器查找配色）——
// C++ 侧 TextEdit::syncRendererFindHighlight 经 bridge 推送（关键词/当前命中序号/
// 大小写），全部命中用 find-match 配色、当前命中用 find-highlight 配色（theme.css
// ::highlight 规则）。选 CSS.highlights 而非 <mark> 包裹：不改动 DOM，不干扰
// ProseMirror 文档模型与 NodeView，天然支持跨节点匹配；重渲染后凭 lastSearch 重放。
// Chromium <105 无此 API 时静默降级（只滚动、不高亮）。
const FIND_MATCH_HL = "md-find-match";
const FIND_CURRENT_HL = "md-find-current";
let lastSearch = null;         // { keyword, index, caseSensitive }：最近一次高亮请求；null=已清除
let lastAppliedSearch = null;  // { key, index, gen }：已应用状态，配合 renderGeneration 幂等去重

// NodeView UI 部件文本（表头工具条/代码块表头/折叠提示/KaTeX 隐藏 MathML 副本）
// 不参与匹配，对齐左侧编辑器"只高亮正文"的语义
const SEARCH_TEXT_REJECT = ".table-block-header, .code-block-header, .code-block-collapsed, .katex-mathml";

function collectTextNodes(root) {
    const nodes = [];
    let text = "";
    const walker = document.createTreeWalker(root, NodeFilter.SHOW_TEXT, {
        acceptNode(node) {
            const el = node.parentElement;
            if (el && el.closest(SEARCH_TEXT_REJECT)) return NodeFilter.FILTER_REJECT;
            return NodeFilter.FILTER_ACCEPT;
        },
    });
    while (walker.nextNode()) {
        nodes.push({ node: walker.currentNode, start: text.length });
        text += walker.currentNode.nodeValue;
    }
    return { nodes, text };
}

// 全文偏移区间 [start, end) → DOM Range（二分定位起始节点，向后找结束节点）
function rangeForOffsets(nodes, start, end) {
    let lo = 0, hi = nodes.length - 1;
    while (lo < hi) {
        const mid = (lo + hi + 1) >> 1;
        if (nodes[mid].start <= start) lo = mid;
        else hi = mid - 1;
    }
    if (nodes.length === 0 || start < nodes[lo].start) return null;
    let idx = lo;
    while (idx + 1 < nodes.length && nodes[idx + 1].start < end) idx++;
    const range = document.createRange();
    range.setStart(nodes[lo].node, start - nodes[lo].start);
    range.setEnd(nodes[idx].node, end - nodes[idx].start);
    return range;
}

function clearSearchHighlight() {
    if (typeof CSS === "undefined" || !CSS.highlights) return;
    CSS.highlights.delete(FIND_MATCH_HL);
    CSS.highlights.delete(FIND_CURRENT_HL);
}

function applySearchHighlight() {
    if (typeof CSS === "undefined" || !CSS.highlights) return;
    clearSearchHighlight();
    lastAppliedSearch = null;
    if (!lastSearch || !lastSearch.keyword) return;
    const pm = document.querySelector(".ProseMirror");
    if (!pm) return;
    const { nodes, text } = collectTextNodes(pm);
    const needle = lastSearch.caseSensitive ? lastSearch.keyword : lastSearch.keyword.toLowerCase();
    const hay = lastSearch.caseSensitive ? text : text.toLowerCase();
    const matchRanges = [];
    const currentRanges = [];
    let from = 0;
    let idx = 0;
    let found;
    while ((found = hay.indexOf(needle, from)) !== -1) {
        const range = rangeForOffsets(nodes, found, found + needle.length);
        if (range) (idx === lastSearch.index ? currentRanges : matchRanges).push(range);
        from = found + needle.length;
        idx++;
    }
    if (matchRanges.length > 0) {
        const hl = new Highlight();
        for (const r of matchRanges) hl.add(r);
        CSS.highlights.set(FIND_MATCH_HL, hl);
    }
    if (currentRanges.length > 0) {
        const hl = new Highlight();
        for (const r of currentRanges) hl.add(r);
        CSS.highlights.set(FIND_CURRENT_HL, hl);
    }
    lastAppliedSearch = {
        key: (lastSearch.caseSensitive ? "1;" : "0;") + lastSearch.keyword,
        index: lastSearch.index,
        gen: renderGeneration,
    };
}

// 幂等入口：状态与代际均未变化时跳过重扫（滚动 valueChanged 路径高频重推）
function reapplySearchHighlight() {
    if (typeof CSS === "undefined" || !CSS.highlights) return;
    if (!lastSearch || !lastSearch.keyword) {
        applySearchHighlight();   // 清除路径（applySearchHighlight 内部处理空态）
        return;
    }
    const key = (lastSearch.caseSensitive ? "1;" : "0;") + lastSearch.keyword;
    if (lastAppliedSearch && lastAppliedSearch.key === key
        && lastAppliedSearch.index === lastSearch.index
        && lastAppliedSearch.gen === renderGeneration)
        return;
    applySearchHighlight();
}

function highlightSearch(keyword, currentIndex, caseSensitive) {
    const kw = keyword == null ? "" : String(keyword);
    lastSearch = kw ? { keyword: kw, index: currentIndex | 0, caseSensitive: !!caseSensitive } : null;
    reapplySearchHighlight();
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

function scrollToLine(line) {
    // §4.6 VS Code 式行号同步：line 为 0 基源码行号（左栏顶部可见行，revealLine 语义）
    // 根因修复：ProseMirror 元素本身不可滚动（无 overflow:auto），页面滚动在 window/documentElement。
    line = Math.max(0, line);
    lastRequestedLine = line;
    ensureAnchorsFresh();   // 布局自上次建表后可能已变（图片等异步加载）
    const max = document.documentElement.scrollHeight - window.innerHeight;
    if (max <= 0) return;
    if (anchorMap && anchorMap.length >= 2) {
        // 锚点映射：行号所在块对齐视口顶部
        applyProgrammaticScroll(lineToPreviewY(line));
    } else {
        // 降级：行号按总行数折算比例（渐进构建期先补偿到已渲染范围）
        const totalLines = lastValue ? lastValue.split("\n").length : 1;
        const ratio = totalLines > 0 ? line / totalLines : 0;
        applyProgrammaticScroll(toRenderedRatio(ratio) * max);
    }
}

// 右栏用户滚动 → 通知 C++（反向同步，回传顶部可见源码行号）
let __lastNotifiedLine = -1;
let __lastNotifiedY = 0;
window.addEventListener("scroll", () => {
    // 程序化产物（同步请求的滚动、其异步事件、回声窗口期）不回传，避免左→右→左回环
    if (__programmaticScroll || isProgrammaticPosition() || performance.now() < __programmaticUntil)
        return;
    __lastProgrammaticY = null;   // 用户已接管滚动位置
    ensureAnchorsFresh();         // 布局可能已变（图片等），回传行号须按新锚点表换算
    const max = document.documentElement.scrollHeight - window.innerHeight;
    if (max <= 0) return;
    const line = previewYToLine(window.scrollY);
    lastUserScrollAt = performance.now();
    if (buildProgress) userScrolledDuringBuild = true;
    // 去抖阈值须像素感知：固定比例阈值对超大文档（30 万行级）相当于数屏死区，
    // 用户翻页会被吞掉；行号维度 0.5 行以内且像素 40px 以内视为未变。
    if (Math.abs(window.scrollY - __lastNotifiedY) < 40 && Math.abs(line - __lastNotifiedLine) < 0.5)
        return;
    __lastNotifiedLine = line;
    __lastNotifiedY = window.scrollY;
    // 用户当前阅读行成为对齐目标：布局变化（图片加载等）后的重放以此为准，
    // 不再把用户拉回旧位置
    lastRequestedLine = line;
    if (window.bridge && typeof window.bridge.onScrollLine === "function") {
        window.bridge.onScrollLine(line);
    }
}, { passive: true });

// —— 布局变化 → 锚点失效重建（VS Code 每次滚动实时查询位置；本实现为快照+失效）——
// 图片/字体/KaTeX/代码高亮异步加载会改变块高：渲染完成时建的锚点表系统性偏小，
// 右栏滚动不到位（实测 d_editor.md：60 张图加载后目标 Y 漂移 9000px+）。
// ResizeObserver 监听高度变化，100ms 防抖合并连续加载；重建后若用户近期未滚动
//（>500ms），按 lastRequestedLine 重放对齐（该值已跟随用户右栏滚动，重放即锚定
// 当前阅读行，不会拉回旧位置）。渐进构建期不重放（由构建流程自行对齐）。
if (typeof ResizeObserver !== "undefined") {
    let layoutSyncTimer = 0;
    new ResizeObserver(() => {
        clearTimeout(layoutSyncTimer);
        layoutSyncTimer = setTimeout(() => {
            if (!lastValue) return;
            buildAnchorMap();
            if (!buildProgress
                    && performance.now() - lastUserScrollAt > 500
                    && lastRequestedLine > 0) {
                reapplyScroll();
            }
        }, 100);
    }).observe(document.body);
}

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
        // 表格/代码块包裹结构的 NodeView（闪烁根治，见 nodeViews.js 头注）
        .use(tableView)
        .use(codeBlockView)
        .create();

    setupBridge({
        onSetMarkdown: renderMarkdown,
        onSetMode: () => { /* 阶段二 */ },
        onApplyTheme: applyTheme,
        onSetLayout: applyLayout,
        onScrollToLine: scrollToLine,
        onSearchHighlight: highlightSearch,
        onRetranslate: () => retranslateCodeBlocks(),
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

// Standalone 调试/自动化验证钩子：仅无 QWebChannel（本地浏览器，bridge.js
// standalone 降级路径）时暴露；QWebEngine 宿主先加载 qwebchannel.js，不会进此分支
if (typeof QWebChannel === "undefined") {
    window.__mdTest = {
        render: renderMarkdown,
        buildAnchorMap,
        getAnchorMap: () => anchorMap,
        getAnchorMode: () => lastAnchorMode,
        topRenderedBlocks,
        scrollToLine,
        lineToPreviewY,
        previewYToLine,
        getLastRequestedLine: () => lastRequestedLine,
        rootId: ROOT_ID,
        // 搜索高亮（本地浏览器验证）：highlight + 状态探针
        highlightSearch,
        searchHighlightCounts: () => ({
            match: (CSS.highlights && CSS.highlights.get(FIND_MATCH_HL) || { size: 0 }).size,
            current: (CSS.highlights && CSS.highlights.get(FIND_CURRENT_HL) || { size: 0 }).size,
        }),
    };
}

boot().catch((e) => console.error("Milkdown boot failed:", e));
