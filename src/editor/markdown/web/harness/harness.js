// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// harness.js —— NodeView 包裹层防闪烁回归验证（配合 harness.html，本地浏览器用）
//
// 场景对齐“实时阅览编辑”路径：连续两次 setMarkdown（等价左栏 300ms 节流后的两次
// 全量 replaceAll），断言：
//   T1 首次渲染：table 位于 .table-block>.table-wrapper 内（NodeView 包裹生效）
//   T2 重渲染（表格内容未变）：.table-block DOM 身份保留（=== 同一元素）——
//      旧方案此处包裹层被 renderDescs 拆除重建，即闪烁根因
//   T3 重渲染（表格内容已变）：包裹结构完整且行数正确（update 原地更新路径）
//   T4 代码块：.code-block 表头/复制按钮/pre 结构 + 重渲染后 DOM 身份保留
//   T5 新增表格：增量文档中第二个表格同样被包裹
// 结果写入 #harness-result（JSON），失败置 failed 类，document.title 同步 PASS/FAIL。

import "../main.js";

const mdBase = [
    "# 标题",
    "",
    "段落文字，编辑时触发重渲染。",
    "",
    "| 列A | 列B |",
    "| --- | --- |",
    "| a1 | b1 |",
    "| a2 | b2 |",
    "",
    "```cpp",
    "int main() {",
    "    return 0;",
    "}",
    "```",
].join("\n");

// T2：仅追加段落（表格未变）
const mdAppendParagraph = mdBase + "\n\n追加的一段文字。";

// T3：表格加一行 + 改一个单元格
const mdTableEdited = [
    "# 标题",
    "",
    "段落文字，编辑时触发重渲染。",
    "",
    "| 列A | 列B |",
    "| --- | --- |",
    "| a1 | b1 |",
    "| a2 | 改动 |",
    "| a3 | b3 |",
    "",
    "```cpp",
    "int main() {",
    "    return 0;",
    "}",
    "```",
].join("\n");

// T5：再追加第二个表格
const mdSecondTable = mdTableEdited + [
    "",
    "| x | y |",
    "| --- | --- |",
    "| 1 | 2 |",
].join("\n");

const results = [];
function check(name, ok, detail) {
    results.push({ name, ok: !!ok, detail: detail || "" });
    if (!ok) console.error("[harness] FAIL:", name, detail || "");
}

function waitFrames(n) {
    // 跨宏任务 + rAF 双重等待：覆盖 renderDescs 同步更新与 setTimeout(0) 间隙
    return new Promise((resolve) => {
        let left = n;
        const tick = () => (left-- > 0 ? requestAnimationFrame(() => setTimeout(tick, 0)) : resolve());
        setTimeout(tick, 0);
    });
}

function waitFor(cond, timeoutMs) {
    const start = performance.now();
    return new Promise((resolve, reject) => {
        const poll = () => {
            let v;
            try { v = cond(); } catch (e) { return reject(e); }
            if (v) return resolve(v);
            if (performance.now() - start > (timeoutMs || 10000)) return reject(new Error("timeout"));
            setTimeout(poll, 50);
        };
        poll();
    });
}

async function run() {
    const out = document.getElementById("harness-result");
    const finish = () => {
        const failed = results.some((r) => !r.ok);
        out.textContent = JSON.stringify(results, null, 2);
        out.classList.toggle("failed", failed);
        document.title = failed ? "HARNESS-FAIL" : "HARNESS-PASS";
    };

    try {
        const test = await waitFor(() => (window.__mdTest && document.querySelector(".milkdown, .ProseMirror") ? window.__mdTest : null));
        const root = document.getElementById(test.rootId);

        // —— T1 首次渲染 ——
        test.render(mdBase);
        await waitFor(() => root.querySelector(".table-block table tbody tr"));
        await waitFrames(3);
        const tableBlock1 = root.querySelector(".table-block");
        const wrapper1 = root.querySelector(".table-block .table-wrapper");
        check("T1_initial_table_wrapped", !!tableBlock1 && !!wrapper1 && !!wrapper1.querySelector("table tbody tr"),
            tableBlock1 ? "ok" : ".table-block missing");
        check("T1_initial_table_direct_child_of_pm",
            tableBlock1 && tableBlock1.parentElement && tableBlock1.parentElement.classList.contains("ProseMirror"),
            "包裹层应为 .ProseMirror 直接子节点");

        const codeBlock1 = root.querySelector(".code-block");
        check("T4_initial_code_block_structure",
            !!codeBlock1
            && !!codeBlock1.querySelector(".code-block-header .code-block-toggle")
            && !!codeBlock1.querySelector(".code-block-lang")
            && !!codeBlock1.querySelector(".code-block-copy")
            && !!codeBlock1.querySelector("pre > code"),
            codeBlock1 ? "ok" : ".code-block missing");

        // —— T2 重渲染：表格未变，包裹层 DOM 身份必须保留（防闪烁核心断言）——
        test.render(mdAppendParagraph);
        await waitFor(() => root.textContent.includes("追加的一段文字"));
        await waitFrames(3);
        const tableBlock2 = root.querySelector(".table-block");
        check("T2_rerender_table_block_identity_preserved", tableBlock2 === tableBlock1,
            tableBlock2 === tableBlock1 ? "ok" : "包裹层被重建（会闪烁）");
        const wrapper2 = root.querySelector(".table-block .table-wrapper");
        check("T2_rerender_wrapper_identity_preserved", wrapper2 === wrapper1,
            wrapper2 === wrapper1 ? "ok" : "wrapper 被重建");

        const codeBlock2 = root.querySelector(".code-block");
        check("T4_rerender_code_block_identity_preserved", codeBlock2 === codeBlock1,
            codeBlock2 === codeBlock1 ? "ok" : "代码块包裹被重建");

        // —— T3 重渲染：表格内容变化，原地更新路径 ——
        test.render(mdTableEdited);
        await waitFor(() => {
            const rows = root.querySelectorAll(".table-block tbody tr");
            return rows.length >= 4 ? rows : null;
        });
        await waitFrames(3);
        const tableBlock3 = root.querySelector(".table-block");
        check("T3_table_edited_block_identity_preserved", tableBlock3 === tableBlock1,
            tableBlock3 === tableBlock1 ? "ok" : "内容变更不应重建包裹层");
        check("T3_table_edited_row_count",
            root.querySelectorAll(".table-block tbody tr").length === 4,
            "rows=" + root.querySelectorAll(".table-block tbody tr").length);
        check("T3_table_edited_cell_text",
            root.querySelector(".table-block tbody").textContent.includes("改动"), "单元格文本未更新");

        // —— T5 新增第二个表格 ——
        test.render(mdSecondTable);
        await waitFor(() => root.querySelectorAll(".table-block").length >= 2 ? true : null);
        await waitFrames(3);
        const blocks = root.querySelectorAll(".table-block");
        check("T5_second_table_wrapped", blocks.length === 2
            && !!blocks[1].querySelector(".table-wrapper table tbody tr"),
            "blocks=" + blocks.length);

        // —— 无包裹残留：不应再出现“裸 table”（无 .table-wrapper 祖直接悬挂）——
        const bareTables = Array.from(root.querySelectorAll("table")).filter((t) => !t.closest(".table-wrapper"));
        check("T6_no_bare_table_outside_wrapper", bareTables.length === 0,
            bareTables.length + " bare table(s)");
    } catch (e) {
        check("harness_exception", false, String(e && e.message ? e.message : e));
    }
    finish();
}

// main.js import 即启动 boot；等 ProseMirror 挂载后开始驱动
if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", () => setTimeout(run, 0));
} else {
    setTimeout(run, 0);
}
