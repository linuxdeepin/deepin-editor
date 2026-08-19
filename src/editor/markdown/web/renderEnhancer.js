// SPDX-FileCopyrightText: 2026 UnionCTechnology Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// renderEnhancer.js —— 渲染后处理（静态部分）
//
// §4.8.8：在 Milkdown 默认 DOM 之上做无副作用的幂等加工，为 CSS 容器查询提供挂载点。
// 交互部分（粘性表头/复制按钮/图片右键菜单）阶段 3.5 启用。

const TABLE_WRAPPER_CLASS = "table-wrapper";

// 把每个 <table> 包裹进 .table-wrapper（横向滚动容器 + 容器查询锚点）
// 幂等：已包裹的表格跳过
export function wrapTables(root) {
    if (!root) return;
    const tables = root.querySelectorAll("table");
    tables.forEach((table) => {
        if (table.parentElement && table.parentElement.classList.contains(TABLE_WRAPPER_CLASS)) return;
        const wrapper = document.createElement("div");
        wrapper.className = TABLE_WRAPPER_CLASS;
        table.parentNode.insertBefore(wrapper, table);
        wrapper.appendChild(table);
    });
}

// 统一入口：每次 Milkdown 重渲染后执行一次
export function enhance(root) {
    wrapTables(root);
}
