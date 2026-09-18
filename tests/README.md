<!--
SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
SPDX-License-Identifier: CC-BY-4.0
-->

# deepin-editor 单元测试（Google Test 框架）

基于 Google Test + stub-ext 的单元测试框架，统一入口为 `test-prj-running.sh`。

## 目录结构

```
tests/
├── 3rdparty/stub/          # stub-ext 打桩工具
├── cmake/                  # UnitTestUtils.cmake 测试辅助函数
├── report_generator/       # Python 报告生成器（CSV/HTML，可选）
├── test-prj-running.sh     # 统一启动脚本：构建 + 运行 + 覆盖率 + 摘要
├── gen-ut-summary.py       # ut-summary.json 生成脚本
└── <模块>/                 # 各类测试目录（每个类一个 test_* 可执行目标）
```

## 构建

由根工程 `CMakeLists.txt` 的 `BUILD_TESTS`（默认 ON）引入：

```bash
cmake -S . -B build -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

必须使用 `Debug` 模式以启用 gcov 覆盖率插桩（`-fprofile-arcs -ftest-coverage`）。

## 运行（统一入口）

```bash
cd tests && ./test-prj-running.sh
```

脚本完成：清理并重建 `build/` → 编译 → 逐个运行 `test_*` 测试目标（offscreen，
生成 gtest XML）→ lcov 采集覆盖率并生成 HTML → 输出汇总到 `build-ut/`
（`report/*.xml`、`html/`、`ut-summary.json`、`asan_deepin-editor.log`），
退出码为全部测试的聚合结果。

## 依赖

- Google Test：系统包 `libgtest-dev`（≥1.12）
- lcov / gcov（覆盖率采集）
- Python 3（ut-summary / report_generator 使用）
