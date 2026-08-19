// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// milkdownMathPlugins.js —— Milkdown 数学公式插件
//
// 移植自 uos-ai milkdownMathPlugins.ts，去除 TypeScript 类型注解。
// 基于 remark-math + KaTeX 实现行内公式（$...$）和块级公式（$$...$$）。
// 提供 normalizeMathDelimiters() 将 LLM 常见的 \(...\)/\[...\] 转为 $...$/$$...$$。

import katex from "katex";
import remarkMath from "remark-math";
import { $remark, $nodeSchema, $inputRule } from "@milkdown/kit/utils";
import { nodeRule } from "@milkdown/kit/prose";
import { InputRule } from "@milkdown/kit/prose/inputrules";

/* ==================== LaTeX 定界符规范化 ==================== */
export function normalizeMathDelimiters(markdown) {
    let result = markdown.replace(/\\\[([\s\S]*?)\\\]/g, (_m, content) => `$$${content}$$`);
    result = result.replace(/\\\((.+?)\\\)/g, (_m, content) => `$${content}$`);
    return result;
}

/* ==================== remark-math 解析插件 ==================== */
const remarkMathPlugin = $remark("remarkMath", () => remarkMath);

/* ==================== 行内公式 $...$ ==================== */
const MATH_INLINE_ID = "math_inline";
const mathInlineSchema = $nodeSchema(MATH_INLINE_ID, () => ({
    group: "inline",
    inline: true,
    atom: true,
    attrs: { value: { default: "" } },
    parseDOM: [{
        tag: `span[data-type="${MATH_INLINE_ID}"]`,
        getAttrs: (dom) => ({ value: dom.dataset.value ?? "" }),
    }],
    toDOM: (node) => {
        const code = node.attrs.value;
        const dom = document.createElement("span");
        dom.dataset.type = MATH_INLINE_ID;
        dom.dataset.value = code;
        dom.classList.add("math-inline");
        try { katex.render(code, dom, { throwOnError: false }); }
        catch { dom.textContent = code; }
        return dom;
    },
    parseMarkdown: {
        match: (node) => node.type === "inlineMath",
        runner: (state, node, type) => { state.addNode(type, { value: node.value }); },
    },
    toMarkdown: {
        match: (node) => node.type.name === MATH_INLINE_ID,
        runner: (state, node) => { state.addNode("inlineMath", undefined, node.attrs.value); },
    },
}));
const mathInlineInputRule = $inputRule((ctx) =>
    nodeRule(/(?<!\$)\$([^$\n]+)\$(?!\$)$/, mathInlineSchema.type(ctx), {
        getAttr: (m) => ({ value: m[1] ?? "" }),
    }),
);

/* ==================== 块级公式 $$...$$ ==================== */
const MATH_BLOCK_ID = "math_block";
const mathBlockSchema = $nodeSchema(MATH_BLOCK_ID, () => ({
    group: "block",
    atom: true,
    attrs: { value: { default: "" } },
    parseDOM: [{
        tag: `div[data-type="${MATH_BLOCK_ID}"]`,
        getAttrs: (dom) => ({ value: dom.dataset.value ?? "" }),
    }],
    toDOM: (node) => {
        const code = node.attrs.value || "";
        const wrapper = document.createElement("div");
        wrapper.dataset.type = MATH_BLOCK_ID;
        wrapper.dataset.value = code;
        wrapper.classList.add("math-block");
        if (code.trim()) {
            const preview = document.createElement("div");
            preview.classList.add("math-block__preview");
            try { katex.render(code, preview, { throwOnError: false, displayMode: true }); }
            catch { preview.textContent = code; }
            wrapper.appendChild(preview);
        }
        return wrapper;
    },
    parseMarkdown: {
        match: (node) => node.type === "math",
        runner: (state, node, type) => { state.addNode(type, { value: (node.value) || "" }); },
    },
    toMarkdown: {
        match: (node) => node.type.name === MATH_BLOCK_ID,
        runner: (state, node) => { state.addNode("math", undefined, (node.attrs.value) || ""); },
    },
}));
const mathBlockInputRule = $inputRule((ctx) =>
    new InputRule(/^\$\$((?:[^$\n]|\$(?!\$))+)\$\$$/, (state, match, start, end) => {
        const value = (match[1] ?? "").trim();
        if (!value) return null;
        const type = mathBlockSchema.type(ctx);
        const $from = state.doc.resolve(start);
        const blockFrom = $from.before($from.depth);
        const blockTo = $from.after($from.depth);
        return state.tr.replaceWith(blockFrom, blockTo, type.create({ value }));
    }),
);

export const mathPlugins = [
    ...remarkMathPlugin,
    ...mathInlineSchema,
    ...mathBlockSchema,
    mathInlineInputRule,
    mathBlockInputRule,
];
