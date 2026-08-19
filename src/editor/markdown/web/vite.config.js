// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// vite.config.js —— 打包 Milkdown 为 IIFE 单文件，供 QWebEngine qrc 加载
import { defineConfig } from "vite";
import { resolve } from "path";

export default defineConfig({
    build: {
        lib: {
            entry: resolve(__dirname, "main.js"),
            name: "DeepinMarkdownRenderer",
            fileName: () => "main.bundle.js",
            formats: ["iife"],
        },
        outDir: "build",
        emptyOutDir: true,
        minify: false,
        rollupOptions: {
            output: {
                inlineDynamicImports: true,
            },
        },
    },
});
