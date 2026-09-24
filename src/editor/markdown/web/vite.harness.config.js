// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// vite.harness.config.js —— 防闪烁回归验证页（harness/）的打包配置，不进 qrc。
// 与 vite.config.js 同型，仅 entry/产物名不同。用法：npm run build:harness
import { defineConfig } from "vite";
import { resolve } from "path";

export default defineConfig({
    build: {
        lib: {
            entry: resolve(__dirname, "harness/harness.js"),
            name: "DeepinMarkdownHarness",
            fileName: () => "harness.bundle.js",
            formats: ["iife"],
        },
        outDir: "build/harness",
        emptyOutDir: true,
        minify: false,
        rollupOptions: {
            output: {
                inlineDynamicImports: true,
            },
        },
    },
});
